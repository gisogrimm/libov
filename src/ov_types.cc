/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 2021 Giso Grimm
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

#include "ov_types.h"
#include "../tascar/libtascar/include/tscconfig.h"
#include <iostream>
#include <nlohmann/json.hpp>

// #define SHOWDEBUG

#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__      \
            << " " << #x << "=" << x << std::endl
#endif

#define DEBUGNEQ(a, b)                                                         \
  if(a != b)                                                                   \
  std::cerr << __FILE__ << ":" << __LINE__ << ": (" << #a << "==" << a         \
            << ")!=(" << #b << "==" << b << ")" << std::endl
#define DEBUGNEQ2(a, b)                                                        \
  if(a != b)                                                                   \
  std::cerr << __FILE__ << ":" << __LINE__ << ": (" << #a << ")!=(" << #b      \
            << ")" << std::endl

pos_t default_pos = {
    0.0, // x
    0.0, // y
    0.0, // z
};

pos_t default_roomsize = {
    7.5,  // x
    13.0, // y
    5.0,  // z
};

zyx_euler_t default_rot = {
    0.0,
    0.0,
    0.0,
};

stage_device_t default_device = {
    0,           // stage_device_id_t id
    "",          // uid
    "",          // std::string label;
    {},          // std::vector<device_channel_t> channels;
    default_pos, // pos_t position;
    default_rot, // zyx_euler_t orientation;
    1.0,         // double gain;
    false,       // bool mute;
    5.0,         // double senderjitter;
    5.0,         // double receiverjitter;
    true,        // bool sendlocal;
    false,       // bool receivedownmix;
    false,       // bool senddownmix;
    false,       // bool nozita:
    false,       // bool hiresping;
};

stage_t default_stage = {
    "",                  // std::string host;
    0,                   // port_t port;
    0,                   // secret_t pin;
    render_settings_t(), // render_settings_t rendersettings;
    "",                  // std::string thisdeviceid;
    0,                   // stage_device_id_t thisstagedeviceid;
    std::map<stage_device_id_t,
             stage_device_t>(), // std::map<stage_device_id_t,
                                // stage_device_t> stage;
    default_device,             // stage_device_t thisdevice;
};

bool operator!=(const pos_t& a, const pos_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const zyx_euler_t& a, const zyx_euler_t& b)
{
  return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
}

bool operator!=(const audio_device_t& a, const audio_device_t& b)
{
  if((a.drivername != b.drivername) || (a.devicename != b.devicename) ||
     (a.srate != b.srate) || (a.periodsize != b.periodsize) ||
     (a.numperiods != b.numperiods) || (a.priority != b.priority)) {
#ifdef SHOWDEBUG
    DEBUGNEQ(a.drivername, b.drivername);
    DEBUGNEQ(a.devicename, b.devicename);
    DEBUGNEQ(a.srate, b.srate);
    DEBUGNEQ(a.periodsize, b.periodsize);
    DEBUGNEQ(a.numperiods, b.numperiods);
    DEBUGNEQ(a.priority, b.priority);
#endif
    return true;
  }
  return false;
}

bool operator==(const channel_plugin_t& a, const channel_plugin_t& b)
{
  return (a.name == b.name) && (a.params == b.params);
}

bool operator!=(const device_channel_t& a, const device_channel_t& b)
{
  if(!(a.plugins == b.plugins)) {
    std::cout << "a=";
    for(const auto& pl : a.plugins) {
      std::cout << pl.name << ":[";
      for(const auto& par : pl.params) {
        std::cout << " " << par.first << ":" << par.second;
      }
      std::cout << " ] ";
    }
    std::cout << "\n";
    std::cout << "b=";
    for(const auto& pl : b.plugins) {
      std::cout << pl.name << ":[";
      for(const auto& par : pl.params) {
        std::cout << " " << par.first << ":" << par.second;
      }
      std::cout << " ] ";
    }
    std::cout << "\n";
  }
  return (a.sourceport != b.sourceport) || (a.gain != b.gain) ||
         (a.position != b.position) || (a.directivity != b.directivity) ||
         (a.name != b.name) || (!(a.plugins == b.plugins));
}

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b)
{
  if(a.size() != b.size()) {
#ifdef SHOWDEBUG
    DEBUGNEQ(a.size(), b.size());
#endif
    return true;
  }
  for(size_t k = 0; k < a.size(); ++k)
    if(a[k] != b[k]) {
#ifdef SHOWDEBUG
      DEBUGNEQ2(a[k], b[k]);
#endif
      return true;
    }
  return false;
}

