#ifndef COMMON_H
#define COMMON_H

/**
 * @defgroup operationmodes Operation modes of devices
 */

/**
 * @defgroup networkprotocol Definition of the network protocol
 */

#include <getopt.h>
#include <iostream>
#include <mutex>

#include "ov_types.h"

#ifndef DEBUG
#define DEBUG(x)                                                               \
  std::cerr << __FILE__ << ":" << __LINE__ << ": " << #x << "=" << x           \
            << std::endl
#endif

/**
 * Maximum buffer size in bytes.
 *
 * This size limits the maximum size of unpacked messages, which can
 * be BUFSIZE-HEADERLEN. Currently no check against mtu of network
 * connection is performed.
 */
#define BUFSIZE 4096

/// Maximum number of clients in a stage
#define MAX_STAGE_ID 32
/// Special stage ID of server
#define STAGE_ID_SERVER 0xff

/**
 * @ingroup networkprotocol
 * Numbers of port_t with special meanings reserved for control packages
 */
enum {
  /// Register or update a device in a session
  PORT_REGISTER,
  /// Query all device IDs in a session
  PORT_LISTCID,
  PORT_PING,
  PORT_PONG,
  PORT_PEERLATREP,
  PORT_SEQREP,
  PORT_SETLOCALIP,
  MAXSPECIALPORT
};

extern int verbose;

void log(int portno, const std::string& s, int v = 1);

/**
 * Set priority of current thread.
 * @param prio New priority
 * No error is reported if setting thread priority fails.
 */
void set_thread_prio(unsigned int prio);

void app_usage(const std::string& app_name, struct option* opt,
               const std::string& app_arg = "", const std::string& help = "");

/**
 * @ingroup operationmodes
 * Bit mask describing device operation mode
 */
typedef uint8_t epmode_t;

/**
 * @ingroup operationmodes
 * Use peer-to-peer mode
 */
#define B_PEER2PEER 1
/**
 * @ingroup operationmodes
 * Can receive only downmixed signals
 */
#define B_DOWNMIXONLY 2
/**
 * @ingroup operationmodes
 * Using a proxy, do not send audio to this client.
 */
#define B_DONOTSEND 4

// the message header is a byte array with:
// - secret
// - stage_device_id
// - sequence number
// - user data

/**
 * @ingroup networkprotocol
 * Position of stage device ID within a packed message
 */
#define POS_CALLERID sizeof(secret_t)
/**
 * @ingroup networkprotocol
 * Position of port number within a packed message
 */
#define POS_PORT (POS_CALLERID + sizeof(stage_device_id_t))
/**
 * @ingroup networkprotocol
 * Position of sequence number within a packed message
 */
#define POS_SEQ (POS_PORT + sizeof(port_t))
/**
 * @ingroup networkprotocol
 * Length of header in a packed message
 */
#define HEADERLEN (POS_SEQ + sizeof(sequence_t))

/**
 * @ingroup networkprotocol
 * Get session secret in a packed message
 * @param m Packed message
 * @return Reference to session secret variable within message
 */
inline secret_t& msg_secret(char* m)
{
  return *((secret_t*)m);
};
/**
 * @ingroup networkprotocol
 * Get device ID in a packed message
 * @param m Packed message
 * @return Reference to device ID variable within message
 */
inline stage_device_id_t& msg_callerid(char* m)
{
  return *((stage_device_id_t*)(&m[POS_CALLERID]));
};
/**
 * @ingroup networkprotocol
 * Get port number in a packed message
 * @param m Packed message
 * @return Reference to port number variable within message
 */
inline port_t& msg_port(char* m)
{
  return *((port_t*)(&m[POS_PORT]));
};
/**
 * @ingroup networkprotocol
 * Get sequence number in a packed message
 * @param m Packed message
 * @return Reference to sequence number variable within message
 */
inline sequence_t& msg_seq(char* m)
{
  return *((sequence_t*)(&m[POS_SEQ]));
};
/**
 * @ingroup networkprotocol
 * Get session secret in a packed message
 * @param m Packed message
 * @return Reference to session secret variable within message
 */
