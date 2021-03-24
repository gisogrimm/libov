#include "ov_render_tascar.h"
#include "soundcardtools.h"
#include <fstream>
#include <iostream>
#include <jack/jack.h>

#ifndef ZITAPATH
#define ZITAPATH ""
#endif

bool ov_render_tascar_t::metronome_t::operator!=(const metronome_t& a)
{
  return (a.bpb != bpb) || (a.bpm != bpm) || (a.bypass != bypass) ||
         (a.delay != delay) || (a.level != level);
}

ov_render_tascar_t::metronome_t::metronome_t()
    : bpb(4), bpm(120.0), bypass(true), delay(40.0), level(65)
{
}

ov_render_tascar_t::metronome_t::metronome_t(const nlohmann::json& js)
    : bpb(my_js_value(js, "bpb", 4)), bpm(my_js_value(js, "bpm", 120.0)),
      bypass(!my_js_value(js, "active", false)),
      delay(my_js_value(js, "delay", 40.0)),
      level(my_js_value(js, "level", 65.0))
{
}

void session_add_connect(tsccfg::node_t e_session, const std::string& src,
                         const std::string& dest)
{
  tsccfg::node_t e_port(tsccfg::node_add_child(e_session, "connect"));
  tsccfg::node_set_attribute(e_port, "src", src);
  tsccfg::node_set_attribute(e_port, "dest", dest);
}

void ov_render_tascar_t::metronome_t::set_xmlattr(tsccfg::node_t em,
                                                  tsccfg::node_t ed) const
{
  tsccfg::node_set_attribute(em, "bpm", std::to_string(bpm));
  tsccfg::node_set_attribute(em, "bpb", std::to_string(bpb));
  tsccfg::node_set_attribute(em, "bypass", (bypass ? "true" : "false"));
  tsccfg::node_set_attribute(ed, "delay", "0 " + std::to_string(0.001 * delay));
  tsccfg::node_set_attribute(em, "a1", std::to_string(level));
  tsccfg::node_set_attribute(em, "ao", std::to_string(level));
}

void ov_render_tascar_t::metronome_t::update_osc(TASCAR::osc_server_t* srv,
                                                 const std::string& dev) const
{
  if(srv) {
    lo_message msg(lo_message_new());
    lo_message_add_float(msg, 1);
    lo_arg** loarg(lo_message_get_argv(msg));
    loarg[0]->f = bpm;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bpm").c_str(), msg);
    loarg[0]->f = bypass;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bypass").c_str(),
        msg);
    loarg[0]->f = bpb;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/bpb").c_str(), msg);
    loarg[0]->f = level;
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/a1").c_str(), msg);
    srv->dispatch_data_message(
        (std::string("/") + dev + ".metronome/ap*/metronome/ao").c_str(), msg);
    lo_message_free(msg);
  }
}

bool file_exists(const std::string& fname)
{
  try {
    std::ifstream ifh(fname);
    if(ifh.good())
      return true;
    return false;
  }
  catch(...) {
    return false;
  }
}

std::string get_zita_path()
{
  std::string zitapath(ZITAPATH);
#ifdef __APPLE__
  // Running on mac will result in either having zita-n2j besides the executable
  // or inside an app bundle
  if(file_exists("./zita-n2j")) {
    zitapath = "./";
    std::cout << "Found zita beside executable" << std::endl;
  } else if(file_exists("./DigitalStage.app/Contents/MacOS/zita-n2j")) {
    zitapath = "./DigitalStage.app/Contents/MacOS/";
    std::cout << "Found zita inside app bundle" << std::endl;
  }
#endif
  return zitapath;
}

TASCAR::pos_t to_tascar(const pos_t& src)
{
  return TASCAR::pos_t(src.x, src.y, src.z);
}

TASCAR::zyx_euler_t to_tascar(const zyx_euler_t& src)
{
  return TASCAR::zyx_euler_t(src.z, src.y, src.x);
}

void sendpinglog(stage_device_id_t cid, double tms, const endpoint_t& ep,
                 void* addr)
{
  if(addr) {
    lo_send((lo_address)addr, "/ping", "id", cid, tms);
    lo_send((lo_address)addr, "/pinga", "iiid", cid, ep.sin_addr.s_addr,
            ep.sin_port, tms);
  }
}

ov_render_tascar_t::ov_render_tascar_t(const std::string& deviceid,
                                       port_t pinglogport_)
    : ov_render_base_t(deviceid), h_jack(NULL), h_webmixer(NULL), tascar(NULL),
      ovboxclient(NULL), pinglogport(pinglogport_), pinglogaddr(nullptr),
      inputports({"system:capture_1", "system:capture_2"}),
      headtrack_tauref(33.315), selfmonitor_delay(0.0),
      zita_path(get_zita_path()), is_proxy(false), use_proxy(false)
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::ov_render_tascar_t" << std::endl;
#endif
  // avoid problems with number format in xml file:
  setlocale(LC_ALL, "C");
  audiodevice = {"jack", "hw:1", 48000, 96, 2};
  if(pinglogport)
    pinglogaddr =
        lo_address_new("localhost", std::to_string(pinglogport_).c_str());
}

ov_render_tascar_t::~ov_render_tascar_t()
{
  if(is_session_active())
    end_session();
  if(is_audio_active())
    stop_audiobackend();
  if(pinglogaddr)
    lo_address_free(pinglogaddr);
}

