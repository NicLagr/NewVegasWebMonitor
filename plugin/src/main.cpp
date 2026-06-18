// FNVWebSocket — NVSE plugin (Phase 2b: stats + inventory + caps + position).
//
// All game reads run on the MAIN THREAD via kMessage_MainGameLoop. Stats are read
// every frame; the inventory walk (heavier) is throttled to ~1/sec. Results go into
// a thread-safe snapshot the WebSocket thread serializes. Every game access is
// null-guarded — a bad read here crashes FNV.
#include "nvse/PluginAPI.h"
#include "nvse/GameAPI.h"
#include "nvse/GameObjects.h"
#include "nvse/GameForms.h"
#include "nvse/GameExtraData.h"
#include "nvse/GameRTTI.h"

#include "snapshot.h"
#include "ws_server.h"
#include "cmd_queue.h"

#include <thread>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>

// g_thePlayer is defined by the compiled xNVSE GameObjects.cpp.

static PluginHandle            g_pluginHandle = kPluginHandle_Invalid;
static NVSEMessagingInterface* g_messaging    = nullptr;

// Cached inventory payloads, rebuilt ~1/sec on the main thread.
static std::string g_cats, g_weapons, g_apparel, g_aid, g_notes, g_misc;
static float       g_caps  = 0;
static int         g_frame = 0;
static std::unordered_map<UInt32, TESForm*> g_formById; // refID -> form, for command lookup

static void logf(const char* fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    if (FILE* f = std::fopen("FNVWebSocket.log", "a")) { std::fprintf(f, "%s\n", buf); std::fclose(f); }
}

static std::string jsonEscape(const char* s) {
    std::string o;
    if (!s) return o;
    for (; *s; ++s) {
        char c = *s;
        if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
        else if ((unsigned char)c < 0x20) { /* skip control chars */ }
        else o.push_back(c);
    }
    return o;
}

static const char* weaponTypeStr(UInt8 t) {
    // Map FNV weapon-type to the app's icon enum (see weaponIcons.ts).
    switch (t) {
        case 0:  return "HandToHandMelee"; // hand to hand
        case 1:  return "OneHandAxe";      // 1h melee (blade)
        case 2:  return "TwoHandAxe";      // 2h melee
        case 3:  return "OneHandSword";    // 1h pistol  -> pistol icon
        case 4:  return "TwoHandSword";    // 2h rifle   -> rifle icon
        default: return "OneHandSword";
    }
}

static float itemValue(TESForm* f) {
    TESValueForm* v = DYNAMIC_CAST(f, TESForm, TESValueForm);
    return v ? (float)v->value : 0.0f;
}
static float itemWeight(TESForm* f) {
    TESWeightForm* w = DYNAMIC_CAST(f, TESForm, TESWeightForm);
    return w ? w->weight : 0.0f;
}

// Append the BaseItem fields shared by every category. Caller opens the `{`.
// `value` is the (condition-adjusted) value to display.
static void appendBase(std::string& o, TESForm* f, int count, float value) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "\"formId\":\"0x%08X\",\"count\":%d,\"value\":%.0f,\"weight\":%.2f,",
                  f->refID, count, value, itemWeight(f));
    o += buf;
    o += "\"name\":\""; o += jsonEscape(f->GetTheName()); o += "\",";
    o += "\"isFavorite\":false,\"isStolen\":false";
}

