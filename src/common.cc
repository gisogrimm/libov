/*
 * This file is part of the ovbox software tool, see <http://orlandoviols.com/>.
 *
 * Copyright (c) 2020 2021 Giso Grimm
 */
/*
 * ovbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * ovbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with ovbox. If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <ctime>
#include <iomanip>
#include <sodium.h>
#include <string.h>

std::mutex logmutex;
int verbose(1);

void log(int portno, const std::string& s, int v)
{
  if(v > verbose)
    return;
  std::lock_guard<std::mutex> guard(logmutex);
  std::time_t t(std::time(nullptr));
  std::cerr << std::put_time(std::gmtime(&t), "%c") << " [" << portno << "] "
            << s << std::endl;
}

void set_thread_prio(unsigned int prio)
{
  if(prio > 0) {
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) != 0) {
      std::cerr << "Warning: unable to set thread priority to " << prio
                << ".\n";
    }
  }
}

void app_usage(const std::string& app_name, struct option* opt,
               const std::string& app_arg, const std::string& help)
{
  std::cout << "Usage:\n\n" << app_name << " [options] " << app_arg << "\n\n";
  if(!help.empty())
    std::cout << help << "\n\n";
  std::cout << "Options:\n\n";
  while(opt->name) {
    std::cout << "  -" << (char)(opt->val) << " " << (opt->has_arg ? "#" : "")
              << "\n  --" << opt->name << (opt->has_arg ? "=#" : "") << "\n\n";
    opt++;
  }
}

size_t packmsg(char* destbuf, size_t maxlen, secret_t secret,
               stage_device_id_t callerid, port_t destport, sequence_t seq,
               const char* msg, size_t msglen)
{
  if(maxlen < HEADERLEN + msglen)
    return 0;
  msg_secret(destbuf) = secret;
  msg_callerid(destbuf) = callerid;
  msg_port(destbuf) = destport;
  msg_seq(destbuf) = seq;
  memcpy(&(destbuf[HEADERLEN]), msg, msglen);
  return HEADERLEN + msglen;
}

size_t addmsg(char* destbuf, size_t maxlen, size_t currentlen, const char* msg,
              size_t msglen)
{
  if(maxlen < currentlen + msglen)
    return 0;
  memcpy(&(destbuf[currentlen]), msg, msglen);
  return currentlen + msglen;
}

size_t encryptmsg(char* destmsg, size_t maxlen, const char* srcmsg,
                  size_t msglen, const uint8_t* pubkey)
{
  if(maxlen < msglen + crypto_box_SEALBYTES)
    return 0;
  // copy the header part of the data:
  memcpy(destmsg, srcmsg, HEADERLEN);
  // encrypt the rest of the message:
  crypto_box_seal((uint8_t*)(&(destmsg[HEADERLEN])),
                  (uint8_t*)(&(srcmsg[HEADERLEN])), msglen - HEADERLEN, pubkey);
  return msglen + crypto_box_SEALBYTES;
}

size_t decryptmsg(char* destmsg, const char* srcmsg, size_t msglen,
                  const uint8_t* pubkey, const uint8_t* seckey)
{
  if(msglen <= HEADERLEN) {
    // error, return original message:
    memcpy(destmsg, srcmsg, msglen);
    return msglen;
  }
  // copy the header part of the data:
  memcpy(destmsg, srcmsg, HEADERLEN);
  // decrypt the rest of the message:
  if(crypto_box_seal_open((uint8_t*)(&(destmsg[HEADERLEN])),
                          (uint8_t*)(&(srcmsg[HEADERLEN])), msglen - HEADERLEN,
                          pubkey, seckey) != 0) {
    // error, return original message:
    memcpy(destmsg, srcmsg, msglen);
    return msglen;
  }
  return msglen - crypto_box_SEALBYTES;
}

/*
 * Local Variables:
 * compile-command: "make -C .."
 * End:
 */
