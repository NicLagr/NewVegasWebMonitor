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
#include "nvse/GameEffects.h"
#include "nvse/GameExtraData.h"
#include "nvse/GameRTTI.h"
#include "nvse/GameData.h"

#include "snapshot.h"
#include "ws_server.h"
#include "cmd_queue.h"

#include <thread>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

// g_thePlayer is defined by the compiled xNVSE GameObjects.cpp.

static PluginHandle            g_pluginHandle = kPluginHandle_Invalid;
static NVSEMessagingInterface* g_messaging    = nullptr;
static NVSEConsoleInterface*   g_console      = nullptr; // for RunScriptLine (radio tune/off)

// Currently-tuned Pip-Boy radio entry (global). FNV 1.4.0.525.
struct RadioEntry { TESObjectREFR* radioRef; void* ptr04; UInt32 unk08[7]; };

// Cached inventory payloads, rebuilt ~1/sec on the main thread.
static std::string g_cats, g_weapons, g_apparel, g_aid, g_notes, g_misc;
static std::string g_hotspots; // discovered map markers, {"hot":[...]}
static std::string g_quests;   // quest journal, {"quests":[...]}
static std::string g_radio;    // pip-boy radio, {"on":bool,"station":...}
static float       g_caps  = 0;
static int         g_frame = 0;
static std::unordered_map<UInt32, TESForm*> g_formById;        // refID -> form, for command lookup
static std::unordered_map<UInt32, TESObjectREFR*> g_markerById; // map-marker refID -> ref, for fast-travel

// Engine "move a reference to a marker ref" (the teleport behind the script
// MoveTo). __cdecl(refToMove, targetMarker, 0, 0, 0). FNV 1.4.0.525.
typedef void (__cdecl *MoveToMarker_t)(TESObjectREFR*, TESObjectREFR*, void*, void*, void*);
static const MoveToMarker_t EngineMoveToMarker = (MoveToMarker_t)0x5CCB20;

// Player's custom map marker (the red X you place on the Pip-Boy world map).
// thiscall on the player. Set: (x, y, z, worldspace/cell as TESForm*); Clear: ().
// Addresses from JohnnyGuitar's SetCustomMapMarker/ClearCustomMapMarker (FNV 1.4.0.525).
typedef void (__thiscall *SetCustomMarker_t)(void* player, float x, float y, float z, TESForm* space);
typedef void (__thiscall *ClearCustomMarker_t)(void* player);
static const SetCustomMarker_t   EngineSetCustomMarker   = (SetCustomMarker_t)0x952E60;
static const ClearCustomMarker_t EngineClearCustomMarker = (ClearCustomMarker_t)0x952F90;

// Last exterior worldspace the player was in — used as the marker's worldspace
// when placing a custom marker (the web map is always the Mojave world map).
static TESWorldSpace* g_lastWorldSpace = nullptr;

static std::unordered_map<UInt32, TESQuest*> g_questById; // quest refID -> quest, for quest_set_active

// Engine "rebuild a quest's tracked targets" (refreshes the compass/HUD after
// changing the active quest). thiscall(quest, &questTargetList, &questObjectiveList).
typedef void (__thiscall *UpdateQuestTargets_t)(TESQuest*, void*, void*);
static const UpdateQuestTargets_t EngineUpdateQuestTargets = (UpdateQuestTargets_t)0x60F110;

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

// Pip-Boy weapon DAM, computed by the ENGINE's own routine (no formula guessing).
// This replicates the core of JIP LN's ExtraContainerChanges::EntryData::
// CalculateWeaponDamage (credit: JazzIsParis): it calls the engine per-attack
// damage function at 0x644CE0, which already applies skill, condition, and
// projectile count exactly as the in-game item card does. FNV 1.4.0.525 addrs.
//   entry     = the ExtraContainerChanges::EntryData* for the weapon stack
//   owner     = the actor (player); the engine reads owner->avOwner at +0xA4
//   condition = health ratio 0..1
//   ammo      = ammo form (may be null)
// MUST be called on the main thread.
static __declspec(naked) float __fastcall EngineWeaponDamage(
    void* /*entry, ecx*/, void* /*edx unused*/, void* /*owner*/, float /*condition*/, void* /*ammo*/) {
    __asm {
        push    esi
        mov     esi, ecx
        cmp     dword ptr [esp+0x10], 0   // ammo != null ?
        setnz   dl
        movzx   edx, dl
        push    edx
        push    ecx
        push    0
        push    0
        push    0x3F800000                // 1.0f
        push    dword ptr [esp+0x20]      // condition
        push    dword ptr [ecx+8]         // entry->type (the weapon)
        mov     ecx, [esp+0x24]           // owner
        add     ecx, 0xA4                 // -> owner->avOwner
        push    ecx
        mov     eax, 0x644CE0
        call    eax
        add     esp, 0x20
        pop     esi
        ret     0xC                       // __fastcall: clean owner/condition/ammo
    }
}

// Inventory icon DDS path for an item form. xNVSE doesn't expose the icon
// member uniformly (armor nests it in bipedModel, misc/key don't declare it),
// so read the TESIcon's ddsPath by raw offset. A TESIcon is BaseFormComponent
// (vtbl @ 0) + String ddsPath @ +4, and String.m_data (char*) is its first
// field, so ddsPath.m_data = *(char**)(form + iconOffset + 4). FNV 1.4.0.525.
static const char* iconDdsPath(TESForm* f) {
    UInt32 iconOff;
    switch (f->typeID) {
        case kFormType_TESObjectWEAP: iconOff = 0x5C; break;
        case kFormType_AlchemyItem:   iconOff = 0x6C; break;
        case kFormType_TESObjectMISC: iconOff = 0x5C; break;
        case kFormType_TESAmmo:       iconOff = 0x5C; break;
        case kFormType_BGSNote:       iconOff = 0x54; break;
        case kFormType_TESKey:        iconOff = 0x5C; break;          // : TESObjectMISC
        case kFormType_TESObjectARMO: iconOff = 0x70 + 0x8C; break;   // bipedModel.icon[0] (male)
        default: return nullptr;
    }
    return *(const char* const*)((const char*)f + iconOff + 4);
}

// Normalize the DDS icon path to the app's web path: lowercase, forward
// slashes, no leading "textures/", no ".dds". The app loads the matching
// public/icons/pipboy/<path>.webp. "" when the form has no icon.
static std::string iconWebPath(TESForm* f) {
    const char* p = iconDdsPath(f);
    if (!p || !*p) return "";
    std::string s;
    for (; *p; ++p) {
        char c = *p;
        if (c == '\\') c = '/';
        else if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        s.push_back(c);
    }
    if (s.rfind("textures/", 0) == 0) s.erase(0, 9);
    if (s.size() > 4 && s.compare(s.size() - 4, 4, ".dds") == 0) s.erase(s.size() - 4);
    return s;
}

// Append the BaseItem fields shared by every category. Caller opens the `{`.
// `value` is the (condition-adjusted) value to display.
static void appendBase(std::string& o, TESForm* f, int count, float value) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "\"formId\":\"0x%08X\",\"count\":%d,\"value\":%.0f,\"weight\":%.2f,",
                  f->refID, count, value, itemWeight(f));
    o += buf;
    o += "\"name\":\""; o += jsonEscape(f->GetTheName()); o += "\",";
    o += "\"iconPath\":\""; o += jsonEscape(iconWebPath(f).c_str()); o += "\",";
    o += "\"isFavorite\":false,\"isStolen\":false";
}

