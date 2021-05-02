/*
 * Copyright (c) 2021 delude88, Giso Grimm, Tobias Hegemann
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

#ifndef DS_AUTH_SERVICE_H
#define DS_AUTH_SERVICE_H

#include <pplx/pplxtasks.h>
#include <string>

namespace ds {
  class ds_auth_service_t {
  public:
    /**
     * Create a new auth service class by using the given url to the auth
     * service
     * @param authUrl
     */
    explicit ds_auth_service_t(const std::string& authUrl);

    /**
     * Verify the given token
     * @param token
     * @return task object, use wait() and get() on it to receive the bool value
     */
    pplx::task<bool> verifyToken(const std::string& token);

    /**
     * Verify the given token
     * @param token
     * @return true if token is valid, otherwise false
     */
    bool verifyTokenSync(const std::string& token);

    /**
     * Sign into the auth service with given email and password
     * @param email
     * @param password
     * @return task object, use wait() and get() on it to receive the value
     */
    pplx::task<std::string> signIn(const std::string& email,
                                   const std::string& password);

    /**
     * Sign into the auth service with given email and password
     * @param email
     * @param password
     * @return valid token when successful, otherwise empty string
     */
    std::string signInSync(const std::string& email,
                           const std::string& password);

    /**
     * Sign out of the auth service and invalidate the given token
     * @param token
     * @return task object, use wait() and get() on it to receive the bool value
     */
    pplx::task<bool> signOut(const std::string& token);

    /**
     * Sign out of the auth service and invalidate the given token
     * @param token
     * @return true if successful, otherwise false
     */
    bool signOutSync(const std::string& token);

  private:
    std::string url;
  };

} // namespace ds

#endif // DS_AUTH_SERVICE_H