void ov_render_tascar_t::create_virtual_acoustics(tsccfg::node_t e_session,
                                                  tsccfg::node_t e_rec,
                                                  tsccfg::node_t e_scene)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  // list of ports on which TASCAR will wait before attempting to connect:
  std::vector<std::string> waitports;
  // b_sender is true if this device is sending audio. If this
  // device is not sending audio, then the stage layout will
  // differ.
  bool b_sender(!thisdev.channels.empty());
  if(b_sender) {
    // set position and orientation of receiver:
    tsccfg::node_t e_pos(tsccfg::node_add_child(e_rec, "position"));
    tsccfg::node_set_text(
        e_pos, "0 " + TASCAR::to_string(to_tascar(
                          stage.stage[stage.rendersettings.id].position)));
    tsccfg::node_t e_rot(tsccfg::node_add_child(e_rec, "orientation"));
    tsccfg::node_set_text(
        e_rot, "0 " + TASCAR::to_string(to_tascar(
                          stage.stage[stage.rendersettings.id].orientation)));
  }
  // the stage is not empty, which means we are on a stage.  width of
  // stage in degree, for listener-only devices (for sending devices
  // we take positions transferred from database server):
  double stagewidth(160);
  double az(-0.5 * stagewidth);
  double daz(stagewidth / (stage.stage.size() - (!b_sender)) * (M_PI / 180.0));
  az = az * (M_PI / 180.0) - 0.5 * daz;
  double radius(1.2);
  // create sound sources:
  for(auto stagemember : stage.stage) {
    if(stagemember.second.channels.size()) {
      // create sound source only for sending devices:
      if(b_sender || (stagemember.second.id != thisdev.id)) {
        TASCAR::pos_t pos(to_tascar(stagemember.second.position));
        TASCAR::zyx_euler_t rot(to_tascar(stagemember.second.orientation));
        if(!b_sender) {
          // this device is not sending, overwrite stage layout:
          az += daz;
          pos.x = radius * cos(az);
          pos.y = -radius * sin(az);
          pos.z = 0;
          rot.z = (180 / M_PI * (-az + M_PI));
          rot.y = 0;
          rot.x = 0;
        }
        // create a sound source for each device on stage:
        tsccfg::node_t e_src(tsccfg::node_add_child(e_scene, "source"));
        if(stagemember.second.id == thisdev.id) {
          // in case of self-monitoring, this source is called "ego":
          tsccfg::node_set_attribute(e_src, "name", "ego");
        } else {
          tsccfg::node_set_attribute(e_src, "name",
                                     get_stagedev_name(stagemember.second.id));
        }
        tsccfg::node_set_attribute(e_src, "dlocation", to_string(pos));
        tsccfg::node_t e_rot(tsccfg::node_add_child(e_src, "orientation"));
        tsccfg::node_set_text(e_rot, "0 " + TASCAR::to_string(rot));
        // e_src->set_attribute("dorientation", to_string(rot));
        uint32_t kch(0);
        for(auto ch : stagemember.second.channels) {
          // create a sound for each channel:
          ++kch;
          tsccfg::node_t e_snd(tsccfg::node_add_child(e_src, "sound"));
          tsccfg::node_set_attribute(e_snd, "maxdist", "50");
          tsccfg::node_set_attribute(e_snd, "gainmodel", "1");
          tsccfg::node_set_attribute(e_snd, "delayline", "false");
          tsccfg::node_set_attribute(e_snd, "id", ch.id);
          double gain(ch.gain * stagemember.second.gain);
          if(stagemember.second.id == thisdev.id) {
            // connect self-monitoring source ports:
            tsccfg::node_set_attribute(e_snd, "connect", ch.sourceport);
            gain *= stage.rendersettings.egogain;
          } else {
            // if not self-monitor then decrease gain:
            gain *= 0.6;
          }
          tsccfg::node_set_attribute(e_snd, "gain",
                                     TASCAR::to_string(20.0 * log10(gain)));
          // set relative channel positions:
          TASCAR::pos_t chpos(to_tascar(ch.position));
          tsccfg::node_set_attribute(e_snd, "x", TASCAR::to_string(chpos.x));
          tsccfg::node_set_attribute(e_snd, "y", TASCAR::to_string(chpos.y));
          tsccfg::node_set_attribute(e_snd, "z", TASCAR::to_string(chpos.z));
          if(ch.directivity == "omni")
            tsccfg::node_set_attribute(e_snd, "type", "omni");
          if(ch.directivity == "cardioid")
            tsccfg::node_set_attribute(e_snd, "type", "cardioidmod");
          if((stagemember.second.id == thisdev.id) &&
             (selfmonitor_delay > 0.0)) {
            tsccfg::node_t e_plugs(tsccfg::node_add_child(e_snd, "plugins"));
            tsccfg::node_t e_delay(tsccfg::node_add_child(e_plugs, "delay"));
            tsccfg::node_set_attribute(
                e_delay, "delay", TASCAR::to_string(selfmonitor_delay * 0.001));
          }
        }
      }
    }
  }
  if(stage.rendersettings.renderism) {
    // create shoebox room:
    tsccfg::node_t e_rvb(tsccfg::node_add_child(e_scene, "facegroup"));
    tsccfg::node_set_attribute(e_rvb, "name", "room");
    tsccfg::node_set_attribute(
        e_rvb, "shoebox",
        TASCAR::to_string(to_tascar(stage.rendersettings.roomsize)));
    tsccfg::node_set_attribute(
        e_rvb, "reflectivity",
        TASCAR::to_string(1.0 - stage.rendersettings.absorption));
    tsccfg::node_set_attribute(e_rvb, "damping",
                               TASCAR::to_string(stage.rendersettings.damping));
  }
  if(stage.rendersettings.renderreverb) {
    // create reverb engine:
    tsccfg::node_t e_rvb(tsccfg::node_add_child(e_scene, "reverb"));
    tsccfg::node_set_attribute(e_rvb, "type", "simplefdn");
    tsccfg::node_set_attribute(
        e_rvb, "volumetric",
        TASCAR::to_string(to_tascar(stage.rendersettings.roomsize)));
    tsccfg::node_set_attribute(e_rvb, "image", "false");
    tsccfg::node_set_attribute(e_rvb, "fdnorder", "5");
    tsccfg::node_set_attribute(e_rvb, "dw", "60");
    tsccfg::node_set_attribute(
        e_rvb, "absorption",
        TASCAR::to_string(stage.rendersettings.absorption));
    tsccfg::node_set_attribute(e_rvb, "damping",
                               TASCAR::to_string(stage.rendersettings.damping));
    tsccfg::node_set_attribute(
        e_rvb, "gain",
        TASCAR::to_string(20 * log10(stage.rendersettings.reverbgain)));
  }
  // ambient sounds:
  if(stage.rendersettings.ambientsound.size()) {
    std::string hashname(url2localfilename(stage.rendersettings.ambientsound));
    // test if file exists:
    std::ifstream ambif(hashname);
    if(ambif.good()) {
      tsccfg::node_t e_diff(tsccfg::node_add_child(e_scene, "diffuse"));
      tsccfg::node_set_attribute(e_diff, "name", "ambient");
      tsccfg::node_set_attribute(
          e_diff, "size",
          TASCAR::to_string(to_tascar(stage.rendersettings.roomsize)));
      tsccfg::node_t e_plug(tsccfg::node_add_child(e_diff, "plugins"));
      tsccfg::node_t e_snd(tsccfg::node_add_child(e_plug, "sndfile"));
      tsccfg::node_set_attribute(e_snd, "name", hashname);
      tsccfg::node_set_attribute(
          e_snd, "level", TASCAR::to_string(stage.rendersettings.ambientlevel));
      tsccfg::node_set_attribute(e_snd, "loop", "0");
      tsccfg::node_set_attribute(e_snd, "transport", "false");
      tsccfg::node_set_attribute(e_snd, "license", "CC0");
      tsccfg::node_set_attribute(e_snd, "resample", "true");
    }
  }
  // configure extra modules:
  tsccfg::node_t e_mods(tsccfg::node_add_child(e_session, "modules"));
  tsccfg::node_t e_jackrec(tsccfg::node_add_child(e_mods, "jackrec"));
  tsccfg::node_set_attribute(e_jackrec, "url", "osc.udp://localhost:9000/");
  tsccfg::node_add_child(e_mods, "touchosc");
  // create zita-n2j receivers:
  // this variable holds the path to zita
  // binaries, or empty (default) for system installed:
  const std::string zitapath(get_zita_path());
  for(auto stagemember : stage.stage) {
    // only create a network receiver when the stage member is sending audio:
    if(stagemember.second.channels.size()) {
      std::string chanlist;
      for(uint32_t k = 0; k < stagemember.second.channels.size(); ++k) {
        if(k)
          chanlist += ",";
        chanlist += std::to_string(k + 1);
      }
      // do not create a network receiver for local device:
      if(stage.thisstagedeviceid != stagemember.second.id) {
        std::string clientname(get_stagedev_name(stagemember.second.id));
        tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
        double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
        // provide access to path!
        tsccfg::node_set_attribute(
            e_sys, "command",
            zitapath + "zita-n2j --chan " + chanlist + " --jname " +
                clientname + "." + stage.thisdeviceid + " --buf " +
                TASCAR::to_string(buff) + " 0.0.0.0 " +
                TASCAR::to_string(4464 + 2 * stagemember.second.id));
        tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
        // create connections:
        for(size_t c = 0; c < stagemember.second.channels.size(); ++c) {
          if(stage.thisstagedeviceid != stagemember.second.id) {
            std::string srcport(clientname + "." + stage.thisdeviceid +
                                ":out_" + std::to_string(c + 1));
            std::string destport("render." + stage.thisdeviceid + ":" +
                                 clientname + "." + std::to_string(c) + ".0");
            waitports.push_back(srcport);
            session_add_connect(e_session, srcport, destport);
          }
        }
        if(stage.rendersettings.secrec > 0) {
          // create a secondary network receiver with additional jitter buffer:
          if(stage.thisstagedeviceid != stagemember.second.id) {
            std::string clientname(get_stagedev_name(stagemember.second.id) +
                                   "_sec");
            std::string netclientname(
                "n2j_" + std::to_string(stagemember.second.id) + "_sec");
            tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
            double buff(thisdev.receiverjitter +
                        stagemember.second.senderjitter);
            tsccfg::node_set_attribute(
                e_sys, "command",
                zitapath + "zita-n2j --chan " + chanlist + " --jname " +
                    netclientname + "." + stage.thisdeviceid + " --buf " +
                    TASCAR::to_string(stage.rendersettings.secrec + buff) +
                    " 0.0.0.0 " +
                    TASCAR::to_string(4464 + 2 * stagemember.second.id + 100));
            tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
            // create also a route with correct gain settings:
            tsccfg::node_t e_route(tsccfg::node_add_child(e_mods, "route"));
            tsccfg::node_set_attribute(e_route, "name", clientname);
            tsccfg::node_set_attribute(
                e_route, "channels",
                std::to_string(stagemember.second.channels.size()));
            tsccfg::node_set_attribute(
                e_route, "gain",
                TASCAR::to_string(20 * log10(stagemember.second.gain)));
            tsccfg::node_set_attribute(e_route, "connect",
                                       netclientname + ":out_[0-9]*");
          }
        }
      }
    }
  }
  // when a second network receiver is used then also create a bus
  // with a delayed version of the self monitor:
  if((stage.rendersettings.secrec > 0) && (thisdev.channels.size() > 0)) {
    std::string clientname(get_stagedev_name(thisdev.id) + "_sec");
    tsccfg::node_t mod(tsccfg::node_add_child(e_mods, "route"));
    tsccfg::node_set_attribute(mod, "name", clientname);
    tsccfg::node_set_attribute(mod, "channels",
                               std::to_string(thisdev.channels.size()));
    tsccfg::node_t plgs(tsccfg::node_add_child(mod, "plugins"));
    tsccfg::node_t del(tsccfg::node_add_child(plgs, "delay"));
    tsccfg::node_set_attribute(
        del, "delay",
        TASCAR::to_string(0.001 *
                          (stage.rendersettings.secrec +
                           thisdev.receiverjitter + thisdev.senderjitter)));
    tsccfg::node_set_attribute(mod, "gain",
                               TASCAR::to_string(20 * log10(thisdev.gain)));
    int chn(0);
    for(auto ch : thisdev.channels) {
      session_add_connect(e_session, ch.sourceport,
                          clientname + ":in." + std::to_string(chn));
      ++chn;
    }
  }
  if(thisdev.channels.size() > 0) {
    // create metronome:
    tsccfg::node_t e_routemetro(tsccfg::node_add_child(e_mods, "route"));
    tsccfg::node_set_attribute(e_routemetro, "name",
                               stage.thisdeviceid + ".metronome");
    tsccfg::node_set_attribute(e_routemetro, "channels", "2");
    tsccfg::node_t e_mplug(tsccfg::node_add_child(e_routemetro, "plugins"));
    metronome.set_xmlattr(tsccfg::node_add_child(e_mplug, "metronome"),
                          tsccfg::node_add_child(e_mplug, "delay"));
    // create network sender:
    tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
    tsccfg::node_set_attribute(
        e_sys, "command",
        zitapath + "zita-j2n --chan " +
            std::to_string(thisdev.channels.size()) + " --jname " +
            stage.thisdeviceid + "_sender --16bit 127.0.0.1 " +
            std::to_string(4464 + 2 * stage.thisstagedeviceid));
    tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-j2n");
    int chn(0);
    for(auto ch : thisdev.channels) {
      ++chn;
      session_add_connect(e_session, ch.sourceport,
                          stage.thisdeviceid + "_sender:in_" +
                              std::to_string(chn));
      session_add_connect(e_session, stage.thisdeviceid + ".metronome:out.0",
                          stage.thisdeviceid + "_sender:in_" +
                              std::to_string(chn));
      session_add_connect(e_session, stage.thisdeviceid + ".metronome:out.1",
                          "render." + stage.thisdeviceid + ":ego." +
                              std::to_string(chn - 1) + ".0");
      waitports.push_back(stage.thisdeviceid + "_sender:in_" +
                          std::to_string(chn));
    }
  }
  tsccfg::node_t e_wait = tsccfg::node_add_child(e_mods, "waitforjackport");
  tsccfg::node_set_attribute(e_wait,"name",stage.thisdeviceid+".waitforports");
  for(auto port : waitports) {
    tsccfg::node_t e_p = tsccfg::node_add_child(e_wait, "port");
    tsccfg::node_set_text(e_p, port);
  }
  // head tracking:
  if(stage.rendersettings.headtracking) {
    tsccfg::node_t e_head = tsccfg::node_add_child(e_mods, "ovheadtracker");
    if(stage.rendersettings.headtrackingport > 0)
      tsccfg::node_set_attribute(
          e_head, "url",
          "osc.udp://localhost:" +
              std::to_string(stage.rendersettings.headtrackingport) + "/");
    std::vector<std::string> actor;
    if(stage.rendersettings.headtrackingrotrec)
      actor.push_back("/" + stage.thisdeviceid + "/master");
    if(stage.rendersettings.headtrackingrotsrc) {
      actor.push_back("/" + stage.thisdeviceid + "/ego");
      tsccfg::node_set_attribute(e_head, "roturl", "osc.udp://localhost:9870/");
      tsccfg::node_set_attribute(e_head, "rotpath",
                                 "/*/" + get_stagedev_name(thisdev.id) +
                                     "/zyxeuler");
    }
    tsccfg::node_set_attribute(e_head, "actor", TASCAR::vecstr2str(actor));
    tsccfg::node_set_attribute(
        e_head, "autoref",
        std::to_string(1.0 - exp(-1.0 / (30.0 * headtrack_tauref))));
    tsccfg::node_set_attribute(e_head, "levelpattern", "/*/ego/*");
    tsccfg::node_set_attribute(e_head, "name", stage.thisdeviceid);
    tsccfg::node_set_attribute(e_head, "send_only_quaternion", "true");
    if(stage.rendersettings.headtrackingrotrec ||
       stage.rendersettings.headtrackingrotsrc)
      tsccfg::node_set_attribute(e_head, "apply_rot", "true");
    else
      tsccfg::node_set_attribute(e_head, "apply_rot", "false");
    tsccfg::node_set_attribute(e_head, "apply_loc", "false");
  }
}

