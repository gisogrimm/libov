#ifndef DS_SERVICE
#define DS_SERVICE

#include "ds_json_types.h"
#include "ds_store.h"
#include "ov_types.h"
#include "soundcardtools.h"
#include <atomic>
#include <cpprest/ws_client.h>
#include <thread>

using namespace web::websockets::client;

namespace ds {
  class ds_service_t {

  public:
    ds_service_t(ov_render_base_t& backend, std::string api_url);

    ~ds_service_t();

    void start(const std::string& token);

    void stop();

  protected:
    void service();

    void on_sound_devices_change();

    void send(const std::string& event, const std::string& message);

    void sendAsync(const std::string& event, const std::string& message);

    ov_render_base_t& backend_;

  private:
    // Threading
    bool running_;
    std::thread servicethread_;
    std::atomic<bool> quitrequest_;

    // Connection related
    websocket_callback_client wsclient;
    std::string api_url_;
    std::string token_;

    sound_card_tools_t* sound_card_tools;
    ds_store_t store;

    void update_stage_member(const std::string& stageMemberId);

    void update_stage_member_track(const std::string& trackId);

    // void rerender_stage_member(const std::string &stageMemberId);
    bool is_sending_audio();

    std::vector<std::string>
    getStageMemberIdsByGroupId(const std::string& groupId);
  };
} // namespace ds

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
