#include "RTSPsession.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "RTP.h"
#include "global.h"
#include "mediafile.h"

int RTSPRequest::parseRequest(const std::string& req_str) {
  std::stringstream ss(req_str);
  std::string line;
  bool is_requestLine = true;

  while (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      // 如果后续有 Body (比如 POST/ANNOUNCE)，可以在这里处理
      // 对于 OPTIONS/DESCRIBE 等简单请求，直接跳出循环即可
      break;
    }

    if (is_requestLine) {
      if (parseRequestLine(line) != 0) {
        std::cerr << "Request Line Parse Error: " << line << std::endl;
        return -1;
      }
      is_requestLine = false;
    } else {
      if (parseOneLine(line) != 0) {
        std::cerr << "Header Line Parse Error: " << line << std::endl;
        return -1;
      }
    }
  }
  return 0;
}
int RTSPRequest::parseRequestLine(const std::string& line) {
  std::stringstream ss(line);
  std::string method_str;
  if (ss >> method_str >> url_ >> version_) {
    method_ = Utils::Str2Method(method_str);
    return 0;
  }
  return -1;
}
int RTSPRequest::parseOneLine(const std::string& line) {
  auto pos = line.find(":");
  if (pos == std::string::npos) {
    return -1;
  }
  std::string key = Utils::trim(line.substr(0, pos));
  std::string value = Utils::trim(line.substr(pos + 1));
  if (key == "CSeq") {
    seq_ = std::stoi(value);
  } else if (key == "Session") {
    session_id_ = value;
  } else if (key == "Transport") {  // 解析协议及UDP端口号
    size_t pos = value.find("client_port=");
    if (pos != std::string::npos) {
      // "client_port=" 长度为 12
      const char* start_ptr = value.c_str() + pos + 12;

      uint16_t rtp_port = 0;
      uint16_t rtcp_port = 0;

      if (sscanf(start_ptr, "%hu-%hu", &rtp_port, &rtcp_port) == 2) {
        client_port_[0] = rtp_port;
        client_port_[1] = rtcp_port;
      }
    }
  }
  return 0;
}

void RTSPReply::generateSDP() {
  std::stringstream ss;

  // 基础头部
  ss << "v=0\r\n";
  ss << "o=- " << session_id_ << " 1 IN IP4 " << "127.0.0.1" << "\r\n";
  ss << "s=Simple RTSP Server\r\n";
  ss << "c=IN IP4 0.0.0.0\r\n";
  ss << "t=0 0\r\n";

  // 视频轨道  H.264
  // 96 是动态负载类型，通常 H.264 用 96
  ss << "m=video 0 RTP/AVP 96\r\n";
  ss << "a=rtpmap:96 H264/90000\r\n";

  ss << "a=fmtp:96 "
        "packetization-mode=1\r\n";

  ss << "a=control:track0\r\n";
  sdp_ = ss.str();
}

std::string RTSPReply::toString() const {
  std::stringstream ss;
  // Status Line (例如: RTSP/1.0 200 OK)
  ss << "RTSP/1.0 " << static_cast<int>(status_code_) << " "
     << Utils::Code2Str(status_code_) << "\r\n";
  // Common Headers
  ss << "CSeq: " << seq_ << "\r\n";
  if (!session_id_.empty()) {
    ss << "Session: " << session_id_ << "\r\n";
  }
  // Specific Headers
  if (status_code_ == StatusCode::OK) {
    switch (method_) {
      case RTSPMethod::OPTIONS:
        ss << "Public: " << options_ << "\r\n";
        break;

      case RTSPMethod::DESCRIBE:
        ss << "Content-Type: application/sdp\r\n";
        ss << "Content-Length: " << sdp_.size() << "\r\n";
        ss << "Content-Base: " << "rtsp://127.0.0.1:8554/live" << "\r\n";
        break;

      case RTSPMethod::SETUP:
        ss << "Transport: " << transport_reply_ << "\r\n";
        break;
      case RTSPMethod::PLAY:
        if (!range_.empty()) {
          ss << "Range: " << range_ << "\r\n";
        }
        break;

      default:
        break;
    }
  }
  ss << "\r\n";
  if (method_ == RTSPMethod::DESCRIBE && !sdp_.empty()) {
    ss << sdp_;
  }

  return ss.str();
}

RTSPSession::RTSPSession(net::io_context& ioc)
    : session_id_(Utils::GenerateUUID()),
      client_socket_(ioc),
      RTP_socket_(ioc),
      RTCP_socket_(ioc),
      timer_(ioc) {
  read_buffer_.resize(4096);
}

RTSPSession::~RTSPSession() {
  clearFile();
  if (RTP_socket_.is_open()) {
    RTP_socket_.close();
  }
  if (RTCP_socket_.is_open()) {
    RTCP_socket_.close();
  }
}

tcp::socket& RTSPSession::Socket() { return client_socket_; }

void RTSPSession::pickRequest() {
  auto self = shared_from_this();
  client_socket_.async_read_some(
      boost::asio::buffer(read_buffer_),
      [this, self](boost::system::error_code ec,
                   std::size_t bytes_transferred) {
        if (!ec) {
          in_buffer_.append(read_buffer_.data(), bytes_transferred);
          analysRequestAndMakeReply();
          pickRequest();
        } else {
          if (ec != boost::asio::error::eof) {
            std::cerr << "Read Error: " << ec.message() << std::endl;
          }
        }
      });
}

void RTSPSession::analysRequestAndMakeReply() {
  while (true) {
    auto end_pos = in_buffer_.find("\r\n\r\n");
    if (end_pos == std::string::npos) {
      return;
    }
    size_t msg_len = end_pos + 4;
    std::string req_str = in_buffer_.substr(0, msg_len);
    in_buffer_.erase(0, msg_len);
    RTSPRequest req;
    std::cout << "请求： " << req_str;
    if (req.parseRequest(req_str) != 0) {
      std::cerr << "Parse Error" << std::endl;
      // 发送 400 Bad Request
      handleBadRequest(req);
      return;
    }
    RTSPReply reply;
    reply.seq_ = req.seq_;
    reply.method_ = req.method_;
    switch (req.method_) {
      case RTSPMethod::OPTIONS:
        handleOptions(req, reply);
        break;
      case RTSPMethod::DESCRIBE:
        handleDescribe(req, reply);
        break;
      case RTSPMethod::SETUP:
        handleSetup(req, reply);
        break;
      case RTSPMethod::PLAY:
        handlePlay(req, reply);
        break;
      case RTSPMethod::TEARDOWN:
        handleTeardown(req, reply);
        break;
      default:
        reply.status_code_ = StatusCode::METHOD_NOT_ALLOWED;
        break;
    }
    std::cout << "回复： " << reply.toString();
    sendReply(reply);
  }
}

void RTSPSession::handleOptions(const RTSPRequest& req, RTSPReply& reply) {
  reply.status_code_ = StatusCode::OK;
  reply.options_ = "OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN";
}

void RTSPSession::handleDescribe(const RTSPRequest& req, RTSPReply& reply) {
  reply.status_code_ = StatusCode::OK;
  reply.generateSDP();  // 生成 SDP 信息的函数
}

void RTSPSession::handleSetup(const RTSPRequest& req, RTSPReply& reply) {
  reply.status_code_ = StatusCode::OK;
  reply.session_id_ = session_id_;

  reply.transport_reply_ =
      "RTP/AVP;unicast;client_port=" + std::to_string(req.client_port_[0]) +
      "-" + std::to_string(req.client_port_[1]) + ";server_port=55000-55001";

  // 初始化 RTP和RTCP 发送的 Socket
  if (!RTP_socket_.is_open()) {
    RTP_socket_.open(udp::v4());
    RTP_socket_.bind(udp::endpoint(udp::v4(), 55000));
    auto client_ip = client_socket_.remote_endpoint().address();
    RTP_client_endpoint_ = udp::endpoint(client_ip, req.client_port_[0]);
  }
  if (!RTCP_socket_.is_open()) {
    RTCP_socket_.open(udp::v4());
    RTCP_socket_.bind(udp::endpoint(udp::v4(), 55001));
    auto client_ip = client_socket_.remote_endpoint().address();
    RTCP_client_endpoint_ = udp::endpoint(client_ip, req.client_port_[1]);
  }
}

void RTSPSession::handlePlay(const RTSPRequest& req, RTSPReply& reply) {
  reply.status_code_ = StatusCode::OK;
  reply.session_id_ = session_id_;
  reply.range_ = "npt=0.000-9.000";
  if (!video_file_.is_open()) {
    video_file_.open(
        "/home/ranx/work/edoyun/videoRTSPServer/data/"
        "TheaterSquare_3840x2160.h264",
        std::ios::binary);
  }
  // 开始推流逻辑
  startRtpSending();
}

void RTSPSession::handleTeardown(const RTSPRequest& req, RTSPReply& reply) {
  reply.status_code_ = StatusCode::OK;
  reply.session_id_ = session_id_;
  // 清理资源，准备关闭连接
  clearFile();
  if (RTP_socket_.is_open()) {
    RTP_socket_.close();
  }
  if (RTCP_socket_.is_open()) {
    RTCP_socket_.close();
  }
}

void RTSPSession::handleBadRequest(const RTSPRequest& req) {}

void RTSPSession::sendReply(const RTSPReply& reply) {
  std::string response_str = reply.toString();

  auto self = shared_from_this();

  boost::asio::async_write(client_socket_, boost::asio::buffer(response_str),
                           [this, self](boost::system::error_code ec,
                                        std::size_t bytes_transferred) {
                             if (ec) {
                               // 处理发送错误，比如打印日志或断开连接
                               // handleError(ec);
                             } else {
                               // 发送成功
                               // 如果是 TEARDOWN 的响应，发送完可能需要主动关闭
                               // socket
                             }
                           });
}

void RTSPSession::clearFile() {
  video_file_.clear();
  video_file_.seekg(0);
  if (video_file_.is_open()) {
    video_file_.close();
  }
  timer_.cancel();
}