static void rebuildInventory(PlayerCharacter* p) {
    std::string weapons, apparel, aid, notes, misc;
    int nWeap = 0, nApp = 0, nAid = 0, nNote = 0, nMisc = 0;
    float caps = 0;

    g_formById.clear();
    ExtraContainerChanges* cc = ExtraContainerChanges::GetForRef((TESObjectREFR*)p);
    ExtraContainerChanges::EntryDataList* list = cc ? cc->GetEntryDataList() : nullptr;
    if (list) {
        for (auto it = list->Begin(); !it.End(); ++it) {
            ExtraContainerChanges::EntryData* e = it.Get();
            if (!e || !e->type) continue;
            TESForm* f = e->type;
            const int count = e->countDelta;
            if (count <= 0) continue;
            g_formById[f->refID] = f;

            if (f->refID == 0x0000000F) { caps = (float)count; continue; } // Caps

            // Condition % + equipped state from per-stack extra data (weapons/armor only).
            int    condPct = 100;
            bool   equipped = false;
            UInt32 dbgMaxH = 0;
            float  dbgCur  = -1.0f;
            if (f->typeID == kFormType_TESObjectWEAP || f->typeID == kFormType_TESObjectARMO) {
                dbgMaxH = (f->typeID == kFormType_TESObjectWEAP)
                    ? ((TESObjectWEAP*)f)->health.health
                    : ((TESObjectARMO*)f)->health.health;
                bool gotHealth = false;
                if (e->extendData) {
                    for (auto xit = e->extendData->Begin(); !xit.End(); ++xit) {
                        ExtraDataList* xdl = xit.Get();
                        if (!xdl) continue;
                        if (xdl->GetByType(kExtraData_Worn) || xdl->GetByType(kExtraData_WornLeft))
                            equipped = true;
                        if (!gotHealth) {
                            ExtraHealth* h = (ExtraHealth*)xdl->GetByType(kExtraData_Health);
                            if (h) {
                                dbgCur = h->health;
                                if (dbgMaxH > 0) {
                                    int pc = (int)((h->health / (float)dbgMaxH) * 100.0f);
                                    condPct = pc < 0 ? 0 : (pc > 100 ? 100 : pc);
                                }
                                gotHealth = true;
                            }
                        }
                    }
                }
                // DEBUG: log equipped weapon/armor so we can verify the value/damage/DT math.
                if (equipped) {
                    const char* nm = f->GetTheName(); if (!nm) nm = "?";
                    if (f->typeID == kFormType_TESObjectWEAP) {
                        TESObjectWEAP* w = (TESObjectWEAP*)f;
                        logf("[dbg] WEAP %s baseDmg=%u maxH=%u cur=%.0f cond=%d skillAV=%u skill=%.0f baseVal=%.0f",
                             nm, w->attackDmg.damage, dbgMaxH, dbgCur, condPct,
                             w->weaponSkill, p->avOwner.Fn_03(w->weaponSkill), itemValue(f));
                    } else {
                        TESObjectARMO* a = (TESObjectARMO*)f;
                        logf("[dbg] ARMO %s DT=%.1f AR=%u maxH=%u cur=%.0f cond=%d baseVal=%.0f",
                             nm, a->damageThreshold, a->armorRating, dbgMaxH, dbgCur, condPct, itemValue(f));
                    }
                }
            }

            // Condition-adjusted value (Pip-Boy shows value × condition). For
            // non-degradable items condPct stays 100, so this is just base value.
            const float condF = condPct / 100.0f;
            const float effValue = itemValue(f) * condF;

            switch (f->typeID) {
                case kFormType_TESObjectWEAP: {
                    TESObjectWEAP* w = (TESObjectWEAP*)f;
                    // FNV displayed DAM = base × condition(0.5 floor) × skill(0.5..1.0).
                    const UInt32 baseDmg = w->attackDmg.damage;
                    float skill = p->avOwner.Fn_03(w->weaponSkill);
                    if (skill < 0.0f) skill = 0.0f; else if (skill > 100.0f) skill = 100.0f;
                    const int dispDmg = (int)(baseDmg * (0.5f + 0.5f * condF) * (0.5f + 0.5f * skill / 100.0f) + 0.5f);
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    char b[320];
                    std::snprintf(b, sizeof(b), ",\"categoryType\":\"Weapon\",\"damage\":%d,\"baseDamage\":%u,"
                        "\"condition\":%d,\"isEquipped\":%s,\"equippedHand\":%s,\"isTwoHanded\":false,"
                        "\"weaponType\":\"%s\",\"equipSlots\":[],\"enchantment\":null,\"enchantmentCharge\":null}",
                        dispDmg, baseDmg, condPct,
                        equipped ? "true" : "false", equipped ? "\"right\"" : "null",
                        weaponTypeStr(w->eWeaponType));
                    o += b;
                    if (nWeap++) weapons += ","; weapons += o; break;
                }
                case kFormType_TESObjectARMO: {
                    TESObjectARMO* a = (TESObjectARMO*)f;
                    const char* cls = (a->armorFlags & 1) ? "Heavy" : "Light";
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    char b[220];
                    std::snprintf(b, sizeof(b), ",\"categoryType\":\"Apparel\",\"damageThreshold\":%.0f,"
                        "\"condition\":%d,\"armorType\":\"%s\",\"bodySlots\":[\"Body\"],"
                        "\"isEquipped\":%s,\"equipSlots\":[],\"enchantment\":null}",
                        a->damageThreshold, condPct, cls, equipped ? "true" : "false");
                    o += b;
                    if (nApp++) apparel += ","; apparel += o; break;
                }
                case kFormType_AlchemyItem: {
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    o += ",\"categoryType\":\"Potion\",\"effects\":[]}";
                    if (nAid++) aid += ","; aid += o; break;
                }
                case kFormType_BGSNote:
                case kFormType_TESObjectBOOK: {
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    o += ",\"categoryType\":\"Book\",\"description\":\"\"}";
                    if (nNote++) notes += ","; notes += o; break;
                }
                default: { // misc, keys, ammo, etc.
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    o += ",\"categoryType\":\"Misc\"}";
                    if (nMisc++) misc += ","; misc += o; break;
                }
            }
        }
    }

    g_weapons = "{\"items\":[" + weapons + "],\"ammo\":[]}";
    g_apparel = "{\"items\":[" + apparel + "]}";
    g_aid     = "{\"items\":[" + aid + "]}";
    g_notes   = "{\"items\":[" + notes + "]}";
    g_misc    = "{\"items\":[" + misc + "],\"gems\":[]}";
    g_caps    = caps;

    // Categories drive the inventory subtabs (only show non-empty ones).
    std::string cats;
    auto addCat = [&](const char* id, const char* label, int n) {
        if (!n) return;
        char b[128];
        std::snprintf(b, sizeof(b), "%s{\"categoryId\":\"%s\",\"name\":\"%s\",\"count\":%d}",
                      cats.empty() ? "" : ",", id, label, n);
        cats += b;
    };
    addCat("Weapons", "Weapons", nWeap);
    addCat("Apparel", "Apparel", nApp);
    addCat("Potions", "Aid",     nAid);
    addCat("Books",   "Notes",   nNote);
    addCat("Misc",    "Misc",    nMisc);
    g_cats = "{\"categories\":[" + cats + "]}";
}

