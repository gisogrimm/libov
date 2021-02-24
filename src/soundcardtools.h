#ifndef SOUNDCARDTOOLS_H
#define SOUNDCARDTOOLS_H

#include <string>
#include <vector>
#include <soundio/soundio.h>

struct snddevname_t {
    std::string dev;
    std::string desc;
};

// get a list of available device names. Implementation may depend on OS and
// audio backend
std::vector<snddevname_t> list_sound_devices();

std::string url2localfilename(const std::string &url);


// NEW ALTERNATIVE: Use SoundCardTools
struct sound_card_t {
    std::string id;
    std::string name;
    int num_input_channels;
    int num_output_channels;
    int sample_rate;
    std::vector<int> sample_rates;
    double software_latency;
    bool is_default;
};

class sound_card_tools_t {

public:
    sound_card_tools_t();

    ~sound_card_tools_t();

    std::vector<sound_card_t> get_sound_devices();

    std::vector<struct SoundIoDevice *> get_input_sound_devices();

    std::vector<struct SoundIoDevice *> get_output_sound_devices();

    struct SoundIo *get_sound_io();

private:
    struct SoundIo *soundio;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
