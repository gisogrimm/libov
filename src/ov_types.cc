#include "ov_types.h"
#include <iostream>
#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << __PRETTY_FUNCTION__      \
            << " " << #x << "=" << x << std::endl
#endif

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
    : audiodevice({"", "", 48000, 96, 2}),
      stage({"",
             0,
             0,
             {0,
              {0, 0, 0},
              0.6,
              0.7,
              -8,
              true,
              false,
              false,
              "ortf",
              1,
              true,
              "",
              "",
              std::unordered_map<std::string, std::string>(),
              std::vector<port_t>(),
              0,
              false,
              true,
              0},
             deviceid,
             0,
             std::map<stage_device_id_t, stage_device_t>(),
             {0,
              "",
              std::vector<device_channel_t>(),
              {0, 0, 0},
              {0, 0, 0},
              0,
              false,
              5,
              5,
              true}}),
      session_active(false), audio_active(false)
{
}

void ov_render_base_t::start_session()
{
  session_active = true;
}

void ov_render_base_t::end_session()
{
  session_active = false;
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
  DEBUG((audiodevice != audiodevice_));
  DEBUG(audio_active);
  // audio backend changed or was not active before:
  if((audiodevice != audiodevice_) || (!audio_active)) {
    audiodevice = audiodevice_;
    bool session_was_active(session_active);
    // audio was active before, so we need to restart:
    if(audio_active) {
      if(session_active)
        end_session();
      stop_audiobackend();
    }
    // now start audio:
    start_audiobackend();
    // if a sesion was active then restart session:
    if(session_was_active)
      start_session();
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

void ov_render_base_t::set_stage_device_gain(stage_device_id_t stagedeviceid,
                                             double gain)
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
    start_session();
  }
}

void ov_render_base_t::getbitrate(double& txrate, double& rxrate) {}

const char* get_libov_version()
{
  return OVBOXVERSION;
}

bool operator!=(const std::map<stage_device_id_t, stage_device_t>& a,
                const std::map<stage_device_id_t, stage_device_t>& b)
{
  if(a.size() != b.size())
    return true;
  auto a_it = a.begin();
  auto b_it = b.begin();
  while((a_it != a.end()) && (b_it != b.end())) {
    if(a_it->first != b_it->first)
      return true;
    if(a_it->second != b_it->second)
      return true;
    ++a_it;
    ++b_it;
  }
  return false;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
