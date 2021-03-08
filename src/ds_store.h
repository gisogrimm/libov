//
// Created by Tobias Hegemann on 24.02.21.
//

#ifndef DS_STORE_H
#define DS_STORE_H

#include "ds_json_types.h"
#include <boost/optional.hpp>
#include <nlohmann/json.hpp>

namespace ds {
  class ds_store_t {
  public:
    ds_store_t();
    ~ds_store_t();

    void setLocalDevice(const nlohmann::json localDevice);

    void updateLocalDevice(const nlohmann::json update);

    boost::optional<const ds::json::device_t> getLocalDevice();

    void setLocalUser(const nlohmann::json localUser);

    boost::optional<const ds::json::user_t> getLocalUser();

    void createUser(const nlohmann::json user);

    void updateUser(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::user_t> readUser(const std::string& id);

    void removeUser(const std::string& id);

    void removeUsers();

    void createStage(const nlohmann::json stage);

    void updateStage(const std::string& stageId, const nlohmann::json update);

    boost::optional<const ds::json::stage_t>
    readStage(const std::string& stageId);

    void removeStage(const std::string& stageId);

    void removeStages();

    void createGroup(const nlohmann::json group);

    void updateGroup(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::group_t> readGroup(const std::string& id);

    void removeGroup(const std::string& id);

    void removeGroups();

    void createCustomGroupPosition(const nlohmann::json customGroup);

    void updateCustomGroupPosition(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::custom_group_position_t>
    readCustomGroupPosition(const std::string& id);

    void removeCustomGroupPosition(const std::string& id);

    void removeCustomGroupPositions();

    void createCustomGroupVolume(const nlohmann::json customGroup);

    void updateCustomGroupVolume(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::custom_group_volume_t>
    readCustomGroupVolume(const std::string& id);

    void removeCustomGroupVolume(const std::string& id);

    void removeCustomGroupVolumes();

    void createStageMember(const nlohmann::json stageMember);

    void updateStageMember(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::stage_member_t>
    readStageMember(const std::string& id);

    const std::vector<const ds::json::stage_member_t>
    readStageMembersByStage(const std::string& stageId);

    const std::vector<const ds::json::stage_member_t>
    readStageMembersByGroup(const std::string& groupId);

    void removeStageMember(const std::string& id);

    void removeStageMembers();

    void createCustomStageMemberPosition(const nlohmann::json customStageMember);

    void updateCustomStageMemberPosition(const std::string& id,
                                 const nlohmann::json update);

    boost::optional<const ds::json::custom_stage_member_position_t>
    readCustomStageMemberPosition(const std::string& id);

    boost::optional<const ds::json::custom_stage_member_position_t>
    readCustomStageMemberPositionByStageMember(const std::string& stageMemberId);

    void removeCustomStageMemberPosition(const std::string& id);

    void removeCustomStageMemberPositions();

    void createCustomStageMemberVolume(const nlohmann::json customStageMember);

    void updateCustomStageMemberVolume(const std::string& id,
                                 const nlohmann::json update);

    boost::optional<const ds::json::custom_stage_member_volume_t>
    readCustomStageMemberVolume(const std::string& id);

    boost::optional<const ds::json::custom_stage_member_volume_t>
    readCustomStageMemberVolumeByStageMember(const std::string& stageMemberId);

    void removeCustomStageMemberVolume(const std::string& id);

    void removeCustomStageMemberVolumes();

    void createSoundCard(const nlohmann::json soundCard);

    void updateSoundCard(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::soundcard_t>
    readSoundCard(const std::string& id);

    boost::optional<const ds::json::soundcard_t>
    readSoundCardByName(const std::string& name);

    void removeSoundCard(const std::string& id);

    void removeSoundCards();

    void createOvTrack(const nlohmann::json ovTrack);

    void updateOvTrack(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::ov_track_t>
    readOvTrack(const std::string& id);

    const std::vector<ds::json::ov_track_t> readOvTracks();

    void removeOvTrack(const std::string& id);

    void removeOvTracks();

    void createRemoteOvTrack(const nlohmann::json remoteOvTrack);

    void updateRemoteOvTrack(const std::string& id,
                             const nlohmann::json update);

    boost::optional<const ds::json::remote_ov_track_t>
    readRemoteOvTrack(const std::string& id);

    void removeRemoteOvTrack(const std::string& id);

    void removeRemoteOvTracks();

    void createCustomRemoteOvTrackPosition(const nlohmann::json customRemoteOvTrack);

    void updateCustomRemoteOvTrackPosition(const std::string& id,
                                   const nlohmann::json update);

    boost::optional<const ds::json::custom_remote_ov_track_position_t>
    readCustomRemoteOvTrackPosition(const std::string& id);

    void removeCustomRemoteOvTrackPosition(const std::string& id);

    void removeCustomRemoteOvTrackPositions();

    void createCustomRemoteOvTrackVolume(const nlohmann::json customRemoteOvTrack);

    void updateCustomRemoteOvTrackVolume(const std::string& id,
                                   const nlohmann::json update);

    boost::optional<const ds::json::custom_remote_ov_track_volume_t>
    readCustomRemoteOvTrackVolume(const std::string& id);

    void removeCustomRemoteOvTrackVolume(const std::string& id);

    void removeCustomRemoteOvTrackVolumes();

    /* CUSTOMIZED METHODS */

    const std::string& getCurrentStageId();

    void setCurrentStageId(const std::string& stageId);

    boost::optional<const ds::json::stage_t> getCurrentStage();

    boost::optional<const ds::json::stage_member_t> getCurrentStageMember();

    boost::optional<const ds::json::custom_stage_member_position_t>
    getCustomStageMemberPositionByStageMemberId(const std::string& stageMemberId);

    boost::optional<const ds::json::custom_stage_member_volume_t>
    getCustomStageMemberVolumeByStageMemberId(const std::string& stageMemberId);

    boost::optional<const ds::json::custom_group_position_t>
    getCustomGroupPositionByGroupId(const std::string& customStageMemberId);

    boost::optional<const ds::json::custom_group_volume_t>
    getCustomGroupVolumeByGroupId(const std::string& customStageMemberId);

    boost::optional<const ds::json::custom_remote_ov_track_position_t>
    getCustomOvTrackPositionByOvTrackId(const std::string& ovTrackId);

    boost::optional<const ds::json::custom_remote_ov_track_volume_t>
    getCustomOvTrackVolumeByOvTrackId(const std::string& ovTrackId);

    const std::vector<const ds::json::remote_ov_track_t>
    getRemoteOvTracksByStageMemberId(const std::string& stageMemberId);

    void dump();

  private:
    std::recursive_mutex local_device_mutex_;
    std::recursive_mutex local_user_mutex_;
    std::recursive_mutex users_mutex_;
    std::recursive_mutex stages_mutex_;
    std::recursive_mutex groups_mutex_;
    std::recursive_mutex custom_group_positions_mutex_;
    std::recursive_mutex custom_group_volumes_mutex_;
    std::recursive_mutex stage_members_mutex_;
    std::recursive_mutex custom_stage_member_positions_mutex_;
    std::recursive_mutex custom_stage_member_volumes_mutex_;
    std::recursive_mutex sound_cards_mutex_;
    std::recursive_mutex ov_tracks_mutex_;
    std::recursive_mutex remote_ov_tracks_mutex_;
    std::recursive_mutex custom_remote_ov_track_positions_mutex_;
    std::recursive_mutex custom_remote_ov_track_volumes_mutex_;
    std::recursive_mutex current_stage_id_mutex_;

    // Internal data store
    std::map<std::string, nlohmann::json> customRemoteOvTrackPositions_;
    std::map<std::string, nlohmann::json> customRemoteOvTrackVolumes_;
    std::map<std::string, nlohmann::json> remoteOvTracks_;
    std::map<std::string, nlohmann::json> customStageMemberTrackPositions_;
    std::map<std::string, nlohmann::json> customStageMemberTrackVolumes_;
    std::map<std::string, nlohmann::json> stageMemberTracks_;
    std::map<std::string, nlohmann::json> customStageMemberPositions_;
    std::map<std::string, nlohmann::json> customStageMemberVolumes_;
    std::map<std::string, nlohmann::json> stageMembers_;
    std::map<std::string, nlohmann::json> customGroupPositions_;
    std::map<std::string, nlohmann::json> customGroupVolumes_;
    std::map<std::string, nlohmann::json> groups_;
    std::map<std::string, nlohmann::json> stages_;
    std::map<std::string, nlohmann::json> ovTracks_;
    std::map<std::string, nlohmann::json> soundCards_;
    std::map<std::string, nlohmann::json> users_;
    nlohmann::json localDevice_;
    nlohmann::json localUser_;
    std::string currentStageId_;
    std::string currentStageMemberId_;

    // Helper data stores
    std::map<std::string, std::string> customGroupPositionIdByGroupId_;
    std::map<std::string, std::string> customGroupVolumeIdByGroupId_;
    std::map<std::string, std::vector<std::string>> stageMemberIdsByStageId_;
    std::map<std::string, std::vector<std::string>> stageMemberIdsByGroupId_;
    std::map<std::string, std::vector<std::string>>
        remoteOvTrackIdsByStageMemberId_;
    std::map<std::string, std::string> customStageMemberVolumeIdByStageMemberId_;
    std::map<std::string, std::string> customStageMemberPositionIdByStageMemberId_;
    std::map<std::string, std::string> customOvTrackPositionIdByOvTrackId_;
    std::map<std::string, std::string> customOvTrackVolumeIdByOvTrackId_;
    std::map<std::string, std::string> soundCardIdByName_;
  };
} // namespace ds

#endif // DS_STORE_H