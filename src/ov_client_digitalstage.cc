#include "ov_client_digitalstage.h"
#include <string>

ov_client_digitalstage_t::ov_client_digitalstage_t(ov_render_base_t& backend)
    : ov_client_base_t(backend), runservice_(true), quitrequest_(false)
{
  this->api_service_ = new ds::ds_service_t(backend, API_SERVER);
  this->auth_service_ = new ds::ds_auth_service_t(AUTH_SERVER);
}

ov_client_digitalstage_t::~ov_client_digitalstage_t() {
    delete this->api_service_;
    delete this->auth_service_;
}

void ov_client_digitalstage_t::start_service()
{
  runservice_ = true;
  // TODO: Obtain email and password dynamically - but how? filebased?

  std::string email = "test@digital-stage.org";
  std::string password = "testesttest";
  // Fetch api token
  this->token_ = this->auth_service_->signIn(email, password);

  // Run service
  this->api_service_->start(this->token_);
}

void ov_client_digitalstage_t::stop_service()
{
  if( !this->token_.empty() )
    this->auth_service_->signOut(this->token_);
  runservice_ = false;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */