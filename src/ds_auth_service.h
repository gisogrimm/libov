//
// Created by Tobias Hegemann on 24.02.21.
//

#ifndef DS_AUTH_SERVICE_H
#define DS_AUTH_SERVICE_H

#include <string>

namespace ds {
  class ds_auth_service_t {
  public:
    bool verifyToken(const std::string& token);

    /**
     * Create a new auth service class by using the given url to the auth
     * service
     * @param authUrl
     */
    ds_auth_service_t(const std::string& authUrl);

    /**
     * Sign into the auth service with given email and password
     * @param email
     * @param password
     * @return valid token when successful, otherwise empty string
     */
    std::string signIn(const std::string& email, const std::string& password);

    /**
     * Sign out of the auth service and invalidate the given token
     * @param token
     * @return true if successful, otherwise false
     */
    bool signOut(const std::string& token);

  private:
    std::string url;
  };

} // namespace ds

#endif // DS_AUTH_SERVICE_H
