#pragma once
#include <string>
#include <map>
#include <functional>
#include <vector>

struct HttpResponse {
    long statusCode = 0;
    std::string body;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse Get(const std::string& url,
                     const std::map<std::string, std::string>& headers = {});

    HttpResponse Post(const std::string& url,
                      const std::string& body,
                      const std::string& contentType = "application/x-www-form-urlencoded",
                      const std::map<std::string, std::string>& headers = {});

    bool Download(const std::string& url,
                  const std::string& filePath,
                  std::function<void(size_t downloaded, size_t total)> progress = nullptr);
};
