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
#include <iostream>
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
    "",          // std::string label;
    {},          // std::vector<device_channel_t> channels;
    default_pos, // pos_t position;
    default_rot, // zyx_euler_t orientation;
    1.0,         // double gain;
    false,       // bool mute;
    5.0,         // double senderjitter;
    5.0,         // double receiverjitter;
    true,        // bool sendlocal;
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
  return (a.drivername != b.drivername) || (a.devicename != b.devicename) ||
         (a.srate != b.srate) || (a.periodsize != b.periodsize) ||
         (a.numperiods != b.numperiods);
}

bool operator!=(const device_channel_t& a, const device_channel_t& b)
{
  return (a.sourceport != b.sourceport) || (a.gain != b.gain) ||
         (a.position != b.position) || (a.directivity != b.directivity);
}

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b)
{
  if(a.size() != b.size())
    return true;
  for(size_t k = 0; k < a.size(); ++k)
    if(a[k] != b[k])
      return true;
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
  return (a.id != b.id) || (a.label != b.label) || (a.channels != b.channels) ||
         (a.position != b.position) || (a.orientation != b.orientation) ||
         (a.gain != b.gain) || (a.mute != b.mute) ||
         (a.senderjitter != b.senderjitter) ||
         (a.receiverjitter != b.receiverjitter) || (a.sendlocal != b.sendlocal);
}

bool operator!=(const render_settings_t& a, const render_settings_t& b)
{
  return (a.id != b.id) || (a.roomsize != b.roomsize) ||
         (a.absorption != b.absorption) || (a.damping != b.damping) ||
         (a.reverbgain != b.reverbgain) || (a.renderreverb != b.renderreverb) ||
         (a.renderism != b.renderism) || (a.distancelaw != b.distancelaw) ||
         (a.rawmode != b.rawmode) || (a.rectype != b.rectype) ||
         (a.egogain != b.egogain) || (a.mastergain != b.mastergain) ||
         (a.peer2peer != b.peer2peer) || (a.outputport1 != b.outputport1) ||
         (a.outputport2 != b.outputport2) || (a.secrec != b.secrec) ||
         (a.xports != b.xports) || (a.xrecport != b.xrecport) ||
         (a.headtracking != b.headtracking) ||
         (a.headtrackingrotrec != b.headtrackingrotrec) ||
         (a.headtrackingrotsrc != b.headtrackingrotsrc) ||
         (a.headtrackingport != b.headtrackingport) ||
         (a.ambientsound != b.ambientsound) ||
         (a.ambientlevel != b.ambientlevel) || (a.lmetertc != b.lmetertc) ||
         (a.lmeterfw != b.lmeterfw) || (a.delaycomp != b.delaycomp);
}

render_settings_t::render_settings_t()
    : id(0),                              // stage_device_id_t id;
      roomsize(default_roomsize),         // pos_t roomsize;
      absorption(0.6),                    // double absorption;
      damping(0.7),                       // double damping;
      reverbgain(1.0),                    // double reverbgain;
      renderreverb(true),                 // bool renderreverb;
      renderism(false),                   // bool renderism;
      distancelaw(false), rawmode(false), // bool rawmode;
      rectype("hrtf"),                    // std::string rectype;
      egogain(1.0),                       // double egogain;
      mastergain(1.0),                    // double mastergain;
      peer2peer(true),                    // bool peer2peer;
      outputport1(""),                    // std::string outputport1;
      outputport2(""),                    // std::string outputport2;
      xports(
          std::unordered_map<std::string,
                             std::string>()), // std::unordered_map<std::string,
      // std::string> xports;
      xrecport(std::vector<port_t>()), // std::vector<port_t> xrecport;
      secrec(0),                       // double secrec;
      headtracking(false),             // bool headtracking;
      headtrackingrotrec(true),        // bool headtrackingrotrec;
      headtrackingrotsrc(true),        // bool headtrackingrotsrc;
      headtrackingport(0),             // port_t headtrackingport;
      ambientsound(""),                // std::string ambientsound;
      ambientlevel(50),                // double ambientlevel;
      lmetertc(0.5),                   // double levelmeter_tc;
      lmeterfw("Z"),                   // std::string lmeterfw;
      delaycomp(17.0)                  // double delaycomp;
{
}

ov_render_base_t::ov_render_base_t(const std::string& deviceid)
    : audiodevice({"", "", 48000, 96, 2}), stage(default_stage),
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

const bool ov_render_base_t::is_session_active() const
{
  return session_active;
}

const bool ov_render_base_t::is_audio_active() const
{
  return audio_active;
}

const std::string& ov_render_base_t::get_deviceid() const
{
  return stage.thisdeviceid;
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
    audiodevice = audiodevice_;
    // audio was active before, so we need to restart:
    if(audio_active) {
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
    const stage_device_id_t& stagedeviceid, double gain)
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

const bool ov_render_base_t::need_restart() const
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
    const device_channel_id_t& channeldeviceid, double gain)
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

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