// FNV ExtraMapMarker type (0-14) -> frontend KnownMapHotspotType icon key.
static const char* mapMarkerTypeStr(unsigned t) {
    switch (t) {
        case 1:  return "City";          // kType_City
        case 2:  return "Settlement";    // kType_Settlement
        case 3:  return "Camp";          // kType_Encampment
        case 4:  return "Monument";      // kType_NaturalLandmark
        case 5:  return "Cave";          // kType_Cave
        case 6:  return "Factory";       // kType_Factory
        case 7:  return "Monument";      // kType_Memorial
        case 8:  return "MilitaryBase";  // kType_Military
        case 9:  return "Building";      // kType_Office
        case 10: return "Ruins";         // kType_TownRuins
        case 11: return "Ruins";         // kType_UrbanRuins
        case 12: return "Ruins";         // kType_SewerRuins
        case 13: return "Building";      // kType_Metro
        case 14: return "Vault";         // kType_Vault
        default: return "Building";      // kType_None / unknown
    }
}

// The player's known map markers live at PlayerCharacter+0x7C8 as a
// tList<MapMarkerInfo> (not exposed by xNVSE). FNV 1.4.0.525 layout.
struct MapMarkerInfo {
    ExtraMapMarker::MarkerData* markerData;
    TESObjectREFR*              markerRef;
};

// Build {"hot":[...]} from discovered (visible) map markers. Pure read; the
// world position is the same raw coord space the map projection expects.
static void rebuildHotspots(PlayerCharacter* p) {
    std::string hot;
    int n = 0;
    g_markerById.clear();
    auto* list = (tList<MapMarkerInfo>*)((char*)p + 0x7C8);
    for (auto it = list->Begin(); !it.End(); ++it) {
        MapMarkerInfo* mi = it.Get();
        if (!mi || !mi->markerData || !mi->markerRef) continue;
        ExtraMapMarker::MarkerData* md = mi->markerData;
        const unsigned flags = md->flags;
        if (!(flags & ExtraMapMarker::kFlag_Visible)) continue; // only discovered
        const bool canTravel = (flags & ExtraMapMarker::kFlag_CanTravel) != 0;
        const unsigned type  = md->type;
        const char* name = md->fullName.name.m_data;
        if (!name || !*name) name = mi->markerRef->GetTheName();
        if (!name) name = "";
        TESObjectREFR* r = mi->markerRef;
        g_markerById[r->refID] = r; // for fast-travel lookup
        char b[320];
        std::snprintf(b, sizeof(b),
            "%s{\"type\":\"%s\",\"typeId\":%u,\"refId\":\"0x%08X\",\"name\":\"%s\","
            "\"x\":%.1f,\"y\":%.1f,\"isVisible\":true,\"canFastTravel\":%s}",
            n ? "," : "", mapMarkerTypeStr(type), type, r->refID,
            jsonEscape(name).c_str(), r->posX, r->posY, canTravel ? "true" : "false");
        hot += b; n++;
    }
    g_hotspots = "{\"hot\":[" + hot + "]}";
}

