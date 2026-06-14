// Minimal single-threaded HTTP server for POST /run using raw sockets.
// Avoids cpp-httplib (which does not compile reliably on MinGW in this environment).

#include "server.h"
#include "runtime.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <ctime>
#include <cstdint>


#if defined(_WIN32)
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32")
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

namespace {

static std::string jsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (unsigned char ch : s) {
    switch (ch) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (ch < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", ch);
          out += buf;
        } else {
          out += static_cast<char>(ch);
        }
    }
  }
  return out;
}

static bool endsWithJs(const std::string& filename) {
  if (filename.size() < 3) return false;
  auto dotPos = filename.rfind('.');
  if (dotPos == std::string::npos) return false;
  std::string ext = filename.substr(dotPos);
  for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return ext == ".js";
}

static std::string makeTempJsFilename() {
  // GCC/MinGW in this environment may not expose <chrono> reliably for some libstdc++ configs.
  // Use a simple monotonic-ish value from time(nullptr) + RNG.
  std::uint64_t t = static_cast<std::uint64_t>(std::time(nullptr));
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  return "temp_" + std::to_string(t) + "_" + std::to_string(dist(gen)) + ".js";
}


static bool recvAll(int sock, std::string& out, size_t maxBytes) {
  out.clear();
  std::vector<char> buf(8192);
  while (out.size() < maxBytes) {
#if defined(_WIN32)
    int n = ::recv(sock, buf.data(), static_cast<int>(buf.size()), 0);
#else
    ssize_t n = ::recv(sock, buf.data(), buf.size(), 0);
#endif
    if (n <= 0) return !out.empty();
    out.append(buf.data(), buf.data() + n);
    // Stop if we already have the whole headers and content-length is satisfied.
    if (out.find("\r\n\r\n") != std::string::npos) {
      auto pos = out.find("\r\n\r\n");
      std::string headers = out.substr(0, pos);
      auto clPos = headers.find("Content-Length:");
      if (clPos != std::string::npos) {
        size_t endLine = headers.find("\r\n", clPos);
        std::string clStr = headers.substr(clPos + std::strlen("Content-Length:"));
        // trim
        clStr.erase(0, clStr.find_first_not_of(" \t"));
        clStr.erase(clStr.find_last_not_of(" \t") + 1);
        size_t cl = 0;
        try { cl = static_cast<size_t>(std::stoul(clStr)); } catch (...) { return false; }
        size_t headerBytes = pos + 4;
        if (out.size() >= headerBytes + cl) return true;
      }
    }
  }
  return false;
}

static std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

struct MultipartFile {
  std::string filename;
  std::string content;
};

static bool extractMultipartFile(const std::string& body,
                                  const std::string& boundary,
                                  MultipartFile& fileOut) {
  fileOut = MultipartFile{};

  const std::string boundaryLine = "--" + boundary;
  size_t cur = 0;

  // Find first boundary
  size_t bpos = body.find(boundaryLine, cur);
  if (bpos == std::string::npos) return false;
  cur = bpos + boundaryLine.size();

  while (true) {
    // Each part begins with "--boundary\r\n" or "--boundary--\r\n"
    if (cur + 2 <= body.size() && body.compare(cur, 2, "--") == 0) {
      return false;
    }

    // Skip leading CRLF
    if (cur + 2 <= body.size() && body.compare(cur, 2, "\r\n") == 0) cur += 2;

    // Part headers end
    size_t headerEnd = body.find("\r\n\r\n", cur);
    if (headerEnd == std::string::npos) return false;

    std::string partHeaders = body.substr(cur, headerEnd - cur);

    // Find start of part content
    size_t contentStart = headerEnd + 4;

    // Next boundary
    size_t nextB = body.find(boundaryLine, contentStart);
    if (nextB == std::string::npos) return false;

    std::string partContent = body.substr(contentStart, nextB - contentStart);

    // Trim trailing CRLF that belongs to the boundary separator
    while (!partContent.empty() && (partContent.back() == '\r' || partContent.back() == '\n')) {
      partContent.pop_back();
    }

    // Detect file field: look for name="file" and a filename=
    // Parse name and filename from headers (simple parsing).
    bool isFileField = false;
    std::string filename;

    // name
    auto namePos = partHeaders.find("name=\"");
    if (namePos != std::string::npos) {
      auto q2 = partHeaders.find('"', namePos + 6);
      if (q2 != std::string::npos) {
        std::string nameVal = partHeaders.substr(namePos + 6, q2 - (namePos + 6));
        if (nameVal == "file") isFileField = true;
      }
    }

    // filename
    auto fnPos = partHeaders.find("filename=\"");
    if (fnPos != std::string::npos) {
      auto q2 = partHeaders.find('"', fnPos + 10);
      if (q2 != std::string::npos) filename = partHeaders.substr(fnPos + 10, q2 - (fnPos + 10));
    }

    if (isFileField && !filename.empty()) {
      fileOut.filename = filename;
      fileOut.content = std::move(partContent);
      return true;
    }

    cur = nextB + boundaryLine.size();
  }
}

static std::string httpOkJson(const std::string& json) {
  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\n";
  oss << "Content-Type: application/json; charset=utf-8\r\n";
  oss << "Content-Length: " << json.size() << "\r\n";
  oss << "Connection: close\r\n\r\n";
  oss << json;
  return oss.str();
}

static std::string httpErr(int status, const std::string& json) {
  std::ostringstream oss;
  oss << "HTTP/1.1 " << status << " " << (status == 400 ? "Bad Request" : "Error") << "\r\n";
  oss << "Content-Type: application/json; charset=utf-8\r\n";
  oss << "Content-Length: " << json.size() << "\r\n";
  oss << "Connection: close\r\n\r\n";
  oss << json;
  return oss.str();
}

} // namespace

int Server::run() {
#if defined(_WIN32)
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    std::cerr << "WSAStartup failed" << std::endl;
    return 1;
  }
#endif

  const int port = 8081;

  int listenSock = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
  if (listenSock < 0) {
    std::cerr << "socket() failed" << std::endl;
    return 1;
  }

  int yes = 1;
#if defined(_WIN32)
  ::setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
  ::setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (::bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    std::cerr << "Bind failed" << std::endl;
#if defined(_WIN32)
    ::closesocket(listenSock);
    WSACleanup();
#else
    ::close(listenSock);
#endif
    return 1;
  }

  if (::listen(listenSock, 8) != 0) {
    std::cerr << "listen() failed" << std::endl;
#if defined(_WIN32)
    ::closesocket(listenSock);
    WSACleanup();
#else
    ::close(listenSock);
#endif
    return 1;
  }

  std::cout << "Server listening on port 8081..." << std::endl;

  while (true) {
    sockaddr_in clientAddr{};
#if defined(_WIN32)
    int clientLen = sizeof(clientAddr);
    int clientSock = ::accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#else
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock = ::accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
#endif
    if (clientSock < 0) continue;

    std::string req;
    const size_t MAX = 220 * 1024; // slightly above 200KB limit
    bool got = recvAll(clientSock, req, MAX);
    if (!got) {
#if defined(_WIN32)
      ::closesocket(clientSock);
#else
      ::close(clientSock);
#endif
      continue;
    }

    size_t headerEnd = req.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
#if defined(_WIN32)
      ::closesocket(clientSock);
#else
      ::close(clientSock);
#endif
      continue;
    }

    std::string headers = req.substr(0, headerEnd);
    std::string body = req.substr(headerEnd + 4);

    // Parse request line
    auto lineEnd = headers.find("\r\n");
    std::string requestLine = (lineEnd == std::string::npos) ? headers : headers.substr(0, lineEnd);

    bool isOptions = requestLine.rfind("OPTIONS", 0) == 0;
    bool isPostRun = requestLine.find("POST /run") != std::string::npos;

    if (isOptions) {
      std::string resp = "HTTP/1.1 200 OK\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Headers: *\r\n"
                         "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                         "Content-Length: 0\r\n"
                         "Connection: close\r\n\r\n";
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    if (!isPostRun) {
      std::string json = R"({"success":false,"error":"Not found"})";
      std::string resp = httpErr(404, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    // CORS headers + json response.
    // Extract boundary from Content-Type
    std::string boundary;
    {
      auto ctPos = headers.find("Content-Type:");
      if (ctPos != std::string::npos) {
        size_t endLine = headers.find("\r\n", ctPos);
        std::string ct = headers.substr(ctPos, (endLine == std::string::npos ? headers.size() : endLine) - ctPos);
        auto bPos = ct.find("boundary=");
        if (bPos != std::string::npos) {
          boundary = trim(ct.substr(bPos + std::strlen("boundary=")));
          // Strip optional quotes
          if (!boundary.empty() && boundary.front() == '"') {
            if (boundary.size() >= 2 && boundary.back() == '"') {
              boundary = boundary.substr(1, boundary.size() - 2);
            }
          }
        }
      }
    }

    if (boundary.empty()) {
      std::string json = R"({"success":false,"error":"Invalid multipart boundary"})";
      std::string resp = httpErr(400, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    MultipartFile file;
    bool ok = extractMultipartFile(body, boundary, file);
    if (!ok) {
      std::string json = R"({"success":false,"error":"No file uploaded"})";
      std::string resp = httpErr(400, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    if (file.filename.empty() || file.content.empty()) {
      std::string json = R"({"success":false,"error":"No file uploaded"})";
      std::string resp = httpErr(400, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    if (!endsWithJs(file.filename)) {
      std::string json = R"({"success":false,"error":"Only .js files allowed"})";
      std::string resp = httpErr(400, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    if (file.content.size() > 200 * 1024) {
      std::string json = R"({"success":false,"error":"File too large"})";
      std::string resp = httpErr(400, json);
#if defined(_WIN32)
      ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
      ::closesocket(clientSock);
#else
      ::send(clientSock, resp.c_str(), resp.size(), 0);
      ::close(clientSock);
#endif
      continue;
    }

    const std::string tempFile = makeTempJsFilename();
    {
      std::ofstream out(tempFile, std::ios::binary);
      if (!out.is_open()) {
        std::string json = R"({"success":false,"error":"Cannot create temp file"})";
        std::string resp = httpErr(500, json);
#if defined(_WIN32)
        ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
        ::closesocket(clientSock);
#else
        ::send(clientSock, resp.c_str(), resp.size(), 0);
        ::close(clientSock);
#endif
        continue;
      }
      out.write(file.content.data(), static_cast<std::streamsize>(file.content.size()));
    }

    RuntimeResult result = executeNodeFile(tempFile, 5000);
    std::remove(tempFile.c_str());

    std::string json;
    if (result.success) {
      json = std::string("{\"success\":true,\"output\":\"") + jsonEscape(result.output) + "\"}";
    } else {
      json = std::string("{\"success\":false,\"error\":\"") + jsonEscape(result.error) + "\"}";
    }

    // Add CORS header manually for browser.
    std::string resp = httpOkJson(json);
    // inject CORS after status line
    {
      std::string cors = "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Headers: *\r\n"
                         "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
      // find end of first line
      size_t firstLineEnd = resp.find("\r\n");
      if (firstLineEnd != std::string::npos) {
        resp.insert(firstLineEnd + 2, cors);
      }
    }

#if defined(_WIN32)
    ::send(clientSock, resp.c_str(), static_cast<int>(resp.size()), 0);
    ::closesocket(clientSock);
#else
    ::send(clientSock, resp.c_str(), resp.size(), 0);
    ::close(clientSock);
#endif
  }

#if defined(_WIN32)
  ::closesocket(listenSock);
  WSACleanup();
#else
  ::close(listenSock);
#endif

  return 0;
}

