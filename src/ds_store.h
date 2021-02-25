//
// Created by Tobias Hegemann on 24.02.21.
//

#ifndef DS_STORE_H
#define DS_STORE_H

#include <nlohmann/json.hpp>

namespace ds {
  struct ds_store_t {
    // Digital Stage, from detailed to outer scope
    std::map<std::string, nlohmann::json> customStageMemberTracks;
    std::map<std::string, nlohmann::json> stageMemberTracks;
    std::map<std::string, nlohmann::json> customStageMembers;
    std::map<std::string, nlohmann::json> stageMembers;
    std::map<std::string, nlohmann::json> customGroups;
    std::map<std::string, nlohmann::json> groups;
    std::map<std::string, nlohmann::json> stages;

    std::map<std::string, nlohmann::json> soundCards;
    std::map<std::string, nlohmann::json> trackPresets;
    nlohmann::json localDevice;
    std::string currentStageId;

    // Helper
    std::map<std::string, std::string> customStageMemberIdByStageMember;

    stage_t stage;
  };
} // namespace ds

#endif // DS_STORE_H
