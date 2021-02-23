#include "spawn_process.h"
#include "errmsg.h"
#include "string.h"
#include <signal.h>
#include <vector>

#include "windows_port.h"

spawn_process_t::spawn_process_t(const std::string& command)
    : h_pipe(NULL), pid(0)
{
  if(!command.empty()) {
    size_t len(command.size() + 100);
    std::vector<char> ctmp(len);
    memset(ctmp.data(), 0, len);
    snprintf(ctmp.data(), len, "sh -c \"%s >/dev/null & echo \\$!\"", command.c_str());
    h_pipe = popen(ctmp.data(), "r");
    if(!h_pipe)
      throw ErrMsg("Unable to start pipe with command \"" + std::string(ctmp.data()) +
                   "\".");
    if(fgets(ctmp.data(), 1024, h_pipe) != NULL) {
      pid = atoi(ctmp.data());
      if(pid == 0)
        throw ErrMsg("Invalid subprocess PID (0) when starting command \"" +
                     command + "\".");
    }
  }
}

spawn_process_t::~spawn_process_t()
{
#ifndef WIN32
  if(pid != 0)
    kill(pid, SIGTERM);
#endif
  if(h_pipe)
    fclose(h_pipe);
}

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
