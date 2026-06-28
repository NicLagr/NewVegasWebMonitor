#include "ws_server.h"
#include "sha1.h"
#include "snapshot.h"
#include "cmd_queue.h"
#include <cstdlib>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cmath>
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
    // Match the KEY (token followed by ':'), not a value that happens to equal it
    // (e.g. "command" appears as the value of "type":"command").
    const std::string needle = "\"" + key + "\"";
    size_t from = 0;
    for (;;) {
        size_t k = json.find(needle, from);
        if (k == std::string::npos) return "";
        size_t p = k + needle.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) ++p;
        if (p >= json.size() || json[p] != ':') { from = k + 1; continue; } // a value, not a key
        size_t q1 = json.find('"', p + 1);
        if (q1 == std::string::npos) return "";
        size_t q2 = json.find('"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return json.substr(q1 + 1, q2 - q1 - 1);
    }
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
        return "{\"language\":\"en\",\"features\":[\"player\",\"player.quests\",\"inventory\",\"map\",\"game.radio\"]}";

    if (id == "map.player") {
        const GameSnapshot s = snapshot_get();
        if (!s.inGame) return "{\"position\":null}";
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "{\"position\":{\"x\":%.1f,\"y\":%.1f,\"z\":%.1f,\"angle\":%.4f,"
            "\"isInterior\":%s,\"worldspace\":\"%s\",\"parentWorldspace\":\"%s\","
            "\"worldspaceFormId\":null,\"parentWorldspaceFormId\":null,\"cell\":null,\"cellFormId\":null}}",
            s.posX, s.posY, s.posZ, s.angle, s.isInterior ? "true" : "false",
            s.worldspace.c_str(), s.worldspace.c_str());
        return buf;
    }
    if (id == "map.hotspots")     { auto s = snapshot_get(); return s.hotspots.empty() ? "{\"hot\":[]}" : s.hotspots; }
    if (id == "map.questMarkers") return "{\"marker\":[]}";

    if (id == "quests.questsList") { auto s = snapshot_get(); return s.quests.empty() ? "{\"quests\":[]}" : s.quests; }
    if (id == "radio.status")      { auto s = snapshot_get(); return s.radio.empty()  ? "{\"on\":false,\"station\":null}" : s.radio; }

    if (id == "inventory.categories") { auto s = snapshot_get(); return s.cats.empty()    ? "{\"categories\":[]}" : s.cats; }
    if (id == "inventory.weapons")    { auto s = snapshot_get(); return s.weapons.empty() ? "{\"items\":[],\"ammo\":[]}" : s.weapons; }
    if (id == "inventory.apparel")    { auto s = snapshot_get(); return s.apparel.empty() ? "{\"items\":[]}" : s.apparel; }
    if (id == "inventory.potions")    { auto s = snapshot_get(); return s.aid.empty()     ? "{\"items\":[]}" : s.aid; }
    if (id == "inventory.books")      { auto s = snapshot_get(); return s.notes.empty()   ? "{\"items\":[]}" : s.notes; }
    if (id == "inventory.misc")       { auto s = snapshot_get(); return s.misc.empty()    ? "{\"items\":[],\"gems\":[]}" : s.misc; }

    if (id == "game.status") {
        const GameSnapshot s = snapshot_get();
        const char* b = s.inGame ? "true" : "false";
        const char* menu = s.inGame ? "false" : "true";
        return std::string("{\"status\":{\"canAct\":") + b +
               ",\"controlsEnabled\":" + b +
               ",\"dead\":false,\"inCombat\":false,\"inDialogue\":false,\"inMainMenu\":" + menu +
               ",\"loading\":false,\"paused\":false}}";
    }

    if (id == "character.stats") {
        const GameSnapshot s = snapshot_get();
        // FNV XP curve: total XP to reach level N is 100*N*(N-1), so the
        // threshold for the next level (current level L) is 100*(L+1)*L.
        const long L = s.level > 0 ? s.level : 1;
        const long xpNext = 100L * (L + 1) * L;
        char buf[1024];
        std::snprintf(buf, sizeof(buf),
            "{\"level\":%d,\"xp\":%.0f,\"xpNext\":%ld,\"caps\":%.0f,"
            "\"health\":%.0f,\"healthBase\":%.0f,\"ap\":%.0f,\"apBase\":%.0f,"
            "\"rads\":%.0f,\"radsMax\":%.0f,\"carryWeight\":%.0f,"
            "\"inventoryWeight\":%.1f,\"karma\":%.0f,"
            "\"special\":{\"strength\":%.0f,\"perception\":%.0f,\"endurance\":%.0f,"
            "\"charisma\":%.0f,\"intelligence\":%.0f,\"agility\":%.0f,\"luck\":%.0f},"
            "\"limbs\":{\"head\":%.0f,\"torso\":%.0f,\"leftArm\":%.0f,"
            "\"rightArm\":%.0f,\"leftLeg\":%.0f,\"rightLeg\":%.0f}}",
            s.level, s.xp, xpNext, s.caps,
            s.health, s.healthMax, s.ap, s.apMax, s.rads, s.radsMax,
            // Pip-Boy floors the displayed inventory weight (210.6 -> 210), so
            // floor here too rather than letting the frontend round up to 211.
            s.carryWeight, std::floor(s.invWeight), s.karma,
            s.special[0], s.special[1], s.special[2], s.special[3],
            s.special[4], s.special[5], s.special[6],
            s.limb[0], s.limb[1], s.limb[2], s.limb[3], s.limb[4], s.limb[5]);
        return buf;
    }

    // inventory.* / map.* — Phase 2b/2c.
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
                const std::string cmd    = find_string_field(payload, "command");
                const std::string formIdS = find_string_field(payload, "formId");
                const uint32_t formId = formIdS.empty() ? 0u
                    : (uint32_t)std::strtoul(formIdS.c_str(), nullptr, 0);
                const long count = find_number_field(payload, "count", 1);
                if (cmd == "player_marker_set") {
                    const float x = (float)find_number_field(payload, "x", 0);
                    const float y = (float)find_number_field(payload, "y", 0);
                    const float z = (float)find_number_field(payload, "z", 0);
                    cmd_push_marker(cmd, x, y, z);
                    log_fmt("[ws] command player_marker_set x=%.0f y=%.0f", x, y);
                } else if (cmd == "player_marker_clear") {
                    cmd_push_marker(cmd, 0.f, 0.f, 0.f);
                    log_fmt("[ws] command player_marker_clear");
                } else if (cmd == "radio_off") {
                    cmd_push("radio_off", 0, 0);
                    log_fmt("[ws] command radio_off");
                } else if (cmd == "radio_tune") {
                    if (formId) cmd_push("radio_tune", formId, 0);
                    log_fmt("[ws] command radio_tune form=0x%08X", formId);
                } else if (cmd == "quest_set_active") {
                    const bool active = payload.find("\"active\":true") != std::string::npos;
                    if (formId) cmd_push(cmd, formId, active ? 1 : 0);
                    log_fmt("[ws] command quest_set_active form=0x%08X active=%d", formId, (int)active);
                } else if (!cmd.empty() && formId) {
                    cmd_push(cmd, formId, (int)count); // executed on the main thread
                    log_fmt("[ws] command %s form=0x%08X", cmd.c_str(), formId);
                }
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