// Build {"quests":[...]} from the player's quest log. The active objectives live
// in PlayerCharacter::questObjectiveList (+0x6BC); the tracked quest pointer is
// activeQuest (+0x6B8). We group objectives by their parent quest and emit the
// QuestJournalEntry shape the frontend expects. FNV 1.4.0.525 offsets.
static void rebuildQuests(PlayerCharacter* p) {
    TESQuest* activeQ = *(TESQuest**)((char*)p + 0x6B8);
    auto* objList = (tList<BGSQuestObjective>*)((char*)p + 0x6BC);

    g_questById.clear();
    // Unique parent quests, in first-seen order.
    std::vector<TESQuest*> quests;
    for (auto it = objList->Begin(); !it.End(); ++it) {
        BGSQuestObjective* o = it.Get();
        if (!o || !o->quest) continue;
        if (std::find(quests.begin(), quests.end(), o->quest) == quests.end()) {
            quests.push_back(o->quest);
            g_questById[o->quest->refID] = o->quest;
        }
    }

    std::string arr; int nq = 0;
    for (TESQuest* q : quests) {
        std::string steps; int ns = 0; bool allDone = true, anyStep = false;
        for (auto it = objList->Begin(); !it.End(); ++it) {
            BGSQuestObjective* o = it.Get();
            if (!o || o->quest != q) continue;
            anyStep = true;
            const bool completed = (o->status & 2) != 0;
            if (!completed) allDone = false;
            const char* txt = o->displayText.m_data ? o->displayText.m_data : "";
            std::string et = jsonEscape(txt);
            // Variable-length text -> build with std::string (a fixed buffer could
            // truncate mid-JSON and break the whole payload).
            char si[40], sf[112];
            std::snprintf(si, sizeof(si), "\"index\":%u,", o->objectiveId);
            std::snprintf(sf, sizeof(sf),
                "\"completed\":%s,\"failed\":false,\"state\":\"\",\"stateRaw\":%u,\"instanceId\":%u}",
                completed ? "true" : "false", o->status, o->objectiveId);
            if (ns) steps += ",";
            steps += "{"; steps += si;
            steps += "\"text\":\""; steps += et; steps += "\",\"textRaw\":\""; steps += et; steps += "\",";
            steps += sf;
            ns++;
        }
        const bool running     = (q->flags & 1) != 0;
        const bool isActive     = (q == activeQ);
        const bool isCompleted  = anyStep && allDone;
        const char* nm = q->GetTheName(); if (!nm) nm = "";
        std::string en = jsonEscape(nm);
        char ids[80], fl[176];
        std::snprintf(ids, sizeof(ids),
            "\"formId\":\"0x%08X\",\"questFormId\":\"0x%08X\",", q->refID, q->refID);
        std::snprintf(fl, sizeof(fl),
            "\"isActive\":%s,\"isRunning\":%s,\"isCompleted\":%s,\"currentStage\":%u,\"currentInstanceId\":0,",
            isActive ? "true" : "false", running ? "true" : "false",
            isCompleted ? "true" : "false", q->currentStage);
        if (nq) arr += ",";
        arr += "{\"type\":\"quest\","; arr += ids;
        arr += "\"questEditorId\":\"\",\"name\":\""; arr += en; arr += "\",\"nameRaw\":\""; arr += en;
        arr += "\",\"description\":\"\",\"descriptionRaw\":\"\",\"descriptionStage\":0,\"questType\":\"\",\"isMisc\":false,";
        arr += fl;
        arr += "\"steps\":["; arr += steps; arr += "]}";
        nq++;
    }
    g_quests = "{\"quests\":[" + arr + "]}";
}

// ExtraRadioData layout (xNVSE lacks the class). radius@0C, rangeType@10,
// staticPerc@14, positionRef@18. rangeType 0 = local radius; !=0 = broadcast
// across the worldspace (always receivable). FNV 1.4.0.525.
struct ExtraRadioDataMin { char _hdr[0x0C]; float radius; UInt32 rangeType; float staticPerc; TESObjectREFR* positionRef; };

// Enumerate receivable radio stations: every ref with ExtraRadioData that the
// player can currently pick up (worldspace broadcasts always; local ones only
// in range). Mirrors how the game/JIP scans (DataHandler::cellArray for
// kExtraData_RadioData). Heavy-ish -> run on a slower cadence than the rest.
// Radio broadcast range type (ExtraRadioData.rangeType), per the GECK:
//   0 = Radius (local), 1 = Everywhere, 2 = World Space (+ linked interiors),
//   3 = Linked Interiors. "Receivable now" = what the Pip-Boy would list.
static std::string g_radioStations = ""; // cached station array body w/ "receivable" flag
static void rebuildRadioStations(PlayerCharacter* p) {
    DataHandler* dh = DataHandler::Get();
    if (!dh) return;
    TESObjectCELL* pcell = p->parentCell;
    const bool playerInterior  = pcell && pcell->IsInterior();
    TESWorldSpace* playerWS = (pcell && !playerInterior) ? pcell->worldSpace : nullptr;
    const float px = p->posX, py = p->posY;

    std::string arr; int n = 0;
    std::unordered_set<std::string> seen;
    auto& cells = dh->cellArray;
    const int count = cells.firstFreeEntry;
    for (int i = 0; i < count && n < 60; i++) {
        TESObjectCELL* c = cells.data[i];
        if (!c) continue;
        for (auto it = c->objectList.Begin(); !it.End(); ++it) {
            TESObjectREFR* r = it.Get();
            if (!r) continue;
            BSExtraData* xd = r->extraDataList.GetByType(kExtraData_RadioData);
            if (!xd) continue;
            if (r->flags & 0x800) continue; // disabled (inactive station) — not real yet
            ExtraRadioDataMin* rd = (ExtraRadioDataMin*)xd;
            const char* nm = r->GetTheName();
            // DEBUG: log every radio ref once so the filter stays verifiable.
            static std::unordered_set<UInt32> dbgRadio;
            if (dbgRadio.insert(r->refID).second)
                logf("[dbg] RADIO %s rangeType=%u radius=%.0f flags=0x%08X",
                     nm ? nm : "?", rd->rangeType, rd->radius, r->flags);
            if (!nm || !*nm) continue;

            // "receivable" = the Pip-Boy would currently list it (its default
            // visibility). The rest are still sent so the UI can offer them under
            // "show hidden", except out-of-range local props (pure clutter).
            bool receivable;
            switch (rd->rangeType) {
                case 1: receivable = true; break;                              // Everywhere
                case 2: receivable = (c->worldSpace && c->worldSpace == playerWS); break; // World Space
                case 3: receivable = playerInterior; break;                    // Linked Interiors
                case 0: {                                                      // Radius (local)
                    TESObjectREFR* o = rd->positionRef ? rd->positionRef : r;
                    const float dx = px - o->posX, dy = py - o->posY;
                    receivable = (rd->radius > 0.0f) && (dx * dx + dy * dy) <= (rd->radius * rd->radius);
                    break;
                }
                default: receivable = true; break;
            }
            // Radius props out of range = every radio object in the world -> skip
            // (would flood "show hidden"). Broadcast types are always listed.
            if (rd->rangeType == 0 && !receivable) continue;
            if (!seen.insert(nm).second) continue; // dedupe by name

            char b[280];
            std::snprintf(b, sizeof(b), "%s{\"refId\":\"0x%08X\",\"name\":\"%s\",\"receivable\":%s}",
                          n ? "," : "", r->refID, jsonEscape(nm).c_str(), receivable ? "true" : "false");
            arr += b; if (++n >= 60) break;
        }
    }
    g_radioStations = arr;
}

