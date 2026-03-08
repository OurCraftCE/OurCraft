#include "HttpClient.h"
#include <curl/curl.h>
#include <fstream>

// ---- callbacks ----

static size_t WriteStringCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* str = static_cast<std::string*>(userdata);
    size_t bytes = size * nmemb;
    str->append(ptr, bytes);
    return bytes;
}

static size_t WriteFileCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* file = static_cast<FILE*>(userdata);
    return fwrite(ptr, size, nmemb, file);
}

struct ProgressData {
    std::function<void(size_t, size_t)> callback;
};

static int XferInfoCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
    auto* data = static_cast<ProgressData*>(clientp);
    if (data && data->callback) {
        data->callback(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0;
}

// ---- HttpClient ----

HttpClient::HttpClient() {}
HttpClient::~HttpClient() {}

static curl_slist* BuildHeaders(const std::map<std::string, std::string>& headers,
                                const char* contentType = nullptr) {
    curl_slist* list = nullptr;
    if (contentType) {
        std::string ct = "Content-Type: ";
        ct += contentType;
        list = curl_slist_append(list, ct.c_str());
    }
    for (auto& [k, v] : headers) {
        std::string h = k + ": " + v;
        list = curl_slist_append(list, h.c_str());
    }
    return list;
}

HttpResponse HttpClient::Get(const std::string& url,
                              const std::map<std::string, std::string>& headers) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) return resp;

    curl_slist* hdrs = BuildHeaders(headers);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.statusCode);
    }

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return resp;
}

HttpResponse HttpClient::Post(const std::string& url,
                               const std::string& body,
                               const std::string& contentType,
                               const std::map<std::string, std::string>& headers) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) return resp;

    curl_slist* hdrs = BuildHeaders(headers, contentType.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.statusCode);
    }

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return resp;
}

bool HttpClient::Download(const std::string& url,
                           const std::string& filePath,
                           std::function<void(size_t, size_t)> progress) {
    FILE* file = nullptr;
    fopen_s(&file, filePath.c_str(), "wb");
    if (!file) return false;

    CURL* curl = curl_easy_init();
    if (!curl) { fclose(file); return false; }

    ProgressData pd{progress};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (progress) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, XferInfoCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &pd);
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(file);

    return (res == CURLE_OK);
}
