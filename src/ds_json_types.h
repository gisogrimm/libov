#ifndef DS_TYPES
#define DS_TYPES

#include <nlohmann/json.hpp>

namespace ds {
  namespace json {
    using nlohmann::json;

    struct device_t {
      std::string _id;
      bool online;
      std::string name;
      bool canAudio;
      bool canVideo;
      bool canOv;
      bool sendAudio;
      bool sendVideo;
      bool receiveAudio;
      bool receiveVideo;

      int senderJitter;
      int receiverJitter;

      std::vector<std::string> soundCardNames;
      std::string soundCardName;
    };

    struct three_dimensional_t {
      double volume;
      bool muted;
      double x = .0;
      double y;
      double z;
      double rX;
      double rY;
      double rZ;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(three_dimensional_t, volume, muted, x, y,
                                       z, rX, rY, rZ)

    struct ov_server_t {
      std::string router;
      std::string ipv4;
      std::string ipv6;
      uint16_t port;
      uint32_t pin;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ov_server_t, router, ipv4, ipv6, port,
                                       pin)

    struct stage_t {
      std::string _id;
      std::string name;
      double width;
      double length;
      double height;
      double absorption;
      double damping;
      ov_server_t ovServer;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_t, _id, name, width, length,
                                       height, absorption, damping)

    struct group_t {
      std::string _id;
      std::string name;
      double volume;
      bool muted;
      double x = .0;
      double y;
      double z;
      double rX;
      double rY;
      double rZ;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(group_t, _id, name, volume, muted, x, y,
                                       z, rX, rY, rZ)

    struct custom_group_t {
      std::string _id;
      double volume;
      bool muted;
      double x = .0;
      double y = .0;
      double z;
      double rX;
      double rY;
      double rZ;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_group_t, _id, volume, muted, x, y,
                                       z, rX, rY, rZ)

    struct stage_member_t {
      std::string _id;
      double volume;
      bool muted;
      double x;
      double y;
      double z;
      double rX;
      double rY;
      double rZ;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_member_t, _id, volume, muted, x, y,
                                       z, rX, rY, rZ)

    struct custom_stage_member_t {
      std::string _id;
      std::string stageMemberId;
      double volume;
      bool muted;
      double x;
      double y;
      double z;
      double rX;
      double rY;
      double rZ;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(custom_stage_member_t, _id,
                                       stageMemberId, volume, muted, x, y, z,
                                       rX, rY, rZ)

    struct soundcard_t {
      std::string _id;
      std::string userId;
      std::string name;
      std::string driver; //'JACK' | 'ALSA' | 'ASIO' | 'WEBRTC'
      int numInputChannels;
      int numOutputChannels;
      bool isDefault;
      std::string trackPresetId;

      std::vector<int> sampleRates;
      int sampleRate;
      double softwareLatency;
      double periodSize;
      int numPeriods;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(soundcard_t, _id, userId, name, driver,
                                       numInputChannels, numOutputChannels,
                                       isDefault, trackPresetId, sampleRates,
                                       sampleRate, softwareLatency, periodSize,
                                       numPeriods)

    struct track_preset_t {
      std::string _id;
      std::string userId;
      std::string soundCardId;
      std::string name;
      std::vector<std::string> outputChannels;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(track_preset_t, _id, userId, soundCardId,
                                       name, outputChannels)

    struct track_t {
      std::string _id;
      std::string trackPresetId;
      int channel;
      bool online;
      double gain;
      std::string directivity;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(track_t, _id, trackPresetId, channel,
                                       online, gain, directivity)

    struct stage_member_ov_t {
      std::string _id;
      std::string stageMemberId;
      int ovId;
      int channel;
      double gain;
      bool online;
      std::string directivity;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(stage_member_ov_t, _id, stageMemberId,
                                       ovId, channel, gain, online, directivity)

    struct user_t {
      std::string _id;
      std::string name;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(user_t, _id, name)
  } // namespace json
} // namespace ds

#endif