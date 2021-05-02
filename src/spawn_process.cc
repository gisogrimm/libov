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

#include "spawn_process.h"
#include "errmsg.h"
#include "string.h"
#include <signal.h>

spawn_process_t::spawn_process_t(const std::string& command)
    : h_pipe(NULL), pid(0)
{
  if(!command.empty()) {
    size_t len(command.size() + 100);
    char ctmp[len];
    memset(ctmp, 0, len);
    snprintf(ctmp, len, "sh -c \"%s >/dev/null & echo \\$!\"", command.c_str());
    h_pipe = popen(ctmp, "r");
    if(!h_pipe)
      throw ErrMsg("Unable to start pipe with command \"" + std::string(ctmp) +
                   "\".");
    if(fgets(ctmp, 1024, h_pipe) != NULL) {
      pid = atoi(ctmp);
      if(pid == 0)
        throw ErrMsg("Invalid subprocess PID (0) when starting command \"" +
                     command + "\".");
    }
  }
}

spawn_process_t::~spawn_process_t()
{
  if(pid != 0)
    kill(pid, SIGTERM);
  if(h_pipe)
    fclose(h_pipe);
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
