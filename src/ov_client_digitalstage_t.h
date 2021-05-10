/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 Giso Grimm, Tobias Hegemann
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox / dsbox. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef OV_CLIENT_DIGITALSTAGE_T_H_
#define OV_CLIENT_DIGITALSTAGE_T_H_

#include "ov_ds_sockethandler_t.h"
#include <DigitalStage/Api/Client.h>
#include <nlohmann/json.hpp>
#include <ov_types.h>
#include <soundcardtools.h>
#include <utility>

class ov_client_digitalstage_t : public ov_client_base_t {
public:
  ov_client_digitalstage_t(ov_render_base_t& backend, std::string  apiUrl, std::string  apiKey);

  bool is_going_to_stop() const override;

  void start_service() override;
  void stop_service() override;

protected:
  void onReady(const DigitalStage::Api::Store* store) noexcept;

private:
  const std::string apiUrl;
  const std::string apiKey;
  std::atomic<bool> shouldQuit;
  std::unique_ptr<DigitalStage::Api::Client> client;
  std::unique_ptr<ov_ds_sockethandler_t> controller;
};

#endif // OV_CLIENT_DIGITALSTAGE_T_H_
