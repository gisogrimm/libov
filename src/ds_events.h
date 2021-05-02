/*
 * Copyright (c) 2021 delude88
 */
/*
 * ov-client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ov-client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ov-client. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DS_EVENTS
#define DS_EVENTS

#include <string>

namespace ds {
  namespace events {
    extern const std::string READY;
    extern const std::string LOCAL_DEVICE_READY;
    extern const std::string DEVICE_CHANGED;
    extern const std::string STAGE_JOINED;
    extern const std::string STAGE_LEFT;
    extern const std::string USER_READY;
    extern const std::string STAGE_ADDED;
    extern const std::string STAGE_CHANGED;
    extern const std::string STAGE_REMOVED;
    extern const std::string REMOTE_USER_ADDED;
    extern const std::string REMOTE_USER_CHANGED;
    extern const std::string REMOTE_USER_REMOVED;
    extern const std::string GROUP_ADDED;
    extern const std::string GROUP_CHANGED;
    extern const std::string GROUP_REMOVED;
    extern const std::string STAGE_MEMBER_ADDED;
    extern const std::string STAGE_MEMBER_CHANGED;
    extern const std::string STAGE_MEMBER_REMOVED;
    extern const std::string CUSTOM_GROUP_VOLUME_ADDED;
    extern const std::string CUSTOM_GROUP_VOLUME_CHANGED;
    extern const std::string CUSTOM_GROUP_VOLUME_REMOVED;
    extern const std::string CUSTOM_GROUP_POSITION_ADDED;
    extern const std::string CUSTOM_GROUP_POSITION_CHANGED;
    extern const std::string CUSTOM_GROUP_POSITION_REMOVED;
    extern const std::string CUSTOM_STAGE_MEMBER_POSITION_ADDED;
    extern const std::string CUSTOM_STAGE_MEMBER_POSITION_CHANGED;
    extern const std::string CUSTOM_STAGE_MEMBER_POSITION_REMOVED;
    extern const std::string CUSTOM_STAGE_MEMBER_VOLUME_ADDED;
    extern const std::string CUSTOM_STAGE_MEMBER_VOLUME_CHANGED;
    extern const std::string CUSTOM_STAGE_MEMBER_VOLUME_REMOVED;
    extern const std::string REMOTE_OV_TRACK_ADDED;
    extern const std::string REMOTE_OV_TRACK_CHANGED;
    extern const std::string REMOTE_OV_TRACK_REMOVED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_VOLUME_ADDED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_VOLUME_CHANGED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_VOLUME_REMOVED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_POSITION_ADDED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_POSITION_CHANGED;
    extern const std::string CUSTOM_REMOTE_OV_TRACK_POSITION_REMOVED;
    extern const std::string SOUND_CARD_ADDED;
    extern const std::string SOUND_CARD_CHANGED;
    extern const std::string SOUND_CARD_REMOVED;
    extern const std::string OV_TRACK_ADDED;
    extern const std::string OV_TRACK_CHANGED;
    extern const std::string OV_TRACK_REMOVED;
  } // namespace events
} // namespace ds

#endif // DS_EVENTS