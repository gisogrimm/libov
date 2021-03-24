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

render_settings_t default_rendersettings = {
    0,                // stage_device_id_t id;
    default_roomsize, // pos_t roomsize;
    0.6,              // double absorption;
    0.7,              // double damping;
    1.0,              // double reverbgain;
    true,             // bool renderreverb;
    false,            // bool renderism;
    false,            // bool rawmode;
    "hrtf",           // std::string rectype;
    1.0,              // double egogain;
    true,             // bool peer2peer;
    "",               // std::string outputport1;
    "",               // std::string outputport2;
    std::unordered_map<std::string,
                       std::string>(), // std::unordered_map<std::string,
                                       // std::string> xports;
    std::vector<port_t>(),             // std::vector<port_t> xrecport;
    0,                                 // double secrec;
    false,                             // bool headtracking;
    true,                              // bool headtrackingrotrec;
    true,                              // bool headtrackingrotsrc;
    0,                                 // port_t headtrackingport;
    "",                                // std::string ambientsound;
    50,                                // double ambientlevel;
};

stage_t default_stage = {
    "",                     // std::string host;
    0,                      // port_t port;
    0,                      // secret_t pin;
    default_rendersettings, // render_settings_t rendersettings;
    "",                     // std::string thisdeviceid;
    0,                      // stage_device_id_t thisstagedeviceid;
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
         (a.renderism != b.renderism) || (a.rawmode != b.rawmode) ||
         (a.rectype != b.rectype) || (a.egogain != b.egogain) ||
         (a.peer2peer != b.peer2peer) || (a.outputport1 != b.outputport1) ||
         (a.outputport2 != b.outputport2) || (a.secrec != b.secrec) ||
         (a.xports != b.xports) || (a.xrecport != b.xrecport) ||
         (a.headtracking != b.headtracking) ||
         (a.headtrackingrotrec != b.headtrackingrotrec) ||
         (a.headtrackingrotsrc != b.headtrackingrotsrc) ||
         (a.headtrackingport != b.headtrackingport) ||
         (a.ambientsound != b.ambientsound) ||
         (a.ambientlevel != b.ambientlevel);
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

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
