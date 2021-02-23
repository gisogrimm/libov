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
#ifdef WIN32
  int pid;
#else
  pid_t pid;
#endif
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
