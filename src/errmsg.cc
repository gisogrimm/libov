/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#include "errmsg.h"
#include <errno.h>
#include <string.h>

ErrMsg::ErrMsg(const std::string& msg) : std::string(msg) {}

ErrMsg::ErrMsg(const std::string& msg, int err)
    : std::string(msg + std::string(strerror(err)))
{
}

ErrMsg::~ErrMsg() throw() {}

const char* ErrMsg::what() const throw()
{
  return c_str();
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
