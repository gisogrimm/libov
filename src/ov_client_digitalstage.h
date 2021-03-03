#ifndef OV_CLIENT_DIGITALSTAGE
#define OV_CLIENT_DIGITALSTAGE

#include "ds_auth_service.h"
#include "ds_service.h"
#include "ov_types.h"
#include <atomic>

#ifndef API_SERVER
#define API_SERVER "https://api.dstage.org"
#endif

#ifndef AUTH_SERVER
#define AUTH_SERVER "https://auth.dstage.org"
#endif

class ov_client_digitalstage_t : public ov_client_base_t {
public:
  ov_client_digitalstage_t(ov_render_base_t& backend, const std::string& token);

  ~ov_client_digitalstage_t();

  void start_service();

  void stop_service();

  bool download_file(const std::string& url, const std::string& dest);

  bool is_going_to_stop() const { return quitrequest_; };

private:
  void service();

  bool runservice_;
  std::atomic<bool> quitrequest_;
  std::string token_;
  std::thread servicethread_;
  // ds::ds_auth_service_t *auth_service_;
  // ds::ds_service_t *api_service_;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */