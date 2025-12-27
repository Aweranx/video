#pragma once
#include <boost/asio/io_context.hpp>
#include <fstream>
#include <string>

#include "global.h"
class RTSPRequest {
 public:
  friend class RTSPSession;
  RTSPRequest() = default;
  ~RTSPRequest() = default;
  int parseRequest(const std::string& req_str);
  int parseOneLine(const std::string& line);
  int parseRequestLine(const std::string& line);

 private:
  RTSPMethod method_;
  std::string url_;
  std::string version_;
  std::string session_id_;
  int seq_;
  uint16_t client_port_[2] = {0, 0};
};

class RTSPReply {
 public:
  friend class RTSPSession;
  RTSPReply() = default;
  ~RTSPReply() = default;
  void generateSDP();
  std::string toString() const;

 private:
  StatusCode status_code_;
  RTSPMethod method_;
  uint16_t client_port_[2] = {0, 0};
  uint16_t server_port_[2] = {0, 0};
  std::string sdp_;
  std::string options_;
  std::string session_id_;
  int seq_;
  std::string transport_reply_;
  std::string range_;
};

class RTSPSession : public std::enable_shared_from_this<RTSPSession> {
 public:
  RTSPSession(net::io_context& ioc);
  ~RTSPSession();
  void pickRequest();
  void analysRequestAndMakeReply();
  void sendReply(const RTSPReply& reply);
  tcp::socket& Socket();
  void handleOptions(const RTSPRequest& req, RTSPReply& reply);
  void handleDescribe(const RTSPRequest& req, RTSPReply& reply);
  void handleSetup(const RTSPRequest& req, RTSPReply& reply);
  void handlePlay(const RTSPRequest& req, RTSPReply& reply);
  void handleTeardown(const RTSPRequest& req, RTSPReply& reply);
  void handleBadRequest(const RTSPRequest& req);

 private:
  std::string session_id_;
  tcp::socket client_socket_;
  std::string in_buffer_;
  std::string read_buffer_;
  udp::socket RTP_socket_;
  udp::endpoint RTP_client_endpoint_;
  udp::socket RTCP_socket_;
  udp::endpoint RTCP_client_endpoint_;
  void clearFile();

  // --- RTP 状态变量 ---
  uint16_t rtp_seq_ = 0;
  uint32_t rtp_timestamp_ = 0;
  const uint32_t rtp_ssrc_ = 0x12345678;  // 随机生成一个 SSRC
  const int fps_ = 60;
  std::ifstream video_file_;
  boost::asio::steady_timer timer_;
  void startRtpSending();
  void sendOneH264Frame();
};