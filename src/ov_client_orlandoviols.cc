/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2021 Giso Grimm, delude88, Tobias Hegemann
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ov_client_orlandoviols.h"
#include "../tascar/libtascar/include/session.h"
#include "errmsg.h"
#include "ov_tools.h"
#include "soundcardtools.h"
#include "udpsocket.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unistd.h>

CURL* curl;

namespace webCURL {

  struct MemoryStruct {
    char* memory;
    size_t size;
  };

  static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb,
                                    void* userp)
  {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;
    char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
  }

} // namespace webCURL

ov_client_orlandoviols_t::ov_client_orlandoviols_t(ov_render_base_t& backend,
                                                   const std::string& lobby)
    : ov_client_base_t(backend), runservice(true), lobby(lobby),
      quitrequest_(false), isovbox(is_ovbox())
{
  // curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if(!curl)
    throw ErrMsg("Unable to initialize curl");
}

void ov_client_orlandoviols_t::start_service()
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::start_service " << std::endl;
#endif
  runservice = true;
  servicethread = std::thread(&ov_client_orlandoviols_t::service, this);
}

void ov_client_orlandoviols_t::stop_service()
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::stop_service " << std::endl;
#endif
  runservice = false;
  if(servicethread.joinable())
    servicethread.join();
}

bool ov_client_orlandoviols_t::report_error(std::string url,
                                            const std::string& device,
                                            const std::string& msg)
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::report_error " << msg << std::endl;
#endif
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?ovclientmsg=" + device;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg.c_str());
  if(curl_easy_perform(curl) != CURLE_OK) {
    free(chunk.memory);
    return false;
  }
  free(chunk.memory);
  return true;
}

bool ov_client_orlandoviols_t::download_file(const std::string& url,
                                             const std::string& dest)
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::download_file " << url << " to "
            << dest << std::endl;
#endif
  CURLcode res;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  res = curl_easy_perform(curl);
  if(res == CURLE_OK) {
    std::ofstream ofh(dest);
    ofh.write(chunk.memory, chunk.size);
    free(chunk.memory);
    return true;
  }
  free(chunk.memory);
  return false;
}

/**
   \brief Pull device configuration.
   \param url Server URL for device REST API, e.g.,
   http://oldbox.orlandoviols.com/ \param device Device ID (typically MAC
   address) \retval hash Hash string of previous device configuration, or empty
   string \return Empty string, or string containing json device configuration
 */
std::string ov_client_orlandoviols_t::device_update(std::string url,
                                                    const std::string& device,
                                                    std::string& hash)
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::device_update " << url << std::endl;
#endif
  char chost[1024];
  memset(chost, 0, 1024);
  std::string hostname;
  if(0 == gethostname(chost, 1023))
    hostname = chost;
  nlohmann::json jsinchannels;
  // std::string jsinchannels("{");
  // uint32_t nch(0);
  for(auto ch : backend.get_input_channel_ids()) {
    jsinchannels.push_back(ch);
    // jsinchannels += "\"" + std::to_string(nch) + "\":\"" + ch + "\",";
    //++nch;
  }
  // if(jsinchannels.size())
  //  jsinchannels.erase(jsinchannels.size() - 1, 1);
  // jsinchannels += "}";
  std::vector<snddevname_t> alsadevs(list_sound_devices());
  nlohmann::json jsalsadevs;
  for(auto d : alsadevs)
    jsalsadevs[d.dev] = d.desc;
  double txrate(0);
  double rxrate(0);
  backend.getbitrate(txrate, rxrate);
  nlohmann::json jsdevice;
  jsdevice["alsadevs"] = jsalsadevs;
  jsdevice["hwinputchannels"] = jsinchannels;
  jsdevice["cpuload"] = backend.get_load();
  jsdevice["bandwidth"]["tx"] = txrate;
  jsdevice["bandwidth"]["rx"] = rxrate;
  jsdevice["localip"] = ep2ipstr(getipaddr());
  jsdevice["version"] = "ovclient-" + std::string(OVBOXVERSION);
  jsdevice["isovbox"] = isovbox;
  jsdevice["pingstats"] = nlohmann::json::parse(backend.get_client_stats());
  std::string curlstrdevice(jsdevice.dump());
  CURLcode res;
  std::string retv;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?ovclient2=" + device + "&hash=" + hash;
  if(hostname.size() > 0)
    url += "&host=" + hostname;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlstrdevice.c_str());
  res = curl_easy_perform(curl);
  if(res == CURLE_OK)
    retv.insert(0, chunk.memory, chunk.size);
  free(chunk.memory);
  std::stringstream ss(retv);
  std::string to;
  bool first(true);
  retv = "";
  while(std::getline(ss, to, '\n')) {
    if(first) {
      if(!to.empty() && (to[0] == '<'))
        throw ErrMsg("Invalid hash code, propably not a REST API response.");
      hash = to;
    } else {
      retv += to + '\n';
    }
    first = false;
  }
  if(retv.size())
    retv.erase(retv.size() - 1, 1);
  return retv;
}

void ov_client_orlandoviols_t::register_device(std::string url,
                                               const std::string& device)
{
#ifdef SHOWDEBUG
  std::cout << "ov_client_orlandoviols_t::register_device " << url
            << " and device " << device << std::endl;
#endif
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  url += "?setver=" + device + "&ver=ovclient-" + OVBOXVERSION;
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  if(curl_easy_perform(curl) != CURLE_OK) {
    free(chunk.memory);
    throw TASCAR::ErrMsg("Unable to register device with url \"" + url + "\".");
  }
  std::string result;
  result.insert(0, chunk.memory, chunk.size);
  free(chunk.memory);
  if(result != "OK")
    throw TASCAR::ErrMsg("The front end did not respond \"OK\".");
}

stage_device_t get_stage_dev(nlohmann::json& dev)
{
  try {
    stage_device_t stagedev;
    stagedev.id = my_js_value(dev, "id", -1);
    stagedev.label = my_js_value(dev, "label", std::string(""));
    nlohmann::json channels(dev["channels"]);
    size_t chcnt(0);
    if(channels.is_array())
      for(auto ch : channels) {
        device_channel_t devchannel;
        devchannel.id = stagedev.label + "_" + std::to_string(stagedev.id) +
                        "_c" + std::to_string(chcnt++);
        devchannel.sourceport = my_js_value(ch, "sourceport", std::string(""));
        devchannel.gain = my_js_value(ch, "gain", 1.0);
        nlohmann::json chpos(ch["position"]);
        devchannel.position.x = my_js_value(chpos, "x", 0.0);
        devchannel.position.y = my_js_value(chpos, "y", 0.0);
        devchannel.position.z = my_js_value(chpos, "z", 0.0);
        devchannel.directivity =
            my_js_value(ch, "directivity", std::string("omni"));
        stagedev.channels.push_back(devchannel);
      }
    /// Position of the stage device in the virtual space:
    nlohmann::json devpos(dev["position"]);
    stagedev.position.x = my_js_value(devpos, "x", 0.0);
    stagedev.position.y = my_js_value(devpos, "y", 0.0);
    stagedev.position.z = my_js_value(devpos, "z", 0.0);
    /// Orientation of the stage device in the virtual space, ZYX Euler angles:
    nlohmann::json devorient(dev["orientation"]);
    stagedev.orientation.z = my_js_value(devorient, "z", 0.0);
    stagedev.orientation.y = my_js_value(devorient, "y", 0.0);
    stagedev.orientation.x = my_js_value(devorient, "x", 0.0);
    /// Linear gain of the stage device:
    stagedev.gain = my_js_value(dev, "gain", 1.0);
    /// Mute flag:
    stagedev.mute = my_js_value(dev, "mute", false);
    // jitter buffer length:
    nlohmann::json jitter(dev["jitter"]);
    stagedev.receiverjitter = my_js_value(jitter, "receive", 5.0);
    stagedev.senderjitter = my_js_value(jitter, "send", 5.0);
    /// send to local IP address if in same network?
    stagedev.sendlocal = my_js_value(dev, "sendlocal", true);
    return stagedev;
  }
  catch(const std::exception& e) {
    throw TASCAR::ErrMsg(std::string("get_stage_dev: ") + e.what());
  }
}

