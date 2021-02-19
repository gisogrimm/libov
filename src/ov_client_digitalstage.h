#ifndef OV_CLIENT_DIGITALSTAGE
#define OV_CLIENT_DIGITALSTAGE

#include "ov_ds_service.h"
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
  ov_client_digitalstage_t(ov_render_base_t& backend);
  void start_service();
  void stop_service();
  bool download_file(const std::string& url, const std::string& dest);
  bool is_going_to_stop() const { return quitrequest_; };

private:
  std::string signIn(const std::string& email, const std::string& password);
  bool signOut(const std::string& token);

  bool runservice_;
  ov_ds_service_t *service_;
  std::atomic<bool> quitrequest_;
  std::string token_;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */