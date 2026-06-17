#include "ws_server.h"
#include "sha1.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <map>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace {

// ---- logging (so you can confirm load + connections from the Wine prefix) ----
void log_line(const char* msg) {
    if (FILE* f = std::fopen("FNVWebSocket.log", "a")) {
        std::fprintf(f, "%s\n", msg);
        std::fclose(f);
    }
}
void log_fmt(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    log_line(buf);
}

uint64_t now_ms() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ---- tiny JSON helpers (spike-grade: the app sends well-formed messages) ----
// Replace with a real JSON parser when hardening (robustness, escaping, etc.).
std::string find_string_field(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    size_t k = json.find(needle);
    if (k == std::string::npos) return "";
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return "";
    size_t q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return "";
    size_t q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

long find_number_field(const std::string& json, const std::string& key, long fallback) {
    const std::string needle = "\"" + key + "\"";
    size_t k = json.find(needle);
    if (k == std::string::npos) return fallback;
    size_t colon = json.find(':', k + needle.size());
    if (colon == std::string::npos) return fallback;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) ++i;
    bool neg = (i < json.size() && json[i] == '-'); if (neg) ++i;
    long val = 0; bool any = false;
    while (i < json.size() && json[i] >= '0' && json[i] <= '9') { val = val * 10 + (json[i]-'0'); ++i; any = true; }
    if (!any) return fallback;
    return neg ? -val : val;
}

// ---- socket helpers ----
bool recv_n(SOCKET s, char* buf, int n) {
    int got = 0;
    while (got < n) {
        int r = recv(s, buf + got, n - got, 0);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

bool send_all(SOCKET s, const char* p, int n) {
    int sent = 0;
    while (sent < n) {
        int r = send(s, p + sent, n - sent, 0);
        if (r <= 0) return false;
        sent += r;
    }
    return true;
}

bool ws_send_text(SOCKET s, const std::string& msg) {
    std::string frame;
    frame.push_back(static_cast<char>(0x81)); // FIN + text opcode
    const size_t n = msg.size();
    if (n < 126) {
        frame.push_back(static_cast<char>(n));
    } else if (n <= 0xFFFF) {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((n >> 8) & 0xFF));
        frame.push_back(static_cast<char>(n & 0xFF));
    } else {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) frame.push_back(static_cast<char>((n >> (i * 8)) & 0xFF));
    }
    frame += msg;
    return send_all(s, frame.data(), static_cast<int>(frame.size()));
}

// Read one WebSocket frame. Returns false on disconnect/error.
bool ws_read_frame(SOCKET s, std::string& out, int& opcode) {
    unsigned char hdr[2];
    if (!recv_n(s, reinterpret_cast<char*>(hdr), 2)) return false;
    opcode = hdr[0] & 0x0F;
    const bool masked = (hdr[1] & 0x80) != 0;
    uint64_t len = hdr[1] & 0x7F;
    if (len == 126) {
        unsigned char ext[2];
        if (!recv_n(s, reinterpret_cast<char*>(ext), 2)) return false;
        len = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
    } else if (len == 127) {
        unsigned char ext[8];
        if (!recv_n(s, reinterpret_cast<char*>(ext), 8)) return false;
        len = 0;
        for (int i = 0; i < 8; ++i) len = (len << 8) | ext[i];
    }
    unsigned char mask[4] = {0,0,0,0};
    if (masked && !recv_n(s, reinterpret_cast<char*>(mask), 4)) return false;
    out.resize(static_cast<size_t>(len));
    if (len && !recv_n(s, &out[0], static_cast<int>(len))) return false;
    if (masked) for (size_t i = 0; i < out.size(); ++i) out[i] ^= mask[i & 3];
    return true;
}

// ---- data payloads (SPIKE: hardcoded; Phase 2 reads these from the game) ----
std::string data_msg(const std::string& id, const std::string& fields) {
    return "{\"type\":\"data\",\"id\":\"" + id + "\",\"ts\":" +
           std::to_string(now_ms()) + ",\"fields\":" + fields + "}";
}

std::string fields_for(const std::string& id) {
    if (id == "system")
        return "{\"language\":\"en\",\"features\":[\"player\",\"inventory\",\"map\"]}";
    if (id == "game.status")
        return "{\"status\":{\"canAct\":true,\"controlsEnabled\":true,\"dead\":false,"
               "\"inCombat\":false,\"inDialogue\":false,\"inMainMenu\":false,"
               "\"loading\":false,\"paused\":false}}";
    // Phase 2 fills these from real game reads (character.stats, inventory.*, map.*).
    return "{}";
}

struct Sub { long freqMs; uint64_t last; };

