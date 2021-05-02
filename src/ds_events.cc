/*
 * Copyright (c) 2021 delude88
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

#include "ds_events.h"

const std::string ds::events::READY = "ready";
const std::string ds::events::LOCAL_DEVICE_READY = "local-device-ready";
const std::string ds::events::DEVICE_CHANGED = "device-changed";
const std::string ds::events::STAGE_JOINED = "s-j";
const std::string ds::events::STAGE_LEFT = "s-l";
const std::string ds::events::USER_READY = "user-ready";
const std::string ds::events::STAGE_ADDED = "s-a";
const std::string ds::events::STAGE_CHANGED = "s-c";
const std::string ds::events::STAGE_REMOVED = "s-r";
const std::string ds::events::REMOTE_USER_ADDED = "r-u-a";
const std::string ds::events::REMOTE_USER_CHANGED = "r-u-c";
const std::string ds::events::REMOTE_USER_REMOVED = "r-u-r";
const std::string ds::events::GROUP_ADDED = "g-a";
const std::string ds::events::GROUP_CHANGED = "g-c";
const std::string ds::events::GROUP_REMOVED = "g-r";
const std::string ds::events::STAGE_MEMBER_ADDED = "sm-a";
const std::string ds::events::STAGE_MEMBER_CHANGED = "sm-c";
const std::string ds::events::STAGE_MEMBER_REMOVED = "sm-r";
const std::string ds::events::CUSTOM_GROUP_POSITION_ADDED = "c-g-p-a";
const std::string ds::events::CUSTOM_GROUP_POSITION_CHANGED = "c-g-p-c";
const std::string ds::events::CUSTOM_GROUP_POSITION_REMOVED = "c-g-p-r";
const std::string ds::events::CUSTOM_GROUP_VOLUME_ADDED = "c-g-v-a";
const std::string ds::events::CUSTOM_GROUP_VOLUME_CHANGED = "c-g-v-c";
const std::string ds::events::CUSTOM_GROUP_VOLUME_REMOVED = "c-g-v-r";
const std::string ds::events::CUSTOM_STAGE_MEMBER_VOLUME_ADDED = "c-sm-v-a";
const std::string ds::events::CUSTOM_STAGE_MEMBER_VOLUME_CHANGED = "c-sm-v-c";
const std::string ds::events::CUSTOM_STAGE_MEMBER_VOLUME_REMOVED = "c-sm-v-r";
const std::string ds::events::CUSTOM_STAGE_MEMBER_POSITION_ADDED = "c-sm-p-a";
const std::string ds::events::CUSTOM_STAGE_MEMBER_POSITION_CHANGED = "c-sm-p-c";
const std::string ds::events::CUSTOM_STAGE_MEMBER_POSITION_REMOVED = "c-sm-p-r";
const std::string ds::events::REMOTE_OV_TRACK_ADDED = "sm-ov-a";
const std::string ds::events::REMOTE_OV_TRACK_CHANGED = "sm-ov-c";
const std::string ds::events::REMOTE_OV_TRACK_REMOVED = "sm-ov-r";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_VOLUME_ADDED =
    "c-sm-ov-v-a";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_VOLUME_CHANGED =
    "c-sm-ov-v-c";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_VOLUME_REMOVED =
    "c-sm-ov-v-r";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_POSITION_ADDED =
    "c-sm-ov-p-a";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_POSITION_CHANGED =
    "c-sm-ov-p-c";
const std::string ds::events::CUSTOM_REMOTE_OV_TRACK_POSITION_REMOVED =
    "c-sm-ov-p-r";
const std::string ds::events::SOUND_CARD_ADDED = "sound-card-added";
const std::string ds::events::SOUND_CARD_CHANGED = "sound-card-changed";
const std::string ds::events::SOUND_CARD_REMOVED = "sound-card-removed";
const std::string ds::events::OV_TRACK_ADDED = "track-added";
const std::string ds::events::OV_TRACK_CHANGED = "track-changed";
const std::string ds::events::OV_TRACK_REMOVED = "track-removed";
