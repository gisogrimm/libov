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
  EXPECT_EQ(true, res);
  EXPECT_EQ(1, pmsg->seq);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(1u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
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
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(2u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
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
  while(sorter.process(&pmsg))
    ;
  //
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  // create a gap, expect buffering:
  msg.pack(sec, id, port, 4, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  // send previously skippied message:
  msg.pack(sec, id, port, 3, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(3, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(4, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  // continue with original sequence:
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(5, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  // continue with original sequence:
  msg.pack(sec, id, port, 6, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(6, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  // statistics:
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(6u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(1u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
}

TEST(sorter, processSeqErr)
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
  EXPECT_EQ(true, res);
  EXPECT_EQ(1, pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  msg.pack(sec, id, port, 7, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(5, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(7, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  msg.pack(sec, id, port, 4, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(4, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  msg.pack(sec, id, port, 6, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(6, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  msg.pack(sec, id, port, 8, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(8, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(7u, stat.received);
  EXPECT_EQ(1u, stat.lost);
  EXPECT_EQ(2u, stat.seqerr_in);
  EXPECT_EQ(1u, stat.seqerr_out);
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
  while(sorter.process(&pmsg))
    ;
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  msg.pack(sec, id, port, 4, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(4, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(5, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(4u, stat.received);
  EXPECT_EQ(1u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
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
  while(sorter.process(&pmsg))
    ;
  // send next package:
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  // skip two packages, expect delivery on next cycle:
  msg.pack(sec, id, port, 5, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(5, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  // next in row, expect delivery:
  msg.pack(sec, id, port, 6, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(6, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(4u, stat.received);
  EXPECT_EQ(2u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
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
  EXPECT_EQ(true, res);
  EXPECT_EQ(1, pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  msg.pack(sec, id, port, 2, "", 0);
  pmsg = &msg;
  res = sorter.process(&pmsg);
  EXPECT_EQ(true, res);
  EXPECT_EQ(2, pmsg->seq);
  res = sorter.process(&pmsg);
  EXPECT_EQ(false, res);
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(4u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
}

TEST(sorter, processJumpFirst)
{
  secret_t sec(1234567);
  stage_device_id_t id(13);
  port_t port(1234);
  message_sorter_t sorter;
  msgbuf_t msg;
  msgbuf_t* pmsg(&msg);
  // pack a message with sequence 1, no content:
  msg.pack(sec, id, port, 1003, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  msg.pack(sec, id, port, 1004, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  msg.pack(sec, id, port, 1005, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  msg.pack(sec, id, port, 1006, "", 0);
  pmsg = &msg;
  while(sorter.process(&pmsg))
    ;
  message_stat_t stat(sorter.get_stat(id));
  EXPECT_EQ(4u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(0u, stat.seqerr_in);
  EXPECT_EQ(0u, stat.seqerr_out);
}

// Local Variables:
// compile-command: "make -C .. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