void handle_client(SOCKET client) {
    // --- HTTP upgrade handshake ---
    std::string req;
    char buf[1024];
    while (req.find("\r\n\r\n") == std::string::npos) {
        int r = recv(client, buf, sizeof(buf), 0);
        if (r <= 0) return;
        req.append(buf, r);
        if (req.size() > 16384) return; // guard
    }
    // Sec-WebSocket-Key is an HTTP header, not JSON — parse it directly.
    std::string key;
    {
        const std::string h = "Sec-WebSocket-Key:";
        size_t p = req.find(h);
        if (p == std::string::npos) { log_line("[ws] no Sec-WebSocket-Key"); return; }
        p += h.size();
        while (p < req.size() && (req[p] == ' ' || req[p] == '\t')) ++p;
        size_t e = req.find("\r\n", p);
        key = req.substr(p, e - p);
    }
    const std::string accept_src = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t digest[20];
    crypto::sha1(reinterpret_cast<const uint8_t*>(accept_src.data()), accept_src.size(), digest);
    const std::string accept = crypto::base64(digest, 20);
    const std::string resp =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    if (!send_all(client, resp.data(), static_cast<int>(resp.size()))) return;
    log_line("[ws] client connected (handshake ok)");

    // --- message loop: poll for frames + emit due subscriptions ---
    std::map<std::string, Sub> subs;
    for (;;) {
        fd_set rf; FD_ZERO(&rf); FD_SET(client, &rf);
        timeval tv; tv.tv_sec = 0; tv.tv_usec = 50000; // 50ms
        int sel = select(0, &rf, nullptr, nullptr, &tv);
        if (sel == SOCKET_ERROR) break;

        if (sel > 0 && FD_ISSET(client, &rf)) {
            std::string payload; int opcode;
            if (!ws_read_frame(client, payload, opcode)) break;
            if (opcode == 0x8) break;                  // close
            if (opcode == 0x9) {                        // ping -> pong
                std::string pong; pong.push_back(static_cast<char>(0x8A));
                pong.push_back(static_cast<char>(payload.size()));
                pong += payload;
                if (!send_all(client, pong.data(), static_cast<int>(pong.size()))) break;
                continue;
            }
            if (opcode != 0x1) continue;                // only handle text

            const std::string type = find_string_field(payload, "type");
            const std::string id   = find_string_field(payload, "id");
            if (type == "heartbeat") {
                ws_send_text(client, "{\"type\":\"heartbeat\",\"ts\":" + std::to_string(now_ms()) + "}");
            } else if (type == "query") {
                ws_send_text(client, data_msg(id, fields_for(id)));
            } else if (type == "subscribe") {
                long freq = find_number_field(payload, "frequency", 500);
                subs[id] = Sub{ freq, 0 };
                ws_send_text(client, data_msg(id, fields_for(id))); // immediate first push
                log_fmt("[ws] subscribe %s @ %ldms", id.c_str(), freq);
            } else if (type == "unsubscribe") {
                if (id.empty()) subs.clear(); else subs.erase(id);
            } else if (type == "unsubscribe_all") {
                subs.clear();
            } else if (type == "command") {
                const std::string cmd = find_string_field(payload, "command");
                log_fmt("[ws] command %s", cmd.c_str());
                ws_send_text(client, "{\"type\":\"commandResult\",\"id\":\"" + id + "\",\"success\":true}");
            }
        }

        const uint64_t now = now_ms();
        for (auto& kv : subs) {
            Sub& sub = kv.second;
            if (now - sub.last >= static_cast<uint64_t>(sub.freqMs)) {
                sub.last = now;
                if (!ws_send_text(client, data_msg(kv.first, fields_for(kv.first)))) { return; }
            }
        }
    }
    log_line("[ws] client disconnected");
}

} // namespace

namespace ws {

void run_server(int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { log_line("[ws] WSAStartup failed"); return; }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) { log_line("[ws] socket() failed"); return; }

    BOOL reuse = TRUE;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 — reachable from Android localhost
    addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        log_fmt("[ws] bind(:%d) failed (WSA %d)", port, WSAGetLastError());
        closesocket(listener); WSACleanup(); return;
    }
    if (listen(listener, 1) == SOCKET_ERROR) {
        log_line("[ws] listen() failed"); closesocket(listener); WSACleanup(); return;
    }
    log_fmt("[ws] listening on 0.0.0.0:%d", port);

    for (;;) {
        SOCKET client = accept(listener, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        handle_client(client);
        closesocket(client);
    }
    // unreachable in the spike (server runs for the game's lifetime)
}

} // namespace ws
