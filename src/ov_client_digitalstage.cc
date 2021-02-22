#include "ov_client_digitalstage.h"
#include <cpprest/filestream.h>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <fstream>
#include <iostream>
#include <string>
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams
using namespace web::json;            // JSON library

#undef U

ov_client_digitalstage_t::ov_client_digitalstage_t(ov_render_base_t& backend)
    : ov_client_base_t(backend), runservice_(true), quitrequest_(false)
{
  this->service_ = new ov_ds_service_t(API_SERVER);
}

void ov_client_digitalstage_t::start_service()
{
  runservice_ = true;
  // TODO: Obtain email and password dynamically - but how? filebased?

  std::string email = "test@digital-stage.org";
  std::string password = "testesttest";
  // Fetch api token
  this->token_ = this->signIn(email, password);

  // Run service
  this->service_->start(this->token_);
}

void ov_client_digitalstage_t::stop_service()
{
  if( !this->token_.empty() )
    this->signOut(this->token_);
  runservice_ = false;
  delete this->service_;
}

std::string ov_client_digitalstage_t::signIn(const std::string& email,
                                             const std::string& password)
{
  const std::string url = AUTH_SERVER;
  auto postJson =
      pplx::create_task([url, email, password]() {
        json::value jsonObject;
        jsonObject[utility::conversions::to_string_t("email")] = json::value::string(utility::conversions::to_string_t(email));
        jsonObject[utility::conversions::to_string_t("password")] = json::value::string(utility::conversions::to_string_t(password));

        return http_client(utility::conversions::to_string_t(url)).request(
            methods::POST, uri_builder(utility::conversions::to_string_t("login")).to_string(),
            jsonObject.serialize(), utility::conversions::to_string_t("application/json"));
      })
          .then([](http_response response) {
            // Check the status code.
            if(response.status_code() != 200) {
              throw std::invalid_argument(
                  "Returned " + std::to_string(response.status_code()));
            }
            // Convert the response body to JSON object.
            return response.extract_json();
          })
          // Parse the user details.
          .then([](json::value jsonObject) { return jsonObject.as_string(); });

  try {
    postJson.wait();
    const std::string token = utility::conversions::to_utf8string(postJson.get());
    return token;
  }
  catch(const std::exception& e) {
    std::cout << "Failed to sign in" << e.what();
  }
  return "";
}

bool ov_client_digitalstage_t::signOut(const std::string& token)
{
  const std::string url = AUTH_SERVER;
  auto postJson =
      pplx::create_task([url, token]() {
        http_client client(utility::conversions::to_string_t(url + "/logout"));
        http_request request(methods::POST);
        request.headers().add(utility::conversions::to_string_t("Content-Type"), utility::conversions::to_string_t("application/json"));
        request.headers().add(utility::conversions::to_string_t("Authorization"), utility::conversions::to_string_t("Bearer " + token));
        return client.request(request);
      }).then([](http_response response) {
        // Check the status code.
        if(response.status_code() != 200) {
          return false;
        }
        // Convert the response body to JSON object.
        return true;
      });

  postJson.wait();
  return postJson.get();
}
/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */