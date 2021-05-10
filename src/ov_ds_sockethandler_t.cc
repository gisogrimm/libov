/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
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
 * Version 3 along with ovbox / dsbox. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "ov_ds_sockethandler_t.h"

using namespace DigitalStage::Api;
using namespace DigitalStage::Types;

bool isValidOvStage(const Stage& stage)
{
  return stage.ovIpv4 && stage.ovPort && stage.ovPin;
}

ov_ds_sockethandler_t::ov_ds_sockethandler_t(ov_render_base_t* renderer_,
                                             DigitalStage::Api::Client* client_)
    : renderer(renderer_), client(client_), insideOvStage(false)
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::ov_ds_sockethandler_t()" << std::endl;
#endif
}
ov_ds_sockethandler_t::~ov_ds_sockethandler_t()
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::~ov_ds_sockethandler_t()" << std::endl;
#endif
  unlisten();
}

void ov_ds_sockethandler_t::enable() noexcept
{
  if(!enabled) {
    listen();
    enabled = true;

    if(!insideOvStage && client->getStore()->isReady()) {
      auto stageId = client->getStore()->getStageId();
      if(stageId) {
        try {
          auto stage = client->getStore()->getStage(*stageId);
          if(!stage)
            throw std::runtime_error("Could not find stage " + *stageId);
          handleStageJoined(*stage, client->getStore());
        }
        catch(std::exception& exception) {
          std::cerr << "Internal error: " << exception.what() << std::endl;
        }
      }
    }
  }
}

void ov_ds_sockethandler_t::disable() noexcept
{
  if(enabled) {
    enabled = false;
    unlisten();
    onStageLeft(client->getStore());
  }
}

