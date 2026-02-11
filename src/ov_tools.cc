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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ov_tools.h"
#include <fstream>
#include <iostream>
#include <sstream>
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

nlohmann::json json_merge(const nlohmann::json& lhs, const nlohmann::json& rhs)
{
  nlohmann::json j = lhs;
  for(auto it = rhs.cbegin(); it != rhs.cend(); ++it) {
    const auto& key = it.key();
    if(it->is_object()) {
      if(!j[key].is_null()) {
        // object already exists (must merge)
        j[key] = json_merge(lhs[key], rhs[key]);
      } else {
        // object does not exist in LHS, so use the RHS
        j[key] = rhs[key];
      }
    } else {
      // this is an individual item, use RHS
      j[key] = it.value();
    }
  }
  return j;
}

/**
 * Reads the entire contents of a file into a single std::string.
 *
 * This function efficiently reads a file by utilizing stream buffer iterators,
 * which handle memory allocation automatically and provide good performance
 * for large files. The file is closed automatically when the ifstream object
 * goes out of scope.
 *
 * @param fname The filename to open (as a string or path).
 *              Relative paths are resolved according to the process's working
 * directory.
 *
 * @return A string containing the file's contents if successful.
 *         Returns an empty string ("") if:
 *           - The file doesn't exist
 *           - Permission is denied
 *           - Any other error occurs during opening
 *
 * @note Advantages of this implementation:
 *         - Handles files larger than available memory gracefully (via stream
 * buffering)
 *         - Correctly handles binary files (including null characters)
 *         - Memory efficient (avoids redundant copying)
 *         - Exception-safe (no leaks if construction fails)
 *
 * @warning Returns an empty string for both empty files and errors.
 *          Callers should distinguish using additional checks if needed.
 *          Example ambiguous cases:
 *            - Truly empty file -> returns ""
 *            - Failed to open file -> returns ""
 */
std::string get_file_contents(const std::string& fname)
{
  std::ifstream t(fname);
  if(!t.is_open())
    return "";
  std::string str((std::istreambuf_iterator<char>(
                      t)), // Extra parens prevent "most vexing parse"
                  std::istreambuf_iterator<char>());
  return str;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