bool operator!=(const stage_device_t& a, const stage_device_t& b)
{
#ifdef SHOWDEBUG
  DEBUGNEQ(a.id, b.id);
  DEBUGNEQ(a.label, b.label);
  DEBUGNEQ2(a.channels, b.channels);
  DEBUGNEQ2(a.position, b.position);
  DEBUGNEQ2(a.orientation, b.orientation);
  DEBUGNEQ(a.gain, b.gain);
  DEBUGNEQ(a.mute, b.mute);
  DEBUGNEQ(a.senderjitter, b.senderjitter);
  DEBUGNEQ(a.receiverjitter, b.receiverjitter);
  DEBUGNEQ(a.sendlocal, b.sendlocal);
#endif
  return (a.id != b.id) || (a.uid != b.uid) || (a.label != b.label) ||
         (a.channels != b.channels) || (a.position != b.position) ||
         (a.orientation != b.orientation) || (a.gain != b.gain) ||
         (a.mute != b.mute) || (a.senderjitter != b.senderjitter) ||
         (a.receiverjitter != b.receiverjitter) ||
         (a.sendlocal != b.sendlocal) ||
         (a.receivedownmix != b.receivedownmix) ||
         (a.senddownmix != b.senddownmix) || (a.nozita != b.nozita) ||
         (a.hiresping != b.hiresping);
}

bool operator!=(const render_settings_t& a, const render_settings_t& b)
{
  if((a.id != b.id) || (a.roomsize != b.roomsize) ||
     (a.absorption != b.absorption) || (a.damping != b.damping) ||
     (a.reverbgain != b.reverbgain) || (a.reverbgainroom != b.reverbgainroom) ||
     (a.reverbgaindev != b.reverbgaindev) ||
     (a.renderreverb != b.renderreverb) || (a.renderism != b.renderism) ||
     (a.distancelaw != b.distancelaw) || (a.rawmode != b.rawmode) ||
     (a.receive != b.receive) || (a.rectype != b.rectype) ||
     (a.egogain != b.egogain) || (a.outputgain != b.outputgain) ||
     (a.peer2peer != b.peer2peer) || (a.outputport1 != b.outputport1) ||
     (a.outputport2 != b.outputport2) || (a.secrec != b.secrec) ||
     (a.xports != b.xports) || (a.xrecport != b.xrecport) ||
     (a.headtracking != b.headtracking) ||
     (a.headtrackingserial != b.headtrackingserial) ||
     (a.headtrackingrotrec != b.headtrackingrotrec) ||
     (a.headtrackingrotsrc != b.headtrackingrotsrc) ||
     (a.headtrackingport != b.headtrackingport) ||
     (a.ambientsound != b.ambientsound) || (a.ambientlevel != b.ambientlevel) ||
     (a.lmetertc != b.lmetertc) || (a.lmeterfw != b.lmeterfw) ||
     (a.delaycomp != b.delaycomp) || (a.decorr != b.decorr) ||
     (a.usetcptunnel != b.usetcptunnel) || (a.encryption != b.encryption)) {
#ifdef SHOWDEBUG
    DEBUGNEQ(a.id, b.id);
    DEBUGNEQ2(a.roomsize, b.roomsize);
    DEBUGNEQ(a.reverbgain, b.reverbgain);
    DEBUGNEQ(a.reverbgainroom, b.reverbgainroom);
    DEBUGNEQ(a.reverbgaindev, b.reverbgaindev);
#endif
    return true;
  }
  return false;
}

