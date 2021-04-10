#include <gtest/gtest.h>

#include "common.h"

TEST(packmsg, packget)
{
  char buf[BUFSIZE];
  secret_t secret(12345678);
  stage_device_id_t callerid(13);
  port_t destport(9876);
  sequence_t seq(4321);
  size_t len(packmsg(buf, BUFSIZE, secret, callerid, destport, seq, "", 0));
  EXPECT_EQ(HEADERLEN, len);
  EXPECT_EQ(secret, msg_secret(buf));
  EXPECT_EQ(callerid, msg_callerid(buf));
  EXPECT_EQ(destport, msg_port(buf));
  EXPECT_EQ(seq, msg_seq(buf));
  char shortbuf[7];
  len = packmsg(shortbuf, 7, secret, callerid, destport, seq, "", 0);
  EXPECT_EQ(0u, len);
}

TEST(msgbuf, age)
{
  msgbuf_t msg;
  msg.set_tick();
  usleep(1000.0);
  double age(msg.get_age());
  ASSERT_NEAR(1.0, age, 0.5);
}

// Local Variables:
// compile-command: "make -C .. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
