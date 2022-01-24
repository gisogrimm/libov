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
#include "../tascar/libtascar/include/tscconfig.h"
#include <signal.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

pid_t system2(const std::string& command, bool noshell = true)
{
  pid_t pid = -1;
#ifndef _WIN32 // Windows has no fork.
  pid = fork();
  DEBUG(pid);
  if(pid < 0) {
    return pid;
  } else if(pid == 0) {
    /// Close all other descriptors for the safety sake.
    for(int i = 3; i < 4096; ++i)
      ::close(i);
    DEBUG(setsid());
    DEBUG(pid);
    int retv = 0;
    if(noshell) {
      DEBUG(pid);
      std::vector<std::string> pars = TASCAR::str2vecstr(command);
      char* vpars[pars.size() + 1];
      for(size_t k = 0; k < pars.size(); ++k) {
        vpars[k] = strdup(pars[k].c_str());
      }
      vpars[pars.size()] = NULL;
      DEBUG(pid);
      if(pars.size()) {
        retv = execvp(pars[0].c_str(), vpars);
      }
      DEBUG(pid);
      for(size_t k = 0; k < pars.size(); ++k) {
        DEBUG(vpars[k]);
        free(vpars[k]);
      }
      DEBUG(pid);
    } else {
      DEBUG(pid);
      retv = execl("/bin/sh", "sh", "-c", command.c_str(), NULL);
      DEBUG(pid);
    }
    DEBUG(retv);
    _exit(1);
  }
#endif
  return pid;
}

spawn_process_t::spawn_process_t(const std::string& command)
    : h_pipe(NULL), pid(0)
{
  DEBUG(command);
  if(!command.empty()) {
    // size_t len(command.size() + 100);
    // char ctmp[len];
    // memset(ctmp, 0, len);
    // snprintf(ctmp, len, "sh -c \"%s >/dev/null & echo \\$!\"",
    // command.c_str()); h_pipe = popen(ctmp, "r"); if(!h_pipe)
    //  throw ErrMsg("Unable to start pipe with command \"" + std::string(ctmp)
    //  +
    //               "\".");
    // if(fgets(ctmp, 1024, h_pipe) != NULL) {
    //  pid = atoi(ctmp);
    //  if(pid == 0)
    //    throw ErrMsg("Invalid subprocess PID (0) when starting command \"" +
    //                 command + "\".");
    //}
    pid = system2(command, true);
  }
}

spawn_process_t::~spawn_process_t()
{
  DEBUG(pid);
  if(pid != 0) {
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
  }
  if(h_pipe)
    fclose(h_pipe);
  DEBUG(pid);
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