render_settings_t::render_settings_t()
    : id(0),                              // stage_device_id_t id;
      roomsize(default_roomsize),         // pos_t roomsize;
      absorption(0.6f),                   // float absorption;
      damping(0.7f),                      // float damping;
      reverbgain(1.0f),                   // float reverbgain;
      reverbgainroom(1.0),                // float reverbgain;
      reverbgaindev(1.0),                 // float reverbgain;
      renderreverb(true),                 // bool renderreverb;
      renderism(false),                   // bool renderism;
      distancelaw(false), rawmode(false), // bool rawmode;
      receive(true),                      // bool receive
      rectype("hrtf"),                    // std::string rectype;
      egogain(1.0),                       // float egogain;
      outputgain(1.0),                    // float outputgain;
      peer2peer(true),                    // bool peer2peer;
      outputport1(""),                    // std::string outputport1;
      outputport2(""),                    // std::string outputport2;
      xports(
          std::unordered_map<std::string,
                             std::string>()), // std::unordered_map<std::string,
      // std::string> xports;
      xrecport(std::vector<port_t>()), // std::vector<port_t> xrecport;
      secrec(0),                       // float secrec;
      headtracking(false),             // bool headtracking;
      headtrackingrotrec(true),        // bool headtrackingrotrec;
      headtrackingrotsrc(true),        // bool headtrackingrotsrc;
      headtrackingport(0),             // port_t headtrackingport;
      ambientsound(""),                // std::string ambientsound;
      ambientlevel(50),                // float ambientlevel;
      lmetertc(0.5),                   // float levelmeter_tc;
      lmeterfw("Z"),                   // std::string lmeterfw;
      delaycomp(2.4f),                 // float delaycomp;
      decorr(0.0),                     // float decorr;
      usetcptunnel(false)              // bool usetcptunnel;
{
}

ov_render_base_t::ov_render_base_t(const std::string& deviceid)
    : audiodevice({"", "", 48000, 96, 2, 40}), stage(default_stage),
      session_active(false), audio_active(false), restart_needed(false)
{
  stage.thisdeviceid = deviceid;
}

void ov_render_base_t::start_session()
{
  session_active = true;
  restart_needed = false;
}

void ov_render_base_t::end_session()
{
  session_active = false;
}

void ov_render_base_t::restart_session_if_needed()
{
  if(restart_needed) {
    if(session_active)
      end_session();
    start_session();
  } else {
    if(!session_active)
      start_session();
  }
}

void ov_render_base_t::start_audiobackend()
{
  audio_active = true;
}

void ov_render_base_t::stop_audiobackend()
{
  audio_active = false;
}

bool ov_render_base_t::is_session_active() const
{
  return session_active;
}

bool ov_render_base_t::is_audio_active() const
{
  return audio_active;
}

const std::string& ov_render_base_t::get_deviceid() const
{
  return stage.thisdeviceid;
}

void ov_render_base_t::update_plugincfg(const std::string& cfg, size_t channel)
{
  if(channel < get_num_inputs()) {
    stage.thisdevice.channels[channel].update_plugin_cfg(cfg);
    if(stage.stage.size() > stage.thisstagedeviceid)
      stage.stage[stage.thisstagedeviceid].channels[channel].update_plugin_cfg(
          cfg);
  }
}

/**
   \brief Update audio configuration, and start backend if changed or not yet
   started.

 */
void ov_render_base_t::configure_audio_backend(
    const audio_device_t& audiodevice_)
{
  // audio backend changed or was not active before:
  if((audiodevice != audiodevice_) || (!audio_active)) {
    // ignore other parameters if set to "manual":
    bool no_restart = (audiodevice.devicename == "manual") &&
                      (audiodevice_.devicename == "manual");
    audiodevice = audiodevice_;
    // audio was active before, so we need to restart:
    if((!no_restart) && audio_active) {
      if(session_active) {
        require_session_restart();
        end_session();
      }
      stop_audiobackend();
    }
    // now start audio:
    start_audiobackend();
  }
}

void ov_render_base_t::set_thisdev(const stage_device_t& stagedevice)
{
  if(stagedevice != stage.thisdevice)
    require_session_restart();
  stage.thisdevice = stagedevice;
}

void ov_render_base_t::add_stage_device(const stage_device_t& stagedevice)
{
  stage.stage[stagedevice.id] = stagedevice;
}

void ov_render_base_t::set_stage(
    const std::map<stage_device_id_t, stage_device_t>& s)
{
  stage.stage = s;
}

void ov_render_base_t::clear_stage()
{
  stage.stage.clear();
}

void ov_render_base_t::rm_stage_device(stage_device_id_t stagedeviceid)
{
  stage.stage.erase(stagedeviceid);
}

void ov_render_base_t::set_stage_device_gain(
    const stage_device_id_t& stagedeviceid, float gain)
{
  if(stage.stage.find(stagedeviceid) != stage.stage.end())
    stage.stage[stagedeviceid].gain = gain;
}

void ov_render_base_t::set_render_settings(
    const render_settings_t& rendersettings,
    stage_device_id_t thisstagedeviceid)
{
  stage.rendersettings = rendersettings;
  stage.thisstagedeviceid = thisstagedeviceid;
}

