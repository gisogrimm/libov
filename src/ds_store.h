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

    void createCustomGroup(const nlohmann::json customGroup);

    void updateCustomGroup(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::custom_group_t>
    readCustomGroup(const std::string& id);

    void removeCustomGroup(const std::string& id);

    void removeCustomGroups();

    void createStageMember(const nlohmann::json stageMember);

    void updateStageMember(const std::string& id, const nlohmann::json update);

    boost::optional<const ds::json::stage_member_t>
    readStageMember(const std::string& id);

    const std::vector<const ds::json::stage_member_t>
    readStageMembersByStage(const std::string& stageId);

    void removeStageMember(const std::string& id);

    void removeStageMembers();

    void createCustomStageMember(const nlohmann::json customStageMember);

    void updateCustomStageMember(const std::string& id,
                                 const nlohmann::json update);

    boost::optional<const ds::json::custom_stage_member_t>
    readCustomStageMember(const std::string& id);

    boost::optional<const ds::json::custom_stage_member_t>
    readCustomStageMemberByStageMember(const std::string& stageMemberId);

    void removeCustomStageMember(const std::string& id);

    void removeCustomStageMembers();

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

    void createCustomRemoteOvTrack(const nlohmann::json customRemoteOvTrack);

    void updateCustomRemoteOvTrack(const std::string& id,
                                   const nlohmann::json update);

    boost::optional<const ds::json::custom_remote_ov_track_t>
    readCustomRemoteOvTrack(const std::string& id);

    void removeCustomRemoteOvTrack(const std::string& id);

    void removeCustomRemoteOvTracks();

    /* CUSTOMIZED METHODS */

    const std::string& getCurrentStageId();

    void setCurrentStageId(const std::string& stageId);

    boost::optional<const ds::json::stage_t> getCurrentStage();

    boost::optional<const ds::json::stage_member_t> getCurrentStageMember();

    boost::optional<const ds::json::custom_stage_member_t>
    getCustomStageMemberByStageMemberId(const std::string& stageMemberId);

    boost::optional<const ds::json::custom_group_t>
    getCustomGroupByGroupId(const std::string& customStageMemberId);

    boost::optional<const ds::json::custom_remote_ov_track_t>
    getCustomOvTrackByOvTrackId(const std::string& ovTrackId);

    const std::vector<const ds::json::remote_ov_track_t>
    getRemoteOvTracksByStageMemberId(const std::string& stageMemberId);

  private:
    std::mutex local_device_mutex_;
    std::mutex local_user_mutex_;
    std::mutex users_mutex_;
    std::mutex stages_mutex_;
    std::mutex groups_mutex_;
    std::mutex custom_groups_mutex_;
    std::mutex stage_members_mutex_;
    std::mutex custom_stage_members_mutex_;
    std::mutex sound_cards_mutex_;
    std::mutex ov_tracks_mutex_;
    std::mutex remote_ov_tracks_mutex_;
    std::mutex custom_remote_ov_tracks_mutex_;
    std::mutex current_stage_id_mutex_;

    // Internal data store
    std::map<std::string, nlohmann::json> customRemoteOvTracks_;
    std::map<std::string, nlohmann::json> remoteOvTracks_;
    std::map<std::string, nlohmann::json> customStageMemberTracks_;
    std::map<std::string, nlohmann::json> stageMemberTracks_;
    std::map<std::string, nlohmann::json> customStageMembers_;
    std::map<std::string, nlohmann::json> stageMembers_;
    std::map<std::string, nlohmann::json> customGroups_;
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
    std::map<std::string, std::string> customGroupIdByGroupId_;
    std::map<std::string, std::vector<std::string>> stageMemberIdsByStageId_;
    std::map<std::string, std::vector<std::string>>
        remoteOvTrackIdsByStageMemberId_;
    std::map<std::string, std::string> customStageMemberIdByStageMemberId_;
    std::map<std::string, std::string> customOvTrackIdByOvTrackId_;
    std::map<std::string, std::string> soundCardIdByName_;
    /*
    std::map<std::string, std::string> customTracksByTrack_;
    std::map<std::string, std::string> trackByStageMember_;
    std::map<std::string, std::string> customStageMemberByStageMember_;
    std::map<std::string, std::string> stageMemberByGroup_;
    std::map<std::string, std::string> groupByStage_;
     */
  };
} // namespace ds

#endif // DS_STORE_H