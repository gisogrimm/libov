//
// Created by Tobias Hegemann on 24.02.21.
//
#include "ds_auth_service.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;

bool ds::ds_auth_service_t::verifyToken(const std::string& token)
{
  const std::string url = this->url;
  auto postJson =
      pplx::create_task([url, token]() {
        http_client client(U(url + "/profile"));
        http_request request(methods::POST);
        request.headers().add(U("Content-Type"), U("application/json"));
        request.headers().add(U("Authorization"), U("Bearer " + token));
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

ds::ds_auth_service_t::ds_auth_service_t(const std::string& authUrl)
{
  this->url = authUrl;
}

std::string ds::ds_auth_service_t::signIn(const std::string& email,
                                          const std::string& password)
{
  const std::string url = this->url;
  auto postJson =
      pplx::create_task([url, email, password]() {
        json::value jsonObject;
        jsonObject[U("email")] = json::value::string(U(email));
        jsonObject[U("password")] = json::value::string(U(password));

        return http_client(U(url)).request(
            methods::POST, uri_builder(U("login")).to_string(),
            jsonObject.serialize(), U("application/json"));
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
    const std::string token = postJson.get();
    return token;
  }
  catch(const std::exception& e) {
    std::cout << "Failed to sign in" << e.what();
  }
  return "";
}

bool ds::ds_auth_service_t::signOut(const std::string& token)
{
  const std::string url = this->url;
  auto postJson =
      pplx::create_task([url, token]() {
        http_client client(U(url + "/logout"));
        http_request request(methods::POST);
        request.headers().add(U("Content-Type"), U("application/json"));
        request.headers().add(U("Authorization"), U("Bearer " + token));
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