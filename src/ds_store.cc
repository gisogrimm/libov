/*
 * Copyright (c) 2021 delude88, Tobias Hegemann, Giso Grimm
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

#include "ds_store.h"
#include <iostream>

ds::ds_store_t::ds_store_t() {}

ds::ds_store_t::~ds_store_t() {}

void ds::ds_store_t::createStage(const nlohmann::json stage)
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  const std::string _id = stage.at("_id").get<std::string>();
  this->stages_[_id] = stage;
}

void ds::ds_store_t::updateStage(const std::string& stageId,
                                 const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  this->stages_[stageId].merge_patch(update);
}

std::optional<const ds::stage_t>
ds::ds_store_t::readStage(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  if(this->stages_.count(stageId) > 0)
    return this->stages_[stageId].get<ds::stage_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeStage(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  this->stages_.erase(stageId);
}

void ds::ds_store_t::createGroup(const nlohmann::json group)
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  const std::string _id = group.at("_id").get<std::string>();
  this->groups_[_id] = group;
}

void ds::ds_store_t::updateGroup(const std::string& id,
                                 const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  this->groups_[id].merge_patch(update);
}

std::optional<const ds::group_t>
ds::ds_store_t::readGroup(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  if(this->groups_.count(id) > 0)
    return this->groups_[id].get<ds::group_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeGroup(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  this->groups_.erase(id);
}

const std::string& ds::ds_store_t::getCurrentStageId()
{
  std::lock_guard<std::recursive_mutex>(this->current_stage_id_mutex_);
  return this->currentStageId_;
}

void ds::ds_store_t::setCurrentStageId(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->current_stage_id_mutex_);
  this->currentStageId_ = stageId;
  // Also update current stage member
}

void ds::ds_store_t::setLocalDevice(const nlohmann::json localDevice)
{
  std::lock_guard<std::recursive_mutex>(this->local_device_mutex_);
  this->localDevice_ = localDevice;
}

std::optional<const ds::device_t> ds::ds_store_t::getLocalDevice()
{
  std::lock_guard<std::recursive_mutex>(this->local_device_mutex_);
  if(!(this->localDevice_.is_null())) {
    return this->localDevice_.get<ds::device_t>();
  }
  return std::nullopt;
}

void ds::ds_store_t::updateLocalDevice(const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->local_device_mutex_);
  this->localDevice_.merge_patch(update);
}

void ds::ds_store_t::setLocalUser(const nlohmann::json localUser)
{
  std::lock_guard<std::recursive_mutex>(this->local_user_mutex_);
  this->localUser_ = localUser;
}

std::optional<const ds::user_t> ds::ds_store_t::getLocalUser()
{
  std::lock_guard<std::recursive_mutex>(this->local_user_mutex_);
  if(!(this->localUser_.is_null()))
    return this->localUser_.get<ds::user_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeStages()
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  this->stages_.clear();
}

void ds::ds_store_t::removeGroups()
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  this->groups_.clear();
}

void ds::ds_store_t::createCustomGroupPosition(const nlohmann::json customGroup)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  const std::string _id = customGroup.at("_id").get<std::string>();
  this->customGroupPositions_[_id] = customGroup;
  this->customGroupPositionIdByGroupId_[customGroup["groupId"]] = _id;
}

void ds::ds_store_t::updateCustomGroupPosition(const std::string& id,
                                               const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  this->customGroupPositions_[id].merge_patch(update);
}

std::optional<const ds::custom_group_position_t>
ds::ds_store_t::readCustomGroupPosition(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  if(this->customGroupPositions_.count(id) > 0)
    return this->customGroupPositions_[id].get<ds::custom_group_position_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeCustomGroupPosition(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  const std::string groupId =
      this->customGroupPositions_[id].at("groupId").get<std::string>();
  this->customGroupPositions_.erase(id);
  this->customGroupPositionIdByGroupId_.erase(groupId);
}

void ds::ds_store_t::removeCustomGroupPositions()
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  this->customGroupPositions_.clear();
  this->customGroupPositionIdByGroupId_.clear();
}

void ds::ds_store_t::createCustomGroupVolume(const nlohmann::json customGroup)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  const std::string _id = customGroup.at("_id").get<std::string>();
  this->customGroupVolumes_[_id] = customGroup;
  this->customGroupVolumeIdByGroupId_[customGroup["groupId"]] = _id;
}

void ds::ds_store_t::updateCustomGroupVolume(const std::string& id,
                                             const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  this->customGroupVolumes_[id].merge_patch(update);
}

std::optional<const ds::custom_group_volume_t>
ds::ds_store_t::readCustomGroupVolume(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  if(this->customGroupVolumes_.count(id) > 0)
    return this->customGroupVolumes_[id].get<ds::custom_group_volume_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeCustomGroupVolume(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  const std::string groupId =
      this->customGroupVolumes_[id].at("groupId").get<std::string>();
  this->customGroupVolumes_.erase(id);
  this->customGroupVolumeIdByGroupId_.erase(groupId);
}

void ds::ds_store_t::removeCustomGroupVolumes()
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  this->customGroupVolumes_.clear();
  this->customGroupVolumeIdByGroupId_.clear();
}

void ds::ds_store_t::createStageMember(const nlohmann::json stageMember)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  const std::string _id = stageMember.at("_id").get<std::string>();
  const std::string stageId = stageMember.at("stageId").get<std::string>();
  const std::string groupId = stageMember.at("groupId").get<std::string>();
  this->stageMembers_[_id] = stageMember;
  this->stageMemberIdsByStageId_[stageId].push_back(_id);
  this->stageMemberIdsByStageId_[groupId].push_back(_id);
  const auto localUser = this->getLocalUser();
  if(localUser && localUser->_id == stageMember["userId"]) {
    this->currentStageMemberId_ = _id;
  }
}

void ds::ds_store_t::updateStageMember(const std::string& id,
                                       const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  this->stageMembers_[id].merge_patch(update);
}

std::optional<const ds::stage_member_t>
ds::ds_store_t::readStageMember(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  if(this->stageMembers_.count(id) > 0) {
    return this->stageMembers_[id].get<ds::stage_member_t>();
  }
  return std::nullopt;
}

const std::vector<ds::stage_member_t>
ds::ds_store_t::readStageMembersByStage(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  std::vector<ds::stage_member_t> stageMembers;
  if(this->stageMemberIdsByStageId_.count(stageId) > 0) {
    for(const auto& stageMemberId : this->stageMemberIdsByStageId_[stageId]) {
      const auto stageMember = this->readStageMember(stageMemberId);
      if(stageMember) {
        stageMembers.push_back(*stageMember);
      }
    }
  }
  return stageMembers;
}

const std::vector<ds::stage_member_t>
ds::ds_store_t::readStageMembersByGroup(const std::string& groupId)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  std::vector<ds::stage_member_t> stageMembers;
  if(this->stageMemberIdsByGroupId_.count(groupId) > 0) {
    for(const auto& stageMemberId : this->stageMemberIdsByGroupId_[groupId]) {
      const auto stageMember = this->readStageMember(stageMemberId);
      if(stageMember) {
        stageMembers.push_back(*stageMember);
      }
    }
  }
  return stageMembers;
}

void ds::ds_store_t::removeStageMember(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  const std::string stageId =
      this->stageMembers_[id].at("stageId").get<std::string>();
  const std::string groupId =
      this->stageMembers_[id].at("groupId").get<std::string>();
  const std::string userId =
      this->stageMembers_[id].at("userId").get<std::string>();
  this->stageMembers_.erase(id);
  std::remove(this->stageMemberIdsByStageId_[stageId].begin(),
              this->stageMemberIdsByStageId_[stageId].end(), id);
  std::remove(this->stageMemberIdsByGroupId_[groupId].begin(),
              this->stageMemberIdsByGroupId_[groupId].end(), id);
  const auto localUser = this->getLocalUser();
  if(localUser && localUser->_id == userId) {
    this->currentStageMemberId_.clear();
  }
}

void ds::ds_store_t::removeStageMembers()
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  this->stageMembers_.clear();
  this->stageMemberIdsByStageId_.clear();
}

void ds::ds_store_t::createCustomStageMemberPosition(
    const nlohmann::json customStageMember)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  std::string _id = customStageMember.at("_id").get<std::string>();
  std::string stageMemberId =
      customStageMember["stageMemberId"].get<std::string>();
  this->customStageMemberPositions_[_id] = customStageMember;
  this->customStageMemberPositionIdByStageMemberId_[stageMemberId] = _id;
}

void ds::ds_store_t::updateCustomStageMemberPosition(
    const std::string& id, const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  this->customStageMemberPositions_[id].merge_patch(update);
  // We may implement the change of the stageMemberId, but this should never
  // happen if backend works as expected
}

std::optional<const ds::custom_stage_member_position_t>
ds::ds_store_t::readCustomStageMemberPosition(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  if(this->customStageMemberPositions_.count(id) > 0) {
    const ds::custom_stage_member_position_t customStageMember =
        this->customStageMemberPositions_[id]
            .get<ds::custom_stage_member_position_t>();
    return customStageMember;
  }
  return std::nullopt;
}

std::optional<const ds::custom_stage_member_position_t>
ds::ds_store_t::readCustomStageMemberPositionByStageMember(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  if(this->customStageMemberPositionIdByStageMemberId_.count(stageMemberId)) {
    return this->readCustomStageMemberPosition(
        this->customStageMemberPositionIdByStageMemberId_[stageMemberId]);
  }
  return std::nullopt;
}

void ds::ds_store_t::removeCustomStageMemberPosition(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  const std::string stageMemberId =
      this->customStageMemberPositions_.at("stageMemberId").get<std::string>();
  this->customStageMemberPositions_.erase(id);
  this->customStageMemberPositionIdByStageMemberId_.erase(stageMemberId);
}

void ds::ds_store_t::removeCustomStageMemberPositions()
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  this->customStageMemberPositions_.clear();
  this->customStageMemberPositionIdByStageMemberId_.clear();
}

void ds::ds_store_t::createCustomStageMemberVolume(
    const nlohmann::json customStageMember)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  std::string _id = customStageMember.at("_id").get<std::string>();
  std::string stageMemberId =
      customStageMember["stageMemberId"].get<std::string>();
  this->customStageMemberVolumes_[_id] = customStageMember;
  this->customStageMemberVolumeIdByStageMemberId_[stageMemberId] = _id;
}

void ds::ds_store_t::updateCustomStageMemberVolume(const std::string& id,
                                                   const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  this->customStageMemberVolumes_[id].merge_patch(update);
  // We may implement the change of the stageMemberId, but this should never
  // happen if backend works as expected
}

std::optional<const ds::custom_stage_member_volume_t>
ds::ds_store_t::readCustomStageMemberVolume(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  if(this->customStageMemberVolumes_.count(id) > 0) {
    const ds::custom_stage_member_volume_t customStageMember =
        this->customStageMemberVolumes_[id]
            .get<ds::custom_stage_member_volume_t>();
    return customStageMember;
  }
  return std::nullopt;
}

std::optional<const ds::custom_stage_member_volume_t>
ds::ds_store_t::readCustomStageMemberVolumeByStageMember(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  if(this->customStageMemberVolumeIdByStageMemberId_.count(stageMemberId)) {
    return this->readCustomStageMemberVolume(
        this->customStageMemberVolumeIdByStageMemberId_[stageMemberId]);
  }
  return std::nullopt;
}

void ds::ds_store_t::removeCustomStageMemberVolume(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  const std::string stageMemberId =
      this->customStageMemberVolumes_.at("stageMemberId").get<std::string>();
  this->customStageMemberVolumes_.erase(id);
  this->customStageMemberVolumeIdByStageMemberId_.erase(stageMemberId);
}

void ds::ds_store_t::removeCustomStageMemberVolumes()
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  this->customStageMemberVolumes_.clear();
  this->customStageMemberVolumeIdByStageMemberId_.clear();
}

void ds::ds_store_t::createSoundCard(const nlohmann::json soundCard)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  const std::string _id = soundCard.at("_id").get<std::string>();
  this->soundCards_[_id] = soundCard;
  this->soundCardIdByName_[soundCard["name"]] = _id;
}

void ds::ds_store_t::updateSoundCard(const std::string& id,
                                     const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  this->soundCards_[id].merge_patch(update);
}

std::optional<const ds::soundcard_t>
ds::ds_store_t::readSoundCard(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  if(this->soundCards_.count(id) > 0)
    return this->soundCards_[id].get<ds::soundcard_t>();
  return std::nullopt;
}

std::optional<const ds::soundcard_t>
ds::ds_store_t::readSoundCardByName(const std::string& name)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  if(this->soundCardIdByName_.count(name) > 0)
    return this->readSoundCard(this->soundCardIdByName_[name]);
  return std::nullopt;
}

void ds::ds_store_t::removeSoundCard(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  const std::string soundCardName =
      this->soundCards_[id].at("name").get<std::string>();
  this->soundCards_.erase(id);
  this->soundCardIdByName_.erase(soundCardName);
}

void ds::ds_store_t::removeSoundCards()
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  this->soundCards_.clear();
  this->soundCardIdByName_.clear();
}

void ds::ds_store_t::createOvTrack(const nlohmann::json ovTrack)
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  const std::string _id = ovTrack.at("_id").get<std::string>();
  this->ovTracks_[_id] = ovTrack;
}

void ds::ds_store_t::updateOvTrack(const std::string& id,
                                   const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  this->ovTracks_[id].merge_patch(update);
}

std::optional<const ds::ov_track_t>
ds::ds_store_t::readOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  if(this->ovTracks_.count(id) > 0)
    return this->ovTracks_[id].get<ds::ov_track_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  this->ovTracks_.erase(id);
}

void ds::ds_store_t::removeOvTracks()
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  this->ovTracks_.clear();
}

void ds::ds_store_t::createRemoteOvTrack(const nlohmann::json remoteOvTrack)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  const std::string _id = remoteOvTrack.at("_id").get<std::string>();
  this->remoteOvTracks_[_id] = remoteOvTrack;
  this->remoteOvTrackIdsByStageMemberId_[remoteOvTrack["stageMemberId"]]
      .push_back(_id);
}

void ds::ds_store_t::updateRemoteOvTrack(const std::string& id,
                                         const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  this->remoteOvTracks_[id].merge_patch(update);
}

std::optional<const ds::remote_ov_track_t>
ds::ds_store_t::readRemoteOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  if(this->remoteOvTracks_.count(id) > 0)
    return this->remoteOvTracks_[id].get<ds::remote_ov_track_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeRemoteOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  const std::string stageMemberId =
      this->remoteOvTracks_[id].at("stageMemberId").get<std::string>();
  std::remove(this->remoteOvTrackIdsByStageMemberId_[stageMemberId].begin(),
              this->remoteOvTrackIdsByStageMemberId_[stageMemberId].end(), id);
  this->remoteOvTracks_.erase(id);
}

void ds::ds_store_t::removeRemoteOvTracks()
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  this->remoteOvTracks_.clear();
  this->remoteOvTrackIdsByStageMemberId_.clear();
}

void ds::ds_store_t::createCustomRemoteOvTrackPosition(
    const nlohmann::json customRemoteOvTrack)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  const std::string _id = customRemoteOvTrack.at("_id").get<std::string>();
  this->customRemoteOvTrackPositions_[_id] = customRemoteOvTrack;
  this->customOvTrackPositionIdByOvTrackId_
      [customRemoteOvTrack["remoteOvTrackId"]] = _id;
}

void ds::ds_store_t::updateCustomRemoteOvTrackPosition(
    const std::string& id, const nlohmann::json update)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  this->customRemoteOvTrackPositions_[id].merge_patch(update);
}

std::optional<const ds::custom_remote_ov_track_position_t>
ds::ds_store_t::readCustomRemoteOvTrackPosition(const std::string& id)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  if(this->customRemoteOvTrackPositions_.count(id) > 0)
    return this->customRemoteOvTrackPositions_[id]
        .get<ds::custom_remote_ov_track_position_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeCustomRemoteOvTrackPosition(const std::string& id)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  const std::string remoteOvTrackId = this->customRemoteOvTrackPositions_[id]
                                          .at("remoteOvTrackId")
                                          .get<std::string>();
  this->customRemoteOvTrackPositions_.erase(id);
  this->customOvTrackPositionIdByOvTrackId_[remoteOvTrackId];
}

void ds::ds_store_t::removeCustomRemoteOvTrackPositions()
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  this->customRemoteOvTrackPositions_.clear();
  this->customOvTrackPositionIdByOvTrackId_.clear();
}

void ds::ds_store_t::createCustomRemoteOvTrackVolume(
    const nlohmann::json customRemoteOvTrack)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  const std::string _id = customRemoteOvTrack.at("_id").get<std::string>();
  this->customRemoteOvTrackVolumes_[_id] = customRemoteOvTrack;
  this->customOvTrackVolumeIdByOvTrackId_
      [customRemoteOvTrack["remoteOvTrackId"]] = _id;
}

void ds::ds_store_t::updateCustomRemoteOvTrackVolume(
    const std::string& id, const nlohmann::json update)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  this->customRemoteOvTrackVolumes_[id].merge_patch(update);
}

std::optional<const ds::custom_remote_ov_track_volume_t>
ds::ds_store_t::readCustomRemoteOvTrackVolume(const std::string& id)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  if(this->customRemoteOvTrackVolumes_.count(id) > 0)
    return this->customRemoteOvTrackVolumes_[id]
        .get<ds::custom_remote_ov_track_volume_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeCustomRemoteOvTrackVolume(const std::string& id)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  const std::string remoteOvTrackId = this->customRemoteOvTrackVolumes_[id]
                                          .at("remoteOvTrackId")
                                          .get<std::string>();
  this->customRemoteOvTrackVolumes_.erase(id);
  this->customOvTrackVolumeIdByOvTrackId_[remoteOvTrackId];
}

void ds::ds_store_t::removeCustomRemoteOvTrackVolumes()
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  this->customRemoteOvTrackVolumes_.clear();
  this->customOvTrackVolumeIdByOvTrackId_.clear();
}

const std::vector<ds::ov_track_t> ds::ds_store_t::readOvTracks()
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  std::vector<ds::ov_track_t> tracks;
  for(const auto& pair : this->ovTracks_) {
    tracks.push_back(pair.second);
  }
  return tracks;
}

std::optional<const ds::stage_t> ds::ds_store_t::getCurrentStage()
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  const std::string currentStageId = this->getCurrentStageId();
  if(!currentStageId.empty()) {
    return this->readStage(currentStageId);
  }
  return std::nullopt;
}

std::optional<const ds::stage_member_t> ds::ds_store_t::getCurrentStageMember()
{
  if(!this->currentStageMemberId_.empty()) {
    return this->readStageMember(this->currentStageMemberId_);
  }
  return std::nullopt;
}

std::optional<const ds::custom_stage_member_position_t>
ds::ds_store_t::getCustomStageMemberPositionByStageMemberId(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_positions_mutex_);
  if(this->customStageMemberPositionIdByStageMemberId_.count(stageMemberId) >
     0) {
    const std::string customStageMemberId =
        this->customStageMemberPositionIdByStageMemberId_[stageMemberId];
    std::optional<const ds::custom_stage_member_position_t> customStageMember =
        this->readCustomStageMemberPosition(customStageMemberId);
    return customStageMember;
  }
  return std::nullopt;
}
std::optional<const ds::custom_stage_member_volume_t>
ds::ds_store_t::getCustomStageMemberVolumeByStageMemberId(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(
      this->custom_stage_member_volumes_mutex_);
  if(this->customStageMemberVolumeIdByStageMemberId_.count(stageMemberId) > 0) {
    const std::string customStageMemberId =
        this->customStageMemberVolumeIdByStageMemberId_[stageMemberId];
    std::optional<const ds::custom_stage_member_volume_t> customStageMember =
        this->readCustomStageMemberVolume(customStageMemberId);
    return customStageMember;
  }
  return std::nullopt;
}

std::optional<const ds::custom_group_position_t>
ds::ds_store_t::getCustomGroupPositionByGroupId(
    const std::string& customStageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_positions_mutex_);
  if(this->customGroupPositionIdByGroupId_.count(customStageMemberId) > 0) {
    return this->readCustomGroupPosition(
        this->customGroupPositionIdByGroupId_[customStageMemberId]);
  }
  return std::nullopt;
}
std::optional<const ds::custom_group_volume_t>
ds::ds_store_t::getCustomGroupVolumeByGroupId(
    const std::string& customStageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->custom_group_volumes_mutex_);
  if(this->customGroupPositionIdByGroupId_.count(customStageMemberId) > 0) {
    return this->readCustomGroupVolume(
        this->customGroupVolumeIdByGroupId_[customStageMemberId]);
  }
  return std::nullopt;
}

std::optional<const ds::custom_remote_ov_track_position_t>
ds::ds_store_t::getCustomOvTrackPositionByOvTrackId(
    const std::string& ovTrackId)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_positions_mutex_);
  if(this->customOvTrackPositionIdByOvTrackId_.count(ovTrackId) > 0) {
    return this->readCustomRemoteOvTrackPosition(
        this->customOvTrackPositionIdByOvTrackId_[ovTrackId]);
  }
  return std::nullopt;
}

std::optional<const ds::custom_remote_ov_track_volume_t>
ds::ds_store_t::getCustomOvTrackVolumeByOvTrackId(const std::string& ovTrackId)
{
  auto lock = std::unique_lock<std::recursive_mutex>(
      this->custom_remote_ov_track_volumes_mutex_);
  if(this->customOvTrackVolumeIdByOvTrackId_.count(ovTrackId) > 0) {
    return this->readCustomRemoteOvTrackVolume(
        this->customOvTrackVolumeIdByOvTrackId_[ovTrackId]);
  }
  return std::nullopt;
}

const std::vector<ds::remote_ov_track_t>
ds::ds_store_t::getRemoteOvTracksByStageMemberId(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  std::vector<ds::remote_ov_track_t> remoteTracks;
  if(this->remoteOvTrackIdsByStageMemberId_.count(stageMemberId) > 0) {
    for(const std::string& remoteOvTrackId :
        this->remoteOvTrackIdsByStageMemberId_[stageMemberId]) {
      const auto remoteOvTrack = this->readRemoteOvTrack(remoteOvTrackId);
      if(remoteOvTrack) {
        remoteTracks.push_back(*remoteOvTrack);
      }
    }
  }
  return remoteTracks;
}

void ds::ds_store_t::createUser(const nlohmann::json user)
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  const std::string _id = user.at("_id").get<std::string>();
  this->users_[_id] = user;
}

void ds::ds_store_t::updateUser(const std::string& id,
                                const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  this->users_[id].merge_patch(update);
}

std::optional<const ds::user_t> ds::ds_store_t::readUser(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  if(this->users_.count(id) > 0)
    return this->users_[id].get<ds::user_t>();
  return std::nullopt;
}

void ds::ds_store_t::removeUser(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  this->users_.erase(id);
}

void ds::ds_store_t::removeUsers()
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  this->users_.clear();
}

void ds::ds_store_t::dump()
{
  std::cout << "----- STATISTICS: ------" << std::endl;

  auto lock1 = std::unique_lock<std::recursive_mutex>(this->users_mutex_);
  auto lock2 = std::unique_lock<std::recursive_mutex>(this->stages_mutex_);
  auto lock3 = std::unique_lock<std::recursive_mutex>(this->groups_mutex_);
  auto lock4 =
      std::unique_lock<std::recursive_mutex>(this->stage_members_mutex_);
  std::cout << this->users_.size() << " users" << std::endl;
  std::cout << this->stages_.size() << " stages" << std::endl;
  std::cout << this->groups_.size() << " groups" << std::endl;
  std::cout << this->stageMembers_.size() << " stage members" << std::endl;
}