void RTSPSession::startRtpSending() {
  auto self = shared_from_this();

  // 设置定时器：每 40ms (1000/25) 触发一次
  timer_.expires_after(std::chrono::milliseconds(1000 / fps_));
  timer_.async_wait([this, self](boost::system::error_code ec) {
    if (!ec) {
      sendOneH264Frame();
      startRtpSending();
    }
  });
}

// ---------------------------------------------------------
// 读取一个NALU并切片发送  起始码不参与发送
// ---------------------------------------------------------
void RTSPSession::sendOneH264Frame() {
  if (!video_file_.is_open() || video_file_.eof()) {
    clearFile();
    return;
  }
  Nalu nalu = Nalu::readNextNalu(video_file_);
  if (!nalu.is_valid || nalu.data.empty()) {
    return;
  }

  // H.264 的时间戳单位是 90000Hz。每帧增加 90000/FPS
  rtp_timestamp_ += (90000 / fps_);

  // 限制包大小, MTU通常为1500
  const size_t MAX_RTP_PAYLOAD_SIZE = 1400;

  // 判断是否需要分片
  if (nalu.data.size() <= MAX_RTP_PAYLOAD_SIZE) {
    //*   0 1 2 3 4 5 6 7 8 9
    //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //*  |F|NRI|  Type   | a single NAL unit ... |
    //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // F : 禁止位，0。
    // NRI: 重要性指示。比如 IDR 帧是 11 (3)，普通 P 帧是 10 (2)。
    // Type: 原始类型。比如 5 代表 IDR，7 代表 SPS
    RTPPacket packet(96, rtp_seq_++, rtp_timestamp_, rtp_ssrc_, true);
    packet.setPayload(nalu.data.data(), nalu.data.size());

    auto pbuffer = std::make_shared<std::vector<uint8_t>>(packet.getBuffer());
    RTP_socket_.async_send_to(boost::asio::buffer(*pbuffer),
                              RTP_client_endpoint_, [pbuffer](auto, auto) {});
  } else {
    // 拆分原来的NALU头，将信息重组为FU indicator和FU Header
    //*  0                   1                   2
    //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
    //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //* | FU indicator  |   FU header   |   FU payload   ...  |
    //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    //*     FU Indicator
    //*    0 1 2 3 4 5 6 7
    //*   +-+-+-+-+-+-+-+-+
    //*   |F|NRI|  Type   |
    //*   +---------------+
    // F和NRI和原始NALU头一致
    // type改为28，28 在 H.264 标准里是 FU-A 分片单元

    //*      FU Header
    //*    0 1 2 3 4 5 6 7
    //*   +-+-+-+-+-+-+-+-+
    //*   |S|E|R|  Type   |
    //*   +---------------+
    // S (Start bit): 开始位。如果是分片的第一个包，置 1；否则置 0。
    // E (End bit): 结束位。如果是分片的最后一个包，置 1；否则置 0。
    // R (Reserved): 保留位，必须是 0。
    // type与NALU头一致

    uint8_t nalu_header = nalu.data[0];
    uint8_t nri = nalu_header & 0x60;   // 与上0110 0000以保留NRI
    uint8_t type = nalu_header & 0x1F;  // 与上0001 1111以保留NALU Type

    // 去掉 NALU 头，只切分数据部分
    const uint8_t* nalu_payload = nalu.data.data() + 1;
    size_t nalu_payload_size = nalu.data.size() - 1;
    size_t offset = 0;

    while (offset < nalu_payload_size) {
      // 计算当前分片大小
      size_t chunk_size =
          std::min(MAX_RTP_PAYLOAD_SIZE, nalu_payload_size - offset);

      // 是不是最后一个分片？(如果是，Marker=true)
      bool is_last_chunk = (offset + chunk_size >= nalu_payload_size);

      // 构建 FU Indicator (1字节)
      // 或上0001 1100，相当于直接把28加在低位
      // 高三位还是F和NRI
      uint8_t fu_indicator = nri | 28;

      // 构建 FU Header (1字节)
      // S(Start), E(End), R(0), Type(原NALU Type)
      uint8_t fu_header = type;
      bool is_start_chunk = (offset == 0);
      // 1000 0000
      if (is_start_chunk) fu_header |= 0x80;  // Set S bit
      // 0100 0000
      else if (is_last_chunk)
        fu_header |= 0x40;  // Set E bit

      // 组装 Payload: FU Indicator + FU Header + Chunk Data
      std::vector<uint8_t> fragment;
      fragment.push_back(fu_indicator);
      fragment.push_back(fu_header);
      fragment.insert(fragment.end(), nalu_payload + offset,
                      nalu_payload + offset + chunk_size);

      // 发送 RTP 包
      // Marker 位：只有整个帧的最后一个包才置为 true
      RTPPacket packet(96, rtp_seq_++, rtp_timestamp_, rtp_ssrc_,
                       is_last_chunk);
      packet.setPayload(fragment.data(), fragment.size());

      // 分片采用同步发送，懒得思考异步逻辑了
      auto buffer = packet.getBuffer();
      RTP_socket_.send_to(boost::asio::buffer(buffer), RTP_client_endpoint_);

      offset += chunk_size;
    }
  }
}