void ov_render_tascar_t::create_raw_dev(tsccfg::node_t e_session)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  // list of ports on which TASCAR will wait before attempting to connect:
  std::vector<std::string> waitports;
  // configure extra modules:
  tsccfg::node_t e_mods(tsccfg::node_add_child(e_session, "modules"));
  tsccfg::node_t e_jackrec = tsccfg::node_add_child(e_mods, "jackrec");
  tsccfg::node_set_attribute(e_jackrec, "url", "osc.udp://localhost:9000/");
  tsccfg::node_add_child(e_mods, "touchosc");
  //
  uint32_t chcnt(0);
  const std::string zitapath = get_zita_path();
  for(auto stagemember : stage.stage) {
    std::string chanlist;
    for(uint32_t k = 0; k < stagemember.second.channels.size(); ++k) {
      if(k)
        chanlist += ",";
      chanlist += std::to_string(k + 1);
    }
    if(stage.thisstagedeviceid != stagemember.second.id) {
      std::string clientname(get_stagedev_name(stagemember.second.id));
      tsccfg::node_t e_sys = tsccfg::node_add_child(e_mods, "system");
      double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
      tsccfg::node_set_attribute(
          e_sys, "command",
          zitapath + "zita-n2j --chan " + chanlist + " --jname " + clientname +
              "." + stage.thisdeviceid + " --buf " + TASCAR::to_string(buff) +
              " 0.0.0.0 " +
              TASCAR::to_string(4464 + 2 * stagemember.second.id));
      tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
      for(size_t c = 0; c < stagemember.second.channels.size(); ++c) {
        ++chcnt;
        if(stage.thisstagedeviceid != stagemember.second.id) {
          std::string srcport(clientname + "." + stage.thisdeviceid + ":out_" +
                              std::to_string(c + 1));
          std::string destport;
          if(chcnt & 1)
            destport = stage.rendersettings.outputport1;
          else
            destport = stage.rendersettings.outputport2;
          if(!destport.empty()) {
            waitports.push_back(srcport);
            tsccfg::node_t e_port =
                tsccfg::node_add_child(e_session, "connect");
            tsccfg::node_set_attribute(e_port, "src", srcport);
            tsccfg::node_set_attribute(e_port, "dest", destport);
          }
        }
      }
      if(stage.rendersettings.secrec > 0) {
        if(stage.thisstagedeviceid != stagemember.second.id) {
          std::string clientname(get_stagedev_name(stagemember.second.id) +
                                 "_sec");
          std::string netclientname(
              "n2j_" + std::to_string(stagemember.second.id) + "_sec");
          tsccfg::node_t e_sys = tsccfg::node_add_child(e_mods, "system");
          double buff(thisdev.receiverjitter + stagemember.second.senderjitter);
          tsccfg::node_set_attribute(
              e_sys, "command",
              zitapath + "zita-n2j --chan " + chanlist + " --jname " +
                  netclientname + "." + stage.thisdeviceid + " --buf " +
                  TASCAR::to_string(stage.rendersettings.secrec + buff) +
                  " 0.0.0.0 " +
                  TASCAR::to_string(4464 + 2 * stagemember.second.id + 100));
          tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
          tsccfg::node_t e_route = tsccfg::node_add_child(e_mods, "route");
          tsccfg::node_set_attribute(e_route, "name", clientname);
          tsccfg::node_set_attribute(
              e_route, "channels",
              std::to_string(stagemember.second.channels.size()));
          tsccfg::node_set_attribute(
              e_route, "gain",
              TASCAR::to_string(20 * log10(stagemember.second.gain)));
          tsccfg::node_set_attribute(e_route, "connect",
                                     netclientname + ":out_[0-9]*");
        }
      }
    }
  }
  // when a second network receiver is used then also create a bus
  // with a delayed version of the self monitor:
  if(stage.rendersettings.secrec > 0) {
    std::string clientname(get_stagedev_name(thisdev.id) + "_sec");
    tsccfg::node_t mod = tsccfg::node_add_child(e_mods, "route");
    tsccfg::node_set_attribute(mod, "name", clientname);
    tsccfg::node_set_attribute(mod, "channels",
                               std::to_string(thisdev.channels.size()));
    tsccfg::node_t plgs = tsccfg::node_add_child(mod, "plugins");
    tsccfg::node_t del = tsccfg::node_add_child(plgs, "delay");
    tsccfg::node_set_attribute(
        del, "delay",
        TASCAR::to_string(0.001 *
                          (stage.rendersettings.secrec +
                           thisdev.receiverjitter + thisdev.senderjitter)));
    tsccfg::node_set_attribute(mod, "gain",
                               TASCAR::to_string(20 * log10(thisdev.gain)));
    int chn(0);
    for(auto ch : thisdev.channels) {
      tsccfg::node_t e_port = tsccfg::node_add_child(e_session, "connect");
      tsccfg::node_set_attribute(e_port, "src", ch.sourceport);
      tsccfg::node_set_attribute(e_port, "dest",
                                 clientname + ":in." + std::to_string(chn));
      ++chn;
    }
  }
  if(thisdev.channels.size() > 0) {
    tsccfg::node_t e_sys = tsccfg::node_add_child(e_mods, "system");
    tsccfg::node_set_attribute(
        e_sys, "command",
        zitapath + "zita-j2n --chan " +
            std::to_string(thisdev.channels.size()) +
            " --jname sender --16bit 127.0.0.1 " +
            std::to_string(4464 + 2 * stage.thisstagedeviceid));
    tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-j2n");
    int chn(0);
    for(auto ch : thisdev.channels) {
      ++chn;
      tsccfg::node_t e_port = tsccfg::node_add_child(e_session, "connect");
      tsccfg::node_set_attribute(e_port, "src", ch.sourceport);
      tsccfg::node_set_attribute(e_port, "dest",
                                 stage.thisdeviceid + "_sender:in_" +
                                     std::to_string(chn));
      waitports.push_back(stage.thisdeviceid + "_sender:in_" +
                          std::to_string(chn));
    }
  }
  tsccfg::node_t e_wait = tsccfg::node_add_child(e_mods, "waitforjackport");
  tsccfg::node_set_attribute(e_wait,"name",stage.thisdeviceid+".waitforports");
  for(auto port : waitports) {
    tsccfg::node_t e_p = tsccfg::node_add_child(e_wait, "port");
    tsccfg::node_set_text(e_p, port);
  }
}

void ov_render_tascar_t::clear_stage()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::clear_stage" << std::endl;
#endif
  ov_render_base_t::clear_stage();
  if(is_session_active()) {
    end_session();
  }
}

void ov_render_tascar_t::start_session()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::start_session" << std::endl;
#endif
  // do whatever needs to be done in base class:
  ov_render_base_t::start_session();
  // create a short link to this device:
  try {
    // xml code for TASCAR configuration:
    TASCAR::xml_doc_t tsc;
    // default TASCAR session settings:
    tsccfg::node_set_name(tsc.root(), "session");
    tsccfg::node_t e_session(tsc.root());
    tsccfg::node_set_attribute(e_session, "srv_port", "9871");
    tsccfg::node_set_attribute(e_session, "duration", "36000");
    tsccfg::node_set_attribute(e_session, "name", stage.thisdeviceid);
    tsccfg::node_set_attribute(e_session, "license", "CC0");
    tsccfg::node_set_attribute(e_session, "levelmeter_tc", "0.5");
    // create virtual acoustics only when not in raw mode:
    if(!stage.rendersettings.rawmode) {
      // create a virtual acoustics "scene":
      tsccfg::node_t e_scene(tsccfg::node_add_child(e_session, "scene"));
      tsccfg::node_set_attribute(e_scene, "name", stage.thisdeviceid);
      // add a main receiver for which the scene is rendered:
      tsccfg::node_t e_rec = tsccfg::node_add_child(e_scene, "receiver");
      // receiver can be "hrtf" or "ortf" (more receivers are possible in
      // ->add_child_text(TASCAR, but only these two can produce audio which is
      // headphone-compatible):
      tsccfg::node_set_attribute(e_rec, "type", stage.rendersettings.rectype);
      if(stage.rendersettings.rectype == "ortf") {
        tsccfg::node_set_attribute(e_rec, "angle", "140");
        tsccfg::node_set_attribute(e_rec, "f6db", "12000");
        tsccfg::node_set_attribute(e_rec, "fmin", "3000");
      }
      tsccfg::node_set_attribute(e_rec, "name", "master");
      // do not add delay (usually used for dynamic scene rendering):
      tsccfg::node_set_attribute(e_rec, "delaycomp", "0.05");
      // connect output ports:
      if(!stage.rendersettings.outputport1.empty()) {
        std::string srcport("master_l");
        if(stage.rendersettings.rectype == "itu51")
          srcport = "master.0L";
        if(stage.rendersettings.rectype == "omni")
          srcport = "master.0";
        session_add_connect(e_session,
                            "render." + stage.thisdeviceid + ":" + srcport,
                            stage.rendersettings.outputport1);
      }
      if(!stage.rendersettings.outputport2.empty()) {
        std::string srcport("master_r");
        if(stage.rendersettings.rectype == "itu51")
          srcport = "master.1R";
        if(stage.rendersettings.rectype == "omni")
          srcport = "master.0";
        session_add_connect(e_session,
                            "render." + stage.thisdeviceid + ":" + srcport,
                            stage.rendersettings.outputport2);
      }
      // the host is not empty when this device is on a stage:
      if(!stage.host.empty()) {
        create_virtual_acoustics(e_session, e_rec, e_scene);
      } else {
        // the stage is empty, which means we play an announcement only.
        tsccfg::node_t e_src(tsccfg::node_add_child(e_scene, "source"));
        tsccfg::node_set_attribute(e_src, "name", "announce");
        tsccfg::node_t e_snd(tsccfg::node_add_child(e_src, "sound"));
        tsccfg::node_set_attribute(e_snd, "maxdist", "50");
        tsccfg::node_set_attribute(e_snd, "gainmodel", "1");
        tsccfg::node_set_attribute(e_snd, "delayline", "false");
        tsccfg::node_set_attribute(e_snd, "x", "4");
        tsccfg::node_t e_plugs(tsccfg::node_add_child(e_snd, "plugins"));
        tsccfg::node_t e_sndfile(tsccfg::node_add_child(e_plugs, "sndfile"));
        tsccfg::node_set_attribute(e_sndfile, "name", folder + "announce.flac");
        tsccfg::node_set_attribute(e_sndfile, "level", "57");
        tsccfg::node_set_attribute(e_sndfile, "transport", "false");
        tsccfg::node_set_attribute(e_sndfile, "resample", "true");
        tsccfg::node_set_attribute(e_sndfile, "loop", "0");
        tsccfg::node_t e_mods(tsccfg::node_add_child(e_session, "modules"));
        tsccfg::node_t e_jackrec = tsccfg::node_add_child(e_mods, "jackrec");
        tsccfg::node_set_attribute(e_jackrec, "url",
                                   "osc.udp://localhost:9000/");
        tsccfg::node_add_child(e_mods, "touchosc");
      }
    } else {
      if(!stage.host.empty()) {
        create_raw_dev(e_session);
      }
    }
    for(auto xport : stage.rendersettings.xports)
      session_add_connect(e_session, xport.first, xport.second);
    if(!stage.host.empty()) {
      ovboxclient = new ovboxclient_t(
          stage.host, stage.port, 4464 + 2 * stage.thisstagedeviceid, 0, 30,
          stage.pin, stage.thisstagedeviceid, stage.rendersettings.peer2peer,
          use_proxy, false, stage.stage[stage.thisstagedeviceid].sendlocal);
      if(stage.rendersettings.secrec > 0)
        ovboxclient->add_extraport(100);
      for(auto p : stage.rendersettings.xrecport)
        ovboxclient->add_receiverport(p, p);
      ovboxclient->add_receiverport(9870, 9871);
      if(pinglogaddr)
        ovboxclient->set_ping_callback(sendpinglog, pinglogaddr);
    }
    tsc.save(folder + "ov-client_debugsession.tsc");
    tascar = new TASCAR::session_t(tsc.save_to_string(),
                                   TASCAR::session_t::LOAD_STRING, "");
    tascar->start();
    // add web mixer tools (node-js server and touchosc interface):
    std::string command;
    std::string ipaddr(ep2ipstr(getipaddr()));
    ipaddr += " '";
    ipaddr += stage.thisdeviceid + " (" + stage.thisdevice.label + ")'";
#ifndef GUI
    if(file_exists("/usr/share/ovclient/webmixer.js")) {
      command = "(cd /usr/share/ovclient/ && node webmixer.js " + ipaddr + ")";
    }
    if(file_exists("webmixer.js")) {
      command = "node webmixer.js " + ipaddr;
    }
    if(!command.empty())
      h_webmixer = new spawn_process_t(command);
#endif
  }
  catch(...) {
    delete tascar;
    tascar = NULL;
    end_session();
    throw;
  }
}

