#pragma once
#include <boost/asio.hpp>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>
#include <unordered_map>
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;
enum class RTSPMethod {
  UNKNOWN = 0,
  OPTIONS,
  DESCRIBE,
  SETUP,
  PLAY,
  TEARDOWN,
  PAUSE,
  GET_PARAMETER,
  SET_PARAMETER
};

enum class StatusCode {
  OK = 200,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  NOT_FOUND = 404,
  METHOD_NOT_ALLOWED = 405,
  SESSION_NOT_FOUND = 454,
  UNSUPPORTED_TRANSPORT = 461,
  INTERNAL_SERVER_ERROR = 500
};

enum class TransportProtocol {
  UDP,
  TCP,
};
enum class MediaType { VIDEO, AUDIO, NONE };

enum class CodecId { H264 = 96, H265 = 265, AAC = 97, PCMA = 8, PCMU = 0 };

class Utils {
 public:
  static RTSPMethod Str2Method(std::string_view methodStr) {
    static const std::unordered_map<std::string_view, RTSPMethod> methodMap = {
        {"OPTIONS", RTSPMethod::OPTIONS},
        {"DESCRIBE", RTSPMethod::DESCRIBE},
        {"SETUP", RTSPMethod::SETUP},
        {"PLAY", RTSPMethod::PLAY},
        {"PAUSE", RTSPMethod::PAUSE},
        {"TEARDOWN", RTSPMethod::TEARDOWN},
        {"GET_PARAMETER", RTSPMethod::GET_PARAMETER},
        {"SET_PARAMETER", RTSPMethod::SET_PARAMETER}};

    auto it = methodMap.find(methodStr);
    if (it != methodMap.end()) {
      return it->second;
    }
    return RTSPMethod::UNKNOWN;
  }

  static std::string_view Method2Str(RTSPMethod method) {
    switch (method) {
      case RTSPMethod::OPTIONS:
        return "OPTIONS";
      case RTSPMethod::DESCRIBE:
        return "DESCRIBE";
      case RTSPMethod::SETUP:
        return "SETUP";
      case RTSPMethod::PLAY:
        return "PLAY";
      case RTSPMethod::PAUSE:
        return "PAUSE";
      case RTSPMethod::TEARDOWN:
        return "TEARDOWN";
      case RTSPMethod::GET_PARAMETER:
        return "GET_PARAMETER";
      case RTSPMethod::SET_PARAMETER:
        return "SET_PARAMETER";
      default:
        return "UNKNOWN";
    }
  }

  static std::string GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    uint32_t random_val = dis(gen);
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
       << random_val;

    return ss.str();
  }

  static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
  }
  static std::string_view Code2Str(StatusCode code) {
    switch (code) {
      case StatusCode::OK:
        return "OK";
      case StatusCode::BAD_REQUEST:
        return "Bad Request";
      case StatusCode::NOT_FOUND:
        return "Not Found";
      case StatusCode::INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
      default:
        return "Unknown";
    }
  }
};