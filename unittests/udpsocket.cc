#include <gtest/gtest.h>

#include "udpsocket.h"

TEST(ovboxsocket, packmsg)
{
  ovbox_udpsocket_t socket(12345678, 13);
  char buf[BUFSIZE];
  size_t len(socket.packmsg(buf, BUFSIZE, 9876, "", 0));
  EXPECT_EQ(HEADERLEN, len);
  EXPECT_EQ(12345678u, msg_secret(buf));
  EXPECT_EQ(13, msg_callerid(buf));
  EXPECT_EQ(9876, msg_port(buf));
  EXPECT_EQ(1, msg_seq(buf));
  len = socket.packmsg(buf, BUFSIZE, 9876, "", 0);
  EXPECT_EQ(HEADERLEN, len);
  EXPECT_EQ(12345678u, msg_secret(buf));
  EXPECT_EQ(13, msg_callerid(buf));
  EXPECT_EQ(9876, msg_port(buf));
  EXPECT_EQ(2, msg_seq(buf));
  len = socket.packmsg(buf, BUFSIZE, 9877, "", 0);
  EXPECT_EQ(HEADERLEN, len);
  EXPECT_EQ(12345678u, msg_secret(buf));
  EXPECT_EQ(13, msg_callerid(buf));
  EXPECT_EQ(9877, msg_port(buf));
  EXPECT_EQ(1, msg_seq(buf));
  len = socket.packmsg(buf, BUFSIZE, 9876, "", 0);
  EXPECT_EQ(HEADERLEN, len);
  EXPECT_EQ(12345678u, msg_secret(buf));
  EXPECT_EQ(13, msg_callerid(buf));
  EXPECT_EQ(9876, msg_port(buf));
  EXPECT_EQ(3, msg_seq(buf));
}

// Local Variables:
// compile-command: "make -C .. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
