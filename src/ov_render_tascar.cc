/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2021 Giso Grimm, Tobias Hegemann
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

#include "ov_render_tascar.h"
#include "soundcardtools.h"
#include <fstream>
#include <iostream>
#include <jack/jack.h>

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
      headtrack_tauref(33.315), zitapath(ZITAPATH), is_proxy(false),
      use_proxy(false), cb_seqerr(nullptr), cb_seqerr_data(nullptr),
      sorter_deadline(5.0), expedited_forwarding_PHB(false),
      render_soundscape(true), jackrec_fileformat("WAV"),
      jackrec_sampleformat("PCM_16")
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
  localip = ep2ipstr(getipaddr());
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

void ov_render_tascar_t::set_allow_systemmods(bool allow)
{
  allow_systemmods = allow;
}

void ov_render_tascar_t::add_secondary_bus(const stage_device_t& stagemember,
                                           tsccfg::node_t& e_mods,
                                           tsccfg::node_t& e_session,
                                           std::vector<std::string>& waitports,
                                           const std::string& zitapath,
                                           const std::string& chanlist)
{
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  std::string clientname(get_stagedev_name(stagemember.id) + "_sec");
  std::string netclientname("n2j_" + std::to_string(stagemember.id) + "_sec");
  tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
  double buff(thisdev.receiverjitter + stagemember.senderjitter);
  tsccfg::node_set_attribute(
      e_sys, "command",
      zitapath + "zita-n2j --chan " + chanlist + " --jname " + netclientname +
          "." + stage.thisdeviceid + " --buf " +
          TASCAR::to_string(stage.rendersettings.secrec + buff) + " 0.0.0.0 " +
          TASCAR::to_string(4464 + 2 * stagemember.id + 100));
  tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
  // create also a route with correct gain settings:
  tsccfg::node_t e_route(tsccfg::node_add_child(e_mods, "route"));
  tsccfg::node_set_attribute(e_route, "name", clientname);
  tsccfg::node_set_attribute(e_route, "channels",
                             std::to_string(stagemember.channels.size()));
  tsccfg::node_set_attribute(e_route, "gain",
                             TASCAR::to_string(20 * log10(stagemember.gain)));
  // tsccfg::node_set_attribute(e_route, "connect", netclientname +
  // ":out_[0-9]*");
  for(size_t c = 0; c < stagemember.channels.size(); ++c) {
    if(stage.thisstagedeviceid != stagemember.id) {
      std::string srcport(netclientname + "." + stage.thisdeviceid + ":out_" +
                          std::to_string(c + 1));
      std::string destport(clientname + ":in." + std::to_string(c));
      waitports.push_back(srcport);
      session_add_connect(e_session, srcport, destport);
    }
  }
}

void ov_render_tascar_t::add_network_receiver(
    const stage_device_t& stagemember, tsccfg::node_t& e_mods,
    tsccfg::node_t& e_session, std::vector<std::string>& waitports,
    uint32_t& chcnt)
{
  if(!stage.rendersettings.receive)
    return;
  // only create a network receiver when the stage member is sending audio:
  stage_device_t& thisdev(stage.stage[stage.thisstagedeviceid]);
  std::string chanlist;
  for(uint32_t k = 0; k < stagemember.channels.size(); ++k) {
    if(k)
      chanlist += ",";
    chanlist += std::to_string(k + 1);
  }
  if(stagemember.senddownmix)
    chanlist = "1,2";
  // do not create a network receiver for local device:
  if((stage.thisstagedeviceid != stagemember.id) &&
     (stagemember.senddownmix == thisdev.receivedownmix)) {
    std::string clientname(get_stagedev_name(stagemember.id));
    std::string n2jclientname(get_stagedev_name(stagemember.id) + "." +
                              stage.thisdeviceid);
    if(stage.rendersettings.rawmode || stage.thisdevice.receivedownmix) {
      n2jclientname = "n2j_" + n2jclientname;
    }
    tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
    double buff(thisdev.receiverjitter + stagemember.senderjitter);
    // provide access to path!
    tsccfg::node_set_attribute(
        e_sys, "command",
        zitapath + "zita-n2j --chan " + chanlist + " --jname " + n2jclientname +
            " --buf " + TASCAR::to_string(buff) + " 0.0.0.0 " +
            TASCAR::to_string(4464 + 2 * stagemember.id));
    tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-n2j");
    if(stage.rendersettings.rawmode || stage.thisdevice.receivedownmix) {
      // create additional route for gain control:
      tsccfg::node_t e_route = tsccfg::node_add_child(e_mods, "route");
      tsccfg::node_set_attribute(e_route, "name",
                                 clientname + "." + stage.thisdeviceid);
      size_t memchannels(stagemember.channels.size());
      if(stagemember.senddownmix)
        memchannels = 2u;
      tsccfg::node_set_attribute(e_route, "channels",
                                 std::to_string(memchannels));
      tsccfg::node_set_attribute(
          e_route, "gain", TASCAR::to_string(20 * log10(stagemember.gain)));
      tsccfg::node_set_attribute(e_route, "connect", n2jclientname + ":out_.*");
      for(size_t c = 0; c < memchannels; ++c) {
        ++chcnt;
        if(stage.thisstagedeviceid != stagemember.id) {
          std::string srcport(clientname + "." + stage.thisdeviceid + ":out." +
                              std::to_string(c));
          std::string destport;
          if(chcnt & 1)
            destport = "master." + stage.thisdeviceid + ":in.0";
          else
            destport = "master." + stage.thisdeviceid + ":in.1";
          if(!destport.empty()) {
            waitports.push_back(srcport);
            session_add_connect(e_session, srcport, destport);
          }
          waitports.push_back(n2jclientname + ":out_" + std::to_string(c + 1));
        }
      }
    } else {
      // not in raw mode:
      // create connections:
      for(size_t c = 0; c < stagemember.channels.size(); ++c) {
        if(stage.thisstagedeviceid != stagemember.id) {
          std::string srcport(n2jclientname + ":out_" + std::to_string(c + 1));
          std::string destport("render." + stage.thisdeviceid + ":" +
                               clientname + "." + std::to_string(c) + ".0");
          waitports.push_back(srcport);
          session_add_connect(e_session, srcport, destport);
        }
      }
    }
    if(stage.rendersettings.secrec > 0) {
      // create a secondary network receiver with additional jitter buffer:
      if(stage.thisstagedeviceid != stagemember.id) {
        add_secondary_bus(stagemember, e_mods, e_session, waitports, zitapath,
                          chanlist);
      }
    }
  }
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
  if(thisdev.senddownmix)
    b_sender = false;
  if(stage.rendersettings.receive) {
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
    double daz(stagewidth / (stage.stage.size() - (!b_sender)) *
               (M_PI / 180.0));
    az = az * (M_PI / 180.0) - 0.5 * daz;
    double radius(1.2);
    // create sound sources:
    for(auto stagemember : stage.stage) {
      if(stagemember.second.channels.size()) {
        // create sound source only for sending devices:
        if((b_sender || (stagemember.second.id != thisdev.id)) &&
           ((!stagemember.second.senddownmix) ||
            (stagemember.second.id == thisdev.id))) {
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
            tsccfg::node_set_attribute(
                e_src, "name", get_stagedev_name(stagemember.second.id));
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
            if(!stage.rendersettings.distancelaw)
              tsccfg::node_set_attribute(e_snd, "gainmodel", "1");
            tsccfg::node_set_attribute(e_snd, "delayline", "false");
            tsccfg::node_set_attribute(e_snd, "id", ch.id);
            double gain(ch.gain * stagemember.second.gain);
            if(stagemember.second.id == thisdev.id) {
              // connect self-monitoring source ports:
              tsccfg::node_set_attribute(e_snd, "connect", ch.sourceport);
              gain *= stage.rendersettings.egogain;
            } else {
              if(!stage.rendersettings.distancelaw)
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
               selfmonitor_onlyreverb) {
              tsccfg::node_set_attribute(e_snd, "layers", "2");
            }
            if((stagemember.second.id == thisdev.id) &&
               (selfmonitor_delay > 0.0)) {
              tsccfg::node_t e_plugs(tsccfg::node_add_child(e_snd, "plugins"));
              tsccfg::node_t e_delay(tsccfg::node_add_child(e_plugs, "delay"));
              tsccfg::node_set_attribute(
                  e_delay, "delay",
                  TASCAR::to_string(selfmonitor_delay * 0.001));
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
          TASCAR::to_string(sqrt(1.0 - stage.rendersettings.absorption)));
      tsccfg::node_set_attribute(
          e_rvb, "damping", TASCAR::to_string(stage.rendersettings.damping));
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
      tsccfg::node_set_attribute(
          e_rvb, "damping", TASCAR::to_string(stage.rendersettings.damping));
      tsccfg::node_set_attribute(
          e_rvb, "gain",
          TASCAR::to_string(20 * log10(stage.rendersettings.reverbgain)));
    }
    // ambient sounds:
    if(stage.rendersettings.ambientsound.size() && render_soundscape) {
      std::string hashname(
          url2localfilename(stage.rendersettings.ambientsound));
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
            e_snd, "level",
            TASCAR::to_string(stage.rendersettings.ambientlevel));
        tsccfg::node_set_attribute(e_snd, "loop", "0");
        tsccfg::node_set_attribute(e_snd, "transport", "false");
        tsccfg::node_set_attribute(e_snd, "channelorder", "FuMa");
        tsccfg::node_set_attribute(e_snd, "license", "CC0");
        tsccfg::node_set_attribute(e_snd, "resample", "true");
      }
    }
  }
  // configure extra modules:
  tsccfg::node_t e_mods(tsccfg::node_add_child(e_session, "modules"));
  tsccfg::node_t e_jackrec(tsccfg::node_add_child(e_mods, "jackrec"));
  tsccfg::node_set_attribute(e_jackrec, "url", "osc.udp://localhost:9000/");
  tsccfg::node_set_attribute(e_jackrec, "sampleformat", jackrec_sampleformat);
  tsccfg::node_set_attribute(e_jackrec, "fileformat", jackrec_fileformat);
  tsccfg::node_set_attribute(e_jackrec, "pattern", "rec*.*");
  tsccfg::node_add_child(e_mods, "touchosc");
  // create zita-n2j receivers:
  // this variable holds the path to zita
  // binaries, or empty (default) for system installed:
  uint32_t chcnt(0);
  for(auto stagemember : stage.stage) {
    if(stagemember.second.channels.size()) {
      add_network_receiver(stagemember.second, e_mods, e_session, waitports,
                           chcnt);
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
  if((thisdev.channels.size() > 0) && (!stage.thisdevice.senddownmix)) {
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
      if(stage.rendersettings.receive)
        session_add_connect(e_session, stage.thisdeviceid + ".metronome:out.1",
                            "render." + stage.thisdeviceid + ":ego." +
                                std::to_string(chn - 1) + ".0");
      waitports.push_back(stage.thisdeviceid + "_sender:in_" +
                          std::to_string(chn));
    }
  }
  if(stage.thisdevice.senddownmix && stage.rendersettings.receive) {
    // create network sender:
    tsccfg::node_t e_sys(tsccfg::node_add_child(e_mods, "system"));
    tsccfg::node_set_attribute(
        e_sys, "command",
        zitapath + "zita-j2n --chan " + std::to_string(2) + " --jname " +
            stage.thisdeviceid + "_sender --16bit 127.0.0.1 " +
            std::to_string(4464 + 2 * stage.thisstagedeviceid));
    tsccfg::node_set_attribute(e_sys, "onunload", "killall zita-j2n");
    session_add_connect(e_session, "render." + stage.thisdeviceid + ":master_l",
                        stage.thisdeviceid + "_sender:in_1");
    session_add_connect(e_session, "render." + stage.thisdeviceid + ":master_r",
                        stage.thisdeviceid + "_sender:in_2");
  }
  tsccfg::node_t e_wait = tsccfg::node_add_child(e_mods, "waitforjackport");
  tsccfg::node_set_attribute(e_wait, "timeout", "5");
  tsccfg::node_set_attribute(e_wait, "name",
                             stage.thisdeviceid + ".waitforports");
  for(auto port : waitports) {
    tsccfg::node_t e_p = tsccfg::node_add_child(e_wait, "port");
    tsccfg::node_set_text(e_p, port);
  }
  tsccfg::node_add_child(e_mods, "sleep");
  // head tracking:
  if(stage.rendersettings.headtracking && stage.rendersettings.receive) {
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
  tsccfg::node_set_attribute(e_jackrec, "sampleformat", jackrec_sampleformat);
  tsccfg::node_set_attribute(e_jackrec, "fileformat", jackrec_fileformat);
  tsccfg::node_set_attribute(e_jackrec, "pattern", "rec*.*");
  tsccfg::node_add_child(e_mods, "touchosc");
  if(stage.rendersettings.receive) {
    tsccfg::node_t e_route = tsccfg::node_add_child(e_mods, "route");
    std::string clname("master." + stage.thisdeviceid);
    tsccfg::node_set_attribute(e_route, "name", clname);
    tsccfg::node_set_attribute(e_route, "channels", "2");
    tsccfg::node_set_attribute(
        e_route, "gain",
        TASCAR::to_string(20 * log10(stage.rendersettings.mastergain)));
    if(stage.rendersettings.outputport1.size())
      session_add_connect(e_session, clname + ":out.0",
                          stage.rendersettings.outputport1);
    if(stage.rendersettings.outputport2.size())
      session_add_connect(e_session, clname + ":out.1",
                          stage.rendersettings.outputport2);
  }
  //
  uint32_t chcnt(0);
  for(auto stagemember : stage.stage) {
    add_network_receiver(stagemember.second, e_mods, e_session, waitports,
                         chcnt);
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
            std::to_string(thisdev.channels.size()) + " --jname " +
            stage.thisdeviceid + "_sender --16bit 127.0.0.1 " +
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
  tsccfg::node_set_attribute(e_wait, "name",
                             stage.thisdeviceid + ".waitforports");
  for(auto port : waitports) {
    tsccfg::node_t e_p = tsccfg::node_add_child(e_wait, "port");
    tsccfg::node_set_text(e_p, port);
  }
  tsccfg::node_add_child(e_mods, "sleep");
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
  //#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::start_session" << std::endl;
  //#endif
  // do whatever needs to be done in base class:
  ov_render_base_t::start_session();
  // create a short link to this device:
  // xml code for TASCAR configuration:
  TASCAR::xml_doc_t tsc;
  // default TASCAR session settings:
  tsccfg::node_set_name(tsc.root(), "session");
  tsccfg::node_t e_session(tsc.root());
  tsccfg::node_set_attribute(e_session, "srv_port", "9871");
  tsccfg::node_set_attribute(e_session, "duration", "36000");
  tsccfg::node_set_attribute(e_session, "name", stage.thisdeviceid);
  tsccfg::node_set_attribute(e_session, "license", "CC0");
  tsccfg::node_set_attribute(e_session, "levelmeter_tc",
                             TASCAR::to_string(stage.rendersettings.lmetertc));
  tsccfg::node_set_attribute(e_session, "levelmeter_weight",
                             stage.rendersettings.lmeterfw);
  // create a virtual acoustics "scene":
  tsccfg::node_t e_scene(tsccfg::node_add_child(e_session, "scene"));
  tsccfg::node_set_attribute(e_scene, "name", stage.thisdeviceid);
  // create virtual acoustics only when not in raw mode:
  if(!(stage.rendersettings.rawmode || stage.thisdevice.receivedownmix)) {
    tsccfg::node_t e_rec(NULL);
    if(stage.rendersettings.receive) {
      // add a main receiver for which the scene is rendered:
      e_rec = tsccfg::node_add_child(e_scene, "receiver");
      // receiver can be "hrtf" or "ortf" (more receivers are possible in
      // ->add_child_text(TASCAR, but only these two can produce audio which is
      // headphone-compatible):
      tsccfg::node_set_attribute(e_rec, "type", stage.rendersettings.rectype);
      tsccfg::node_set_attribute(e_rec, "layers", "1");
      if(stage.rendersettings.rectype == "ortf") {
        tsccfg::node_set_attribute(e_rec, "angle", "140");
        tsccfg::node_set_attribute(e_rec, "f6db", "12000");
        tsccfg::node_set_attribute(e_rec, "fmin", "3000");
      }
      if(stage.rendersettings.decorr > 0) {
        tsccfg::node_set_attribute(e_rec, "decorr", "true");
        tsccfg::node_set_attribute(
            e_rec, "decorr_length",
            TASCAR::to_string(0.001 * stage.rendersettings.decorr));
      }
      tsccfg::node_set_attribute(e_rec, "name", "master");
      tsccfg::node_set_attribute(
          e_rec, "gain",
          TASCAR::to_string(20 * log10(stage.rendersettings.mastergain)));
      // do not add delay (usually used for dynamic scene rendering):
      tsccfg::node_set_attribute(
          e_rec, "delaycomp",
          TASCAR::to_string(stage.rendersettings.delaycomp / 340.0));
      // connect output ports:
      if(!stage.thisdevice.senddownmix) {
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
      }
    }
    // the host is not empty when this device is on a stage:
    if(!stage.host.empty()) {
      create_virtual_acoustics(e_session, e_rec, e_scene);
    } else {
      if(stage.rendersettings.receive) {
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
      }
      tsccfg::node_t e_mods(tsccfg::node_add_child(e_session, "modules"));
      tsccfg::node_t e_jackrec = tsccfg::node_add_child(e_mods, "jackrec");
      tsccfg::node_set_attribute(e_jackrec, "url", "osc.udp://localhost:9000/");
      tsccfg::node_set_attribute(e_jackrec, "sampleformat",
                                 jackrec_sampleformat);
      tsccfg::node_set_attribute(e_jackrec, "fileformat", jackrec_fileformat);
      tsccfg::node_set_attribute(e_jackrec, "pattern", "rec*.*");
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
        use_proxy || (!stage.rendersettings.receive),
        stage.thisdevice.receivedownmix,
        stage.stage[stage.thisstagedeviceid].sendlocal, sorter_deadline,
        stage.thisdevice.senddownmix, use_proxy);
    if(cb_seqerr)
      ovboxclient->set_seqerr_callback(cb_seqerr, cb_seqerr_data);
    if(stage.rendersettings.secrec > 0)
      ovboxclient->add_extraport(100);
    for(auto p : stage.rendersettings.xrecport)
      ovboxclient->add_receiverport(p, p);
    ovboxclient->add_receiverport(9870, 9871);
    if(pinglogaddr)
      ovboxclient->set_ping_callback(sendpinglog, pinglogaddr);
    for(auto proxyclient : proxyclients) {
      ovboxclient->add_proxy_client(proxyclient.first, proxyclient.second);
    }
  }
  if(mczita) {
    std::vector<std::string> waitports;
    tsccfg::node_t e_mods = tsccfg::node_add_child(e_session, "modules");
    tsccfg::node_t e_zit = tsccfg::node_add_child(e_mods, "system");
    std::map<uint32_t, uint32_t> chmap;
    uint32_t och(0);
    for(auto ch : mczitachannels) {
      ++och;
      chmap[ch] = och;
    }
    std::string chlist;
    och = 0;
    for(auto ch : chmap) {
      ++och;
      chlist += std::to_string(ch.first) + ",";
      waitports.push_back("n2j_" + stage.thisdeviceid + "_mc:out_" +
                          std::to_string(ch.first));
      session_add_connect(e_session,
                          "n2j_" + stage.thisdeviceid + "_mc:out_" +
                              std::to_string(ch.first),
                          "system:playback_" + std::to_string(ch.second));
    }
    if(chlist.size())
      chlist.erase(chlist.size() - 1, 1);
    std::string cmd = zitapath + "zita-n2j --chan " + chlist + " --buff " +
                      std::to_string(mczitabuffer) + " --jname " + "n2j_" +
                      stage.thisdeviceid + "_mc " + mczitaaddr + " " +
                      std::to_string(mczitaport) + " " + mczitadevice;
    DEBUG(cmd);
    tsccfg::node_set_attribute(e_zit, "command", cmd);
    tsccfg::node_set_attribute(e_zit, "onunload", "killall zita-n2j");
    tsccfg::node_t e_wait = tsccfg::node_add_child(e_mods, "waitforjackport");
    tsccfg::node_set_attribute(e_wait, "name",
                               stage.thisdeviceid + ".waitforportsmc");
    for(auto port : waitports) {
      tsccfg::node_t e_p = tsccfg::node_add_child(e_wait, "port");
      tsccfg::node_set_text(e_p, port);
    }
  }
  if(tscinclude.size()) {
    tsccfg::node_t e_inc(tsccfg::node_add_child(e_session, "include"));
    tsccfg::node_set_attribute(e_inc, "name", stage.thisdeviceid + ".itsc");
    TASCAR::xml_doc_t itsc(tscinclude, TASCAR::xml_doc_t::LOAD_STRING);
    if(!allow_systemmods) {
      auto modnodes(tsccfg::node_get_children(itsc.root(), "modules"));
      for(auto mod : modnodes) {
        auto sysnodes(tsccfg::node_get_children(mod, "system"));
        for(auto sys : sysnodes)
          tsccfg::node_remove_child(mod, sys);
      }
    }
    itsc.save(stage.thisdeviceid + ".itsc");
    // std::ofstream ofh(stage.thisdeviceid + ".itsc");
    // ofh << tscinclude;
  }
  tsc.save(folder + "ovbox_debugsession.tsc");
  tascar = new TASCAR::session_t(tsc.save_to_string(),
                                 TASCAR::session_t::LOAD_STRING, "");
  try {
    tascar->start();
  }
  catch(const std::exception& e) {
    DEBUG(e.what());
    std::string err(e.what());
    delete tascar;
    tascar = NULL;
    if(ovboxclient)
      delete ovboxclient;
    ovboxclient = NULL;
    // end_session();
    throw ErrMsg(err);
  }
#ifndef GUI
  // add web mixer tools (node-js server and touchosc interface):
  std::string command;
  std::string ipaddr(ep2ipstr(getipaddr()));
  ipaddr += " '";
  ipaddr += stage.thisdeviceid + " (" + stage.thisdevice.label + ")'";
  if(file_exists("/usr/share/ovclient/webmixer.js")) {
    command = "(cd /usr/share/ovclient/ && node webmixer.js " + ipaddr + ")";
  }
  if(file_exists("webmixer.js")) {
    command = "node webmixer.js " + ipaddr;
  }
  if(!command.empty())
    h_webmixer = new spawn_process_t(command);
#endif
  session_ready = true;
}

void ov_render_tascar_t::end_session()
{
#ifdef SHOWDEBUG
  std::cout << "ov_render_tascar_t::end_session" << std::endl;
#endif
  session_ready = false;
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
    char cmd[1024];
    if((audiodevice.devicename != "dummy") &&
       (audiodevice.devicename != "plugdummy")) {
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
      sprintf(cmd,
              "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d alsa -d %s "
              "-r %g -p %d -n %d",
              devname.c_str(), audiodevice.srate, audiodevice.periodsize,
              audiodevice.numperiods);
    } else {
      sprintf(cmd,
              "JACK_NO_AUDIO_RESERVATION=1 jackd --sync -P 40 -d dummy "
              "-r %g -p %d",
              audiodevice.srate, audiodevice.periodsize);
    }
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
    require_session_restart();
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
    require_session_restart();
  }
}

void ov_render_tascar_t::set_stage(
    const std::map<stage_device_id_t, stage_device_t>& s)
{
  // compare with current stage:
  auto p_stage(stage.stage);
  ov_render_base_t::set_stage(s);
  if((p_stage != stage.stage) && is_session_active()) {
    require_session_restart();
  }
  // compare gains:
}

void ov_render_tascar_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                               double gain)
{
  DEBUG(gain);
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
      DEBUG(pattern);
      DEBUG(port.size());
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
    require_session_restart();
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

std::string ov_render_tascar_t::get_zita_path()
{
  return zitapath;
}

void ov_render_tascar_t::set_zita_path(const std::string& path)
{
  zitapath = path;
}

void ov_render_tascar_t::set_extra_config(const std::string& js)
{
  try {
    if(!js.empty()) {
      bool restart_session(false);
      // parse extra configuration:
      nlohmann::json xcfg(nlohmann::json::parse(js));
      std::string prev_tscinclude(tscinclude);
      tscinclude = my_js_value(xcfg, "tscinclude", tscinclude);
      if(prev_tscinclude != tscinclude)
        restart_session = true;
      if(xcfg["network"].is_object()) {
        double new_deadline =
            my_js_value(xcfg["network"], "deadline", sorter_deadline);
        if(new_deadline != sorter_deadline) {
          sorter_deadline = new_deadline;
          if(ovboxclient)
            ovboxclient->set_reorder_deadline(sorter_deadline);
        }
        bool new_expedited_forwarding_PHB = my_js_value(
            xcfg["network"], "expeditedforwarding", expedited_forwarding_PHB);
        if(new_expedited_forwarding_PHB != expedited_forwarding_PHB) {
          expedited_forwarding_PHB = new_expedited_forwarding_PHB;
          if(expedited_forwarding_PHB && ovboxclient)
            ovboxclient->set_expedited_forwarding_PHB();
        }
      }
      if(xcfg["headtrack"].is_object())
        headtrack_tauref = my_js_value(xcfg["headtrack"], "tauref", 33.315);
      if(xcfg["monitor"].is_object()) {
        selfmonitor_delay = my_js_value(xcfg["monitor"], "delay", 0.0);
        bool new_onlyreverb = my_js_value(xcfg["monitor"], "onlyreverb", false);
        if(new_onlyreverb != selfmonitor_onlyreverb) {
          selfmonitor_onlyreverb = new_onlyreverb;
          restart_session = true;
        }
      }
      if(xcfg["render"].is_object()) {
        bool prev(render_soundscape);
        render_soundscape = my_js_value(xcfg["render"], "soundscape", true);
        if(prev != render_soundscape)
          restart_session = true;
      }
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
      if(xcfg["jackrec"].is_object()) {
        std::string new_jackrec_sampleformat =
            my_js_value(xcfg["jackrec"], "sampleformat", std::string("PCM_16"));
        if(new_jackrec_sampleformat != jackrec_sampleformat) {
          jackrec_sampleformat = new_jackrec_sampleformat;
          restart_session = true;
        }
        std::string new_jackrec_fileformat =
            my_js_value(xcfg["jackrec"], "fileformat", std::string("WAV"));
        if(new_jackrec_fileformat != jackrec_fileformat) {
          jackrec_fileformat = new_jackrec_fileformat;
          restart_session = true;
        }
      }
      if(xcfg["proxy"].is_object()) {
        bool cfg_is_proxy = my_js_value(xcfg["proxy"], "isproxy", false);
        bool cfg_use_proxy = my_js_value(xcfg["proxy"], "useproxy", false);
        std::string cfg_proxyip;
        if(cfg_use_proxy)
          cfg_proxyip = my_js_value(xcfg["proxy"], "proxyip", std::string(""));
        nlohmann::json js_proxyclients(xcfg["proxy"]["clients"]);
        std::map<stage_device_id_t, std::string> cfg_proxyclients;
        if(cfg_is_proxy && js_proxyclients.is_array()) {
          for(auto proxyclient : js_proxyclients) {
            int32_t cfg_proxy_id = my_js_value(proxyclient, "id", -1);
            std::string cfg_proxy_ip =
                my_js_value(proxyclient, "ip", std::string(""));
            if((cfg_proxy_id >= 0) && (cfg_proxy_id < MAX_STAGE_ID) &&
               cfg_proxy_ip.size() && (cfg_proxy_ip != localip))
              cfg_proxyclients[cfg_proxy_id] = cfg_proxy_ip;
          }
        }
        if((cfg_is_proxy != is_proxy) || (cfg_use_proxy != use_proxy) ||
           (cfg_proxyclients != proxyclients) || (cfg_proxyip != proxyip))
          restart_session = true;
        is_proxy = cfg_is_proxy;
        use_proxy = cfg_use_proxy;
        proxyclients = cfg_proxyclients;
        proxyip = cfg_proxyip;
      }
      if(xcfg["mcrec"].is_object()) {
        auto prev_mczita = mczita;
        mczita = my_js_value(xcfg["mcrec"], "use", mczita);
        auto prev_mczitaaddr = mczitaaddr;
        mczitaaddr = my_js_value(xcfg["mcrec"], "addr", mczitaaddr);
        auto prev_mczitaport = mczitaport;
        mczitaport = my_js_value(xcfg["mcrec"], "port", mczitaport);
        auto prev_mczitadevice = mczitadevice;
        mczitadevice = my_js_value(xcfg["mcrec"], "device", mczitadevice);
        auto prev_mczitabuffer = mczitabuffer;
        mczitabuffer = my_js_value(xcfg["mcrec"], "buffer", mczitabuffer);
        auto prev_mczitachannels = mczitachannels;
        mczitachannels = my_js_value(xcfg["mcrec"], "channels", mczitachannels);
        if((prev_mczita != mczita) || (prev_mczitaaddr != mczitaaddr) ||
           (prev_mczitaport != mczitaport) ||
           (prev_mczitadevice != mczitadevice) ||
           (prev_mczitabuffer != mczitabuffer) ||
           (prev_mczitachannels != mczitachannels))
          restart_session = true;
      }
      if(is_session_active() && restart_session) {
        require_session_restart();
      }
    }
  }
  catch(const std::exception& e) {
    throw TASCAR::ErrMsg(std::string("set_extra_config: ") + e.what());
  }
}

void ov_render_tascar_t::set_seqerr_callback(
    std::function<void(stage_device_id_t, sequence_t, sequence_t, port_t,
                       void*)>
        f,
    void* d)
{
  cb_seqerr = f;
  cb_seqerr_data = d;
}

nlohmann::json to_json(const ping_stat_t& ps)
{
  nlohmann::json p;
  p["min"] = ps.t_min;
  p["median"] = ps.t_med;
  p["p99"] = ps.t_p99;
  p["mean"] = ps.t_mean;
  p["received"] = ps.received;
  p["lost"] = ps.lost;
  return p;
}

nlohmann::json to_json(const message_stat_t& ms)
{
  nlohmann::json p;
  p["received"] = ms.received;
  p["lost"] = ms.lost;
  p["seqerr"] = ms.seqerr_in;
  p["seqrecovered"] = ms.seqerr_in - ms.seqerr_out;
  return p;
}

nlohmann::json to_json(const client_stats_t& ms)
{
  nlohmann::json p;
  p["p2p"] = to_json(ms.ping_p2p);
  p["srv"] = to_json(ms.ping_srv);
  p["loc"] = to_json(ms.ping_loc);
  p["packages"] = to_json(ms.packages);
  return p;
}

std::string ov_render_tascar_t::get_client_stats()
{
  if(ovboxclient)
    for(auto dev : stage.stage)
      ovboxclient->update_client_stats(dev.first, client_stats[dev.first]);
  else
    client_stats.clear();
  nlohmann::json jsstat;
  for(auto stat : client_stats)
    if(stat.first != stage.thisstagedeviceid)
      jsstat[stat.first] = to_json(stat.second);
  return jsstat.dump();
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