// Combine the cached station list with the currently-tuned station (read each
// tick from the engine RadioEntry global at 0x11DD42C). Cheap.
static void rebuildRadio() {
    RadioEntry* pr = *(RadioEntry**)0x11DD42C;
    char head[80];
    if (pr && pr->radioRef)
        std::snprintf(head, sizeof(head), "\"on\":true,\"currentRefId\":\"0x%08X\",", pr->radioRef->refID);
    else
        std::snprintf(head, sizeof(head), "\"on\":false,\"currentRefId\":null,");
    g_radio = std::string("{") + head + "\"stations\":[" + g_radioStations + "]}";
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
            float  healthRatio = 1.0f; // precise cur/maxH (Pip-Boy value uses the exact ratio, not the rounded %)
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
                                    float r = h->health / (float)dbgMaxH;
                                    healthRatio = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
                                    int pc = (int)(healthRatio * 100.0f + 0.5f);
                                    condPct = pc < 0 ? 0 : (pc > 100 ? 100 : pc);
                                }
                                gotHealth = true;
                            }
                        }
                    }
                }
                (void)dbgCur; (void)dbgMaxH;
            }

            // Condition-adjusted value. FNV's Pip-Boy scales value by the health
            // ratio raised to ~1.5 (verified: machete 75×0.92^1.5=66, Legion armor
            // 300×0.552^1.5=123). Non-degradable items keep ratio 1.0 → base value.
            const float effValue = itemValue(f) * powf(healthRatio, 1.5f);

            switch (f->typeID) {
                case kFormType_TESObjectWEAP: {
                    TESObjectWEAP* w = (TESObjectWEAP*)f;
                    const UInt32 baseDmg = w->attackDmg.damage;
                    // Pip-Boy DAM straight from the engine's own routine (exact: skill,
                    // condition, projectile count all handled by the game). Returns the
                    // total per-attack damage; for multi-projectile weapons (shotguns)
                    // the card shows per-pellet × count, so report both.
                    const int   nProj   = w->numProjectiles ? w->numProjectiles : 1;
                    const float engDmg  = EngineWeaponDamage(e, 0, p, healthRatio, nullptr);
                    const int   dispDmg = (int)(engDmg + 0.5f);
                    const float perShot = engDmg / (float)nProj;
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    char b[360];
                    std::snprintf(b, sizeof(b), ",\"categoryType\":\"Weapon\",\"damage\":%d,\"baseDamage\":%u,"
                        "\"damagePerShot\":%.1f,\"numProjectiles\":%d,"
                        "\"condition\":%d,\"isEquipped\":%s,\"equippedHand\":%s,\"isTwoHanded\":false,"
                        "\"weaponType\":\"%s\",\"equipSlots\":[],\"enchantment\":null,\"enchantmentCharge\":null}",
                        dispDmg, baseDmg, perShot, nProj, condPct,
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
                    // Text notes (noteType==1) carry a readable body in noteText;
                    // holotapes/images/voice have none (played via "use").
                    const char* body = nullptr;
                    if (f->typeID == kFormType_BGSNote) {
                        BGSNote* note = (BGSNote*)f;
                        if (note->unk07C == 1 && note->noteText) // unk07C = noteType
                            body = note->noteText->Get((TESForm*)note, 'MANT');
                    }
                    std::string o = "{"; appendBase(o, f, count, effValue);
                    o += ",\"categoryType\":\"Book\",\"description\":\"";
                    o += jsonEscape(body ? body : "");
                    o += "\"}";
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
        // Per-(target,action) debounce — the app fires bursts per tap. Key on the
        // full action (FNV-1a hash) so distinct verbs sharing a first letter
        // (use/unequip, player_marker_set/clear) don't collide.
        uint32_t th = 2166136261u;
        for (char ch : c.type) th = (th ^ (uint8_t)ch) * 16777619u;
        const uint64_t key = ((uint64_t)c.formId << 32) | th;
        auto cd = cmdCooldown.find(key);
        if (cd != cmdCooldown.end() && (g_frame - cd->second) < 30) continue;
        cmdCooldown[key] = g_frame;

        // Fast-travel: teleport the player to the discovered map marker.
        if (c.type == "fast_travel") {
            auto mit = g_markerById.find(c.formId);
            if (mit != g_markerById.end() && mit->second)
                EngineMoveToMarker((TESObjectREFR*)p, mit->second, nullptr, nullptr, nullptr);
            continue;
        }

        // Custom map marker (the Pip-Boy world-map "X"). Set at the tapped world
        // coords in the Mojave worldspace; clear removes it.
        if (c.type == "player_marker_set") {
            TESObjectCELL* pcell = p->parentCell;
            TESForm* space = nullptr;
            if (pcell && !pcell->IsInterior() && pcell->worldSpace) space = (TESForm*)pcell->worldSpace;
            else if (g_lastWorldSpace) space = (TESForm*)g_lastWorldSpace;
            if (space) EngineSetCustomMarker(p, c.x, c.y, c.z, space);
            continue;
        }
        if (c.type == "player_marker_clear") { EngineClearCustomMarker(p); continue; }

        // Radio: turn off / tune via the vanilla PipBoyRadio script command.
        if (c.type == "radio_off") {
            if (g_console) g_console->RunScriptLine2("PipBoyRadioOff", nullptr, true);
            continue;
        }
        if (c.type == "radio_tune") {
            if (g_console && c.formId) {
                char line[48];
                std::snprintf(line, sizeof(line), "PipBoyRadio 1 %X", c.formId);
                g_console->RunScriptLine2(line, nullptr, true);
            }
            continue;
        }

        // Track/untrack a quest (the Pip-Boy "active" quest driving the compass).
        if (c.type == "quest_set_active") {
            auto qit = g_questById.find(c.formId);
            if (qit != g_questById.end() && qit->second) {
                TESQuest* q = qit->second;
                TESQuest** activeQ = (TESQuest**)((char*)p + 0x6B8);
                if (c.count) { // track
                    *activeQ = q;
                    EngineUpdateQuestTargets(q, (char*)p + 0x6C4, (char*)p + 0x6BC);
                } else if (*activeQ == q) { // untrack
                    *activeQ = nullptr;
                }
            }
            continue;
        }

        // Inventory actions need the form looked up in the inventory map.
        auto found = g_formById.find(c.formId);
        if (found == g_formById.end() || !found->second) continue;
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

    // SPECIAL (permanent base) — AVs 5..11.
    for (UInt32 i = 0; i < 7; i++)
        s.special[i] = av.Fn_08((UInt32)eActorVal_Strength + i);
    // Limb condition % (Head, Torso, LeftArm, RightArm, LeftLeg, RightLeg) — AVs 25..30.
    for (UInt32 i = 0; i < 6; i++) {
        const UInt32 code = (UInt32)eActorVal_Head + i;
        const float cur = av.Fn_03(code), max = av.Fn_08(code);
        s.limb[i] = (max > 0.0f) ? (cur / max * 100.0f) : 0.0f;
    }

    // Active effects (EFF) — the player's non-terminated active effects, from
    // Actor::magicTarget (@0x94) -> GetEffectList(). Each is tagged hidden when
    // it's an ability-type effect (traits like Four Eyes, crippled-limb effects,
    // perk passives) — the in-game Pip-Boy EFF page hides those; the app shows
    // them only when "Show hidden effects" is enabled.
    {
        std::string eff = "[";
        int n = 0;
        MagicTarget* mt = (MagicTarget*)((char*)p + 0x94);
        EffectNode* fx = mt->GetEffectList();
        if (fx) {
            std::unordered_set<std::string> seen;
            for (auto it = fx->Begin(); !it.End(); ++it) {
                ActiveEffect* ae = it.Get();
                if (!ae || ae->bTerminated) continue;
                MagicItem* mi = ae->magicItem;
                const char* nm = mi ? mi->name.m_data : nullptr;
                if (!nm || !*nm) continue;
                if (!seen.insert(nm).second) continue;     // dedupe by name
                const bool hidden = (ae->spellType == SpellItem::kType_Ability);
                eff += n ? ",{\"name\":\"" : "{\"name\":\"";
                eff += jsonEscape(nm);
                eff += "\",\"hidden\":";
                eff += hidden ? "true" : "false";
                eff += "}";
                if (++n >= 32) break;
            }
        }
        eff += "]";
        s.effects = eff;
    }

    // Position + worldspace (for map; logged so we can calibrate to real coords).
    s.posX = p->posX; s.posY = p->posY; s.posZ = p->posZ; s.angle = p->rotZ;
    TESObjectCELL* cell = p->parentCell;
    s.isInterior = cell->IsInterior();
    if (!s.isInterior && cell->worldSpace) {
        const char* eid = cell->worldSpace->GetName();
        if (eid) s.worldspace = eid;
        g_lastWorldSpace = cell->worldSpace; // remember for custom-marker placement
    }

    // Inventory + map markers + quests + radio: rebuild ~once per second (heavy).
    if ((g_frame++ % 60) == 0) {
        rebuildInventory(p); rebuildHotspots(p); rebuildQuests(p);
        // Station list scans every cell -> only ~every 5s; current station each tick.
        static int radioTick = 0;
        if ((radioTick++ % 5) == 0) rebuildRadioStations(p);
        rebuildRadio();
    }
    s.caps = g_caps; s.cats = g_cats; s.weapons = g_weapons;
    s.apparel = g_apparel; s.aid = g_aid; s.notes = g_notes; s.misc = g_misc;
    s.hotspots = g_hotspots; s.quests = g_quests; s.radio = g_radio;

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
    g_console = (NVSEConsoleInterface*)nvse->QueryInterface(kInterface_Console);
    static std::thread server([]() { ws::run_server(8765); });
    server.detach();
    return true;
}

} // extern "C"
