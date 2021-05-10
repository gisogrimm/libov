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
#ifndef OV_DS_SOCKETHANDLER_H_
#define OV_DS_SOCKETHANDLER_H_

#include <DigitalStage/Api/Client.h>
#include <DigitalStage/Types.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <ov_render_tascar.h>

class ov_ds_sockethandler_t {
public:
  ov_ds_sockethandler_t(ov_render_base_t* renderer_,
                        DigitalStage::Api::Client* client_);
  ~ov_ds_sockethandler_t();

  void enable() noexcept;

  void disable() noexcept;

protected:
  /**
   * This handler will start the OV communication if user is inside an stage
   * with OV capabilities.
   * @param store
   */
  void onReady(const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will start the OV communication, if the stage supports OV and
   * has not yet been started (i.e. by onReady). This is done by syncing the
   * sound card, stage settings and all stage devices.
   * @param stageId
   * @param groupId
   * @param store
   */
  void onStageJoined(const DigitalStage::Types::ID_TYPE& stageId,
                     const DigitalStage::Types::ID_TYPE& groupId,
                     const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will start the OV communication if not yet started (PIN may
   * have changed, etc.) and sync the new stage values (room dimensions, other
   * OV related values)
   * @param stageId
   * @param update
   * @param store
   */
  void onStageChanged(const DigitalStage::Types::ID_TYPE& stageId,
                      const nlohmann::json& update,
                      const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will shut down the OV communication (if necessary)
   * @param store
   */
  void onStageLeft(const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will check for a change of the soundCardId value and sync the
   * sound card if needed
   * @param id
   * @param update
   * @param store
   */
  void onDeviceChanged(const std::string& id, const nlohmann::json& update,
                       const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler might create local audio tracks for all activated channels
   * @param soundCard
   * @param store
   */
  void onSoundCardAdded(const DigitalStage::Types::SoundCard& soundCard,
                        const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will create local audio tracks for all activated channels and
   * remove deprecated local audio tracks for all now disabled channels.
   * @param id
   * @param update
   * @param store
   */
  void onSoundCardChanged(const DigitalStage::Types::ID_TYPE& id,
                          const nlohmann::json& update,
                          const DigitalStage::Api::Store* store) noexcept;

  /**
   * This handler will sync TASCAR with the stage member of the added track.
   * Since TASCAR is not capable of small changes in sessions yet, the whole
   * stage member with all her/his tracks is replaced.
   * @param track
   * @param store
   */
  void onAudioTrackAdded(const DigitalStage::Types::AudioTrack& track,
                         const DigitalStage::Api::Store* store) noexcept;
  /**
   * This handler will sync TASCAR with the stage member of the added track.
   * Since TASCAR is not capable of small changes in sessions yet, the whole
   * stage member with all her/his tracks is replaced.
   * @param track
   * @param store
   */
  void onAudioTrackRemoved(const DigitalStage::Types::AudioTrack& track,
                           const DigitalStage::Api::Store* store) noexcept;

  // The following methods will respond to changes of positions and volumes
  // inside the stage and update all related TASCAR tracks (there are already
  // methods to simply update TASCAR instead of replacing the whole stage
  // member)
  void
  onCustomAudioTrackPositionChanged(const std::string&, const nlohmann::json&,
                                    const DigitalStage::Api::Store*) noexcept;
  void
  onCustomAudioTrackVolumeChanged(const std::string&, const nlohmann::json&,
                                  const DigitalStage::Api::Store*) noexcept;

  /*
  void handleGroupChanged(const DigitalStage::Types::ID_TYPE& id,
                                const nlohmann::json& update,
                                const DigitalStage::Api::Store* store);
  void
  handleCustomGroupPositionAdded(const DigitalStage::Types::CustomGroupPosition&
  position, const DigitalStage::Api::Store* store); void
  handleCustomGroupPositionChanged(const DigitalStage::Types::ID_TYPE& id,
                                         const nlohmann::json& update,
                                         const DigitalStage::Api::Store* store);
  void
  handleCustomGroupPositionRemoved(const DigitalStage::Types::ID_TYPE& id,
                                         const DigitalStage::Api::Store* store);
  void
  handleCustomGroupVolumeAdded(const DigitalStage::Types::CustomGroupVolume&
  volume, const DigitalStage::Api::Store* store); void
  handleCustomGroupVolumeChanged(const DigitalStage::Types::ID_TYPE& id,
                                       const nlohmann::json& update,
                                       const DigitalStage::Api::Store* store);
  void
  handleCustomGroupVolumeRemoved(const DigitalStage::Types::ID_TYPE& id,
                                       const DigitalStage::Api::Store* store);
  void handleStageMemberChanged(const DigitalStage::Types::ID_TYPE& id,
                                const nlohmann::json& update,
                                const DigitalStage::Api::Store* store);
  void
  handleCustomStageMemberPositionAdded(const
  DigitalStage::Types::custom_stage_member_position_t& position, const
  DigitalStage::Api::Store* store); void
  handleCustomStageMemberPositionChanged(const DigitalStage::Types::ID_TYPE& id,
                                         const nlohmann::json& update,
                                         const DigitalStage::Api::Store* store);
  void
  handleCustomStageMemberPositionRemoved(const DigitalStage::Types::ID_TYPE& id,
                                         const DigitalStage::Api::Store* store);
  void
  handleCustomStageMemberVolumeAdded(const
  DigitalStage::Types::custom_stage_member_volume_t& volume, const
  DigitalStage::Api::Store* store); void
  handleCustomStageMemberVolumeChanged(const DigitalStage::Types::ID_TYPE& id,
                                         const nlohmann::json& update,
                                         const DigitalStage::Api::Store* store);
  void
  handleCustomStageMemberVolumeRemoved(const DigitalStage::Types::ID_TYPE& id,
                                         const DigitalStage::Api::Store* store);
  */

private:
  void listen() noexcept;

  void unlisten() noexcept;

  void handleStageJoined(const DigitalStage::Types::Stage& stage,
                         const DigitalStage::Api::Store* store) noexcept(false);

  void syncSoundCard(const DigitalStage::Api::Store* store) noexcept(false);
  /**
   * Helper method to sync the input channels of the sound card with local audio
   * tracks. This will assure that there are local audio tracks only for the
   * current activated input channels.
   * @param soundCard
   */
  void syncInputChannels(const DigitalStage::Api::Store* store) noexcept(false);

  void syncWholeStage(const DigitalStage::Api::Store* store) noexcept(false);

  ov_render_base_t* renderer;
  DigitalStage::Api::Client* client;
  bool insideOvStage;
  bool enabled;
};

#endif // OV_DS_SOCKETHANDLER_H_
