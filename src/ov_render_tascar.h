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

#ifndef OV_RENDER_TASCAR
#define OV_RENDER_TASCAR

#include "../tascar/libtascar/include/session.h"
#include "../tascar/libtascar/include/spawn_process.h"
#include "ov_tools.h"
#include "ovboxclient.h"
#include <lo/lo.h>

#ifndef ZITAPATH
#define ZITAPATH ""
#endif

class ov_render_tascar_t : public ov_render_base_t {
public:
  ov_render_tascar_t(const std::string& deviceid, port_t pinglogport_,
                     bool secondary_);
  ~ov_render_tascar_t();
  void start_session();
  void end_session();
  void clear_stage();
  void start_audiobackend();
  void stop_audiobackend();
  void add_stage_device(const stage_device_t& stagedevice);
  void rm_stage_device(stage_device_id_t stagedeviceid);
  void set_stage(const std::map<stage_device_id_t, stage_device_t>&);
  void set_stage_device_gain(const stage_device_id_t& stagedeviceid,
                             float gain);
  void set_stage_device_channel_gain(const stage_device_id_t& stagedeviceid,
                                     const device_channel_id_t& channeldeviceid,
                                     float gain);
  void
  set_stage_device_channel_position(const stage_device_id_t& stagedeviceid,
                                    const device_channel_id_t& channeldeviceid,
                                    const pos_t& position,
                                    const zyx_euler_t& orientation);
  void set_render_settings(const render_settings_t& rendersettings,
                           stage_device_id_t thisstagedeviceid);
  std::string get_stagedev_name(stage_device_id_t stagedeviceid) const;
  void getbitrate(double& txrate, double& rxrate);
  std::vector<std::string> get_input_channel_ids() const;
  float get_load() const;
  std::vector<float> get_temperature() const;
  void set_extra_config(const std::string&);
  void set_seqerr_callback(std::function<void(stage_device_id_t, sequence_t,
                                              sequence_t, port_t, void*)>
                               cb,
                           void* data);
  std::string get_client_stats();
  std::string get_zita_path();
  std::string get_current_plugincfg_as_json(size_t channel);
  std::string get_all_current_plugincfg_as_json();
  std::string get_objmixcfg_as_json();
  std::string get_level_stat_as_json();
  void get_session_gains(float& outputgain, float& egogain, float& reverbgain,
                         std::map<std::string, std::vector<float>>&);
  void set_zita_path(const std::string& path);
  void set_allow_systemmods(bool);
  void upload_plugin_settings();
  void upload_session_gains();
  void upload_objmix();
  void update_level_stat(int32_t channel, const std::vector<float>& peak,
                         const std::vector<float>& ms);
  size_t get_xruns();
  uint8_t get_encrypt_state();

  class metronome_t {
  public:
    metronome_t();
    metronome_t(const nlohmann::json& js);
    bool operator!=(const metronome_t& a);
    void set_xmlattr(tsccfg::node_t em, tsccfg::node_t ed) const;
    void update_osc(TASCAR::osc_server_t* srv, const std::string& dev) const;

    uint32_t bpb;
    float bpm;
    bool bypass;
    float delay;
    float level;
  };

private:
  void create_virtual_acoustics(tsccfg::node_t session, tsccfg::node_t e_rec,
                                tsccfg::node_t e_scene);
  tsccfg::node_t configure_simplefdn(tsccfg::node_t e_scene);

  void create_raw_dev(tsccfg::node_t session);
  void create_levelmeter_route(tsccfg::node_t e_session,
                               tsccfg::node_t e_modules);
  void add_secondary_bus(const stage_device_t& stagemember,
                         tsccfg::node_t& e_mods, tsccfg::node_t& e_session,
                         std::vector<std::string>& waitports,
                         const std::string& zitapath,
                         const std::string& chanlist);
  void add_network_receiver(const stage_device_t& stagemember,
                            tsccfg::node_t& e_mods, tsccfg::node_t& e_session,
                            std::vector<std::string>& waitports,
                            uint32_t& chcnt);
  // for the time being we (optionally if jack is chosen as an audio
  // backend) start the jack backend. This will be replaced by a more
  // generic audio backend interface:
  TASCAR::spawn_process_t* h_jack = NULL;
  // for the time being we start the webmixer as a local nodejs server
  // on port 8080. This will be replaced (or extended) by a web mixer
  // on the remote configuration interface:
  TASCAR::spawn_process_t* h_webmixer = NULL;
  TASCAR::session_t* tascar = NULL;
  ovboxclient_t* ovboxclient = NULL;
  std::mutex mtx_ovboxclient;
  port_t pinglogport;
  lo_address pinglogaddr;
  std::vector<std::string> inputports;
  float headtrack_tauref;
  // EOG data logging path:
  std::string headtrack_eogpath;
  bool headtrack_autorefzonly = false;
  // self-monitor delay in milliseconds:
  float selfmonitor_delay = 0.0f;
  bool selfmonitor_onlyreverb = false;
  bool selfmonitor_active = true;
  // metronome settings:
  metronome_t metronome;
  std::string zitapath;
  std::map<stage_device_id_t, std::string> proxyclients;
  /**
     \brief serve as proxy
     \ingroup proxymode

     A device which serves as proxy will send incoming messages
     arriving from other networks to all registered proxy clients.
   */
  bool is_proxy;
  /**
     \brief use a proxy
     \ingroup proxymode

     A device which uses a proxy will not receive packages from a
     server or peers outside the same network, but receive the
     packages directly from the proxy instead.

  */
  bool use_proxy;
  std::string proxyip;
  std::string localip;
  std::function<void(stage_device_id_t sender, sequence_t expected,
                     sequence_t received, port_t destport, void* data)>
      cb_seqerr;
  void* cb_seqerr_data;
  std::map<stage_device_id_t, client_stats_t> client_stats;
  float sorter_deadline;
  bool expedited_forwarding_PHB;
  bool render_soundscape;
  bool allow_systemmods = false;
  // user provided TASCAR include file content:
  std::string tscinclude;
  // jackrec file format:
  std::string jackrec_fileformat;
  // jackrec sample format:
  std::string jackrec_sampleformat;
  // add multicast zita-n2j instance:
  bool mczita = false;
  bool mczitasend = false;
  uint32_t mczitasendch = 1;
  std::string mczitaaddr = "";
  uint32_t mczitaport = 10007;
  std::string mczitadevice = "";
  uint32_t mczitabuffer = 10;
  std::vector<uint32_t> mczitachannels = {1, 2};
  bool mczita_autoconnect_rec = false;
  std::string zitasampleformat = "16bit";
  bool useloudspeaker = false;
  uint32_t fdnforwardstages = 0u;
  uint32_t fdnorder = 5u;
  float echoc_maxdist = 4.0;
  uint32_t echoc_nrep = 64;
  float echoc_level = 60.0;
  uint32_t echoc_filterlen = 65;
  std::vector<std::string> ego_source_names;
  bool secondary = false;
  port_t dist_to_other_tascarport = 9870;
  port_t my_tascarport = 9871;
  port_t other_tascarport = 9871;
  port_t portoffset = 0;
  bool emptysessionismonitor = false;
  // user provided MHA configuration:
  std::string mhaconfig;
  // use BCF2000 with default settings:
  bool usebcf2000 = false;
  // OSC message for dispatching to trigger level analysis:
  lo_message msg_level_analysis_trigger;
  std::map<int32_t, std::vector<float>> level_analysis_peak;
  std::map<int32_t, std::vector<float>> level_analysis_ms;
  bool start_webmixer = true;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