void ov_render_tascar_t::end_session()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::end_session" << std::endl;
#endif
  ov_render_base_t::end_session();
  if(h_webmixer)
    delete h_webmixer;
  h_webmixer = NULL;
  if(tascar) {
    tascar->stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    delete tascar;
    tascar = NULL;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  if(ovboxclient) {
    delete ovboxclient;
    ovboxclient = NULL;
  }
}

void ov_render_tascar_t::start_audiobackend()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::start_audiobackend" << std::endl;
#endif
  ov_render_base_t::start_audiobackend();
  if((audiodevice.drivername == "jack") &&
     (audiodevice.devicename != "manual")) {
    if(h_jack)
      delete h_jack;
    std::string devname(audiodevice.devicename);
    if(audiodevice.devicename == "highest") {
      // the device name is not set, use the last one of available
      // devices because this is most likely the one to use (e.g.,
      // external sound card):
      auto devs(list_sound_devices());
      if(!devs.empty())
        devname = devs.rbegin()->dev;
    }
    if(audiodevice.devicename == "plughighest") {
      // the device name is not set, use the last one of available
      // devices because this is most likely the one to use (e.g.,
      // external sound card):
      auto devs(list_sound_devices());
      if(!devs.empty())
        devname = std::string("plug") + devs.rbegin()->dev;
    }
    char cmd[1024];
#ifdef __APPLE__
    // Check if jack is already running
    // const char *client_name = "run_test";
    jack_options_t options = JackNoStartServer;
    jack_status_t status;
    jack_client_t* jackClient = jack_client_open("run_test", options, &status);
    if(jackClient != nullptr) {
      std::cout << "[ov_render_tascar] Jack is already running" << std::endl;
      jack_client_close(jackClient);
      return;
    }
    sprintf(cmd,
            "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d coreaudio -d %s "
            "-r %g -p %d -n %d",
            devname.c_str(), audiodevice.srate, audiodevice.periodsize,
            audiodevice.numperiods);
    std::cout << "[ov_render_tascar] Starting jack server" << std::endl;
#else
      sprintf(cmd,
              "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d alsa -d %s "
              "-r %g -p %d -n %d",
              devname.c_str(), audiodevice.srate, audiodevice.periodsize,
              audiodevice.numperiods);
#endif
    h_jack = new spawn_process_t(cmd);
    // replace sleep by testing for jack presence with timeout:
    sleep(7);
  }
  // get list of input ports:
  jack_client_t* jc;
  jack_options_t opt((jack_options_t)(JackNoStartServer));
  jack_status_t jstat;
  jc = jack_client_open("listchannels", opt, &jstat);
  if(jc) {
    std::vector<std::string> ports;
    const char** pp_ports(
        jack_get_ports(jc, NULL, NULL, JackPortIsOutput | JackPortIsPhysical));
    if(pp_ports) {
      const char** p(pp_ports);
      while(*p) {
        ports.push_back(*p);
        ++p;
      }
      jack_free(pp_ports);
    }
    jack_client_close(jc);
    inputports = ports;
  } else {
    inputports = {"system:capture_1", "system:capture_2"};
  }
}

void ov_render_tascar_t::stop_audiobackend()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::stop_audiobackend" << std::endl;
#endif
  ov_render_base_t::stop_audiobackend();
  if(h_jack) {
    delete h_jack;
    h_jack = NULL;
    // wait for jack to clean up properly:
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void ov_render_tascar_t::add_stage_device(const stage_device_t& stagedevice)
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::add_stage_device" << stagedevice.id
            << std::endl;
#endif
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::add_stage_device(stagedevice);
  if((p_stage != stage.stage) && is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::rm_stage_device" << stagedeviceid
            << std::endl;
#endif
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::rm_stage_device(stagedeviceid);
  if((p_stage != stage.stage) && is_session_active()) {
    end_session();
    start_session();
  }
}

void ov_render_tascar_t::set_stage(
    const std::map<stage_device_id_t, stage_device_t>& s)
{
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::set_stage(s);
  if((p_stage != stage.stage) && is_session_active()) {
    end_session();
    start_session();
  }
  // compare gains:
}

void ov_render_tascar_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                               double gain)
{
  ov_render_base_t::set_stage_device_gain(stagedeviceid, gain);
  if(is_session_active() && tascar) {
    uint32_t k = 0;
    for(auto ch : stage.stage[stagedeviceid].channels) {
      gain = ch.gain * stage.stage[stagedeviceid].gain;
      if(stagedeviceid == stage.thisstagedeviceid) {
        gain *= stage.rendersettings.egogain;
      } else {
        // if not self-monitor then decrease gain:
        gain *= 0.6;
      }
      std::string pattern("/" + stage.thisdeviceid + "/" +
                          get_stagedev_name(stagedeviceid) + "/" +
                          TASCAR::to_string(k));
      std::vector<TASCAR::Scene::audio_port_t*> port(
          tascar->find_audio_ports(std::vector<std::string>(1, pattern)));
      if(port.size())
        port[0]->set_gain_lin(gain);
      ++k;
    }
  }
}

void ov_render_tascar_t::set_stage_device_channel_gain(
    stage_device_id_t stagedeviceid, device_channel_id_t channeldeviceid,
    double gain)
{
  ov_render_base_t::set_stage_device_channel_gain(stagedeviceid,
                                                  channeldeviceid, gain);
  if(is_session_active() && tascar)
    tascar->sound_by_id(channeldeviceid).set_gain_lin(gain);
}

void ov_render_tascar_t::set_stage_device_channel_position(
    stage_device_id_t stagedeviceid, device_channel_id_t channeldeviceid,
    pos_t position, zyx_euler_t orientation)
{
  ov_render_base_t::set_stage_device_channel_position(
      stagedeviceid, channeldeviceid, position, orientation);
  if(is_session_active() && tascar) {
    tascar->sound_by_id(channeldeviceid).local_position =
        TASCAR::pos_t(position.x, position.y, position.z);
    tascar->sound_by_id(channeldeviceid).local_orientation =
        TASCAR::zyx_euler_t(orientation.z, orientation.y, orientation.x);
  }
}

void ov_render_tascar_t::set_render_settings(
    const render_settings_t& rendersettings,
    stage_device_id_t thisstagedeviceid)
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::set_render_settings " << rendersettings.id
            << ", thisstagedeviceId: " << thisstagedeviceid << std::endl;
