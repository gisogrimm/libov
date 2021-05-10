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

#ifndef SPAWN_PROCESS_H
#define SPAWN_PROCESS_H

#include <string>

/**
 * \brief Spawn a process in the background and stop it
 */
class spawn_process_t {
public:
  spawn_process_t(const std::string& command);
  ~spawn_process_t();

private:
  // on Linux we start the process in a pipe:
  FILE* h_pipe;
  pid_t pid;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
