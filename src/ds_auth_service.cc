/*
 * Copyright (c) 2021 Giso Grimm, delude88
 */
/*
 * ov-client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ov-client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ov-client. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by Tobias Hegemann on 24.02.21.
//
#include "ds_auth_service.h"
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <iostream>
#include <stdexcept>

using namespace web;
using namespace web::http;
using namespace web::http::client;

pplx::task<bool> ds::ds_auth_service_t::verifyToken(const std::string& token)
{
  const std::string url = this->url;
  return pplx::create_task([url, token]() {
           http_client client(U(url + "/profile"));
           http_request request(methods::POST);
           request.headers().add(U("Content-Type"), U("application/json"));
           request.headers().add(U("Authorization"), U("Bearer " + token));
           return client.request(request);
         })
      .then([](http_response response) {
        // Check the status code.
        if(response.status_code() != 200) {
          return false;
        }
        // Convert the response body to JSON object.
        return true;
      });
}

bool ds::ds_auth_service_t::verifyTokenSync(const std::string& token)
{
  auto postJson = this->verifyToken(token);
  try {
    postJson.wait();
    return postJson.get();
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
}

ds::ds_auth_service_t::ds_auth_service_t(const std::string& authUrl)
{
  this->url = authUrl;
}

pplx::task<std::string>
ds::ds_auth_service_t::signIn(const std::string& email,
                              const std::string& password)
{
  const std::string url = this->url;
  return pplx::create_task([url, email, password]() {
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
          throw std::invalid_argument("Returned " +
                                      std::to_string(response.status_code()));
        }
        // Convert the response body to JSON object.
        return response.extract_json();
      })
      // Parse the user details.
      .then([](json::value jsonObject) { return jsonObject.as_string(); });
}

std::string ds::ds_auth_service_t::signInSync(const std::string& email,
                                              const std::string& password)
{
  auto postJson = this->signIn(email, password);
  try {
    postJson.wait();
    return postJson.get();
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return "";
  }
}

pplx::task<bool> ds::ds_auth_service_t::signOut(const std::string& token)
{
  const std::string url = this->url;
  return pplx::create_task([url, token]() {
           http_client client(U(url + "/logout"));
           http_request request(methods::POST);
           request.headers().add(U("Content-Type"), U("application/json"));
           request.headers().add(U("Authorization"), U("Bearer " + token));
           return client.request(request);
         })
      .then([](http_response response) {
        // Check the status code.
        if(response.status_code() != 200) {
          return false;
        }
        // Convert the response body to JSON object.
        return true;
      });
}

bool ds::ds_auth_service_t::signOutSync(const std::string& token)
{
  auto postJson = this->signOut(token);
  try {
    postJson.wait();
    return postJson.get();
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
}