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

        boost::optional<ds::json::device_t> getLocalDevice();

        void createStage(const nlohmann::json stage);

        void updateStage(const std::string &stageId, const nlohmann::json update);

        boost::optional<ds::json::stage_t> readStage(const std::string &stageId);

        void removeStage(const std::string &stageId);

        void removeStages();

        void createGroup(const nlohmann::json group);

        void updateGroup(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::group_t> readGroup(const std::string &id);

        void removeGroup(const std::string &id);

        void removeGroups();

        void createCustomGroup(const nlohmann::json customGroup);

        void updatCustomGroup(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::custom_group_t> readCustomGroup(const std::string &id);

        void removeCustomGroup(const std::string &id);

        void removeCustomGroups();

        void createStageMember(const nlohmann::json stageMember);

        void updateStageMember(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::stage_member_t> readStageMember(const std::string &id);

        void removeStageMember(const std::string &id);

        void removeStageMembers();

        void createCustomStageMember(const nlohmann::json customStageMember);

        void updateCustomStageMember(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::custom_stage_member_t> readCustomStageMember(const std::string &id);

        void removeCustomStageMember(const std::string &id);

        void removeCustomStageMembers();

        void createSoundCard(const nlohmann::json soundCard);

        void updateSoundCard(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::soundcard_t> readSoundCard(const std::string &id);

        void removeSoundCard(const std::string &id);

        void removeSoundCards();

        void createOvTrack(const nlohmann::json ovTrack);

        void updateOvTrack(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::ov_track_t> readOvTrack(const std::string &id);

        void removeOvTrack(const std::string &id);

        void removeOvTracks();

        void createRemoteOvTrack(const nlohmann::json remoteOvTrack);

        void updateRemoteOvTrack(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::remote_ov_track_t> readRemoteOvTrack(const std::string &id);

        void removeRemoteOvTrack(const std::string &id);

        void removeRemoteOvTracks();

        void createCustomRemoteOvTrack(const nlohmann::json customRemoteOvTrack);

        void updateCustomRemoteOvTrack(const std::string &id, const nlohmann::json update);

        boost::optional<ds::json::custom_remote_ov_track_t> readCustomRemoteOvTrack(const std::string &id);

        void removeCustomRemoteOvTrack(const std::string &id);

        void removeCustomRemoteOvTracks();

        const std::string& getCurrentStageId();

        void setCurrentStageId(const std::string& stageId);

    private:
        std::mutex local_device_mutex_;
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

        // Digital Stage, from detailed to outer scope
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
        nlohmann::json localDevice_;
        std::string currentStageId_;

        // Helper
        std::map<std::string, std::string> customTracksByTrack_;
        std::map<std::string, std::string> trackByStageMember_;
        std::map<std::string, std::string> customStageMemberByStageMember_;
        std::map<std::string, std::string> stageMemberByGroup_;
        std::map<std::string, std::string> groupByStage_;

        // OV / Tascar
        //std::vector<stage_device_t> stageDevices_;
        //std::map<std::string, int> stageDeviceByStageMember_;
        //stage_t stage_;
    };
}

#endif //DS_STORE_H