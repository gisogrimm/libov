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
#ifndef WIN32
#include <sys/utsname.h>
#endif
#include <unistd.h>
// #include <filesystem>

CURL* curl;

bool nocancelwait = true;

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
  std::lock_guard<std::mutex> lock(curlmtx);
  if(sodium_init() < 0)
    throw ErrMsg("Unable to initialize libsodium");
  curl = curl_easy_init();
  if(!curl)
    throw ErrMsg("Unable to initialize curl");
#ifdef WIN32
  // Initialize WSA on Windows
  if(WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
    throw ErrMsg("Unable to initialize WSA");
  uname_sysname = "WIN32";
  // Get the OS version
  NTSTATUS status;
  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
  status = RtlGetVersion(&osvi);
  if(status == 0) {
    // Construct the OS version string
    uname_release = std::to_string(osvi.dwMajorVersion) + "." +
                    std::to_string(osvi.dwMinorVersion) + "." +
                    std::to_string(osvi.dwBuildNumber);
  }
  SYSTEM_INFO sysInfo;
  GetNativeSystemInfo(&sysInfo);
  switch(sysInfo.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_INTEL:
    uname_machine = "x86";
  case PROCESSOR_ARCHITECTURE_AMD64:
    uname_machine = "x64";
  case PROCESSOR_ARCHITECTURE_ARM:
    uname_machine = "ARM";
  case PROCESSOR_ARCHITECTURE_ARM64:
    uname_machine = "ARM64";
  case PROCESSOR_ARCHITECTURE_MIPS:
    uname_machine = "MIPS";
  case PROCESSOR_ARCHITECTURE_ALPHA:
    uname_machine = "Alpha";
  case PROCESSOR_ARCHITECTURE_PPC:
    uname_machine = "PowerPC";
  case PROCESSOR_ARCHITECTURE_SHX:
    uname_machine = "SHX";
  case PROCESSOR_ARCHITECTURE_SPARC:
    uname_machine = "SPARC";
  default:
    uname_machine = "Unknown";
  }
#else
  // Get the OS information on non-Windows platforms
  struct utsname buffer;
  if(uname(&buffer) == 0) {
    uname_sysname = buffer.sysname;
    uname_release = buffer.release;
    uname_machine = buffer.machine;
  }
#endif
}

ov_client_orlandoviols_t::~ov_client_orlandoviols_t()
{
#ifdef WIN32
  WSACleanup(); // Clean up Winsock

#endif
}

void ov_client_orlandoviols_t::start_service()
{
  TASCAR::console_log("starting ov_client_orlandoviols service.");
  runservice = true;
  servicethread = std::thread(&ov_client_orlandoviols_t::service, this);
}

void ov_client_orlandoviols_t::stop_service()
{
  runservice = false;
  if(servicethread.joinable())
    servicethread.join();
  TASCAR::console_log("stopped ov_client_orlandoviols service.");
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
  std::lock_guard<std::mutex> lock(curlmtx);
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg.c_str());
  CURLcode err = CURLE_OK;
  if((err = curl_easy_perform(curl)) != CURLE_OK) {
    DEBUG(curl_easy_strerror(err));
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
  // std::string hashname =
  //  std::string("/usr/share/ovclient/sounds/") + url2localfilename(url);
  for(auto hashname :
      {std::string("/usr/share/ovclient/sounds/") + url2localfilename(url),
       url2localfilename(url)}) {
    std::ifstream localfile(hashname, std::ios::binary);
    if(localfile.good()) {
      TASCAR::console_log("Using file \"" + hashname + "\" for URL \"" + url +
                          "\".");
      std::ofstream destfile(dest, std::ios::binary);
      destfile << localfile.rdbuf();
      if(!destfile.good()) {
        std::cerr << "could not copy \"" << hashname << "\" to \"" << dest
                  << "\", cashed from " << url << std::endl;
      }
      return true;
    }
  }
  std::cerr << "could not open cashed file, downloading from " << url
            << std::endl;
  CURLcode res;
  struct webCURL::MemoryStruct chunk;
  chunk.memory =
      (char*)malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;       /* no data at this point */
  std::lock_guard<std::mutex> lock(curlmtx);
  curl_easy_reset(curl);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  res = curl_easy_perform(curl);
  if(res == CURLE_OK) {
    std::ofstream ofh(dest, std::ios::binary);
    ofh.write(chunk.memory, chunk.size);
    free(chunk.memory);
    if(!ofh.good()) {
      std::cerr << "could not write downloaded file to \"" << dest << "\".\n";
    }
    return true;
  } else {
    DEBUG(curl_easy_strerror(res));
  }
  free(chunk.memory);
  return false;
}

unsigned long get_mem_total()
{
  std::string token;
  std::ifstream file("/proc/meminfo");
  while(file >> token) {
    if(token == "MemTotal:") {
      unsigned long mem;
      if(file >> mem) {
        return mem;
      } else {
        return 0;
      }
    }
    // Ignore the rest of the line
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return 0; // Nothing found
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
  for(auto ch : backend.get_input_channel_ids())
    jsinchannels.push_back(ch);
  std::vector<snddevname_t> alsadevs(list_sound_devices());
  nlohmann::json jsalsadevs;
  for(auto d : alsadevs)
    jsalsadevs[d.dev] = d.desc;
  double txrate = 0.0;
  double rxrate = 0.0;
  backend.getbitrate(txrate, rxrate);
  nlohmann::json jsdevice;
  jsdevice["alsadevs"] = jsalsadevs;
  jsdevice["hwinputchannels"] = jsinchannels;
  jsdevice["cpuload"] = backend.get_load();
  jsdevice["thermal"] = backend.get_temperature();
  jsdevice["totalmem"] = get_mem_total();
  jsdevice["bandwidth"]["tx"] = txrate;
  jsdevice["bandwidth"]["rx"] = rxrate;
  jsdevice["localip"] = ep2ipstr(getipaddr());
  jsdevice["version"] = "ovclient-" + std::string(OVBOXVERSION);
  jsdevice["isovbox"] = isovbox;
  jsdevice["pingstats"] = nlohmann::json::parse(backend.get_client_stats());
  jsdevice["networkdevices"] = getnetworkdevices();
  jsdevice["backendperiodsize"] = backend.get_periodsize();
  jsdevice["backendsrate"] = backend.get_srate();
  jsdevice["backendxruns"] = backend.get_xruns();
  jsdevice["uname_sysname"] = uname_sysname;
  jsdevice["uname_release"] = uname_release;
  jsdevice["uname_machine"] = uname_machine;
  jsdevice["levelstats"] =
      nlohmann::json::parse(backend.get_level_stat_as_json());
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
  {
    std::lock_guard<std::mutex> lock(curlmtx);
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlstrdevice.c_str());
    res = curl_easy_perform(curl);
    if(res == CURLE_OK) {
      retv.insert(0, chunk.memory, chunk.size);
    } else {
      DEBUG(curl_easy_strerror(res));
    }
    free(chunk.memory);
  }
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
  CURLcode err = CURLE_OK;
  {
    std::lock_guard<std::mutex> lock(curlmtx);
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCURL::WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    err = curl_easy_perform(curl);
  }
  if(err != CURLE_OK) {
    free(chunk.memory);
    throw TASCAR::ErrMsg("Unable to register device with url \"" + url +
                         "\". " + std::string(curl_easy_strerror(err)));
  }
  std::string result;
  result.insert(0, chunk.memory, chunk.size);
  free(chunk.memory);
  if(result != "OK")
    throw TASCAR::ErrMsg("The front end did not respond \"OK\".");
}

void ov_client_orlandoviols_t::upload_plugin_settings()
{
  nlohmann::json pluginscfg;
  if(backend.is_session_active()) {
    std::string currenteffectplugincfg =
        backend.get_all_current_plugincfg_as_json();
    try {
      if(currenteffectplugincfg != refplugcfg) {
        pluginscfg = nlohmann::json::parse(currenteffectplugincfg);
        size_t chcnt = 0;
        for(const auto& ch : pluginscfg) {
          backend.update_plugincfg(ch.dump(), chcnt);
          ++chcnt;
        }
      }
      refplugcfg = currenteffectplugincfg;
      if(!pluginscfg.is_null()) {
        std::string curlpost(pluginscfg.dump());
        std::string retv;
        struct webCURL::MemoryStruct chunk;
        chunk.memory =
            (char*)malloc(1); /* will be grown as needed by the realloc above */
        chunk.size = 0;       /* no data at this point */
        std::string url = lobby + "?pluginscfg=" + backend.get_deviceid();
        {
          std::lock_guard<std::mutex> lock(curlmtx);
          curl_easy_reset(curl);
          curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
          curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
          curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                           webCURL::WriteMemoryCallback);
          curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
          curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlpost.c_str());
          curl_easy_perform(curl);
          free(chunk.memory);
        }
        nocancelwait = false;
      }
    }
    catch(const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << currenteffectplugincfg << std::endl;
    }
  }
}

void ov_client_orlandoviols_t::upload_session_gains()
{
  // if(backend.is_session_active() && backend.in_room()) {
  //  float outputgain = 1.0f;
  //  float egogain = 1.0f;
  //  float reverbgain = 1.0f;
  //  std::map<std::string, std::vector<float>> othergains;
  //  backend.get_session_gains(outputgain, egogain, reverbgain, othergains);
  //  std::cout << __FILE__ << ":" << __LINE__ << ": upload "
  //            << TASCAR::to_string(TASCAR::lin2db(outputgain), "main %1.1f dB
  //            ")
  //            << TASCAR::to_string(TASCAR::lin2db(egogain), "ego %1.1f dB ")
  //            << TASCAR::to_string(TASCAR::lin2db(reverbgain), "rvb %1.1f dB
  //            ")
  //            << std::endl;
  //  for(const auto& g : othergains) {
  //    std::cerr << "  " << g.first << ": " << g.second << std::endl;
  //  }
  //}
}

void ov_client_orlandoviols_t::upload_objmix()
{
  if(backend.is_session_active()) {
    std::string objmixcfg = backend.get_objmixcfg_as_json();
    if(objmixcfg != "{}") {
      std::string retv;
      struct webCURL::MemoryStruct chunk;
      chunk.memory =
          (char*)malloc(1); /* will be grown as needed by the realloc above */
      chunk.size = 0;       /* no data at this point */
      std::string url = lobby + "?objmixcfg=" + backend.get_deviceid();
      {
        std::lock_guard<std::mutex> lock(curlmtx);
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, "device:device");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         webCURL::WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, objmixcfg.c_str());
        curl_easy_perform(curl);
      }
      free(chunk.memory);
    }
    nocancelwait = false;
  }
}