inline const secret_t& msg_secret(const char* m)
{
  return *((secret_t*)m);
};
/**
 * @ingroup networkprotocol
 * Get device ID in a packed message
 * @param m Packed message
 * @return Reference to device ID variable within message
 */
inline const stage_device_id_t& msg_callerid(const char* m)
{
  return *((stage_device_id_t*)(&m[POS_CALLERID]));
};
/**
 * @ingroup networkprotocol
 * Get port number in a packed message
 * @param m Packed message
 * @return Reference to port number variable within message
 */
inline const port_t& msg_port(const char* m)
{
  return *((port_t*)(&m[POS_PORT]));
};
/**
 * @ingroup networkprotocol
 * Get sequence number in a packed message
 * @param m Packed message
 * @return Reference to sequence number variable within message
 */
inline const sequence_t& msg_seq(const char* m)
{
  return *((sequence_t*)(&m[POS_SEQ]));
};

/**
 * @ingroup networkprotocol
 * Serialize header and original message info a destination buffer for
 * transmission to peers.
 * @param[out] destbuf Start of memory area where the data is stored.
 * @param[in] maxlen Size of the data memory in bytes.
 * @param[in] secret Access code for session
 * @param[in] callerid Identification of sender device
 * @param[in] destport Destination port of message
 * @param[in] seq Sequence number
 * @param[in] msg Start of memory area of original message
 * @param[in] msglen Lenght of original message
 *
 * If successful, the size of the serialized new message will be
 * returned. If the size of the destination buffer is not large enough
 * to pack header and message, then zero is returned.
 */
size_t packmsg(char* destbuf, size_t maxlen, secret_t secret,
               stage_device_id_t callerid, port_t destport, sequence_t seq,
               const char* msg, size_t msglen);

/**
 * @ingroup networkprotocol
 * Append data to a message buffer.
 * @param[out] destbuf Start of memory area where the data is stored.
 * @param[in] maxlen Size of the data memory in bytes.
 * @param[in] currentlen Current fill size bytes.
 * @param[in] msg Start of memory area of the data to be appended.
 * @param[in] msglen Lenght of data.
 *
 * If successful, the size of the serialized new message will be
 * returned. If the size of the destination buffer is not large enough
 * to pack header and message, then zero is returned.
 */
size_t addmsg(char* destbuf, size_t maxlen, size_t currentlen, const char* msg,
              size_t msglen);

/**
 * @ingroup networkprotocol
 * Extract time stamp from message and compare with current time to
 *  get ping time.
 *
 * @param msg Start of memory area where the data is extracted. The
 *   value is incremented by the data needed for the time stamp.
 * @param msglen Length of memory area to read from. The value is
 *   decremented by the length of data needed to read the time stamp.
 *
 * On success, a positive ping time in Milliseconds is returned. If
 * the buffer does not contain sufficient data, -1 is returned.
 */
double get_pingtime(char*& msg, size_t& msglen);

/**
 * @ingroup networkprotocol
 * Container for an unpacked message buffer.
 */
class msgbuf_t {
public:
  /**
   * Default constructor, invalidates and allocates BUFSIZE bytes in
   * buffer
   */
  msgbuf_t();
  ~msgbuf_t();
  /**
   * Unpack a packed message and store result.
   *
   * @param msg Packed source message
   * @param msglen Length of packed source message
   *
   * On success valid member is set to true, otherwise to false
   */
  void unpack(const char* msg, size_t msglen);
  /**
   * Return age of a message in Milliseconds.
   * The age is measured since it was unpacked.
   */
  double get_age();
  /**
   * Reset timer for age measurement.
   */
  void set_tick();
  bool valid; ///< Status of message buffer, if true, the data can be read, if
              ///< false, it can be overwritten
  stage_device_id_t cid; ///< Device ID in session
  port_t destport;       ///< Destination port
  sequence_t seq;        ///< Sequence number
  size_t size;           ///< Actual size of the unpacked message
  char* buffer;          ///< Data containing the unpacked message
private:
  std::chrono::high_resolution_clock::time_point t;
};

#endif

/*
 * Local Variables:
 * mode: c++
 * compile-command: "make -C .."
 * End:
 */