static void ReadGameState() {
    GameSnapshot s;
    PlayerCharacter* p = g_thePlayer ? *g_thePlayer : nullptr;
    if (!p || !p->parentCell) { snapshot_set(s); return; }

    s.inGame = true;

    // Execute queued commands on the main thread (equip/use/unequip), with a
    // ~0.5s per-(item,action) debounce — the app fires bursts per tap, which
    // would otherwise consume multiple aid items.
    static std::unordered_map<uint64_t, int> cmdCooldown;
    for (const PluginCmd& c : cmd_drain()) {
        auto found = g_formById.find(c.formId);
        if (found == g_formById.end() || !found->second) continue;
        const uint64_t key = ((uint64_t)c.formId << 8) | (uint8_t)(c.type.empty() ? 0 : c.type[0]);
        auto cd = cmdCooldown.find(key);
        if (cd != cmdCooldown.end() && (g_frame - cd->second) < 30) continue;
        cmdCooldown[key] = g_frame;

        TESForm* f = found->second;
        if (c.type == "equip" || c.type == "use")      p->EquipItem(f, 1, nullptr, 1, false, 1);
        else if (c.type == "unequip")                  p->UnequipItem(f, 1, nullptr, 1, false, 1);
        // "drop" — TODO (RemoveItem); acked already.
    }

    ActorValueOwner& av = p->avOwner;
    s.level     = (int)av.Fn_0A();
    s.health    = av.Fn_03(eActorVal_Health);
    s.healthMax = av.Fn_08(eActorVal_Health);
    s.ap        = av.Fn_03(eActorVal_ActionPoints);
    s.apMax     = av.Fn_08(eActorVal_ActionPoints);
    s.rads      = av.Fn_03(eActorVal_RadLevel);
    s.radsMax   = 1000.0f;
    s.xp        = av.Fn_03(eActorVal_XP);
    s.carryWeight = av.Fn_08(eActorVal_CarryWeight);
    s.invWeight   = av.Fn_03(eActorVal_InventoryWeight);
    s.karma       = av.Fn_03(eActorVal_Karma);

    // Position + worldspace (for map; logged so we can calibrate to real coords).
    s.posX = p->posX; s.posY = p->posY; s.posZ = p->posZ; s.angle = p->rotZ;
    TESObjectCELL* cell = p->parentCell;
    s.isInterior = cell->IsInterior();
    if (!s.isInterior && cell->worldSpace) {
        const char* eid = cell->worldSpace->GetName();
        if (eid) s.worldspace = eid;
    }

    // Inventory: rebuild ~once per second (heavy).
    if ((g_frame++ % 60) == 0) rebuildInventory(p);
    s.caps = g_caps; s.cats = g_cats; s.weapons = g_weapons;
    s.apparel = g_apparel; s.aid = g_aid; s.notes = g_notes; s.misc = g_misc;

    // Log position periodically so you can read real coords for map calibration.
    if ((g_frame % 180) == 0)
        logf("[ws] pos x=%.0f y=%.0f ws=%s interior=%d", s.posX, s.posY,
             s.worldspace.empty() ? "?" : s.worldspace.c_str(), (int)s.isInterior);

    snapshot_set(s);
}

static void MessageHandler(NVSEMessagingInterface::Message* msg) {
    if (msg->type == NVSEMessagingInterface::kMessage_MainGameLoop) ReadGameState();
}

extern "C" {

__declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name        = "FNVWebSocket";
    info->version     = 1;
    if (nvse->isEditor) return false;
    return true;
}

__declspec(dllexport) bool NVSEPlugin_Load(NVSEInterface* nvse) {
    g_pluginHandle = nvse->GetPluginHandle();
    g_messaging = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
    if (g_messaging)
        g_messaging->RegisterListener(g_pluginHandle, "NVSE", MessageHandler);
    static std::thread server([]() { ws::run_server(8765); });
    server.detach();
    return true;
}

} // extern "C"
