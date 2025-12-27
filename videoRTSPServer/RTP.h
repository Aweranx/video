#pragma once
#include <arpa/inet.h>

#include <cstdint>
#include <cstring>
#include <vector>
/*
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

// struct RTPHeader {
//   // byte 0
//   uint8_t csrc_cnt : 4;
//   uint8_t extension : 1;
//   uint8_t padding : 1;
//   uint8_t version : 2;
//   // byte 1
//   uint8_t payload_type : 7;  // 例如：h264 96,  JPEG 26
//   uint8_t marker : 1;
//   // byte 2-3
//   uint16_t seq;
//   // byte 4-7
//   uint32_t timestamp;
//   // byte 8-11
//   uint32_t ssrc;
// };

class RTPPacket {
 public:
  RTPPacket(uint8_t payload_type, uint16_t seq, uint32_t timestamp,
            uint32_t ssrc, bool marker);

  void setPayload(const uint8_t* data, size_t size);

  // 获取完整数据包，用于 sendto
  std::vector<uint8_t> getBuffer();

 private:
  std::vector<uint8_t> header_;
  std::vector<uint8_t> payload_;
};