#endif
  if((rendersettings != stage.rendersettings) ||
     (thisstagedeviceid != stage.thisstagedeviceid)) {
    ov_render_base_t::set_render_settings(rendersettings, thisstagedeviceid);
    if(is_session_active()) {
      end_session();
      start_session();
    }
  }
}

std::string ov_render_tascar_t::get_stagedev_name(stage_device_id_t id) const
{
  auto stagedev(stage.stage.find(id));
  if(stagedev == stage.stage.end())
    return "";
  return stagedev->second.label + "_" + TASCAR::to_string(id);
}

void ov_render_tascar_t::getbitrate(double& txrate, double& rxrate)
{
  if(ovboxclient)
    ovboxclient->getbitrate(txrate, rxrate);
}

std::vector<std::string> ov_render_tascar_t::get_input_channel_ids() const
{
  return inputports;
}

double ov_render_tascar_t::get_load() const
{
  if(tascar) {
    return 0.01 * tascar->get_cpu_load();
  }
  return 0;
}

void ov_render_tascar_t::set_extra_config(const std::string& js)
{
  try {
    if(!js.empty()) {
      bool restart_session(false);
      // parse extra configuration:
      nlohmann::json xcfg(nlohmann::json::parse(js));
      if(xcfg["headtrack"].is_object())
        headtrack_tauref = my_js_value(xcfg["headtrack"], "tauref", 33.315);
      if(xcfg["monitor"].is_object())
        selfmonitor_delay = my_js_value(xcfg["monitor"], "delay", 0.0);
      if(xcfg["metronome"].is_object()) {
        metronome_t newmetro(xcfg["metronome"]);
        if(newmetro != metronome) {
          if(metronome.delay != newmetro.delay)
            restart_session = true;
          metronome = newmetro;
          if(is_session_active()) {
            metronome.update_osc(tascar, stage.thisdeviceid);
          }
        }
      }
      if(xcfg["proxy"].is_object()) {
        bool cfg_is_proxy = my_js_value(xcfg["proxy"], "isproxy", false);
        bool cfg_use_proxy = my_js_value(xcfg["proxy"], "useproxy", false);
        if((cfg_is_proxy != is_proxy) || (cfg_use_proxy != use_proxy))
          restart_session = true;
        is_proxy = cfg_is_proxy;
        use_proxy = cfg_use_proxy;
      }
      if(is_session_active() && restart_session) {
        end_session();
        start_session();
      }
    }
  }
  catch(const std::exception& e) {
    throw TASCAR::ErrMsg(std::string("set_extra_config: ") + e.what());
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
