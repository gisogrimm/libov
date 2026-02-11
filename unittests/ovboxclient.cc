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

TEST(pingstat, get)
{
  ping_stat_collector_t ps(8);
  ping_stat_t stat;
  EXPECT_EQ(0u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(0.0, stat.t_med);
  EXPECT_EQ(0.0, stat.t_p99);
  EXPECT_EQ(0.0, stat.t_mean);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  EXPECT_EQ(-1.0, stat.t_min);
  EXPECT_EQ(-1.0, stat.t_med);
  EXPECT_EQ(-1.0, stat.t_p99);
  EXPECT_EQ(-1.0, stat.t_mean);
  ++ps.sent;
  ps.update_ping_stat(stat);
  EXPECT_EQ(0u, stat.received);
  EXPECT_EQ(1u, stat.lost);
  ps.add_value(7.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(7.0, stat.t_min);
  EXPECT_EQ(7.0, stat.t_med);
  EXPECT_EQ(7.0, stat.t_p99);
  EXPECT_EQ(7.0, stat.t_mean);
  EXPECT_EQ(1u, stat.received);
  EXPECT_EQ(0u, stat.lost);
  ps.add_value(2.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(2.0, stat.t_min);
  EXPECT_EQ(4.5, stat.t_med);
  EXPECT_EQ(7.0, stat.t_p99);
  EXPECT_EQ(4.5, stat.t_mean);
  ps.add_value(0.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(2.0, stat.t_med);
  EXPECT_EQ(7.0, stat.t_p99);
  EXPECT_EQ(3.0, stat.t_mean);
  ps.add_value(3.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(2.5, stat.t_med);
  EXPECT_EQ(7.0, stat.t_p99);
  EXPECT_EQ(3.0, stat.t_mean);
  ps.add_value(13.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(3.0, stat.t_med);
  EXPECT_EQ(13.0, stat.t_p99);
  EXPECT_EQ(5.0, stat.t_mean);
  ps.add_value(5.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(4.0, stat.t_med);
  EXPECT_EQ(13.0, stat.t_p99);
  EXPECT_EQ(5.0, stat.t_mean);
  ps.add_value(12.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(5.0, stat.t_med);
  EXPECT_EQ(13.0, stat.t_p99);
  EXPECT_EQ(6.0, stat.t_mean);
  ps.add_value(38.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(6.0, stat.t_med);
  EXPECT_EQ(38.0, stat.t_p99);
  EXPECT_EQ(10.0, stat.t_mean);
  ps.add_value(47.0);
  ps.update_ping_stat(stat);
  EXPECT_EQ(0.0, stat.t_min);
  EXPECT_EQ(8.5, stat.t_med);
  EXPECT_EQ(47.0, stat.t_p99);
  EXPECT_EQ(15.0, stat.t_mean);
}

// Local Variables:
// compile-command: "make -C .. unit-tests"
// coding: utf-8-unix
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
