#ifndef OV_TYPES
#define OV_TYPES

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// cartesian coordinates in meters, e.g., for room positions:
struct pos_t {
  // x is forward direction
  double x;
  // y is left direction
  double y;
  // z is up direction
  double z;
};

bool operator!=(const pos_t& a, const pos_t& b);

// Euler angles for rotation, in radians:
struct zyx_euler_t {
  // rotation around z axis:
  double z;
  double y;
  double x;
};

bool operator!=(const zyx_euler_t& a, const zyx_euler_t& b);

/// packet sequence type
typedef int16_t sequence_t;
/// port number type
typedef uint16_t port_t;
/// pin code type
typedef uint32_t secret_t;
/// stage device id type
typedef uint8_t stage_device_id_t;
// stage device channel id type
//TODO: Verify
typedef std::string device_channel_id_t;

struct audio_device_t {
  /// driver name, e.g. jack, ALSA, ASIO
  std::string drivername;
  /// string represenation of device identifier, e.g., hw:1 for ALSA or jack
  std::string devicename;
  /// sampling rate in Hz
  double srate;
  /// number of samples in one period
  unsigned int periodsize;
  /// number of buffers
  unsigned int numperiods;
};

bool operator!=(const audio_device_t& a, const audio_device_t& b);

struct device_channel_t {
  device_channel_id_t id;
  /// Source of channel (used locally only):
  std::string sourceport;
  /// Linear playback gain:
  double gain;
  /// Position relative to stage device origin:
  pos_t position;
  /// source directivity, e.g., omni, cardioid:
  std::string directivity;
};

bool operator!=(const device_channel_t& a, const device_channel_t& b);

struct stage_device_t {
  /// ID within the stage, typically a number from 0 to number of stage devices:
  stage_device_id_t id;
  /// Label of the stage device:
  std::string label;
  /// List of channels the device is providing:
  std::vector<device_channel_t> channels;
  /// Position of the stage device in the virtual space:
  pos_t position;
  /// Orientation of the stage device in the virtual space, ZYX Euler angles:
  zyx_euler_t orientation;
  /// Linear gain of the stage device:
  double gain;
  /// Mute flag:
  bool mute;
  /// sender jitter:
  double senderjitter;
  /// receiver jitter:
  double receiverjitter;
  /// send to local IP if same network:
  bool sendlocal;
};

bool operator!=(const std::vector<device_channel_t>& a,
                const std::vector<device_channel_t>& b);

bool operator!=(const stage_device_t& a, const stage_device_t& b);

struct render_settings_t {
  stage_device_id_t id;
  /// room dimensions:
  pos_t roomsize;
  /// average wall absorption coefficient:
  double absorption;
  /// damping coefficient, defines frequency tilt of T60:
  double damping;
  /// linear gain of reverberation:
  double reverbgain;
  /// flag wether rendering of reverb is required or not:
  bool renderreverb;
  /// flag wether rendering of image source model (ISM) or not:
  bool renderism;
  /// flag wether rendering of virtual acoustics is required:
  bool rawmode;
  /// Receiver type, either ortf or hrtf:
  std::string rectype;
  /// self monitor gain:
  double egogain;
  /// peer-to-peer mode:
  bool peer2peer;
  /// output ports of device master (names may depend on audio backend):
  std::string outputport1;
  std::string outputport2;
  /// the next bit is very specific and likely to change:
  /// extra audio connection, key is the source port, value is the destination:
  std::unordered_map<std::string, std::string> xports;
  /// extra forwarding UDP ports:
  std::vector<port_t> xrecport;
  /// jitterbuffersize for second data receiver (e.g., for recording or
  /// broadcasting):
  double secrec;
  /// load headtracking module:
  bool headtracking;
  /// apply head rotation to receiver:
  bool headtrackingrotrec;
  /// apply head rotation to source:
  bool headtrackingrotsrc;
  /// data logging port:
  port_t headtrackingport;
  // ambient sound file url:
  std::string ambientsound;
  // ambient sound file level in dB:
  double ambientlevel;
};

bool operator!=(const render_settings_t& a, const render_settings_t& b);

struct stage_t {
  /// relay server host name:
  std::string host;
  /// relay server port:
  port_t port;
  /// relay server PIN, a session key which is valid for one session:
  secret_t pin;
  /// rendering settings:
  render_settings_t rendersettings;
  /// Device identifier of this stage device (typically its mac address):
  std::string thisdeviceid;
  /// Numeric identifier of this device within the stage:
  stage_device_id_t thisstagedeviceid;
  /// Devices on the stage:
  std::map<stage_device_id_t, stage_device_t> stage;
  /// This device, also valid if not in a room:
  stage_device_t thisdevice;
};

bool operator!=(const std::map<stage_device_id_t, stage_device_t>& a,
                const std::map<stage_device_id_t, stage_device_t>& b);

class ov_render_base_t {
public:
  /**
     \brief Create a new renderer.
     \param deviceid Device ID string (alphanumeric, no whitespace)
  */
  ov_render_base_t(const std::string& deviceid);
  virtual ~ov_render_base_t(){};
  /**
     \brief Configure the relay server (ov-server)
     \param host IP address or host name of server
     \param port Port number of server
     \param pin Session key / PIN of session
  */
  virtual void set_relay_server(const std::string& host, port_t port,
                                secret_t pin);
  /**
     \brief Start a new session (requires a running audio backend)
  */
  virtual void start_session();
  /**
     \brief End session
  */
  virtual void end_session();
  /**
     \brief Configure audio backend and start (or restart) if required
     \param audio Audio device descriptor
  */
  virtual void configure_audio_backend(const audio_device_t& audio);
  /**
     \brief Setup the local device.
     \param stagedevice Stage device descriptor

     If required and already running, the session is restarted
  */
  virtual void set_thisdev(const stage_device_t& stagedevice);
  /**
     \brief Add a new device to the session
     \param stagedevice Device descriptor
  */
  virtual void add_stage_device(const stage_device_t& stagedevice);
  /**
     \brief Remove a device from the stage
  */
  virtual void rm_stage_device(stage_device_id_t stagedeviceid);
  /**
     \brief Remove all devices from a stage
   */
  virtual void clear_stage();
  /**
     \brief Set all stage devices of a stage
   */
  virtual void set_stage(const std::map<stage_device_id_t, stage_device_t>&);
  /**
     \brief Set output gain of a stage device
     \param stagedeviceid Stage device ID
     \param gain Linear gain
     \todo Discuss exact gain definition
   */
  virtual void set_stage_device_gain(stage_device_id_t stagedeviceid,
                                     double gain);
    /**
       \brief Set output gain of a single device channel of an stage device
       \param stagedeviceid Stage device ID
       \param devicechannelid Device channel ID
       \param gain Linear gain
       \todo @gisogrimm Is this easy to implement? And how should we identify the device channel? Could there maybe a simple string identifier?
     */
     virtual void set_stage_device_channel_gain(stage_device_id_t stagedeviceid,
                                                device_channel_id_t channeldeviceid, // vector index ... but maybe a string identifier?
                                                double gain);

  /**
     \brief Set position and orientation of a stage device
     \param stagedeviceid Stage device ID
     \param position Position of stage device inside stage
     \param orientation Orientation of stage device inside stage
     \todo Please implement me ;)
   */
  virtual void set_stage_device_position(stage_device_id_t stagedeviceid, pos_t position, zyx_euler_t orientation);

  /**
     \brief Set position and orientation of a stage device
     \param stagedeviceid Stage device ID
     \param position Position of stage device inside stage
     \param orientation Orientation of stage device inside stage
     \todo Please implement me ;)
   */
  virtual void set_stage_device_channel_position(stage_device_id_t stagedeviceid, device_channel_id_t channeldeviceid, pos_t position);

  /**
     \brief Set render settings of this device and stage combination
     \param rendersettings Render settings including room acoustics, this device
     gains, connections and similar \param thisstagedeviceid ID of this device
   */
  virtual void set_render_settings(const render_settings_t& rendersettings,
                                   stage_device_id_t thisstagedeviceid);
  /**
     \brief Start audio backend
  */
  virtual void start_audiobackend();
  /**
     \brief Stop audio backend
     \note Make sure that the session is ended before stopping the audio backend
   */
  virtual void stop_audiobackend();
  const bool is_session_active() const;
  const bool is_audio_active() const;
  const std::string& get_deviceid() const;
  virtual void getbitrate(double& txrate, double& rxrate);
  virtual double get_load() const { return 0; };
  virtual std::vector<std::string> get_input_channel_ids() const
  {
    return {"system:capture_1", "system:capture_2"};
  };
  // provide additional configuration as json string:
  virtual void set_extra_config(const std::string&){};

protected:
  audio_device_t audiodevice;
  stage_t stage;

private:
  bool session_active;
  bool audio_active;
};

// This class manages the communication with the frontend and calls
// the corresponding methods of the rendering backend. Current
// realizations are ov_client_orlandoviols_t and
// ov_client_digitalstage_t.
class ov_client_base_t {
public:
  ov_client_base_t(ov_render_base_t& backend) : backend(backend){};
  virtual ~ov_client_base_t(){};
  virtual void start_service() = 0;
  virtual void stop_service() = 0;
  virtual bool is_going_to_stop() const = 0;

protected:
  ov_render_base_t& backend;
};

const char* get_libov_version();

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
