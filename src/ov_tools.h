/*
 * Copyright (c) 2021 Giso Grimm
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

#ifndef OV_TOOLS_H
#define OV_TOOLS_H

#include "errmsg.h"
#include <nlohmann/json.hpp>

// Return true if this process is called from ovbox autorun scripts
bool is_ovbox();

// download a file from an url and save as a given name
// bool download_file(const std::string& url, const std::string& dest);

// simple string replace function:
std::string ovstrrep(std::string s, const std::string& pat,
                     const std::string& rep);

// robust json value function with default value:
template <class T>
T my_js_value(const nlohmann::json& obj, const std::string& key,
              const T& defval)
{
  try {
    if(obj.is_object())
      return obj.value(key, defval);
    return defval;
  }
  catch(const std::exception& e) {
    throw ErrMsg(std::string(e.what()) + " ('" + obj.dump() + "', key: '" +
                 key + "')");
  }
}

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
