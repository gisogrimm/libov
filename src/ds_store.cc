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

boost::optional<const ds::json::stage_t>
ds::ds_store_t::readStage(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  if(this->stages_.count(stageId) > 0)
    return this->stages_[stageId].get<ds::json::stage_t>();
  return boost::none;
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

boost::optional<const ds::json::group_t>
ds::ds_store_t::readGroup(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->groups_mutex_);
  if(this->groups_.count(id) > 0)
    return this->groups_[id].get<ds::json::group_t>();
  return boost::none;
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

boost::optional<const ds::json::device_t> ds::ds_store_t::getLocalDevice()
{
  std::lock_guard<std::recursive_mutex>(this->local_device_mutex_);
  if(!(this->localDevice_.is_null())) {
    return this->localDevice_.get<ds::json::device_t>();
  }
  return boost::none;
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

boost::optional<const ds::json::user_t> ds::ds_store_t::getLocalUser()
{
  std::lock_guard<std::recursive_mutex>(this->local_user_mutex_);
  if(!(this->localUser_.is_null()))
    return this->localUser_.get<ds::json::user_t>();
  return boost::none;
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

void ds::ds_store_t::createCustomGroup(const nlohmann::json customGroup)
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  const std::string _id = customGroup.at("_id").get<std::string>();
  this->customGroups_[_id] = customGroup;
  this->customGroupIdByGroupId_[customGroup["groupId"]] = _id;
}

void ds::ds_store_t::updateCustomGroup(const std::string& id,
                                       const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  this->customGroups_[id].merge_patch(update);
}

boost::optional<const ds::json::custom_group_t>
ds::ds_store_t::readCustomGroup(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  if(this->customGroups_.count(id) > 0)
    return this->customGroups_[id].get<ds::json::custom_group_t>();
  return boost::none;
}

void ds::ds_store_t::removeCustomGroup(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  const std::string groupId =
      this->customGroups_[id].at("groupId").get<std::string>();
  this->customGroups_.erase(id);
  this->customGroupIdByGroupId_.erase(groupId);
}

void ds::ds_store_t::removeCustomGroups()
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  this->customGroups_.clear();
  this->customGroupIdByGroupId_.clear();
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

boost::optional<const ds::json::stage_member_t>
ds::ds_store_t::readStageMember(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  if(this->stageMembers_.count(id) > 0) {
    return this->stageMembers_[id].get<ds::json::stage_member_t>();
  }
  return boost::none;
}

const std::vector<const ds::json::stage_member_t>
ds::ds_store_t::readStageMembersByStage(const std::string& stageId)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  std::vector<const ds::json::stage_member_t> stageMembers;
  if(this->stageMemberIdsByStageId_.count(stageId) > 0) {
    for(const auto& stageMemberId : this->stageMemberIdsByStageId_[stageId]) {
      const auto stageMember = this->readStageMember(stageMemberId);
      if(stageMember) {
        stageMembers.push_back(stageMember.get());
      }
    }
  }
  return stageMembers;
}

const std::vector<const ds::json::stage_member_t>
ds::ds_store_t::readStageMembersByGroup(const std::string& groupId)
{
  std::lock_guard<std::recursive_mutex>(this->stage_members_mutex_);
  std::vector<const ds::json::stage_member_t> stageMembers;
  if(this->stageMemberIdsByGroupId_.count(groupId) > 0) {
    for(const auto& stageMemberId : this->stageMemberIdsByGroupId_[groupId]) {
      const auto stageMember = this->readStageMember(stageMemberId);
      if(stageMember) {
        stageMembers.push_back(stageMember.get());
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

void ds::ds_store_t::createCustomStageMember(
    const nlohmann::json customStageMember)
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  std::string _id = customStageMember.at("_id").get<std::string>();
  std::string stageMemberId =
      customStageMember["stageMemberId"].get<std::string>();
  this->customStageMembers_[_id] = customStageMember;
  this->customStageMemberIdByStageMemberId_[stageMemberId] = _id;
}

void ds::ds_store_t::updateCustomStageMember(const std::string& id,
                                             const nlohmann::json update)
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  this->customStageMembers_[id].merge_patch(update);
  // We may implement the change of the stageMemberId, but this should never
  // happen if backend works as expected
}

boost::optional<const ds::json::custom_stage_member_t>
ds::ds_store_t::readCustomStageMember(const std::string& id)
{
  std::cout << "BLABLA?" << std::endl;
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  std::cout << "BLABLA?" << std::endl;
  if(this->customStageMembers_.count(id) > 0) {
    const ds::json::custom_stage_member_t customStageMember =  this->customStageMembers_[id].get<ds::json::custom_stage_member_t>();
    std::cout << "BLABLABLA" << std::endl;
    return customStageMember;
  }
  return boost::none;
}

boost::optional<const ds::json::custom_stage_member_t>
ds::ds_store_t::readCustomStageMemberByStageMember(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  if(this->customStageMemberIdByStageMemberId_.count(stageMemberId)) {
    return this->readCustomStageMember(
        this->customStageMemberIdByStageMemberId_[stageMemberId]);
  }
  return boost::none;
}

void ds::ds_store_t::removeCustomStageMember(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  const std::string stageMemberId =
      this->customStageMembers_.at("stageMemberId").get<std::string>();
  this->customStageMembers_.erase(id);
  this->customStageMemberIdByStageMemberId_.erase(stageMemberId);
}

void ds::ds_store_t::removeCustomStageMembers()
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  this->customStageMembers_.clear();
  this->customStageMemberIdByStageMemberId_.clear();
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

boost::optional<const ds::json::soundcard_t>
ds::ds_store_t::readSoundCard(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  if(this->soundCards_.count(id) > 0)
    return this->soundCards_[id].get<ds::json::soundcard_t>();
  return boost::none;
}

boost::optional<const ds::json::soundcard_t>
ds::ds_store_t::readSoundCardByName(const std::string& name)
{
  std::lock_guard<std::recursive_mutex>(this->sound_cards_mutex_);
  if(this->soundCardIdByName_.count(name) > 0)
    return this->readSoundCard(this->soundCardIdByName_[name]);
  return boost::none;
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

boost::optional<const ds::json::ov_track_t>
ds::ds_store_t::readOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  if(this->ovTracks_.count(id) > 0)
    return this->ovTracks_[id].get<ds::json::ov_track_t>();
  return boost::none;
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

boost::optional<const ds::json::remote_ov_track_t>
ds::ds_store_t::readRemoteOvTrack(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  if(this->remoteOvTracks_.count(id) > 0)
    return this->remoteOvTracks_[id].get<ds::json::remote_ov_track_t>();
  return boost::none;
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

void ds::ds_store_t::createCustomRemoteOvTrack(
    const nlohmann::json customRemoteOvTrack)
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  const std::string _id = customRemoteOvTrack.at("_id").get<std::string>();
  this->customRemoteOvTracks_[_id] = customRemoteOvTrack;
  this->customOvTrackIdByOvTrackId_[customRemoteOvTrack["remoteOvTrackId"]] =
      _id;
}

void ds::ds_store_t::updateCustomRemoteOvTrack(const std::string& id,
                                               const nlohmann::json update)
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  this->customRemoteOvTracks_[id].merge_patch(update);
}

boost::optional<const ds::json::custom_remote_ov_track_t>
ds::ds_store_t::readCustomRemoteOvTrack(const std::string& id)
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  if(this->customRemoteOvTracks_.count(id) > 0)
    return this->customRemoteOvTracks_[id]
        .get<ds::json::custom_remote_ov_track_t>();
  return boost::none;
}

void ds::ds_store_t::removeCustomRemoteOvTrack(const std::string& id)
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  const std::string remoteOvTrackId =
      this->customRemoteOvTracks_[id].at("remoteOvTrackId").get<std::string>();
  this->customRemoteOvTracks_.erase(id);
  this->customOvTrackIdByOvTrackId_[remoteOvTrackId];
}

void ds::ds_store_t::removeCustomRemoteOvTracks()
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  this->customRemoteOvTracks_.clear();
  this->customOvTrackIdByOvTrackId_.clear();
}

