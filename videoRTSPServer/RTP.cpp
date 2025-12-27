#include "RTP.h"

RTPPacket::RTPPacket(uint8_t payload_type, uint16_t seq, uint32_t timestamp,
                     uint32_t ssrc, bool marker) {
  // 12 byte 的header
  header_.resize(12);

  // Byte 0: V=2(10), P=0, X=0, CC=0 -> 10000000 -> 0x80
  header_[0] = 0x80;

  // Byte 1: M(Marker) + PT(Payload Type)
  // Marker: 一帧的最后一个包标记为 1，其余为 0
  header_[1] = (marker ? 0x80 : 0x00) | (payload_type & 0x7F);

  // Byte 2-3: Sequence Number (Network Byte Order)
  uint16_t seq_n = htons(seq);
  memcpy(&header_[2], &seq_n, 2);

  // Byte 4-7: Timestamp (Network Byte Order)
  uint32_t ts_n = htonl(timestamp);
  memcpy(&header_[4], &ts_n, 4);

  // Byte 8-11: SSRC (Network Byte Order)
  uint32_t ssrc_n = htonl(ssrc);
  memcpy(&header_[8], &ssrc_n, 4);
}

void RTPPacket::setPayload(const uint8_t* data, size_t size) {
  payload_.assign(data, data + size);
}

// 获取完整数据包，用于 sendto
std::vector<uint8_t> RTPPacket::getBuffer() {
  std::vector<uint8_t> buffer = header_;
  buffer.insert(buffer.end(), payload_.begin(), payload_.end());
  return buffer;
}