#define GETJS(structname, field)                                               \
  structname.field = my_js_value(js_##structname, #field, structname.field)

// main frontend interface function:
void ov_client_orlandoviols_t::service()
{
  try {
    register_device(lobby, backend.get_deviceid());
    if(!report_error(lobby, backend.get_deviceid(), ""))
      throw TASCAR::ErrMsg("Unable to reset error message.");
    if(!download_file(lobby + "/announce.flac", folder + "announce.flac"))
      throw TASCAR::ErrMsg("Unable to download announcement file from server.");
  }
  catch(const std::exception& e) {
    quitrequest_ = true;
    std::cerr << "Error: " << e.what() << std::endl;
    std::cerr << "Invalid URL or server may be down." << std::endl;
    return;
  }
  std::string hash;
  double gracetime(9.0);
  while(runservice) {
    try {
      std::string stagecfg(device_update(lobby, backend.get_deviceid(), hash));
      if(!stagecfg.empty()) {
        try {
          nlohmann::json js_stagecfg(nlohmann::json::parse(stagecfg));
          if(!js_stagecfg["frontendconfig"].is_null()) {
            std::ofstream ofh(folder + "ov-client.cfg");
            ofh << js_stagecfg["frontendconfig"].dump();
            quitrequest_ = true;
          }
          if(my_js_value(js_stagecfg, "firmwareupdate", false)) {
            std::ofstream ofh(folder + "ov-client.firmwareupdate");
            quitrequest_ = true;
          }
          if(my_js_value(js_stagecfg, "usedevversion", false)) {
            std::ofstream ofh(folder + "ov-client.usedevversion");
            quitrequest_ = true;
          }
          if(js_stagecfg["wifisettings"].is_object()) {
            std::ofstream ofh(folder + "ov-client.wificfg");
            std::string ssid(my_js_value(js_stagecfg["wifisettings"], "ssid",
                                         std::string("")));
            std::string passwd(my_js_value(js_stagecfg["wifisettings"],
                                           "passwd", std::string("")));
            ofh << ssid << "\n" << passwd << "\n";
            quitrequest_ = true;
          }
          if(!quitrequest_) {
            nlohmann::json js_audio(js_stagecfg["audiocfg"]);
            if(!js_audio.is_null()) {
              audio_device_t audio;
              // backend.clear_stage();
              audio.drivername =
                  my_js_value(js_audio, "driver", std::string("jack"));
              audio.devicename =
                  my_js_value(js_audio, "device", std::string("hw:1"));
              audio.srate = my_js_value(js_audio, "srate", 48000.0);
              audio.periodsize = my_js_value(js_audio, "periodsize", 96);
              audio.numperiods = my_js_value(js_audio, "numperiods", 2);
              backend.configure_audio_backend(audio);
              if(my_js_value(js_audio, "restart", false)) {
                bool session_was_active(backend.is_session_active());
                if(session_was_active)
                  backend.end_session();
                backend.stop_audiobackend();
                backend.start_audiobackend();
                if(session_was_active)
                  backend.require_session_restart();
              }
            }
            nlohmann::json js_rendersettings(js_stagecfg["rendersettings"]);
            if(!js_rendersettings.is_null())
              backend.set_thisdev(get_stage_dev(js_rendersettings));
            nlohmann::json js_stage(js_stagecfg["room"]);
            if(js_stage.is_object()) {
              std::string stagehost(
                  my_js_value(js_stage, "host", std::string("")));
              port_t stageport(my_js_value(js_stage, "port", 0));
              secret_t stagepin(my_js_value(js_stage, "pin", 0u));
              backend.set_relay_server(stagehost, stageport, stagepin);
              nlohmann::json js_roomsize(js_stage["size"]);
              nlohmann::json js_reverb(js_stage["reverb"]);
              render_settings_t rendersettings;
              GETJS(rendersettings, id);
              rendersettings.roomsize.x = my_js_value(js_roomsize, "x", 25.0);
              rendersettings.roomsize.y = my_js_value(js_roomsize, "y", 13.0);
              rendersettings.roomsize.z = my_js_value(js_roomsize, "z", 7.5);
              rendersettings.absorption =
                  my_js_value(js_reverb, "absorption", 0.6);
              rendersettings.damping = my_js_value(js_reverb, "damping", 0.7);
              rendersettings.reverbgain = my_js_value(js_reverb, "gain", 0.4);
              GETJS(rendersettings, renderreverb);
              GETJS(rendersettings, renderism);
              GETJS(rendersettings, distancelaw);
              GETJS(rendersettings, delaycomp);
              // gains, ports and network:
              GETJS(rendersettings, outputport1);
              GETJS(rendersettings, outputport2);
              GETJS(rendersettings, rawmode);
              GETJS(rendersettings, rectype);
              GETJS(rendersettings, secrec);
              GETJS(rendersettings, egogain);
              GETJS(rendersettings, mastergain);
              GETJS(rendersettings, peer2peer);
              // level metering:
              GETJS(rendersettings, lmetertc);
              GETJS(rendersettings, lmeterfw);
              // ambient sound:
              rendersettings.ambientsound =
                  my_js_value(js_stage, "ambientsound", std::string(""));
              rendersettings.ambientlevel =
                  my_js_value(js_stage, "ambientlevel", 0.0);
              rendersettings.ambientsound =
                  ovstrrep(rendersettings.ambientsound, "\\/", "/");
              // if not empty then download file to hash code:
              if(rendersettings.ambientsound.size()) {
                std::string hashname(
                    folder + url2localfilename(rendersettings.ambientsound));
                // test if file already exists:
                std::ifstream ambif(hashname);
                if(!ambif.good()) {
                  if(!download_file(rendersettings.ambientsound, hashname)) {
                    report_error(lobby, backend.get_deviceid(),
                                 "Unable to download ambient sound file from " +
                                     rendersettings.ambientsound);
                  }
                }
                // test if file can be read and has four channels:
                try {
                  TASCAR::sndfile_handle_t sf(hashname);
                  if(sf.get_channels() != 4) {
                    throw ErrMsg("Not in B-Format (" +
                                 std::to_string(sf.get_channels()) +
                                 " channels)");
                  }
                }
                catch(const std::exception& e) {
                  throw ErrMsg("Unable to open ambient sound file from " +
                               rendersettings.ambientsound + ": " + e.what());
                }
              }
              //
              rendersettings.xports.clear();
              nlohmann::json js_xports(js_rendersettings["xport"]);
              if(js_xports.is_array())
                for(auto xp : js_xports) {
                  if(xp.is_array())
                    if((xp.size() == 2) && xp[0].is_string() &&
                       xp[1].is_string()) {
                      std::string key(xp[0].get<std::string>());
                      if(key.size())
                        rendersettings.xports[key] = xp[1].get<std::string>();
                    }
                }
              rendersettings.xrecport.clear();
              nlohmann::json js_xrecports(js_rendersettings["xrecport"]);
              if(js_xrecports.is_array())
                for(auto xrp : js_xrecports) {
                  if(xrp.is_number()) {
                    int p(xrp.get<int>());
                    if(p > 0)
                      rendersettings.xrecport.push_back(p);
                  }
                }
              rendersettings.headtracking =
                  my_js_value(js_rendersettings, "headtracking", false);
              rendersettings.headtrackingrotrec =
                  my_js_value(js_rendersettings, "headtrackingrot", true);
              rendersettings.headtrackingrotsrc =
                  my_js_value(js_rendersettings, "headtrackingrotsrc", true);
              rendersettings.headtrackingport =
                  my_js_value(js_rendersettings, "headtrackingport", 0);
              backend.set_render_settings(
                  rendersettings,
                  my_js_value(js_rendersettings, "stagedevid", 0));
              if(!js_rendersettings["extracfg"].is_null())
                backend.set_extra_config(js_rendersettings["extracfg"].dump());
              nlohmann::json js_stagedevs(js_stagecfg["roomdev"]);
              std::map<stage_device_id_t, stage_device_t> newstage;
              if(js_stagedevs.is_array()) {
                for(auto dev : js_stagedevs) {
                  auto sdev(get_stage_dev(dev));
                  newstage[sdev.id] = sdev;
                }
                backend.set_stage(newstage);
              }
            }
            if(!backend.is_audio_active())
              backend.start_audiobackend();
            backend.restart_session_if_needed();
          }
          report_error(lobby, backend.get_deviceid(), "");
        }
        catch(const std::exception& e) {
          std::cerr << "Error: " << e.what() << std::endl;
          report_error(lobby, backend.get_deviceid(), e.what());
          DEBUG(stagecfg);
        }
      }
      double t(0);
      while((t < gracetime) && runservice) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        t += 0.001;
      }
    }
    catch(const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << "Retrying in 15 seconds." << std::endl;
      sleep(15);
    }
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
