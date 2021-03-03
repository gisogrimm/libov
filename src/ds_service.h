#ifndef DS_SERVICE
#define DS_SERVICE

#include "ds_json_types.h"
#include "ov_types.h"
#include <atomic>
#include <thread>
#include <cpprest/ws_client.h>
#include "ds_store.h"
#include "soundcardtools.h"

using namespace web::websockets::client;

namespace ds {
    class ds_service_t {

    public:
        ds_service_t(ov_render_base_t& backend, std::string api_url);

        ~ds_service_t();

        void start(const std::string &token);

        void stop();

    protected:
        ov_render_base_t& backend_;

    private:
        void service();

        void on_sound_devices_change();

        void send(const std::string &event, const std::string &message);

        void sendAsync(const std::string &event, const std::string &message);

        bool isSendingAudio();

        // Threading
        std::thread servicethread_;

        // Connection related
        websocket_callback_client wsclient;
        std::string api_url_;
        std::string token_;

        sound_card_tools_t *sound_card_tools_;
        ds_store_t *store_;
    };
}

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
