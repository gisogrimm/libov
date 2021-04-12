#include <gtest/gtest.h>

#include "ovboxclient.h"

TEST(sorter, processSameMsg)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(1,pmsg->seq);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

TEST(sorter, processInc)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(2,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

TEST(sorter, processSwap)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 4, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
  msg.pack(sec, id, port, 3, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(3,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(4,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(5,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
  msg.pack(sec, id, port, 6, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(6,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

TEST(sorter, processSkip)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 4, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(4,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(5,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

TEST(sorter, processSkip2)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(2,pmsg->seq);
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(5,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
  msg.pack(sec, id, port, 6, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(6,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

TEST(sorter, processSameSeq)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  bool res(false);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(1,pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(2,pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(2,pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true,res);
  EXPECT_EQ(2,pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false,res);
}

// Local Variables:
// compile-command: "make -C .. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
