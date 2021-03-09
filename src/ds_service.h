#ifndef DS_SERVICE
#define DS_SERVICE

#include "ds_json_types.h"
#include "ds_store.h"
#include "ov_types.h"
#include "soundcardtools.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <cpprest/ws_client.h>

using namespace web::websockets::client;

namespace ds {
  class ds_service_t : public ov_client_base_t {

  public:
    ds_service_t(ov_render_base_t& backend,
                 std::string api_url);

    ~ds_service_t();

    void set_token(const std::string& token);

    const std::string get_token();

    void start_service();

    void stop_service();

    bool is_going_to_stop() const { return quitrequest_; };

  protected:
    ov_render_base_t& backend_;

  private:
    void service();

    void on_sound_devices_change();

    void send(const std::string& event, const std::string& message);

    void sendAsync(const std::string& event, const std::string& message);

    void configureConnection(const ds::json::stage_ov_server_t server);

    void configureAudio(const ds::json::soundcard_t soundcard);

    void startOv();

    void stopOv();

    void createTrack(const std::string& soundCardId, unsigned int channel);

    // TODO: Replace this later with a more detailes TASCAR control
    void syncLocalStageMember();
    void syncRemoteStageMembers();
    void syncGroupVolume(const std::string& groupId);
    void syncGroupPosition(const std::string& groupId);
    void syncStageMemberVolume(const std::string& stageMemberId);
    void syncStageMemberPosition(const std::string& stageMemberId);
    void syncRemoteOvTrackVolume(const std::string& remoteOvTrackId);
    void syncRemoteOvTrackPosition(const std::string& remoteOvTrackId);

    bool isInsideStage();

    // Threading
    std::thread servicethread_;
    std::recursive_mutex backend_mutex_;

    // Connection related
    websocket_callback_client wsclient_;
    std::string api_url_;
    std::string token_;

    sound_card_tools_t* sound_card_tools_;
    ds_store_t* store_;

    std::atomic<bool> ready_;
    std::atomic<bool> quitrequest_;
  };
} // namespace ds

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
