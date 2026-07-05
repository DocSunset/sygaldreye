#include "push_to_talk.hpp"
#include <android/log.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#define TAG "push_to_talk"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static void write_le16(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(static_cast<uint8_t>(x));
    v.push_back(static_cast<uint8_t>(x >> 8));
}
static void write_le32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x));
    v.push_back(static_cast<uint8_t>(x >> 8));
    v.push_back(static_cast<uint8_t>(x >> 16));
    v.push_back(static_cast<uint8_t>(x >> 24));
}

static std::vector<uint8_t> build_wav(const std::vector<float>& pcm, int sr) {
    std::vector<uint8_t> wav;
    uint32_t data_bytes = static_cast<uint32_t>(pcm.size() * sizeof(float));
    wav.reserve(44 + data_bytes);
    // RIFF header
    for (char c : std::string{"RIFF"}) wav.push_back(static_cast<uint8_t>(c));
    write_le32(wav, 36 + data_bytes);
    for (char c : std::string{"WAVEfmt "}) wav.push_back(static_cast<uint8_t>(c));
    write_le32(wav, 18);           // fmt chunk size (18 for IEEE_FLOAT)
    write_le16(wav, 3);            // WAVE_FORMAT_IEEE_FLOAT
    write_le16(wav, 1);            // mono
    write_le32(wav, static_cast<uint32_t>(sr));
    write_le32(wav, static_cast<uint32_t>(sr) * 4); // byte rate
    write_le16(wav, 4);            // block align
    write_le16(wav, 32);           // bits per sample
    write_le16(wav, 0);            // cbSize (extension)
    for (char c : std::string{"data"}) wav.push_back(static_cast<uint8_t>(c));
    write_le32(wav, data_bytes);
    const auto* src = reinterpret_cast<const uint8_t*>(pcm.data());
    wav.insert(wav.end(), src, src + data_bytes);
    return wav;
}

static std::string http_post(const std::string& url, const std::vector<uint8_t>& body) {
    // Parse "http://host:port"
    std::string host;
    int port = 80;
    std::string prefix = "http://";
    std::string rest = (url.substr(0, prefix.size()) == prefix) ? url.substr(prefix.size()) : url;
    auto colon = rest.rfind(':');
    if (colon != std::string::npos) {
        host = rest.substr(0, colon);
        port = std::stoi(rest.substr(colon + 1));
    } else {
        host = rest;
    }

    addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
        LOGE("getaddrinfo failed for %s", host.c_str());
        return {};
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    bool ok = (connect(fd, res->ai_addr, res->ai_addrlen) == 0);
    freeaddrinfo(res);
    if (!ok) { close(fd); LOGE("connect failed"); return {}; }

    std::string req = "POST /transcribe HTTP/1.0\r\nHost: " + host +
                      "\r\nContent-Type: audio/wav\r\nContent-Length: " +
                      std::to_string(body.size()) + "\r\n\r\n";
    send(fd, req.data(), req.size(), 0);
    send(fd, body.data(), body.size(), 0);

    std::string response;
    char buf[512];
    ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf), 0)) > 0)
        response.append(buf, static_cast<size_t>(n));
    close(fd);

    auto sep = response.find("\r\n\r\n");
    return (sep != std::string::npos) ? response.substr(sep + 4) : response;
}

void PushToTalk::set_companion_url(std::string url) { companion_url_ = std::move(url); }

void PushToTalk::feed(const float* samples, int frames) {
    if (!recording_) return;
    buffer_.insert(buffer_.end(), samples, samples + frames);
}

void PushToTalk::begin_recording() {
    recording_ = true;  // append-to-held-buffer: takes accumulate until send/erase
}

void PushToTalk::pause_recording() { recording_ = false; }

void PushToTalk::erase() {
    recording_ = false;
    buffer_.clear();
}

void PushToTalk::end_recording(TranscriptCallback on_result) {
    recording_ = false;
    std::vector<float> captured = std::move(buffer_);
    buffer_.clear();
    std::string url = companion_url_;
    int sr = sample_rate_;
    std::thread([captured = std::move(captured), url, sr, cb = std::move(on_result)]() {
        auto wav = build_wav(captured, sr);
        LOGI("posting %zu-sample WAV to %s", captured.size(), url.c_str());
        std::string body = http_post(url, wav);
        LOGI("transcript response: %s", body.c_str());
        if (cb) cb(body);
    }).detach();
}