const std::vector<ds::json::ov_track_t> ds::ds_store_t::readOvTracks()
{
  std::lock_guard<std::recursive_mutex>(this->ov_tracks_mutex_);
  std::vector<ds::json::ov_track_t> tracks;
  for(const auto& pair : this->ovTracks_) {
    tracks.push_back(pair.second);
  }
  return tracks;
}

boost::optional<const ds::json::stage_t> ds::ds_store_t::getCurrentStage()
{
  std::lock_guard<std::recursive_mutex>(this->stages_mutex_);
  const std::string currentStageId = this->getCurrentStageId();
  if(!currentStageId.empty()) {
    return this->readStage(currentStageId);
  }
  return boost::none;
}

boost::optional<const ds::json::stage_member_t>
ds::ds_store_t::getCurrentStageMember()
{
  if(!this->currentStageMemberId_.empty()) {
    return this->readStageMember(this->currentStageMemberId_);
  }
  return boost::none;
}

boost::optional<const ds::json::custom_stage_member_t>
ds::ds_store_t::getCustomStageMemberByStageMemberId(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->custom_stage_members_mutex_);
  if(this->customStageMemberIdByStageMemberId_.count(stageMemberId) > 0) {
    const std::string customStageMemberId = this->customStageMemberIdByStageMemberId_[stageMemberId];
    std::cout << "HUI" << customStageMemberId << std::endl;
    boost::optional<const ds::json::custom_stage_member_t> customStageMember = this->readCustomStageMember(
        customStageMemberId);
    return customStageMember;
  }
  return boost::none;
}

boost::optional<const ds::json::custom_group_t>
ds::ds_store_t::getCustomGroupByGroupId(const std::string& customStageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->custom_groups_mutex_);
  if(this->customGroupIdByGroupId_.count(customStageMemberId) > 0) {
    return this->readCustomGroup(
        this->customGroupIdByGroupId_[customStageMemberId]);
  }
  return boost::none;
}

boost::optional<const ds::json::custom_remote_ov_track_t>
ds::ds_store_t::getCustomOvTrackByOvTrackId(const std::string& ovTrackId)
{
  auto lock =
      std::unique_lock<std::recursive_mutex>(this->custom_remote_ov_tracks_mutex_);
  if(this->customOvTrackIdByOvTrackId_.count(ovTrackId) > 0) {
    return this->readCustomRemoteOvTrack(
        this->customOvTrackIdByOvTrackId_[ovTrackId]);
  }
  return boost::none;
}

const std::vector<const ds::json::remote_ov_track_t>
ds::ds_store_t::getRemoteOvTracksByStageMemberId(
    const std::string& stageMemberId)
{
  std::lock_guard<std::recursive_mutex>(this->remote_ov_tracks_mutex_);
  std::vector<const ds::json::remote_ov_track_t> remoteTracks;
  if(this->remoteOvTrackIdsByStageMemberId_.count(stageMemberId) > 0) {
    for(const std::string& remoteOvTrackId :
        this->remoteOvTrackIdsByStageMemberId_[stageMemberId]) {
      const auto remoteOvTrack = this->readRemoteOvTrack(remoteOvTrackId);
      if(remoteOvTrack) {
        remoteTracks.push_back(remoteOvTrack.get());
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

boost::optional<const ds::json::user_t>
ds::ds_store_t::readUser(const std::string& id)
{
  std::lock_guard<std::recursive_mutex>(this->users_mutex_);
  if(this->users_.count(id) > 0)
    return this->users_[id].get<ds::json::user_t>();
  return boost::none;
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
  auto lock4 = std::unique_lock<std::recursive_mutex>(this->stage_members_mutex_);
  std::cout << this->users_.size() << " users" << std::endl;
  std::cout << this->stages_.size() << " stages" << std::endl;
  std::cout << this->groups_.size() << " groups" << std::endl;
  std::cout << this->stageMembers_.size() << " stage members" << std::endl;
}