stage_device_t get_stage_dev(nlohmann::json& dev)
{
  try {
    stage_device_t stagedev;
    stagedev.id = (stage_device_id_t)(my_js_value(dev, "id", -1));
    stagedev.uid = my_js_value(dev, "deviceid", std::string(""));
    stagedev.label = my_js_value(dev, "label", std::string(""));
    stagedev.receivedownmix = my_js_value(dev, "receivedownmix", false);
    stagedev.senddownmix = my_js_value(dev, "senddownmix", false);
    stagedev.nozita = my_js_value(dev, "nozita", false);
    stagedev.hiresping = my_js_value(dev, "hiresping", false);
    nlohmann::json channels(dev["channels"]);
    size_t chcnt(0);
    if(channels.is_array())
      for(auto ch : channels) {
        device_channel_t devchannel;
        devchannel.name = my_js_value(ch, "name", std::string(""));
        if(devchannel.name.empty())
          devchannel.id = stagedev.label + "_" + std::to_string(stagedev.id) +
                          "_c" + std::to_string(chcnt++);
        else
          devchannel.id = stagedev.label + "_" + std::to_string(stagedev.id) +
                          "_" + devchannel.name;
        devchannel.sourceport = my_js_value(ch, "sourceport", std::string(""));
        devchannel.gain = my_js_value(ch, "gain", 1.0f);
        nlohmann::json chpos(ch["position"]);
        devchannel.position.x = my_js_value(chpos, "x", 0.0f);
        devchannel.position.y = my_js_value(chpos, "y", 0.0f);
        devchannel.position.z = my_js_value(chpos, "z", 0.0f);
        devchannel.directivity =
            my_js_value(ch, "directivity", std::string("omni"));
        devchannel.update_plugin_cfg(ch["plugins"].dump());
        stagedev.channels.push_back(devchannel);
      }
    /// Position of the stage device in the virtual space:
    nlohmann::json devpos(dev["position"]);
    stagedev.position.x = my_js_value(devpos, "x", 0.0f);
    stagedev.position.y = my_js_value(devpos, "y", 0.0f);
    stagedev.position.z = my_js_value(devpos, "z", 0.0f);
    /// Orientation of the stage device in the virtual space, ZYX Euler angles:
    nlohmann::json devorient(dev["orientation"]);
    stagedev.orientation.z = my_js_value(devorient, "z", 0.0f);
    stagedev.orientation.y = my_js_value(devorient, "y", 0.0f);
    stagedev.orientation.x = my_js_value(devorient, "x", 0.0f);
    /// Linear gain of the stage device:
    stagedev.gain = my_js_value(dev, "gain", 1.0f);
    /// Mute flag:
    stagedev.mute = my_js_value(dev, "mute", false);
    // jitter buffer length:
    nlohmann::json jitter(dev["jitter"]);
    stagedev.receiverjitter = my_js_value(jitter, "receive", 5.0f);
    stagedev.senderjitter = my_js_value(jitter, "send", 5.0f);
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
  TASCAR::tictoc_t tictoc;
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
  double gracetime(7.7);
  while(runservice) {
    try {
      std::string stagecfg(device_update(lobby, backend.get_deviceid(), hash));
      if(!stagecfg.empty()) {
        try {
          nlohmann::json js_stagecfg(nlohmann::json::parse(stagecfg));
          auto newowner = my_js_value(js_stagecfg, "owner", owner);
          if(newowner != owner) {
            owner = newowner;
            std::cout << "Device ID: " << backend.get_deviceid()
                      << " User: " << owner << std::endl;
          }
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
          if(my_js_value(js_stagecfg, "installopenmha", false)) {
            std::ofstream ofh(folder + "ov-client.installopenmha");
            quitrequest_ = true;
          }
          std::string hifiberry =
              my_js_value(js_stagecfg, "usehifiberry", std::string(""));
          if(hifiberry.size()) {
            std::ofstream ofh(folder + "ov-client.hifiberry");
            ofh << hifiberry << std::endl;
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
              audio.srate = my_js_value(js_audio, "srate", 48000.0f);
              audio.periodsize = my_js_value(js_audio, "periodsize", 96);
              audio.numperiods = my_js_value(js_audio, "numperiods", 2);
              audio.priority = my_js_value(js_audio, "priority", 40);
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
            if(!js_rendersettings.is_null()) {
              backend.set_thisdev(get_stage_dev(js_rendersettings));
            }
            nlohmann::json js_stage(js_stagecfg["room"]);
            if(js_stage.is_object()) {
              std::string stagehost(
                  my_js_value(js_stage, "host", std::string("")));
              port_t stageport((port_t)(my_js_value(js_stage, "port", 0)));
              secret_t stagepin(my_js_value(js_stage, "pin", 0u));
              backend.set_relay_server(stagehost, stageport, stagepin);
              nlohmann::json js_roomsize(js_stage["size"]);
              nlohmann::json js_reverb(js_stage["reverb"]);
              render_settings_t rendersettings;
              GETJS(rendersettings, id);
              rendersettings.roomsize.x = my_js_value(js_roomsize, "x", 25.0f);
              rendersettings.roomsize.y = my_js_value(js_roomsize, "y", 13.0f);
              rendersettings.roomsize.z = my_js_value(js_roomsize, "z", 7.5f);
              rendersettings.absorption =
                  my_js_value(js_reverb, "absorption", 0.6f);
              rendersettings.damping = my_js_value(js_reverb, "damping", 0.7f);
              rendersettings.reverbgain = my_js_value(js_reverb, "gain", 0.4f);
              rendersettings.reverbgainroom =
                  my_js_value(js_reverb, "roomgain", 0.4f);
              rendersettings.reverbgaindev =
                  my_js_value(js_reverb, "devgain", 1.0f);
              GETJS(rendersettings, renderreverb);
              GETJS(rendersettings, renderism);
              GETJS(rendersettings, distancelaw);
              GETJS(rendersettings, delaycomp);
              GETJS(rendersettings, decorr);
              // gains, ports and network:
              GETJS(rendersettings, outputport1);
              GETJS(rendersettings, outputport2);
              GETJS(rendersettings, rawmode);
              GETJS(rendersettings, receive);
              GETJS(rendersettings, rectype);
              GETJS(rendersettings, secrec);
              GETJS(rendersettings, egogain);
              GETJS(rendersettings, outputgain);
              GETJS(rendersettings, peer2peer);
              GETJS(rendersettings, usetcptunnel);
              // level metering:
              GETJS(rendersettings, lmetertc);
              GETJS(rendersettings, lmeterfw);
              // ambient sound:
              rendersettings.ambientsound =
                  my_js_value(js_stage, "ambientsound", std::string(""));
              rendersettings.ambientlevel =
                  my_js_value(js_stage, "ambientlevel", 0.0f);
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
                  int p = 0;
                  if(xrp.is_number()) {
                    p = xrp.get<int>();
                  } else if(xrp.is_string()) {
                    p = atoi(xrp.get<std::string>().c_str());
                  }
                  if((p > 0) && (p < (1 << 16)))
                    rendersettings.xrecport.push_back((port_t)p);
                }
              rendersettings.headtracking =
                  my_js_value(js_rendersettings, "headtracking", false);
              rendersettings.headtrackingrotrec =
                  my_js_value(js_rendersettings, "headtrackingrot", true);
              rendersettings.headtrackingrotsrc =
                  my_js_value(js_rendersettings, "headtrackingrotsrc", true);
              rendersettings.headtrackingport =
                  (port_t)my_js_value(js_rendersettings, "headtrackingport", 0);
              backend.set_render_settings(
                  rendersettings, (stage_device_id_t)(my_js_value(
                                      js_rendersettings, "stagedevid", 0)));
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
            refplugcfg = backend.get_all_current_plugincfg_as_json();
            if(backend.is_session_active()) {
              report_error(lobby, backend.get_deviceid(), "");
            }
          }
        }
        catch(const std::exception& e) {
          std::cerr << "Error: " << e.what() << std::endl;
          report_error(lobby, backend.get_deviceid(), e.what());
          DEBUG(stagecfg);
        }
      }
      //      double t(0);
      while((tictoc.toc() < gracetime) && runservice && nocancelwait) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // t += 0.001;
      }
      nocancelwait = true;
      tictoc.tic();
    }
    catch(const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << "Retrying in 15 seconds." << std::endl;
      sleep(15);
    }
  }
  if(backend.is_session_active())
    backend.end_session();
  backend.stop_audiobackend();
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
