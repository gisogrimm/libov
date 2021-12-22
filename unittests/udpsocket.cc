#include <gtest/gtest.h>

#include "udpsocket.h"

//TEST(msgbuf, age)
//{
//  msgbuf_t msg;
//  msg.set_tick();
//  usleep(20000.0);
//  double age(msg.get_age());
//  ASSERT_NEAR(20.0, age, 15.0);
//}

TEST(msgbuf, pack)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  msgbuf_t msg;
  msg.pack(sec, id, port, 1, "", 0);
  EXPECT_EQ(true,msg.valid);
}

TEST(msgbuf, copy)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  msgbuf_t msg;
  msg.pack(sec, id, port, 1, "", 0);
  EXPECT_EQ(true,msg.valid);
  msgbuf_t msg2;
  msg2.copy(msg);
  EXPECT_EQ(msg.valid,msg2.valid);
  EXPECT_EQ(msg.cid,msg2.cid);
  EXPECT_EQ(msg.destport,msg2.destport);
  EXPECT_EQ(msg.seq,msg2.seq);
  EXPECT_EQ(msg.size,msg2.size);
  msg2.unpack(msg2.size+HEADERLEN);
  EXPECT_EQ(msg.valid,msg2.valid);
  EXPECT_EQ(msg.cid,msg2.cid);
  EXPECT_EQ(msg.destport,msg2.destport);
  EXPECT_EQ(msg.seq,msg2.seq);
  EXPECT_EQ(msg.size,msg2.size);
}

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
