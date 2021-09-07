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

#include "ov_tools.h"
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x           \
            << std::endl
#endif

#ifdef LINUX

static pid_t get_process_parent_id(const pid_t pid)
{
  pid_t ppid(0);
  char buffer[BUFSIZ];
  sprintf(buffer, "/proc/%d/stat", pid);
  FILE* fp = fopen(buffer, "r");
  if(fp) {
    size_t size = fread(buffer, sizeof(char), sizeof(buffer), fp);
    if(size > 0) {
      // See: http://man7.org/linux/man-pages/man5/proc.5.html section
      // /proc/[pid]/stat
      strtok(buffer, " ");              // (1) pid  %d
      strtok(NULL, " ");                // (2) comm  %s
      strtok(NULL, " ");                // (3) state  %c
      char* s_ppid = strtok(NULL, " "); // (4) ppid  %d
      ppid = atoi(s_ppid);
    }
    fclose(fp);
  }
  return ppid;
}

static void get_process_name(const pid_t pid, char* name, char* arg)
{
  name[0] = '\0';
  arg[0] = '\0';
  char procfile[BUFSIZ];
  sprintf(procfile, "/proc/%d/cmdline", pid);
  FILE* f = fopen(procfile, "r");
  if(f) {
    size_t size;
    size = fread(name, sizeof(char), sizeof(procfile), f);
    if(size > 0) {
      if('\n' == name[size - 1])
        name[size - 1] = '\0';
      if(size > strlen(name)) {
        strcpy(arg, &(name[strlen(name) + 1]));
      }
    }
    fclose(f);
  }
}
#endif

bool is_ovbox()
{
#ifdef LINUX
  // check if parent is the autorun script:
  pid_t ppid(get_process_parent_id(get_process_parent_id(getppid())));
  char name[BUFSIZ];
  memset(name, 0, BUFSIZ);
  char arg[BUFSIZ];
  memset(arg, 0, BUFSIZ);
  get_process_name(ppid, name, arg);
  return strcmp(arg, "/home/pi/autorun") == 0;
#else
  // if not on LINUX this can not be an ovbox:
  return false;
#endif
}

std::string ovstrrep(std::string s, const std::string& pat,
                     const std::string& rep)
{
  std::string out_string("");
  std::string::size_type len = pat.size();
  std::string::size_type pos;
  while((pos = s.find(pat)) < s.size()) {
    out_string += s.substr(0, pos);
    out_string += rep;
    s.erase(0, pos + len);
  }
  s = out_string + s;
  return s;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
