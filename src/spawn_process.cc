/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2021 Giso Grimm
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

#include "spawn_process.h"
#include "../tascar/libtascar/include/tascar_os.h"
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "common.h"

spawn_process_t::spawn_process_t(const std::string& command)
{
  if(!command.empty()) {
    pid = TASCAR::system(command.c_str(), false);
  }
}

spawn_process_t::~spawn_process_t()
{
  if(pid != 0) {
    killpg(pid, SIGTERM);
    waitpid(pid, NULL, 0);
  }
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