void ov_ds_sockethandler_t::onReady(const Store* store) noexcept
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::onReady" << std::endl;
#endif
  // Check if inside an ov stage
  auto stageId = store->getStageId();
  if(stageId) {
    try {
      auto stage = store->getStage(*stageId);
      if(!stage)
        throw std::runtime_error("Could not find stage " + *stageId);
      handleStageJoined(*stage, store);
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}
void ov_ds_sockethandler_t::onStageJoined(
    const DigitalStage::Types::ID_TYPE& stageId,
    const DigitalStage::Types::ID_TYPE&,
    const DigitalStage::Api::Store* store) noexcept
{
  if(store->isReady()) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onStageJoined" << std::endl;
#endif
    try {
      auto stage = store->getStage(stageId);
      if(!stage)
        throw std::runtime_error("Could not find joined stage " + stageId);
      handleStageJoined(*stage, store);
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::onStageChanged(
    const DigitalStage::Types::ID_TYPE& stageId, const nlohmann::json& update,
    const DigitalStage::Api::Store* store) noexcept
{
  // Does this change affect the stage, the user is currently joined?
  if(stageId == *store->getStageId()) {
    if(insideOvStage) {
      // We are inside an ov stage already, just sync the relay server and stage
      // settings (if outdated)
      if(update.count("ovIpv4") > 0 || update.count("ovPort") > 0 ||
         update.count("ovPin") > 0) {
        // Sync relay server
        try {
          auto stage = store->getStage(stageId);
          if(!stage)
            throw std::runtime_error("Could not find stage " + stageId);
          renderer->set_relay_server(*stage->ovIpv4, *stage->ovPort,
                                     *stage->ovPin);
        }
        catch(std::exception& exception) {
          std::cerr << "Internal error: " << exception.what() << std::endl;
        }
      }
      if(update.count("width") > 0 || update.count("length") > 0 ||
         update.count("height") > 0 || update.count("reflection") > 0 ||
         update.count("absorption") > 0 ||
         update.count("ovRenderAmbient") > 0 ||
         update.count("ovAmbientSoundUrl") > 0 ||
         update.count("ovAmbientLevel") > 0) {
        // Sync stage settings
        try {
          syncWholeStage(store);
        }
        catch(std::exception& exception) {
          std::cerr << "Internal error: " << exception.what() << std::endl;
        }
      }
    } else {
      // Maybe this stage does support ov now
      if(update.count("audioType") && update["audioType"] == "ov") {
        try {
          // ok, handle this like the user would have joined the stage right now
          auto stage = store->getStage(stageId);
          if(!stage)
            throw std::runtime_error("Could not find stage " + stageId);
          handleStageJoined(*stage, store);
        }
        catch(std::exception& exception) {
          std::cerr << "Internal error: " << exception.what() << std::endl;
        }
      }
    }
  }
}

void ov_ds_sockethandler_t::onStageLeft(
    const DigitalStage::Api::Store*) noexcept
{
  if(insideOvStage) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onStageLeft" << std::endl;
#endif
    insideOvStage = false;
    renderer->clear_stage();
    renderer->stop_audiobackend();
  }
}

void ov_ds_sockethandler_t::onDeviceChanged(
    const std::string&, const nlohmann::json& update,
    const DigitalStage::Api::Store* store) noexcept
{
  if(insideOvStage && update.count("soundCardId")) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onDeviceChanged - soundCardId changed"
              << std::endl;
#endif
    try {
      syncSoundCard(store);
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::onSoundCardAdded(
    const DigitalStage::Types::SoundCard& soundCard,
    const DigitalStage::Api::Store* store) noexcept
{
  if(insideOvStage) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onSoundCardAdded" << std::endl;
#endif
    try {
      auto localDevice = store->getLocalDevice();
      if(!localDevice)
        throw std::runtime_error("No local device found");
      if(localDevice->soundCardId == soundCard._id) {
        syncSoundCard(store);
      }
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::onSoundCardChanged(
    const DigitalStage::Types::ID_TYPE& id, const json& update,
    const DigitalStage::Api::Store* store) noexcept
{
  if(insideOvStage) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onSoundCardChanged" << std::endl;
#endif
    try {
      auto localDevice = store->getLocalDevice();
      if(!localDevice)
        throw std::runtime_error("No local device found");
      if(localDevice->soundCardId == id) {
        // Update TASCAR audio configuration (if necessary)
        if(update.count("sampleRate") > 0 || update.count("periodSize") > 0 ||
           update.count("numPeriods") > 0 || update.count("driver") > 0) {
          syncSoundCard(store);
        }
      }
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::onAudioTrackAdded(
    const DigitalStage::Types::AudioTrack& track,
    const DigitalStage::Api::Store* store) noexcept
{
  if(insideOvStage && track.stageId == *store->getStageId()) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onAudioTrackAdded" << std::endl;
#endif
    try {
      syncWholeStage(store);
      renderer->restart_session_if_needed();
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::onAudioTrackRemoved(
    const DigitalStage::Types::AudioTrack& track,
    const DigitalStage::Api::Store* store) noexcept
{
  if(insideOvStage && track.stageId == *store->getStageId()) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onAudioTrackRemoved" << std::endl;
#endif
    try {
      syncWholeStage(store);
      renderer->restart_session_if_needed();
    }
    catch(std::exception& exception) {
      std::cerr << "Internal error: " << exception.what() << std::endl;
    }
  }
}

void ov_ds_sockethandler_t::handleStageJoined(
    const Stage& stage, const DigitalStage::Api::Store* store) noexcept(false)
{
  if(isValidOvStage(stage)) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::handleStageJoined" << std::endl;
#endif
    // Fetch all necessary models
    auto localStageDevice = store->getStageDevice();
    if(!localStageDevice)
      throw std::runtime_error("Local stage device missing");
    auto localDevice = store->getLocalDevice();
    if(!localDevice)
      throw std::runtime_error("Local device missing");
    if(!localDevice->soundCardId)
      throw std::runtime_error("No sound card specified");
    auto soundCard = store->getSoundCard(*localDevice->soundCardId);
    if(!soundCard)
      throw std::runtime_error("Sound card missing");

    // We have to build up session
    // - set the relay server
    renderer->set_relay_server(*stage.ovIpv4, *stage.ovPort, *stage.ovPin);
    // - configure audio
    syncSoundCard(store);
    // - set room settings
    render_settings_t stageSettings;
    stageSettings.id = localStageDevice->order;
    stageSettings.roomsize.x = stage.width;
    stageSettings.roomsize.y = stage.length;
    stageSettings.roomsize.z = stage.height;
    stageSettings.roomsize.z = stage.height;
    stageSettings.absorption = stage.absorption;
    stageSettings.damping = stage.reflection;
    stageSettings.reverbgain =
        localDevice->ovReverbGain ? *localDevice->ovReverbGain : 0.6;
    stageSettings.renderreverb =
        localDevice->ovRenderReverb && *localDevice->ovRenderReverb;
    stageSettings.renderism =
        localDevice->ovRenderISM && *localDevice->ovRenderISM;
    std::vector<std::string> outputChannels;
    for(auto& channel : soundCard->outputChannels) {
      if(channel.second) {
        outputChannels.push_back(channel.first);
      }
    }
    stageSettings.outputport1 = outputChannels.empty() ? "" : outputChannels[0];
    stageSettings.outputport2 =
        outputChannels.size() > 1 ? outputChannels[1] : "";
    stageSettings.rawmode = localDevice->ovRawMode && *localDevice->ovRawMode;
    stageSettings.rectype =
        localDevice->ovReceiverType ? *localDevice->ovReceiverType : "ortf";
    stageSettings.secrec = 0.0;
    stageSettings.egogain = localDevice->egoGain ? *localDevice->egoGain : 0.6;
    stageSettings.mastergain = 1.0;
    stageSettings.peer2peer = localDevice->ovP2p && *localDevice->ovP2p;
    renderer->set_render_settings(stageSettings, localStageDevice->order);

    syncWholeStage(store);

    renderer->restart_session_if_needed();
    insideOvStage = true;
  }
}

void ov_ds_sockethandler_t::syncSoundCard(
    const DigitalStage::Api::Store* store) noexcept(false)
{
  auto localDevice = store->getLocalDevice();
  if(!localDevice)
    throw std::runtime_error("Local device missing");
  if(localDevice->soundCardId) {
    // A sound card is active
    auto soundCard = store->getSoundCard(*localDevice->soundCardId);
    if(!soundCard)
      throw std::runtime_error("Could not find sound card " +
                               *localDevice->soundCardId);
    // Create or replace TASCAR audio configuration
    audio_device_t audioDevice;
    audioDevice.srate = soundCard->sampleRate;
    audioDevice.periodsize = soundCard->periodSize;
    audioDevice.numperiods = soundCard->numPeriods;
    audioDevice.drivername = soundCard->driver ? *soundCard->driver : "jack";
    audioDevice.devicename = soundCard->uuid;
    renderer->configure_audio_backend(audioDevice);

    bool session_was_active(renderer->is_session_active());
    if(session_was_active)
      renderer->end_session();
    renderer->stop_audiobackend();
    renderer->start_audiobackend();
    if(session_was_active)
      renderer->require_session_restart();

    // Sync input channels
    syncInputChannels(store);
  } else {
    // No sound card is active, shut down audio
    renderer->stop_audiobackend();
  }
}

void ov_ds_sockethandler_t::syncInputChannels(const Store* store) noexcept(
    false)
{
  // Sync input channels
  auto localDevice = store->getLocalDevice();
  if(!localDevice)
    throw std::runtime_error("No local device found");
  auto soundCard = store->getSoundCard(*localDevice->soundCardId);
  if(!soundCard)
    throw std::runtime_error("Sound card missing");
  auto stageId = store->getStageId();
  if(!stageId)
    throw std::runtime_error("No stage id specified");

  auto audioTracks = store->getAudioTracks();
  if(soundCard->inputChannels.empty()) {
    // Remove all local audio tracks related to this device
    for(auto& audioTrack : audioTracks) {
      if(audioTrack.deviceId == localDevice->_id) {
        client->send(SendEvents::REMOVE_AUDIO_TRACK, audioTrack._id);
      }
    }
  } else {
    // Propagate local tracks for all new input channels
    for(auto& channel : soundCard->inputChannels) {
      if(channel.second) {
        // Lookup local audio track
        if(std::none_of(audioTracks.begin(), audioTracks.end(),
                        [channel, localDevice](const AudioTrack& track) {
                          return track.deviceId == localDevice->_id &&
                                 track.ovSourcePort == channel.first;
                        })) {
          // Create audio track
          nlohmann::json payload = {{"type", "ov"},
                                    {"ovSourcePort", channel.first},
                                    {"stageId", *stageId}};
          client->send(SendEvents::CREATE_AUDIO_TRACK, payload);
        }
      }
    }
    // Clean up deprecated tracks
    for(auto& audioTrack : audioTracks) {
      if(audioTrack.ovSourcePort && audioTrack.deviceId == localDevice->_id) {
        std::cout << "Checking " << audioTrack._id << std::endl;
        if(soundCard->inputChannels.count(*audioTrack.ovSourcePort) == 0 ||
           !soundCard->inputChannels.at(*audioTrack.ovSourcePort)) {
          std::cout << "Channel is gone " << audioTrack._id << std::endl;
          client->send(SendEvents::REMOVE_AUDIO_TRACK, audioTrack._id);
        }
      }
    }
  }
}

void ov_ds_sockethandler_t::syncWholeStage(
    const DigitalStage::Api::Store* store)
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::syncWholeStage" << std::endl;
#endif
  try {
    auto stageDevices = store->getStageDevices();
    std::map<stage_device_id_t, stage_device_t> stage_devices;
    for(auto& stageDevice : stageDevices) {
      auto localDevice = store->getLocalDevice();
      if(!localDevice)
        throw std::runtime_error("No local device specified");
      auto user = store->getUser(stageDevice.userId);
      if(!user)
        throw std::runtime_error("Could not find user " + stageDevice.userId);

      stage_device_t stage_device;
      stage_device.id = stageDevice.order;
      stage_device.label = user->name;
      stage_device.senderjitter =
          *localDevice->ovSenderJitter; // TODO: use StageDevice instead of
                                        // local device?
      stage_device.receiverjitter =
          *localDevice->ovReceiverJitter; // TODO: use StageDevice instead of
      std::vector<device_channel_t> device_channels;
      auto tracks = store->getAudioTracksByStageDevice(stageDevice._id);
      std::cout << "Have " << tracks.size() << " tracks" << std::endl;
      for(auto& track : tracks) {
        if(track.type == "ov") {
          device_channel_t device_channel;
          device_channel.id = track._id;
          device_channel.gain = track.volume;
          device_channel.directivity = track.directivity;
          device_channel.position = {track.x, track.y, track.z};
          if(stageDevice.deviceId == localDevice->_id) {
            device_channel.sourceport = *track.ovSourcePort;
          }
          device_channels.push_back(device_channel);
        }
      }
      stage_device.channels = device_channels;

      // TODO: REPLACE WITH DATA MODEL
      stage_device.position = {0, 0, 0};
      stage_device.orientation = {0, 0, 0};
      stage_device.mute = false;

      stage_device.sendlocal = true;

      stage_devices[stage_device.id] = stage_device;
      if(stageDevice.deviceId == localDevice->_id) {
        std::cout << "THIS DEV" << std::endl;
        renderer->set_thisdev(stage_device);
      }
    }
    renderer->set_stage(stage_devices);
  }
  catch(std::exception& exception) {
    std::cerr << "Internal error: " << exception.what() << std::endl;
  }
}

void ov_ds_sockethandler_t::listen() noexcept
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::syncWholeStage" << std::endl;
#endif
  // Add handler
  client->ready.connect(&ov_ds_sockethandler_t::onReady, this);
  client->deviceChanged.connect(&ov_ds_sockethandler_t::onDeviceChanged, this);
  client->stageJoined.connect(&ov_ds_sockethandler_t::onStageJoined, this);
  client->stageChanged.connect(&ov_ds_sockethandler_t::onStageChanged, this);
  client->stageLeft.connect(&ov_ds_sockethandler_t::onStageLeft, this);
  client->soundCardAdded.connect(&ov_ds_sockethandler_t::onSoundCardAdded,
                                 this);
  client->soundCardChanged.connect(&ov_ds_sockethandler_t::onSoundCardChanged,
                                   this);
  client->audioTrackAdded.connect(&ov_ds_sockethandler_t::onAudioTrackAdded,
                                  this);
  client->audioTrackRemoved.connect(&ov_ds_sockethandler_t::onAudioTrackRemoved,
                                    this);
  client->customAudioTrackPositionChanged.connect(
      &ov_ds_sockethandler_t::onCustomAudioTrackPositionChanged, this);
  client->customAudioTrackVolumeChanged.connect(
      &ov_ds_sockethandler_t::onCustomAudioTrackVolumeChanged, this);
}

void ov_ds_sockethandler_t::unlisten() noexcept
{
#ifdef SHOWDEBUG
  std::cout << "ov_ds_sockethandler_t::unlisten" << std::endl;
#endif
  // Remove handler
  client->ready.disconnect(&ov_ds_sockethandler_t::onReady, this);
  client->deviceChanged.disconnect(&ov_ds_sockethandler_t::onDeviceChanged,
                                   this);
  client->stageJoined.disconnect(&ov_ds_sockethandler_t::onStageJoined, this);
  client->stageChanged.disconnect(&ov_ds_sockethandler_t::onStageChanged, this);
  client->stageLeft.disconnect(&ov_ds_sockethandler_t::onStageLeft, this);
  client->soundCardAdded.disconnect(&ov_ds_sockethandler_t::onSoundCardAdded,
                                    this);
  client->soundCardChanged.disconnect(
      &ov_ds_sockethandler_t::onSoundCardChanged, this);
  client->audioTrackAdded.disconnect(&ov_ds_sockethandler_t::onAudioTrackAdded,
                                     this);
  client->audioTrackRemoved.disconnect(
      &ov_ds_sockethandler_t::onAudioTrackRemoved, this);

  if(renderer) {
    renderer->clear_stage();
    renderer->stop_audiobackend();
  }
}
void ov_ds_sockethandler_t::onCustomAudioTrackPositionChanged(
    const std::string&, const nlohmann::json&,
    const DigitalStage::Api::Store*) noexcept
{
  if(insideOvStage) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onAudioTrackPositionChanged"
              << std::endl;
#endif
    // TODO: Implement
  }
}
void ov_ds_sockethandler_t::onCustomAudioTrackVolumeChanged(
    const std::string&, const nlohmann::json&,
    const DigitalStage::Api::Store*) noexcept
{
  if(insideOvStage) {
#ifdef SHOWDEBUG
    std::cout << "ov_ds_sockethandler_t::onAudioTrackVolumeChanged"
              << std::endl;
#endif
    // TODO: Implement
  }
}