void ov_render_base_t::set_relay_server(const std::string& host, port_t port,
                                        secret_t pin)
{
  bool restart(false);
  if(is_session_active() &&
     ((stage.host != host) || (stage.port != port) || (stage.pin != pin)))
    restart = true;
  stage.host = host;
  stage.port = port;
  stage.pin = pin;
  if(restart) {
    end_session();
    require_session_restart();
  }
}

bool ov_render_base_t::need_restart() const
{
  return restart_needed;
}

void ov_render_base_t::require_session_restart()
{
  restart_needed = true;
}

void ov_render_base_t::getbitrate(double& txrate, double& rxrate) {}

const char* get_libov_version()
{
  return OVBOXVERSION;
}

bool operator!=(const std::map<stage_device_id_t, stage_device_t>& a,
                const std::map<stage_device_id_t, stage_device_t>& b)
{
#ifdef SHOWDEBUG
  DEBUGNEQ(a.size(), b.size());
#endif
  if(a.size() != b.size())
    return true;
  auto a_it = a.begin();
  auto b_it = b.begin();
  while((a_it != a.end()) && (b_it != b.end())) {
#ifdef SHOWDEBUG
    DEBUGNEQ(a_it->first, b_it->first);
#endif
    if(a_it->first != b_it->first)
      return true;
#ifdef SHOWDEBUG
    DEBUGNEQ2(a_it->second, b_it->second);
#endif
    if(a_it->second != b_it->second)
      return true;
    ++a_it;
    ++b_it;
  }
  return false;
}

void ov_render_base_t::set_stage_device_channel_gain(
    const stage_device_id_t& stagedeviceid,
    const device_channel_id_t& channeldeviceid, float gain)
{
  DEBUG(stagedeviceid);
  DEBUG(channeldeviceid);
  DEBUG(gain);
}

void ov_render_base_t::set_stage_device_position(
    const stage_device_id_t& stagedeviceid, const pos_t& position,
    const zyx_euler_t& orientation)
{
  DEBUG(stagedeviceid);
}

void ov_render_base_t::set_stage_device_channel_position(
    const stage_device_id_t& stagedeviceid,
    const device_channel_id_t& channeldeviceid, const pos_t& position,
    const zyx_euler_t& orientation)
{
  DEBUG(stagedeviceid);
  DEBUG(channeldeviceid);
}

ping_stat_t::ping_stat_t()
    : t_min(0), t_med(0), t_p99(0), t_mean(0), received(0), lost(0),
      state_sent(0), state_received(0)
{
}
message_stat_t::message_stat_t()
    : received(0u), lost(0u), seqerr_in(0u), seqerr_out(0u)
{
}

void message_stat_t::operator+=(const message_stat_t& src)
{
  received += src.received;
  lost += src.lost;
  seqerr_in += src.seqerr_in;
  seqerr_out += src.seqerr_out;
}

void message_stat_t::operator-=(const message_stat_t& src)
{
  received -= src.received;
  lost -= src.lost;
  seqerr_in -= src.seqerr_in;
  seqerr_out -= src.seqerr_out;
}

void device_channel_t::update_plugin_cfg(const std::string& jscfg)
{
  nlohmann::json jsplugins = nlohmann::json::parse(jscfg);
  size_t pcnt = 0;
  for(auto it = jsplugins.begin(); it != jsplugins.end(); ++it) {
    std::string pname;
    if(pcnt < plugins.size())
      pname = plugins[pcnt].name;
    auto plug = it.key();
    auto cfg = it.value();
    channel_plugin_t cplug;
    if(plug == pname)
      cplug = plugins[pcnt];
    cplug.name = plug;
    for(auto it = cfg.begin(); it != cfg.end(); ++it) {
      auto param = it.key();
      auto val = it.value();
      if(val.is_string())
        cplug.params[param] = val.get<std::string>();
      else if(val.is_number()) {
        double v = val.get<double>();
        cplug.params[param] = TASCAR::to_string(v);
      } else if(val.is_boolean()) {
        bool v = val.get<bool>();
        cplug.params[param] = TASCAR::to_string(v);
      }
    }
    if(pcnt < plugins.size())
      plugins[pcnt] = cplug;
    else
      plugins.push_back(cplug);
    ++pcnt;
  }
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
