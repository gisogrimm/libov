/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2021 Giso Grimm, delude88
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

#ifndef SOUNDCARDTOOLS_H
#define SOUNDCARDTOOLS_H

#ifndef LINUX
#include <soundio/soundio.h>
#endif
#include <string>
#include <vector>

struct snddevname_t {
  std::string dev;
  std::string desc;
};

// get a list of available device names. Implementation may depend on OS and
// audio backend
std::vector<snddevname_t> list_sound_devices();

std::string url2localfilename(const std::string& url);

// NEW ALTERNATIVE: Use SoundCardTools
struct sound_card_t {
  std::string id;
  std::string name;
  unsigned int num_input_channels;
  unsigned int num_output_channels;
  int sample_rate;
  std::vector<int> sample_rates;
  double software_latency;
  bool is_default;
};

#ifndef LINUX

class sound_card_tools_t {

public:
  sound_card_tools_t();

  ~sound_card_tools_t();

  std::vector<sound_card_t> get_sound_devices();

  std::vector<struct SoundIoDevice*> get_input_sound_devices();

  std::vector<struct SoundIoDevice*> get_output_sound_devices();

  struct SoundIo* get_sound_io();

private:
  struct SoundIo* soundio;
};

#endif

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
