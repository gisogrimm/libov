#include "soundcardtools.h"

#ifndef __APPLE__
#include <alsa/asoundlib.h>
#endif

#include "common.h"

std::vector<snddevname_t> list_sound_devices()
{
  std::vector<snddevname_t> retv;
#ifndef __APPLE__
  char** hints;
  int err;
  char** n;
  char* name;
  char* desc;

  /* Enumerate sound devices */
  err = snd_device_name_hint(-1, "pcm", (void***)&hints);
  if(err != 0) {
    std::cerr << "Warning: unable to get name hints (list_sound_devices)"
              << std::endl;
    return retv;
  }
  n = hints;
  while(*n != NULL) {
    name = snd_device_name_get_hint(*n, "NAME");
    desc = snd_device_name_get_hint(*n, "DESC");
    if(strncmp("hw:", name, 3) == 0) {
      snddevname_t dname;
      dname.dev = name;
      dname.desc = desc;
      if(dname.desc.find("\n") != std::string::npos)
        dname.desc.erase(dname.desc.find("\n"));
      retv.push_back(dname);
    }
    if(name && strcmp("null", name))
      free(name);
    if(desc && strcmp("null", desc))
      free(desc);
    n++;
  }
  // Free hint buffer too
  snd_device_name_free_hint((void**)hints);
  if(retv.empty()) {
    int card(-1);
    while(snd_card_next(&card) == 0) {
      if(card == -1)
        break;
      snddevname_t dname;
      char* card_name(NULL);
      if(0 == snd_card_get_name(card, &card_name)) {
        dname.dev = "hw:" + std::to_string(card);
        dname.desc = card_name;
        free(card_name);
        retv.push_back(dname);
      }
    }
  }
#else

#endif
  return retv;
}

std::string url2localfilename(const std::string& url)
{
  if(url.empty())
    return url;
  std::string extension(url);
  size_t pos = extension.find("?");
  if(pos != std::string::npos)
    extension.erase(pos, extension.size() - pos);
  pos = extension.find(".");
  if(pos == std::string::npos)
    extension = "";
  else {
    while((pos = extension.find(".")) != std::string::npos)
      extension.erase(0, pos + 1);
    if(!extension.empty())
      extension = "." + extension;
  }
  return std::to_string(std::hash<std::string>{}(url)) + extension;
}

sound_card_tools_t::sound_card_tools_t()
{
  this->soundio = soundio_create();
  soundio_connect(this->soundio); // use default driver (not always jack)
}

sound_card_tools_t::~sound_card_tools_t()
{
  soundio_disconnect(this->soundio);
  delete this->soundio;
}

std::vector<sound_card_t> sound_card_tools_t::get_sound_devices()
{
  const std::vector<struct SoundIoDevice*> input_sound_devices =
      this->get_input_sound_devices();
  const std::vector<struct SoundIoDevice*> output_sound_devices =
      this->get_output_sound_devices();
  int default_input = soundio_default_input_device_index(this->soundio);

  // JACK can only use one soundcard at a time (except on linux), so reduce the
  // sound devices
  std::vector<sound_card_t> soundcards;
  for(int i = 0; i < input_sound_devices.size(); i++) {
    struct SoundIoDevice* input_sound_device = input_sound_devices[i];
    struct SoundIoDevice* output_sound_device = nullptr;
    for(auto output_sound_dev : output_sound_devices) {
      if(strcmp(output_sound_dev->id, input_sound_device->id) == 0) {
        std::cout << "FOUND OUTPUT FOR INPUT " << std::endl;
        output_sound_device = output_sound_dev;
      } else {
        std::cerr << "OUTPUT does not match " << output_sound_dev->id
                  << input_sound_device->id << std::endl;
      }
    }
    if(output_sound_device != nullptr) {
      sound_card_t soundcard;
      soundcard.id = std::string(input_sound_device->id);
      soundcard.name = std::string(input_sound_device->name);
      soundcard.num_input_channels =
          input_sound_device->current_layout.channel_count;
      soundcard.num_output_channels =
          output_sound_device->current_layout.channel_count;
      soundcard.sample_rate = input_sound_device->sample_rate_current;
      for(int j = 0; j < input_sound_devices[i]->sample_rate_count; j += 1) {
        struct SoundIoSampleRateRange* range =
            &input_sound_device->sample_rates[i];
        soundcard.sample_rates.push_back(range->min);
      }
      soundcard.is_default = default_input == i;
      soundcard.software_latency =
          input_sound_devices[i]->software_latency_current;
      soundcards.push_back(soundcard);
    } else {
      std::cerr << "NO MATCHING OUTPUT FOUND FOR " << input_sound_device->id
                << std::endl;
    }
  }
  return soundcards;
}

std::vector<struct SoundIoDevice*> sound_card_tools_t::get_input_sound_devices()
{
  soundio_flush_events(this->soundio); // Flush the sound devices
  int input_count = soundio_input_device_count(this->soundio);
  int default_input = soundio_default_input_device_index(this->soundio);
  std::vector<struct SoundIoDevice*> input_sound_devices;
  for(int i = 0; i < input_count; i += 1) {
    struct SoundIoDevice* device = soundio_get_input_device(this->soundio, i);
    input_sound_devices.push_back(device);
    soundio_device_unref(device);
  }
  return input_sound_devices;
}

std::vector<struct SoundIoDevice*>
sound_card_tools_t::get_output_sound_devices()
{
  soundio_flush_events(this->soundio); // Flush the sound devices
  int output_count = soundio_output_device_count(this->soundio);
  std::vector<struct SoundIoDevice*> output_sound_devices;
  for(int i = 0; i < output_count; i += 1) {
    struct SoundIoDevice* device = soundio_get_output_device(this->soundio, i);
    output_sound_devices.push_back(device);
    soundio_device_unref(device);
  }
  return output_sound_devices;
}

struct SoundIo* sound_card_tools_t::get_sound_io()
{
  return this->soundio;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */