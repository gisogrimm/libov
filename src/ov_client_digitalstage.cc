#include "ov_client_digitalstage.h"
#include <string>

ov_client_digitalstage_t::ov_client_digitalstage_t(ov_render_base_t& backend, const std::string &token)
    : ov_client_base_t(backend), token_(token), runservice_(true), quitrequest_(false)
{
  //this->api_service_ = new ds::ds_service_t(backend, API_SERVER);
  //this->auth_service_ = new ds::ds_auth_service_t(AUTH_SERVER);
}

ov_client_digitalstage_t::~ov_client_digitalstage_t() {
    //delete this->api_service_;
    //delete this->auth_service_;
}

void ov_client_digitalstage_t::start_service()
{
    this->runservice_ = true;
    this->servicethread_ = std::thread(&ov_client_digitalstage_t::service, this);
}

void ov_client_digitalstage_t::stop_service()
{
    this->runservice_ = false;
    this->servicethread_.join();
}

void ov_client_digitalstage_t::service() {
    std::cout << this->backend.get_deviceid() << std::endl;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */