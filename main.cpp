// main.cpp — MKLIUKANG1 Player v1.1
// Три звуковых движка: FluidSynth, ADLMIDI, OPNMIDI
// + плейлист по папкам, PAK browser, Help, контекстное меню, VGM-экспорт OPN2
#include <SDL2/SDL.h>
#include <fluidsynth.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <zlib.h>

#ifdef _WIN32
#include <windows.h>
#include "resource.h"
#endif

#include "vgm_export.h"

namespace fs = std::filesystem;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int main(int argc, char* argv[]);
    return main(__argc, __argv);
}
#endif

// ==================================================================
//                         ЯЗЫК
// ==================================================================
enum LangId { LANG_EN = 0, LANG_RU = 1 };
LangId g_lang = LANG_EN;

struct UIStrings {
    std::string now_playing, track, sound_engine, output, paused;
    std::string hint_nav, hint_lr, hint_pause, hint_lang, hint_exit;
    std::string playlist;
    std::string pak_browser, pak_entries, pak_hint, pak_empty;
    std::string help_title, help_hotkeys, help_about, help_close, about_text;
    std::string ctx_play, ctx_pause, ctx_next, ctx_prev;
    std::string ctx_open, ctx_save, ctx_help, ctx_exit;
    std::string ctx_vgm_opn2, ctx_vgm_adl;
    std::string ctx_vgm_opl3, ctx_vgm_opl2, ctx_vgm_ym2151;
};

static const UIStrings STR_EN = {
    "NOW PLAYING","TRACK","SOUND ENGINE","OUTPUT","PAUSED",
    "UP/DN: TRACK","L/R: ENGINE","SPACE: PAUSE","L: LANG","ESC: EXIT",
    "PLAYLIST",
    "PAK BROWSER","entries","P / ESC: CLOSE   WHEEL: SCROLL   LMB on track: PLAY","(empty)",
    "HELP","HOTKEYS","ABOUT","Close (Esc / H)",
    "MKLIUKANG1 Player v1.1 — music cover player by MKLIUKANG1 for different sound engines.||Engines: FluidSynth (SF2 / General MIDI instruments), libADLMIDI (OPL3 FM synthesis), libOPNMIDI (OPN2 FM synthesis).||Features: playlist grouped by folders, 32-band real-time equalizer, PAK browser, VGM export (OPN2), English / Russian UI.||Created by MKLIUKANG1.",
    "Play","Pause","Next track","Previous track",
    "Open file...","Save settings","Help","Exit",
    "Export to VGM (OPN2)","Export to VGM (ADL)",
    "Export to VGM (OPL3 / ADL)",
    "Export to VGM (OPL2 / ADL)",
    "Export to VGM (YM2151 / Fluid)"
};
static const UIStrings STR_RU = {
    "СЕЙЧАС ИГРАЕТ","ТРЕК","ЗВУК. ДВИЖОК","ВЫВОД","ПАУЗА",
    "ВВЕРХ/ВНИЗ: ТРЕК","ВЛЕВО/ВПРАВО: ДВИЖОК","ПРОБЕЛ: ПАУЗА","L: ЯЗЫК","ESC: ВЫХОД",
    "СПИСОК ТРЕКОВ",
    "СОДЕРЖИМОЕ PAK","записей","P / ESC: ЗАКРЫТЬ   КОЛЕСО: ПРОКРУТКА   ЛКМ по треку: ИГРАТЬ","(пусто)",
    "СПРАВКА","ГОРЯЧИЕ КЛАВИШИ","О ПРОГРАММЕ","Закрыть (Esc / H)",
    "MKLIUKANG1 Player v1.1 — проигрыватель музыкальных каверов от MKLIUKANG1 под разные звуковые движки.||Движки: FluidSynth (SF2 / инструменты General MIDI), libADLMIDI (FM-синтез OPL3), libOPNMIDI (FM-синтез OPN2).||Возможности: плейлист с группировкой по папкам, 32-полосный эквалайзер, обозреватель PAK, экспорт VGM (OPN2), русский / английский интерфейс.||Создатель: MKLIUKANG1.",
    "Играть","Пауза","Следующий трек","Предыдущий трек",
    "Открыть файл...","Сохранить настройки","Справка","Выход",
    "Экспорт в VGM (OPN2)","Экспорт в VGM (ADL)",
    "Экспорт в VGM (OPL3 / ADL)",
    "Экспорт в VGM (OPL2 / ADL)",
    "Экспорт в VGM (YM2151 / Fluid)"
};
const UIStrings& L() { return (g_lang == LANG_RU) ? STR_RU : STR_EN; }

// ==================================================================
//              ДИНАМИЧЕСКАЯ ЗАГРУЗКА ADLMIDI / OPNMIDI
// ==================================================================
#ifdef _WIN32
typedef void* ADL_MIDIPlayer;
typedef ADL_MIDIPlayer (*adl_init_t)(int);
typedef void (*adl_close_t)(ADL_MIDIPlayer);
typedef int (*adl_openFile_t)(ADL_MIDIPlayer, const char*);
typedef int (*adl_play_t)(ADL_MIDIPlayer, int, short*);
typedef void (*adl_stop_t)(ADL_MIDIPlayer);
typedef void (*adl_pause_t)(ADL_MIDIPlayer, int);
typedef int (*adl_openBankFile_t)(ADL_MIDIPlayer, const char*);
typedef void (*adl_setReverb_t)(ADL_MIDIPlayer, int);
typedef const char* (*adl_errorString_t)(void);

typedef void* OPN2_MIDIPlayer;
typedef OPN2_MIDIPlayer (*opn_init_t)(int);
typedef void (*opn_close_t)(OPN2_MIDIPlayer);
typedef int (*opn_openFile_t)(OPN2_MIDIPlayer, const char*);
typedef int (*opn_play_t)(OPN2_MIDIPlayer, int, short*);
typedef void (*opn_stop_t)(OPN2_MIDIPlayer);
typedef void (*opn_pause_t)(OPN2_MIDIPlayer, int);
typedef int (*opn_openBankFile_t)(OPN2_MIDIPlayer, const char*);
typedef const char* (*opn_errorString_t)(void);

static adl_init_t          p_adl_init          = nullptr;
static adl_close_t         p_adl_close         = nullptr;
static adl_openFile_t      p_adl_openFile      = nullptr;
static adl_play_t          p_adl_play          = nullptr;
static adl_stop_t          p_adl_stop          = nullptr;
static adl_pause_t         p_adl_pause         = nullptr;
static adl_openBankFile_t  p_adl_openBankFile  = nullptr;
static adl_setReverb_t     p_adl_setReverb     = nullptr;
static adl_errorString_t   p_adl_errorString   = nullptr;

static opn_init_t          p_opn_init          = nullptr;
static opn_close_t         p_opn_close         = nullptr;
static opn_openFile_t      p_opn_openFile      = nullptr;
static opn_play_t          p_opn_play          = nullptr;
static opn_stop_t          p_opn_stop          = nullptr;
static opn_pause_t         p_opn_pause         = nullptr;
static opn_openBankFile_t  p_opn_openBankFile  = nullptr;
static opn_errorString_t   p_opn_errorString   = nullptr;

static HMODULE g_adl_dll = nullptr;
static HMODULE g_opn_dll = nullptr;
static bool    g_adl_available = false;
static bool    g_opn_available = false;

static void try_load_adlmidi() {
    g_adl_dll = LoadLibraryA("libADLMIDI.dll");
    if (!g_adl_dll) { std::cerr << "ADLMIDI not available\n"; return; }
    p_adl_init         = (adl_init_t)         GetProcAddress(g_adl_dll, "adl_init");
    p_adl_close        = (adl_close_t)        GetProcAddress(g_adl_dll, "adl_close");
    p_adl_openFile     = (adl_openFile_t)     GetProcAddress(g_adl_dll, "adl_openFile");
    p_adl_play         = (adl_play_t)         GetProcAddress(g_adl_dll, "adl_play");
    p_adl_stop         = (adl_stop_t)         GetProcAddress(g_adl_dll, "adl_stop");
    p_adl_pause        = (adl_pause_t)        GetProcAddress(g_adl_dll, "adl_pause");
    p_adl_openBankFile = (adl_openBankFile_t) GetProcAddress(g_adl_dll, "adl_openBankFile");
    p_adl_setReverb    = (adl_setReverb_t)    GetProcAddress(g_adl_dll, "adl_setReverb");
    p_adl_errorString  = (adl_errorString_t)  GetProcAddress(g_adl_dll, "adl_errorString");
    g_adl_available = p_adl_init && p_adl_play && p_adl_close;
    if (g_adl_available) std::cout << "ADLMIDI loaded OK\n";
}

static void try_load_opnmidi() {
    g_opn_dll = LoadLibraryA("libOPNMIDI.dll");
    if (!g_opn_dll) { std::cerr << "OPNMIDI not available\n"; return; }
    p_opn_init          = (opn_init_t)          GetProcAddress(g_opn_dll, "opn2_init");
    p_opn_close         = (opn_close_t)         GetProcAddress(g_opn_dll, "opn2_close");
    p_opn_openFile      = (opn_openFile_t)      GetProcAddress(g_opn_dll, "opn2_openFile");
    p_opn_play          = (opn_play_t)          GetProcAddress(g_opn_dll, "opn2_play");
    p_opn_stop          = (opn_stop_t)          GetProcAddress(g_opn_dll, "opn2_stop");
    p_opn_pause         = (opn_pause_t)         GetProcAddress(g_opn_dll, "opn2_pause");
    p_opn_openBankFile  = (opn_openBankFile_t)  GetProcAddress(g_opn_dll, "opn2_openBankFile");
    p_opn_errorString   = (opn_errorString_t)   GetProcAddress(g_opn_dll, "opn2_errorString");
    g_opn_available = p_opn_init && p_opn_play && p_opn_close;
    if (g_opn_available) std::cout << "OPNMIDI loaded OK\n";
}

static void unload_midi_dlls() {
    if (g_adl_dll) { FreeLibrary(g_adl_dll); g_adl_dll = nullptr; }
    if (g_opn_dll) { FreeLibrary(g_opn_dll); g_opn_dll = nullptr; }
}
#else
static void try_load_adlmidi() {}
static void try_load_opnmidi() {}
static void unload_midi_dlls() {}
#endif

// ==================================================================
//                       АУДИО ДВИЖКИ
// ==================================================================
enum EngineType { ENGINE_FLUIDSYNTH, ENGINE_ADLMIDI, ENGINE_OPNMIDI };
EngineType current_engine = ENGINE_FLUIDSYNTH;

fluid_settings_t* synth_settings = nullptr;
fluid_synth_t*    synth          = nullptr;
fluid_player_t*   player         = nullptr;

ADL_MIDIPlayer  adl_player = nullptr;
OPN2_MIDIPlayer opn_player = nullptr;

SDL_AudioDeviceID g_audio_dev = 0;

int  current_track_idx = 0;
int  current_sf_idx    = 0;
int  active_sf_id      = -1;
bool is_paused         = false;

int  g_adl_current_bank = 0;
int  g_opn_current_bank = 0;

std::string g_temp_dir;

// ---------- Плейлист ----------
bool g_show_playlist   = true;
int  g_playlist_width  = 300;
int  g_playlist_scroll = 0;
int  g_playlist_row_h  = 22;
int  g_playlist_hover  = -1;

// ---------- PAK browser ----------
bool g_show_pak_browser = false;
int  g_pak_scroll       = 0;
int  g_pak_row_h        = 18;

// ---------- Help window ----------
bool g_help_active = false;

// ---------- Context menu ----------
bool g_ctx_active    = false;
int  g_ctx_x         = 0;
int  g_ctx_y         = 0;
int  g_ctx_selected  = -1;

// ---------- Векторы контента (объявлены раньше — это критично) ----------
std::vector<std::string> midi_tracks;
std::vector<std::string> soundfonts;
std::vector<std::string> adl_banks;
std::vector<std::string> opn_banks;
std::vector<std::string> g_all_pak_names;

// ---------- Группировка по папкам ----------
struct FolderEntry {
    std::string name;
    int  start    = 0;
    int  count    = 0;
    bool expanded = true;
};

struct DisplayRow {
    enum Type { FOLDER, TRACK };
    Type type;
    int  folder_idx = 0;
    int  track_idx  = -1;
};

std::vector<FolderEntry> g_folders;
std::vector<DisplayRow>  g_display_rows;

std::string folder_of_path(const std::string& path) {
    if (path.rfind("@EXT@", 0) == 0) return "External";
    size_t first = path.find('/');
    if (first == std::string::npos) return "(root)";
    size_t last = path.find_last_of('/');
    if (last <= first + 1) return "(root)";
    return path.substr(first + 1, last - first - 1);
}

void build_folder_index() {
    g_folders.clear();
    if (midi_tracks.empty()) return;

    std::map<std::string, std::vector<std::string>> by_folder;
    for (auto& t : midi_tracks) by_folder[folder_of_path(t)].push_back(t);

    std::vector<std::string> reordered;
    for (auto& kv : by_folder) {
        auto& folder = kv.first;
        auto& tracks = kv.second;
        std::sort(tracks.begin(), tracks.end());
        FolderEntry fe;
        fe.name  = folder;
        fe.start = (int)reordered.size();
        fe.count = (int)tracks.size();
        fe.expanded = true;
        g_folders.push_back(fe);
        for (auto& t : tracks) reordered.push_back(t);
    }
    midi_tracks = std::move(reordered);
}

void build_display_rows() {
    g_display_rows.clear();
    for (int fi = 0; fi < (int)g_folders.size(); ++fi) {
        DisplayRow r;
        r.type = DisplayRow::FOLDER;
        r.folder_idx = fi;
        g_display_rows.push_back(r);
        if (g_folders[fi].expanded) {
            for (int k = 0; k < g_folders[fi].count; ++k) {
                DisplayRow t;
                t.type = DisplayRow::TRACK;
                t.folder_idx = fi;
                t.track_idx = g_folders[fi].start + k;
                g_display_rows.push_back(t);
            }
        }
    }
}

// ==================================================================
//                       УПАКОВКА РЕСУРСОВ
// ==================================================================
struct PakEntry { std::string name; std::vector<char> data; };

class ResourcePack {
public:
    bool load_from_memory(const char* buf, size_t size) {
        if (size < 12 || std::memcmp(buf, "MPAK", 4) != 0) return false;
        uint32_t version = 0, count = 0;
        std::memcpy(&version, buf + 4, 4);
        std::memcpy(&count,   buf + 8, 4);
        if (version != 2) return false;
        size_t off = 12;
        entries_.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            if (off + 4 > size) return false;
            uint32_t name_len = 0, orig_size = 0, data_size = 0;
            std::memcpy(&name_len, buf + off, 4); off += 4;
            if (off + name_len + 9 > size) return false;
            std::string name(buf + off, name_len); off += name_len;
            std::memcpy(&orig_size, buf + off, 4); off += 4;
            std::memcpy(&data_size, buf + off, 4); off += 4;
            uint8_t flag = (uint8_t)buf[off]; off += 1;
            if (off + data_size > size) return false;
            PakEntry e;
            e.name = std::move(name);
            if (flag == 0) {
                e.data.assign(buf + off, buf + off + data_size);
            } else {
                e.data.resize(orig_size);
                uLongf dst_len = orig_size;
                int rc = uncompress((Bytef*)e.data.data(), &dst_len,
                                    (const Bytef*)(buf + off), data_size);
                if (rc != Z_OK || dst_len != orig_size) return false;
            }
            off += data_size;
            entries_.push_back(std::move(e));
        }
        return true;
    }
    std::vector<std::string> list_by_prefix(const std::string& prefix) const {
        std::vector<std::string> out;
        for (auto& e : entries_)
            if (e.name.rfind(prefix, 0) == 0) out.push_back(e.name);
        return out;
    }
    const std::vector<PakEntry>& entries() const { return entries_; }
    void release_data() {
        for (auto& e : entries_) std::vector<char>().swap(e.data);
        std::vector<PakEntry>().swap(entries_);
    }
private:
    std::vector<PakEntry> entries_;
};

ResourcePack g_pack;

static std::vector<char> load_embedded_pak() {
#ifdef _WIN32
    HMODULE hmod = GetModuleHandle(nullptr);
    HRSRC hrsrc = FindResource(hmod, MAKEINTRESOURCE(IDR_PAK), RT_RCDATA);
    if (!hrsrc) return {};
    DWORD sz = SizeofResource(hmod, hrsrc);
    HGLOBAL hglob = LoadResource(hmod, hrsrc);
    void* p = LockResource(hglob);
    return std::vector<char>((char*)p, (char*)p + sz);
#else
    return {};
#endif
}

static std::string unpack_pak_to_temp() {
#ifdef _WIN32
    char tmp[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmp);
    std::string dir = std::string(tmp) + "MidiPlayer";
#else
    std::string dir = "/tmp/MidiPlayer";
#endif
    std::error_code ec;
    fs::create_directories(dir, ec);
    for (auto& e : g_pack.entries()) {
        fs::path out_path = fs::path(dir) / e.name;
        fs::create_directories(out_path.parent_path(), ec);
        std::ofstream f(out_path, std::ios::binary);
        if (f) f.write(e.data.data(), (std::streamsize)e.data.size());
    }
    return dir;
}

// ==================================================================
//                     ОТКРЫТИЕ MIDI ИЗ ФАЙЛА + НАСТРОЙКИ
// ==================================================================
#ifdef _WIN32
static std::string openMidiDialogW() {
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"Music files\0*.mid;*.midi;*.rmi;*.kar;*.hmi;*.hmp;*.xmi\0All Files\0*.*\0\0";
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = L"Open music file";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, filename, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string r(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, filename, -1, &r[0], len, nullptr, nullptr);
    return r;
}
#else
static std::string openMidiDialogW() { return ""; }
#endif

static void saveSettings() {
    std::ofstream f("settings.ini");
    if (!f) return;
    f << "[player]\n";
    f << "engine="    << (int)current_engine    << "\n";
    f << "sf_idx="    << current_sf_idx         << "\n";
    f << "adl_bank="  << g_adl_current_bank     << "\n";
    f << "opn_bank="  << g_opn_current_bank     << "\n";
    f << "lang="      << (int)g_lang            << "\n";
    std::cout << "Settings saved to settings.ini\n";
}

static void loadSettings() {
    std::ifstream f("settings.ini");
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '[' || line[0] == '#' || line[0] == ';') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if      (k == "engine")   { int e = atoi(v.c_str()); if (e >= 0 && e <= 2) current_engine = (EngineType)e; }
        else if (k == "sf_idx")   current_sf_idx = std::max(0, atoi(v.c_str()));
        else if (k == "adl_bank") g_adl_current_bank = std::max(0, atoi(v.c_str()));
        else if (k == "opn_bank") g_opn_current_bank = std::max(0, atoi(v.c_str()));
        else if (k == "lang")     g_lang = (LangId)std::min(1, std::max(0, atoi(v.c_str())));
    }
    std::cout << "Settings loaded from settings.ini\n";
}

// ==================================================================
//                     ШРИФТ
// ==================================================================
const unsigned char font_bits[][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00},
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},{0x36,0x36,0x7f,0x36,0x7f,0x36,0x36,0x00},
    {0x12,0x3f,0x43,0x3e,0x07,0x7c,0x24,0x00},{0x00,0x63,0x63,0x18,0x0c,0x46,0x63,0x00},
    {0x1c,0x36,0x1c,0x3a,0x6e,0x33,0x6f,0x00},{0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00},{0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00},
    {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00},{0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x00,0x03,0x06,0x0c,0x18,0x30,0x60,0x00},
    {0x3e,0x63,0x67,0x6f,0x7b,0x63,0x3e,0x00},{0x0c,0x1c,0x0c,0x0c,0x0c,0x0c,0x3e,0x00},
    {0x3e,0x63,0x06,0x1c,0x30,0x63,0x7f,0x00},{0x3e,0x63,0x06,0x1e,0x06,0x63,0x3e,0x00},
    {0x06,0x0e,0x1e,0x36,0x7f,0x06,0x06,0x00},{0x7f,0x60,0x7e,0x03,0x03,0x63,0x3e,0x00},
    {0x1e,0x30,0x60,0x7e,0x63,0x63,0x3e,0x00},{0x7f,0x63,0x03,0x06,0x0c,0x18,0x18,0x00},
    {0x3e,0x63,0x63,0x3e,0x63,0x63,0x3e,0x00},{0x3e,0x63,0x63,0x7f,0x03,0x06,0x3c,0x00},
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},{0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    {0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00},{0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00},
    {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00},{0x3e,0x63,0x06,0x0c,0x18,0x00,0x18,0x00},
    {0x3e,0x63,0x6f,0x6b,0x6b,0x60,0x3e,0x00},{0x18,0x3c,0x66,0x66,0x7f,0x66,0x66,0x00},
    {0x7e,0x33,0x33,0x3e,0x33,0x33,0x7e,0x00},{0x1e,0x33,0x60,0x60,0x60,0x33,0x1e,0x00},
    {0x7c,0x36,0x33,0x33,0x33,0x36,0x7c,0x00},{0x7f,0x30,0x30,0x3c,0x30,0x30,0x7f,0x00},
    {0x7f,0x30,0x30,0x3c,0x30,0x30,0x30,0x00},{0x1e,0x33,0x60,0x6f,0x63,0x33,0x1f,0x00},
    {0x66,0x66,0x66,0x7f,0x66,0x66,0x66,0x00},{0x3e,0x0c,0x0c,0x0c,0x0c,0x0c,0x3e,0x00},
    {0x03,0x03,0x03,0x03,0x03,0x43,0x3e,0x00},{0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00},
    {0x30,0x30,0x30,0x30,0x30,0x30,0x7f,0x00},{0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00},
    {0x63,0x67,0x6f,0x7b,0x73,0x63,0x63,0x00},{0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00},
    {0x7e,0x63,0x63,0x7e,0x30,0x30,0x30,0x00},{0x3e,0x63,0x63,0x63,0x6b,0x67,0x3e,0x03},
    {0x7e,0x63,0x63,0x7e,0x78,0x6c,0x66,0x00},{0x3e,0x63,0x38,0x0e,0x07,0x63,0x3e,0x00},
    {0x7f,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00},
    {0x63,0x63,0x63,0x63,0x63,0x34,0x1c,0x00},{0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00},
    {0x63,0x63,0x34,0x1c,0x34,0x63,0x63,0x00},{0x63,0x63,0x34,0x1c,0x0c,0x0c,0x0c,0x00},
    {0x7f,0x03,0x06,0x1c,0x30,0x60,0x7f,0x00},{0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00},
    {0x00,0x60,0x30,0x1c,0x06,0x03,0x01,0x00},{0x3c,0x0d,0x0c,0x0c,0x0c,0x0c,0x3c,0x00},
    {0x10,0x38,0x6c,0xc6,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff},
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00}
};

const unsigned char cyrillic_bits[][8] = {
    {0x18,0x3c,0x66,0x66,0x7f,0x66,0x66,0x00},{0x7e,0x60,0x60,0x7e,0x63,0x63,0x7e,0x00},
    {0x7e,0x33,0x33,0x3e,0x33,0x33,0x7e,0x00},{0x7e,0x60,0x60,0x60,0x60,0x60,0x60,0x00},
    {0x1e,0x36,0x36,0x36,0x36,0x7f,0x41,0x41},{0x7f,0x30,0x30,0x3c,0x30,0x30,0x7f,0x00},
    {0x66,0x66,0x66,0xff,0xff,0x66,0x66,0x00},{0x3e,0x63,0x03,0x1e,0x03,0x63,0x3e,0x00},
    {0x63,0x73,0x7b,0x7f,0x6f,0x67,0x63,0x00},{0x18,0x00,0x73,0x7b,0x7f,0x6f,0x67,0x63},
    {0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00},{0x0f,0x1f,0x1b,0x33,0x33,0x33,0x63,0x00},
    {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00},{0x66,0x66,0x66,0x7f,0x66,0x66,0x66,0x00},
    {0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00},{0x7f,0x63,0x63,0x63,0x63,0x63,0x63,0x00},
    {0x7e,0x63,0x63,0x7e,0x60,0x60,0x60,0x00},{0x1e,0x33,0x60,0x60,0x60,0x33,0x1e,0x00},
    {0x7f,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0x63,0x63,0x33,0x1e,0x0c,0x18,0x70,0x00},
    {0x0c,0x3f,0x6d,0x6d,0x6d,0x3f,0x0c,0x00},{0x63,0x63,0x34,0x1c,0x34,0x63,0x63,0x00},
    {0x66,0x66,0x66,0x66,0x66,0x7e,0x06,0x06},{0x66,0x66,0x66,0x3e,0x06,0x06,0x06,0x00},
    {0x6d,0x6d,0x6d,0x6d,0x6d,0x6d,0x7f,0x00},{0x6d,0x6d,0x6d,0x6d,0x6d,0x7f,0x03,0x03},
    {0x60,0x60,0x60,0x7e,0x63,0x63,0x7e,0x00},{0xc6,0xc6,0xc6,0xf6,0xde,0xde,0xf6,0x00},
    {0x60,0x60,0x60,0x7e,0x63,0x63,0x7e,0x00},{0x3e,0x63,0x03,0x1f,0x03,0x63,0x3e,0x00},
    {0x6e,0x7b,0x7b,0x7b,0x7b,0x7b,0x6e,0x00},{0x3f,0x63,0x63,0x3f,0x1b,0x33,0x63,0x00},
    {0x24,0x00,0x7f,0x30,0x3c,0x30,0x7f,0x00}
};

static uint32_t next_utf8(const std::string& s, size_t& i) {
    if (i >= s.size()) return 0;
    unsigned char c = (unsigned char)s[i];
    uint32_t cp; int extra;
    if ((c & 0x80) == 0)         { cp = c;         extra = 0; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F;  extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F;  extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07;  extra = 3; }
    else                         { i++; return '?'; }
    i++;
    for (int j = 0; j < extra && i < s.size(); ++j, ++i)
        cp = (cp << 6) | ((unsigned char)s[i] & 0x3F);
    return cp;
}

static const unsigned char* glyph_for(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') cp -= 32;
    if (cp >= 0x0430 && cp <= 0x044F) cp -= 0x20;
    if (cp == 0x0451) cp = 0x0401;
    if (cp >= 32 && cp <= 96)          return font_bits[cp - 32];
    if (cp == 0x0401)                  return cyrillic_bits[32];
    if (cp >= 0x0410 && cp <= 0x042F)  return cyrillic_bits[cp - 0x0410];
    if (cp == ' ')                     return nullptr;
    return font_bits[0];
}

static std::map<uint64_t, SDL_Texture*> g_glyph_cache;
static Uint8 g_text_r = 255, g_text_g = 255, g_text_b = 255, g_text_a = 255;

static void set_text_color(Uint8 R, Uint8 G, Uint8 B, Uint8 A = 255) {
    g_text_r = R; g_text_g = G; g_text_b = B; g_text_a = A;
}

static SDL_Texture* get_glyph_texture(SDL_Renderer* r, uint32_t cp, int scale) {
    uint64_t key = ((uint64_t)cp << 8) | (uint64_t)scale;
    auto it = g_glyph_cache.find(key);
    if (it != g_glyph_cache.end()) return it->second;

    const unsigned char* bits = glyph_for(cp);
    if (!bits) { g_glyph_cache[key] = nullptr; return nullptr; }

    int size = 8 * scale;
    SDL_Texture* tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_STATIC, size, size);
    if (!tex) { g_glyph_cache[key] = nullptr; return nullptr; }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    std::vector<uint32_t> pixels(size * size, 0x00000000);
    for (int row = 0; row < 8; ++row) {
        unsigned char b = bits[row];
        for (int col = 0; col < 8; ++col) {
            if (b & (1 << (7 - col))) {
                for (int dy = 0; dy < scale; ++dy)
                    for (int dx = 0; dx < scale; ++dx) {
                        int px = col * scale + dx;
                        int py = row * scale + dy;
                        pixels[py * size + px] = 0xFFFFFFFF;
                    }
            }
        }
    }
    SDL_UpdateTexture(tex, nullptr, pixels.data(), size * 4);
    g_glyph_cache[key] = tex;
    return tex;
}

static void draw_glyph(SDL_Renderer* r, uint32_t cp, int x, int y, int scale) {
    SDL_Texture* tex = get_glyph_texture(r, cp, scale);
    if (!tex) return;
    SDL_SetTextureColorMod(tex, g_text_r, g_text_g, g_text_b);
    SDL_SetTextureAlphaMod(tex, g_text_a);
    SDL_Rect dst = { x, y, 8 * scale, 8 * scale };
    SDL_RenderCopy(r, tex, nullptr, &dst);
}

static int utf8_length(const std::string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = (unsigned char)s[i];
        if ((c & 0xC0) != 0x80) n++;
        if      ((c & 0x80) == 0)    i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else                         i += 1;
    }
    return n;
}

static void draw_string(SDL_Renderer* r, const std::string& s,
                        int x, int y, int scale = 1) {
    int cur_x = x;
    size_t i = 0;
    while (i < s.size()) {
        uint32_t cp = next_utf8(s, i);
        if (cp == 0) break;
        draw_glyph(r, cp, cur_x, y, scale);
        cur_x += 8 * scale + 2;
    }
}

static int text_width(const std::string& s, int scale = 1) {
    int n = utf8_length(s);
    if (n <= 0) return 0;
    return n * (8 * scale + 2) - 2;
}

static std::string utf8_substr(const std::string& s, int start_sym, int n_sym) {
    std::string out;
    int cur = 0, collected = 0;
    size_t i = 0;
    while (i < s.size() && collected < n_sym) {
        size_t start = i;
        next_utf8(s, i);
        if (cur >= start_sym) { out.append(s, start, i - start); collected++; }
        cur++;
    }
    return out;
}

static std::string utf8_truncate(const std::string& s, int n_sym) {
    if (utf8_length(s) <= n_sym) return s;
    return utf8_substr(s, 0, n_sym);
}

// ==================================================================
//                       ЭКВАЛАЙЗЕР
// ==================================================================
static const int SAMPLE_RATE      = 44100;
static const int ANALYSIS_SAMPLES = 1024;
static const int NUM_BANDS        = 32;

static int16_t  g_mono_ring[ANALYSIS_SAMPLES];
static volatile int g_mono_pos = 0;

struct EqBar { float value = 0.0f; float smoothed = 0.0f; };
static std::vector<EqBar> g_eq_bars;
static float g_band_coeffs[NUM_BANDS];

static void init_equalizer() {
    g_eq_bars.assign(NUM_BANDS, EqBar{});
    float fmin = 60.0f, fmax = 12000.0f;
    for (int i = 0; i < NUM_BANDS; ++i) {
        float t = (float)i / (float)(NUM_BANDS - 1);
        float f = fmin * powf(fmax / fmin, t);
        g_band_coeffs[i] = 2.0f * cosf(2.0f * (float)M_PI * f / SAMPLE_RATE);
    }
    for (auto& b : g_mono_ring) b = 0;
}

// ==================================================================
//                       АУДИО-КОЛБЭК
// ==================================================================
static void audio_callback(void*, Uint8* stream, int len) {
    int frames = len / 4;
    int16_t* out = (int16_t*)stream;

    switch (current_engine) {
        case ENGINE_FLUIDSYNTH:
            if (synth) fluid_synth_write_s16(synth, frames, out, 0, 2, out, 1, 2);
            else       std::memset(stream, 0, len);
            break;
        case ENGINE_ADLMIDI:
            if (p_adl_play && adl_player) p_adl_play(adl_player, frames * 2, (short*)stream);
            else                          std::memset(stream, 0, len);
            break;
        case ENGINE_OPNMIDI:
            if (p_opn_play && opn_player) p_opn_play(opn_player, frames * 2, (short*)stream);
            else                          std::memset(stream, 0, len);
            break;
    }

    int p = g_mono_pos;
    for (int i = 0; i < frames; ++i) {
        int16_t l = out[i*2], r = out[i*2 + 1];
        g_mono_ring[p] = (int16_t)(((int32_t)l + r) >> 1);
        p = (p + 1) % ANALYSIS_SAMPLES;
    }
    g_mono_pos = p;
}

static float goertzel_power(const int16_t* samples, int n, float coeff) {
    float s1 = 0.0f, s2 = 0.0f;
    for (int i = 0; i < n; ++i) {
        float x = (float)samples[i] / 32768.0f;
        float s0 = x + coeff * s1 - s2;
        s2 = s1; s1 = s0;
    }
    return s1 * s1 + s2 * s2 - coeff * s1 * s2;
}

void update_equalizer(bool playing) {
    if (!g_audio_dev) return;
    static int16_t snap[ANALYSIS_SAMPLES];
    std::memcpy(snap, g_mono_ring, sizeof(g_mono_ring));
    int pos = g_mono_pos;

    static int16_t linear[ANALYSIS_SAMPLES];
    int head = ANALYSIS_SAMPLES - pos;
    if (head > 0) std::memcpy(linear, snap + pos, head * sizeof(int16_t));
    if (pos  > 0) std::memcpy(linear + head, snap, pos * sizeof(int16_t));

    for (int b = 0; b < NUM_BANDS; ++b) {
        float power = goertzel_power(linear, ANALYSIS_SAMPLES, g_band_coeffs[b]);
        float rms = sqrtf(power) / (float)ANALYSIS_SAMPLES;
        float db  = 20.0f * log10f(rms + 1e-7f);
        float val = (db + 70.0f) / 60.0f;
        val = std::clamp(val, 0.0f, 1.0f);
        float t = (float)b / (NUM_BANDS - 1);
        val *= (1.15f - 0.30f * t);
        auto& bar = g_eq_bars[b];
        bar.value = val;
        if (val > bar.smoothed) bar.smoothed = val;
        else                    bar.smoothed += (val - bar.smoothed) * 0.20f;
        if (!playing) bar.smoothed *= 0.85f;
        bar.smoothed = std::clamp(bar.smoothed, 0.0f, 1.0f);
    }
}

void draw_equalizer(SDL_Renderer* r, int x, int y, int w, int h) {
    int n = (int)g_eq_bars.size();
    if (n <= 0 || w < 4) return;
    int gap = 2;
    int bar_w = (w - (n - 1) * gap) / n;
    if (bar_w < 1) return;
    for (int i = 0; i < n; ++i) {
        int bx = x + i * (bar_w + gap);
        int bh = (int)(g_eq_bars[i].smoothed * h);
        if (bh < 1) bh = 1;
        int by = y + h - bh;
        float ratio = (float)bh / h;
        Uint8 R, G, B;
        if (ratio < 0.55f) { R = (Uint8)(ratio/0.55f*255); G = 230; B = 70; }
        else               { R = 255; G = (Uint8)((1.0f-(ratio-0.55f)/0.45f)*230); B = 60; }
        SDL_SetRenderDrawColor(r, R, G, B, 255);
        SDL_Rect bar = { bx, by, bar_w, bh };
        SDL_RenderFillRect(r, &bar);
    }
}

// ==================================================================
//                       АНИМАЦИЯ
// ==================================================================
struct Particle { float x, y, vx, vy; Uint8 R, G, B; int size; };
static std::vector<Particle> g_particles;
static int g_last_w = 0, g_last_h = 0;

static void init_particles(int w, int h) {
    g_particles.clear();
    g_particles.reserve(20);
    for (int i = 0; i < 20; ++i) {
        Particle p;
        p.x = (float)(rand() % (w > 0 ? w : 800));
        p.y = (float)(rand() % (h > 0 ? h : 600));
        p.vx = ((rand() % 100) - 50) / 400.0f;
        p.vy = -((rand() % 20) + 8) / 200.0f;
        p.R = 60 + rand() % 80;
        p.G = 100 + rand() % 100;
        p.B = 180 + rand() % 75;
        p.size = 1 + rand() % 2;
        g_particles.push_back(p);
    }
    g_last_w = w; g_last_h = h;
}

static void update_particles(int w, int h) {
    if (w != g_last_w || h != g_last_h) init_particles(w, h);
    for (auto& p : g_particles) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.y < -10 || p.x < -20 || p.x > w + 20) {
            p.x = (float)(rand() % (w > 0 ? w : 800));
            p.y = (float)h + 5;
            p.vy = -((rand() % 20) + 8) / 200.0f;
            p.vx = ((rand() % 100) - 50) / 400.0f;
        }
    }
}

static SDL_Texture* g_bg_tex = nullptr;
static int g_bg_w = 0, g_bg_h = 0;

static void ensure_bg(SDL_Renderer* r, int w, int h) {
    if (g_bg_tex && g_bg_w == w && g_bg_h == h) return;
    if (g_bg_tex) { SDL_DestroyTexture(g_bg_tex); g_bg_tex = nullptr; }
    g_bg_tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STATIC, w, h);
    if (!g_bg_tex) return;
    std::vector<uint32_t> px(w * h);
    for (int y = 0; y < h; ++y) {
        float k = 1.0f - (float)y / h * 0.55f;
        Uint8 R = (Uint8)(14 * k + 4);
        Uint8 G = (Uint8)(18 * k + 6);
        Uint8 B = (Uint8)(32 * k + 12);
        uint32_t c = (R << 24) | (G << 16) | (B << 8) | 0xFF;
        for (int x = 0; x < w; ++x) px[y * w + x] = c;
    }
    SDL_UpdateTexture(g_bg_tex, nullptr, px.data(), w * 4);
    g_bg_w = w; g_bg_h = h;
}

static void draw_top_bar(SDL_Renderer* r, int x0, int w, uint32_t t) {
    int seg = w / 4;
    for (int s = 0; s < 4; ++s) {
        int R = 40 + 60 * s / 3;
        int G = 90 + 70 * (3 - s) / 3;
        SDL_SetRenderDrawColor(r, (Uint8)R, (Uint8)G, 220, 255);
        SDL_Rect rc = { x0 + s * seg, 0, seg + 1, 3 };
        SDL_RenderFillRect(r, &rc);
    }
    int beam = (int)((t / 8) % (w + 300)) - 150;
    for (int i = 0; i < 4; ++i) {
        float b = 1.0f - (float)i / 4.0f;
        int bx = x0 + beam + i * 20;
        SDL_SetRenderDrawColor(r,
            (Uint8)std::min(255, 220 + (int)(b * 35)),
            (Uint8)std::min(255, 180 + (int)(b * 75)),
            (Uint8)std::min(255, 230 + (int)(b * 25)), 255);
        SDL_Rect rc = { bx, 0, 20, 3 };
        SDL_RenderFillRect(r, &rc);
    }
}

static void draw_corner_marks(SDL_Renderer* r, int x, int y, int w, int h, uint32_t t) {
    int cx[4] = { x + 14, x + w - 14, x + 14, x + w - 14 };
    int cy[4] = { y + 14, y + 14, y + h - 14, y + h - 14 };
    SDL_SetRenderDrawColor(r, 100, 160, 240, 255);
    for (int k = 0; k < 4; ++k) {
        float angle = t * 0.002f + k * (float)M_PI / 2.0f;
        int prev_x = 0, prev_y = 0;
        for (int i = 0; i <= 4; ++i) {
            float a = angle + i * (float)M_PI * 2.0f / 4.0f;
            int px = cx[k] + (int)(cosf(a) * 5.0f);
            int py = cy[k] + (int)(sinf(a) * 5.0f);
            if (i > 0) SDL_RenderDrawLine(r, prev_x, prev_y, px, py);
            prev_x = px; prev_y = py;
        }
    }
}

static void draw_mini_eq(SDL_Renderer* r, int x, int y, int w, int h, uint32_t t, bool active) {
    int nb = 3;
    int gap = 1;
    int bw = (w - (nb - 1) * gap) / nb;
    if (bw < 1) bw = 1;
    for (int i = 0; i < nb; ++i) {
        int bh;
        if (active) {
            int idx = 4 + i * (NUM_BANDS / 4);
            bh = (int)(g_eq_bars[idx].smoothed * h);
            if (bh < 2) bh = 2;
        } else {
            bh = 2;
        }
        int bx = x + i * (bw + gap);
        int by = y + h - bh;
        Uint8 R = (Uint8)(80 + i * 30);
        Uint8 G = 220;
        Uint8 B = (Uint8)(120 - i * 20);
        SDL_SetRenderDrawColor(r, R, G, B, 255);
        SDL_Rect rc = { bx, by, bw, bh };
        SDL_RenderFillRect(r, &rc);
    }
}

static void draw_glow_border(SDL_Renderer* r, int x, int y, int w, int h, uint32_t t,
                             Uint8 baseR, Uint8 baseG, Uint8 baseB) {
    float pulse = (sinf(t * 0.0025f) + 1.0f) * 0.5f;
    SDL_SetRenderDrawColor(r,
        (Uint8)std::min(255, (int)(baseR * (0.4f + pulse * 0.3f))),
        (Uint8)std::min(255, (int)(baseG * (0.4f + pulse * 0.3f))),
        (Uint8)std::min(255, (int)(baseB * (0.4f + pulse * 0.3f))), 255);
    SDL_Rect outer = { x - 1, y - 1, w + 2, h + 2 };
    SDL_RenderDrawRect(r, &outer);

    SDL_SetRenderDrawColor(r,
        (Uint8)std::min(255, (int)(baseR * (0.7f + pulse * 0.3f))),
        (Uint8)std::min(255, (int)(baseG * (0.7f + pulse * 0.3f))),
        (Uint8)std::min(255, (int)(baseB * (0.7f + pulse * 0.3f))), 255);
    SDL_Rect inner = { x, y, w, h };
    SDL_RenderDrawRect(r, &inner);
}

static void draw_header_beam(SDL_Renderer* r, int x, int y, int w, int h, uint32_t t) {
    int beam_x = (int)((t / 3) % (w + 120)) - 60;
    for (int i = 0; i < 60; ++i) {
        float b = 1.0f - std::abs(i - 30) / 30.0f;
        int bx = beam_x + i;
        if (bx < x || bx >= x + w) continue;
        int R = (Uint8)std::min(255, 22 + (int)(b * 70));
        int G = (Uint8)std::min(255, 26 + (int)(b * 80));
        int B = (Uint8)std::min(255, 38 + (int)(b * 110));
        SDL_SetRenderDrawColor(r, R, G, B, 255);
        SDL_Rect rc = { bx, y, 1, h };
        SDL_RenderFillRect(r, &rc);
    }
}

// ==================================================================
//                       ВСПОМОГАТЕЛЬНЫЕ
// ==================================================================
std::string get_clean_name(const std::string& path) {
    if (path.rfind("@EXT@", 0) == 0)
        return path.substr(5);
    std::string name = path.substr(path.find_last_of("/\\") + 1);
    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

void shutdown_engines() {
    if (player)         { delete_fluid_player(player); player = nullptr; }
    if (synth)          { delete_fluid_synth(synth); synth = nullptr; }
    if (synth_settings) { delete_fluid_settings(synth_settings); synth_settings = nullptr; }
    active_sf_id = -1;

    if (p_adl_close && adl_player) { p_adl_close(adl_player); adl_player = nullptr; }
    if (p_opn_close && opn_player) { p_opn_close(opn_player); opn_player = nullptr; }
}

void init_engine(EngineType eng) {
    bool was_locked = false;
    if (g_audio_dev) { SDL_LockAudioDevice(g_audio_dev); was_locked = true; }

    shutdown_engines();
    current_engine = eng;

    switch (eng) {
        case ENGINE_FLUIDSYNTH:
            synth_settings = new_fluid_settings();
            fluid_settings_setint(synth_settings, "synth.polyphony", 48);
            fluid_settings_setint(synth_settings, "synth.cpu-cores", 1);
            fluid_settings_setint(synth_settings, "synth.reverb.active", 0);
            fluid_settings_setint(synth_settings, "synth.chorus.active", 0);
            synth  = new_fluid_synth(synth_settings);
            player = new_fluid_player(synth);
            if (!soundfonts.empty()) {
                std::string sf_path = g_temp_dir + "/" + soundfonts[current_sf_idx];
                active_sf_id = fluid_synth_sfload(synth, sf_path.c_str(), 1);
                if (active_sf_id == FLUID_FAILED)
                    std::cerr << "SoundFont load FAILED: " << sf_path << "\n";
            }
            break;

        case ENGINE_ADLMIDI:
            if (g_adl_available && p_adl_init) {
                adl_player = p_adl_init(SAMPLE_RATE);
                if (!adl_player) std::cerr << "ADLMIDI init failed\n";
                else {
                    if (!adl_banks.empty() && p_adl_openBankFile) {
                        std::string bank = g_temp_dir + "/" + adl_banks[0];
                        p_adl_openBankFile(adl_player, bank.c_str());
                    }
                    if (p_adl_setReverb) p_adl_setReverb(adl_player, 0);
                }
            }
            break;

        case ENGINE_OPNMIDI:
            if (g_opn_available && p_opn_init) {
                opn_player = p_opn_init(SAMPLE_RATE);
                if (!opn_player) std::cerr << "OPNMIDI init failed\n";
                else {
                    if (!opn_banks.empty() && p_opn_openBankFile) {
                        std::string bank = g_temp_dir + "/" + opn_banks[0];
                        p_opn_openBankFile(opn_player, bank.c_str());
                    }
                }
            }
            break;
    }

    if (was_locked) SDL_UnlockAudioDevice(g_audio_dev);
}

void open_audio_device() {
    if (g_audio_dev) {
        SDL_PauseAudioDevice(g_audio_dev, 1);
        SDL_CloseAudioDevice(g_audio_dev);
        g_audio_dev = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_SetHint(SDL_HINT_AUDIODRIVER, "directsound");
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec want{}, have{};
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 4096;
    want.callback = audio_callback;
    g_audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (g_audio_dev) SDL_PauseAudioDevice(g_audio_dev, 0);
}

void play_track(SDL_Window* window, int idx) {
    if (idx < 0 || idx >= (int)midi_tracks.size()) return;
    current_track_idx = idx;
    is_paused = false;

    std::string rel = midi_tracks[idx];
    std::string midi_path;
    if (rel.rfind("@EXT@", 0) == 0) midi_path = rel.substr(5);
    else                           midi_path = g_temp_dir + "/" + rel;

    std::string clean = get_clean_name(rel);
    SDL_SetWindowTitle(window, ("MKLIUKANG1 Player - " + clean).c_str());

    if (g_audio_dev) SDL_LockAudioDevice(g_audio_dev);

    switch (current_engine) {
        case ENGINE_FLUIDSYNTH:
            if (synth) {
                if (player) {
                    fluid_player_stop(player);
                    delete_fluid_player(player);
                    player = nullptr;
                }
                player = new_fluid_player(synth);
                fluid_synth_system_reset(synth);
                if (player && fluid_player_add(player, midi_path.c_str()) != FLUID_FAILED)
                    fluid_player_play(player);
            }
            break;

        case ENGINE_ADLMIDI:
            if (p_adl_stop && adl_player) p_adl_stop(adl_player);
            if (p_adl_openFile && adl_player) {
                if (p_adl_openFile(adl_player, midi_path.c_str()) != 0)
                    std::cerr << "ADLMIDI open FAILED\n";
            }
            break;

        case ENGINE_OPNMIDI:
            if (p_opn_stop && opn_player) p_opn_stop(opn_player);
            if (p_opn_openFile && opn_player) {
                if (p_opn_openFile(opn_player, midi_path.c_str()) != 0)
                    std::cerr << "OPNMIDI open FAILED\n";
            }
            break;
    }

    if (g_audio_dev) SDL_UnlockAudioDevice(g_audio_dev);
}

void next_track(SDL_Window* w) {
    if (midi_tracks.empty()) return;
    int nxt = (current_track_idx + 1) % (int)midi_tracks.size();
    play_track(w, nxt);
}
void prev_track(SDL_Window* w) {
    if (midi_tracks.empty()) return;
    int prv = (current_track_idx - 1 + (int)midi_tracks.size()) % (int)midi_tracks.size();
    play_track(w, prv);
}

// ==================================================================
//                       ПАНЕЛЬ ПЛЕЙЛИСТА
// ==================================================================
int playlist_hit_test(int mx, int my, int panel_x, int panel_y,
                      int panel_h, int header_h) {
    if (mx < panel_x || mx >= panel_x + g_playlist_width) return -1;
    if (my < panel_y + header_h) return -1;
    int row = (my - panel_y - header_h) / g_playlist_row_h + g_playlist_scroll;
    if (row < 0 || row >= (int)g_display_rows.size()) return -1;
    return row;
}

void ensure_active_track_visible(int visible_rows) {
    int active_row = 0;
    for (int i = 0; i < (int)g_display_rows.size(); ++i) {
        if (g_display_rows[i].type == DisplayRow::TRACK &&
            g_display_rows[i].track_idx == current_track_idx) {
            active_row = i;
            break;
        }
    }
    if (active_row < g_playlist_scroll)
        g_playlist_scroll = active_row;
    else if (active_row >= g_playlist_scroll + visible_rows)
        g_playlist_scroll = active_row - visible_rows + 1;
    if (g_playlist_scroll < 0) g_playlist_scroll = 0;
    int max_scroll = std::max(0, (int)g_display_rows.size() - visible_rows);
    if (g_playlist_scroll > max_scroll) g_playlist_scroll = max_scroll;
}

void draw_playlist(SDL_Renderer* r, int panel_x, int panel_y, int panel_w, int panel_h,
                   int mouse_x, int mouse_y, uint32_t t) {
    for (int y = 0; y < panel_h; y += 4) {
        float k = 1.0f - (float)y / panel_h * 0.4f;
        SDL_SetRenderDrawColor(r, (Uint8)(16*k+8), (Uint8)(18*k+10), (Uint8)(26*k+18), 255);
        SDL_Rect rc = { panel_x, panel_y + y, panel_w, std::min(4, panel_h - y) };
        SDL_RenderFillRect(r, &rc);
    }

    SDL_SetRenderDrawColor(r, 22, 26, 38, 255);
    SDL_Rect header = { panel_x, panel_y, panel_w, 26 };
    SDL_RenderFillRect(r, &header);
    draw_header_beam(r, panel_x, panel_y, panel_w, 26, t);

    const UIStrings& S = L();
    set_text_color(90, 140, 210);
    draw_string(r, S.playlist, panel_x + 12, panel_y + 8, 1);

    std::string count_str = std::to_string(current_track_idx + 1) +
                            "/" + std::to_string(midi_tracks.size());
    int cw = text_width(count_str, 1);
    set_text_color(140, 160, 200);
    draw_string(r, count_str, panel_x + panel_w - cw - 12, panel_y + 8, 1);

    SDL_SetRenderDrawColor(r, 40, 50, 70, 255);
    SDL_Rect sep = { panel_x, panel_y + 26, panel_w, 1 };
    SDL_RenderFillRect(r, &sep);

    int list_y = panel_y + 26;
    int list_h = panel_h - 26;
    int max_chars = (panel_w - 50) / 10;
    if (max_chars < 4) max_chars = 4;

    int visible_rows = list_h / g_playlist_row_h;
    ensure_active_track_visible(visible_rows);

    SDL_Rect clip = { panel_x, list_y, panel_w, list_h };
    SDL_RenderSetClipRect(r, &clip);

    g_playlist_hover = -1;
    int row = g_playlist_scroll;
    for (int i = 0; i < visible_rows && row < (int)g_display_rows.size(); ++i, ++row) {
        int y = list_y + i * g_playlist_row_h;
        SDL_Rect row_rect = { panel_x, y, panel_w, g_playlist_row_h };
        const auto& dr = g_display_rows[row];
        bool is_hover = (mouse_x >= panel_x && mouse_x < panel_x + panel_w &&
                         mouse_y >= y && mouse_y < y + g_playlist_row_h);

        if (dr.type == DisplayRow::FOLDER) {
            const auto& fe = g_folders[dr.folder_idx];
            SDL_SetRenderDrawColor(r, 26, 32, 46, 255);
            SDL_RenderFillRect(r, &row_rect);
            SDL_SetRenderDrawColor(r, 50, 70, 110, 255);
            SDL_Rect ln = { panel_x, y + g_playlist_row_h - 1, panel_w, 1 };
            SDL_RenderFillRect(r, &ln);

            set_text_color(120, 170, 240);
            std::string arrow = fe.expanded ? "V" : ">";
            draw_string(r, arrow, panel_x + 8, y + 7, 1);

            std::string fname = utf8_truncate(fe.name, max_chars - 6);
            set_text_color(180, 200, 240);
            draw_string(r, fname, panel_x + 8 + text_width(arrow, 1) + 6, y + 7, 1);

            std::string fc = "[" + std::to_string(fe.count) + "]";
            int fcw = text_width(fc, 1);
            set_text_color(110, 140, 190);
            draw_string(r, fc, panel_x + panel_w - fcw - 12, y + 7, 1);

            if (is_hover) g_playlist_hover = row;
        } else {
            int tidx = dr.track_idx;
            bool is_active = (tidx == current_track_idx);

            if (is_active) {
                for (int gx = 0; gx < panel_w; gx += 8) {
                    float k = 1.0f - (float)gx / panel_w * 0.4f;
                    SDL_SetRenderDrawColor(r, (Uint8)(35*k+10), (Uint8)(90*k+20), (Uint8)(160*k+30), 255);
                    SDL_Rect rc = { panel_x + gx, y, std::min(8, panel_w - gx), g_playlist_row_h };
                    SDL_RenderFillRect(r, &rc);
                }
                float pulse = (sinf(t * 0.004f) + 1.0f) * 0.5f;
                SDL_SetRenderDrawColor(r, (Uint8)(90+pulse*60), (Uint8)(170+pulse*40), 240, 255);
                SDL_Rect acc = { panel_x, y, 3, g_playlist_row_h };
                SDL_RenderFillRect(r, &acc);
                set_text_color(240, 245, 255);
            } else if (is_hover) {
                SDL_SetRenderDrawColor(r, 28, 34, 50, 255);
                SDL_RenderFillRect(r, &row_rect);
                set_text_color(200, 215, 240);
                g_playlist_hover = row;
            } else {
                set_text_color(160, 175, 200);
            }

            int num_x = panel_x + 28;
            if (is_active) draw_mini_eq(r, num_x - 16, y + 6, 12, 10, t, true);

            std::string num = std::to_string(tidx + 1);
            draw_string(r, num, num_x, y + 7, 1);

            std::string name = get_clean_name(midi_tracks[tidx]);
            name = utf8_truncate(name, max_chars - 4);
            draw_string(r, name, num_x + text_width(num, 1) + 8, y + 7, 1);
        }
    }

    SDL_RenderSetClipRect(r, nullptr);

    if ((int)g_display_rows.size() > visible_rows && visible_rows > 0) {
        int total = (int)g_display_rows.size();
        float rt = (float)g_playlist_scroll / total;
        float rh = (float)visible_rows / total;
        int bt = list_y + (int)(rt * list_h);
        int bh = (int)(rh * list_h);
        if (bh < 20) bh = 20;
        if (bt + bh > list_y + list_h) bt = list_y + list_h - bh;

        SDL_SetRenderDrawColor(r, 60, 80, 120, 200);
        SDL_Rect bar = { panel_x + panel_w - 6, bt, 4, bh };
        SDL_RenderFillRect(r, &bar);
    }

    draw_glow_border(r, panel_x, panel_y, panel_w, panel_h, t, 90, 140, 220);
}

// ==================================================================
//                       PAK BROWSER
// ==================================================================
void draw_pak_browser(SDL_Renderer* r, int w, int h, uint32_t t) {
    const UIStrings& S = L();

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    SDL_Rect full = { 0, 0, w, h };
    SDL_RenderFillRect(r, &full);

    int bx = 40, by = 40;
    int bw = w - 80, bh = h - 80;

    SDL_SetRenderDrawColor(r, 14, 18, 30, 245);
    SDL_Rect bg = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &bg);

    SDL_SetRenderDrawColor(r, 22, 28, 44, 255);
    SDL_Rect hdr = { bx, by, bw, 30 };
    SDL_RenderFillRect(r, &hdr);
    draw_header_beam(r, bx, by, bw, 30, t);

    set_text_color(120, 180, 250);
    draw_string(r, S.pak_browser, bx + 12, by + 10, 1);

    std::string count = std::to_string(g_all_pak_names.size()) + " " + S.pak_entries;
    int cw = text_width(count, 1);
    set_text_color(140, 160, 200);
    draw_string(r, count, bx + bw - cw - 12, by + 10, 1);

    int list_y  = by + 30;
    int list_h  = bh - 30 - 22;
    int row_h   = g_pak_row_h;
    int visible = list_h / row_h;
    int total   = (int)g_all_pak_names.size();
    int max_scroll = std::max(0, total - visible);
    if (g_pak_scroll > max_scroll) g_pak_scroll = max_scroll;
    if (g_pak_scroll < 0)          g_pak_scroll = 0;

    SDL_Rect clip = { bx, list_y, bw, list_h };
    SDL_RenderSetClipRect(r, &clip);

    if (total == 0) {
        set_text_color(140, 160, 200);
        draw_string(r, S.pak_empty, bx + 16, list_y + 12, 1);
    } else {
        int max_chars = (bw - 40) / 10;
        if (max_chars < 8) max_chars = 8;

        for (int i = 0; i < visible && i + g_pak_scroll < total; ++i) {
            int idx = i + g_pak_scroll;
            int y   = list_y + i * row_h;

            SDL_SetRenderDrawColor(r, (idx % 2) ? 16 : 20,
                                      (idx % 2) ? 20 : 24,
                                      (idx % 2) ? 32 : 38, 220);
            SDL_Rect rr = { bx, y, bw, row_h };
            SDL_RenderFillRect(r, &rr);

            const std::string& name = g_all_pak_names[idx];
            Uint8 R = 200, G = 210, B = 230;
            if      (name.rfind("midi/",       0) == 0) { R = 180; G = 220; B = 255; }
            else if (name.rfind("soundfonts/", 0) == 0) { R = 220; G = 200; B = 140; }
            else if (name.rfind("banks/adl/",  0) == 0) { R = 200; G = 170; B = 240; }
            else if (name.rfind("banks/opn/",  0) == 0) { R = 140; G = 230; B = 180; }

            set_text_color(R, G, B);
            std::string disp = utf8_truncate(name, max_chars);
            draw_string(r, disp, bx + 16, y + 4, 1);
        }
    }

    SDL_RenderSetClipRect(r, nullptr);

    if (total > visible && visible > 0) {
        float rt  = (float)g_pak_scroll / total;
        float rh  = (float)visible / total;
        int   bt  = list_y + (int)(rt * list_h);
        int   bhh = std::max(20, (int)(rh * list_h));
        if (bt + bhh > list_y + list_h) bt = list_y + list_h - bhh;
        SDL_SetRenderDrawColor(r, 80, 110, 170, 220);
        SDL_Rect bar = { bx + bw - 6, bt, 4, bhh };
        SDL_RenderFillRect(r, &bar);
    }

    set_text_color(150, 170, 210);
    draw_string(r, S.pak_hint, bx + 12, by + bh - 20, 1);

    draw_glow_border(r, bx, by, bw, bh, t, 90, 140, 220);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// ==================================================================
//                       HELP WINDOW
// ==================================================================
static void draw_help_window(SDL_Renderer* r, int w, int h) {
    const UIStrings& S = L();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    SDL_Rect full = { 0, 0, w, h };
    SDL_RenderFillRect(r, &full);

    int dw = std::min(w - 60, 820);
    int dh = std::min(h - 40, 720);
    int dx = (w - dw) / 2;
    int dy = (h - dh) / 2;

    SDL_SetRenderDrawColor(r, 22, 26, 40, 250);
    SDL_Rect dlg = { dx, dy, dw, dh };
    SDL_RenderFillRect(r, &dlg);

    SDL_SetRenderDrawColor(r, 40, 60, 100, 255);
    SDL_Rect hdr = { dx, dy, dw, 40 };
    SDL_RenderFillRect(r, &hdr);

    set_text_color(200, 220, 255);
    draw_string(r, S.help_title, dx + 16, dy + 12, 2);

    int cX = dx + 20, cY = dy + 54;
    int cW = dw - 40, cH = dh - 114;
    SDL_Rect clipRect = { cX, cY, cW, cH };
    SDL_RenderSetClipRect(r, &clipRect);

    int y = cY, lineH = 18;

    set_text_color(140, 180, 240);
    draw_string(r, S.help_hotkeys, cX, y, 1);
    y += 26;

    const char* keysEN[] = {
        "Up / Down", "Space", "Left / Right",
        "1 / 2 / 3", "L", "Tab", "P", "H", "Esc"
    };
    const char* descEN[] = {
        "Previous / next track",
        "Pause / resume",
        "Engine previous / next",
        "Switch engine: 1 Fluid, 2 ADL, 3 OPN2",
        "Toggle language (EN / RU)",
        "Show / hide playlist",
        "Show / hide PAK browser",
        "Show this help window",
        "Exit program"
    };
    const char* keysRU[] = {
        "Вверх / Вниз", "Пробел", "Влево / Вправо",
        "1 / 2 / 3", "L", "Tab", "P", "H", "Esc"
    };
    const char* descRU[] = {
        "Предыдущий / следующий трек",
        "Пауза / продолжить",
        "Движок назад / вперёд",
        "Переключить движок: 1 Fluid, 2 ADL, 3 OPN2",
        "Сменить язык (RU / EN)",
        "Показать / скрыть плейлист",
        "Показать / скрыть содержимое PAK",
        "Открыть это окно справки",
        "Выход из программы"
    };
    const char** kk = (g_lang == LANG_RU) ? keysRU : keysEN;
    const char** dd = (g_lang == LANG_RU) ? descRU : descEN;
    for (int i = 0; i < 9; ++i) {
        set_text_color(240, 220, 120);
        draw_string(r, kk[i], cX, y, 1);
        set_text_color(220, 230, 250);
        draw_string(r, dd[i], cX + 200, y, 1);
        y += lineH;
    }

    y += 14;
    set_text_color(140, 180, 240);
    draw_string(r, S.help_about, cX, y, 1);
    y += 26;

    std::string text = S.about_text;
    std::vector<std::string> lines;
    std::string cur;
    size_t i = 0;
    int maxChars = cW / 10;
    while (i < text.size()) {
        if (i + 1 < text.size() && text[i] == '|' && text[i+1] == '|') {
            lines.push_back(cur); cur.clear(); i += 2; continue;
        }
        if (text[i] == '\n') { lines.push_back(cur); cur.clear(); ++i; continue; }
        cur.push_back(text[i]);
        if ((int)cur.size() >= maxChars) {
            size_t sp = cur.find_last_of(' ');
            if (sp != std::string::npos && sp > (size_t)maxChars / 2) {
                std::string rest = cur.substr(sp + 1);
                cur = cur.substr(0, sp);
                lines.push_back(cur);
                cur = rest;
            } else {
                lines.push_back(cur);
                cur.clear();
            }
        }
        ++i;
    }
    if (!cur.empty()) lines.push_back(cur);

    set_text_color(210, 220, 240);
    for (auto& ln : lines) {
        draw_string(r, ln, cX, y, 1);
        y += lineH;
        if (y > cY + cH - lineH) break;
    }
    SDL_RenderSetClipRect(r, nullptr);

    int btnW = 200, btnH = 34;
    int btnX = dx + (dw - btnW) / 2;
    int btnY = dy + dh - 48;
    SDL_SetRenderDrawColor(r, 55, 100, 160, 255);
    SDL_Rect btn = { btnX, btnY, btnW, btnH };
    SDL_RenderFillRect(r, &btn);
    SDL_SetRenderDrawColor(r, 120, 180, 240, 255);
    SDL_RenderDrawRect(r, &btn);
    int tw = text_width(S.help_close, 1);
    set_text_color(240, 245, 255);
    draw_string(r, S.help_close, btnX + (btnW - tw) / 2, btnY + 9, 1);

    SDL_SetRenderDrawColor(r, 120, 170, 240, 255);
    SDL_RenderDrawRect(r, &dlg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

// ==================================================================
//                       CONTEXT MENU
// ==================================================================
enum CtxItem {
    CTX_PLAY_PAUSE = 0,
    CTX_NEXT,
    CTX_PREV,
    CTX_SEP1,
    CTX_OPEN,
    CTX_VGM_OPN2,
    CTX_VGM_OPL3,
    CTX_VGM_OPL2,
    CTX_VGM_YM2151,
    CTX_SEP2,
    CTX_SAVE,
    CTX_HELP,
    CTX_SEP3,
    CTX_EXIT,
    CTX_COUNT
};

static std::string ctx_label(int i) {
    const UIStrings& S = L();
    switch (i) {
        case CTX_PLAY_PAUSE: return is_paused ? S.ctx_play : S.ctx_pause;
        case CTX_NEXT:       return S.ctx_next;
        case CTX_PREV:       return S.ctx_prev;
        case CTX_OPEN:       return S.ctx_open;
        case CTX_VGM_OPN2:   return S.ctx_vgm_opn2;
        case CTX_VGM_OPL3:   return S.ctx_vgm_opl3;
        case CTX_VGM_OPL2:   return S.ctx_vgm_opl2;
        case CTX_VGM_YM2151: return S.ctx_vgm_ym2151;
        case CTX_SAVE:       return S.ctx_save;
        case CTX_HELP:       return S.ctx_help;
        case CTX_EXIT:       return S.ctx_exit;
    }
    return "";
}

static bool ctx_is_sep(int i) { return i == CTX_SEP1 || i == CTX_SEP2 || i == CTX_SEP3; }

static void draw_context_menu(SDL_Renderer* r) {
    if (!g_ctx_active) return;
    int itemH = 26, sepH = 8;
    int menuW = 260;
    int menuH = 8;
    for (int i = 0; i < CTX_COUNT; ++i)
        menuH += ctx_is_sep(i) ? sepH : itemH;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 20, 24, 34, 250);
    SDL_Rect bg = { g_ctx_x, g_ctx_y, menuW, menuH };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 80, 120, 180, 255);
    SDL_RenderDrawRect(r, &bg);

    int y = g_ctx_y + 4;
    for (int i = 0; i < CTX_COUNT; ++i) {
        if (ctx_is_sep(i)) {
            SDL_SetRenderDrawColor(r, 60, 80, 120, 180);
            SDL_RenderDrawLine(r, g_ctx_x + 6, y + sepH/2, g_ctx_x + menuW - 6, y + sepH/2);
            y += sepH;
            continue;
        }
        SDL_Rect row = { g_ctx_x + 2, y, menuW - 4, itemH };
        if (i == g_ctx_selected) {
            SDL_SetRenderDrawColor(r, 50, 80, 140, 255);
            SDL_RenderFillRect(r, &row);
        }
        set_text_color(220, 230, 250);
        draw_string(r, ctx_label(i), g_ctx_x + 12, y + 5, 1);
        y += itemH;
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static int ctx_menu_width()  { return 260; }

static int ctx_hit_item(int mx, int my) {
    if (!g_ctx_active) return -1;
    if (mx < g_ctx_x || mx >= g_ctx_x + ctx_menu_width()) return -1;
    int itemH = 26, sepH = 8;
    int y = g_ctx_y + 4;
    for (int i = 0; i < CTX_COUNT; ++i) {
        if (ctx_is_sep(i)) { y += sepH; continue; }
        if (my >= y && my < y + itemH) return i;
        y += itemH;
    }
    return -1;
}

// ==================================================================
//                       ИНТЕРФЕЙС
// ==================================================================
void draw_interface(SDL_Renderer* renderer, uint32_t t) {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    ensure_bg(renderer, w, h);
    if (g_bg_tex) SDL_RenderCopy(renderer, g_bg_tex, nullptr, nullptr);
    else          SDL_RenderClear(renderer);

    int panel_w = g_show_playlist ? g_playlist_width : 0;
    int main_x  = panel_w;
    int main_w  = w - panel_w;

    if (g_show_playlist) {
        draw_playlist(renderer, 0, 0, panel_w, h, mx, my, t);
    }

    SDL_Rect main_clip = { main_x, 0, main_w, h };
    SDL_RenderSetClipRect(renderer, &main_clip);

    update_particles(main_w, h);
    for (auto& p : g_particles) {
        SDL_SetRenderDrawColor(renderer, p.R, p.G, p.B, 255);
        SDL_Rect rc = { main_x + (int)p.x, (int)p.y, p.size, p.size };
        SDL_RenderFillRect(renderer, &rc);
    }

    draw_top_bar(renderer, main_x, main_w, t);

    const UIStrings& S = L();
    int cx = main_x + 24;

    set_text_color(90, 140, 210);
    draw_string(renderer, S.now_playing, cx, 22, 1);

    std::string lang_tag = (g_lang == LANG_EN) ? "EN" : "RU";
    int lang_w = text_width(lang_tag, 1);
    SDL_SetRenderDrawColor(renderer, 60, 80, 120, 255);
    SDL_Rect lang_bg = { w - lang_w - 34, 18, lang_w + 16, 16 };
    SDL_RenderFillRect(renderer, &lang_bg);
    set_text_color(200, 220, 255);
    draw_string(renderer, lang_tag, w - lang_w - 26, 22, 1);

    int title_scale = 2;
    if (main_w >= 800)  title_scale = 3;
    if (main_w >= 1200) title_scale = 4;
    int char_w    = 8 * title_scale + 2;
    int max_chars = (main_w - 48) / char_w;
    if (max_chars < 6) max_chars = 6;

    std::string title = midi_tracks.empty() ? "-" : get_clean_name(midi_tracks[current_track_idx]);
    int title_len = utf8_length(title);
    std::string displayed;
    if (title_len <= max_chars) {
        displayed = title;
    } else {
        std::string padded = title + "     ";
        int padded_len = title_len + 5;
        int offset = (int)((t / 90) % padded_len);
        std::string cycled = utf8_substr(padded, offset, max_chars);
        if (utf8_length(cycled) < max_chars)
            cycled += utf8_substr(title, 0, max_chars - utf8_length(cycled));
        displayed = cycled;
    }

    int title_y = 44;
    int box_h = 8 * title_scale + 8;
    {
        float pulse = (sinf(t * 0.0035f) + 1.0f) * 0.5f;
        SDL_Rect outer = { cx - 6, title_y - 6, main_w - 36, box_h + 4 };
        SDL_Rect inner = { cx - 4, title_y - 4, main_w - 40, box_h };

        SDL_SetRenderDrawColor(renderer, (Uint8)(30+pulse*40), (Uint8)(70+pulse*40), 180, 255);
        SDL_RenderDrawRect(renderer, &outer);
        SDL_SetRenderDrawColor(renderer, (Uint8)(50+pulse*60), (Uint8)(110+pulse*50), 220, 255);
        SDL_Rect mid = { cx - 5, title_y - 5, main_w - 38, box_h + 2 };
        SDL_RenderDrawRect(renderer, &mid);
        SDL_SetRenderDrawColor(renderer, (Uint8)(80+pulse*80), (Uint8)(160+pulse*60), 240, 255);
        SDL_RenderDrawRect(renderer, &inner);
    }

    set_text_color(240, 245, 255);
    draw_string(renderer, displayed, cx, title_y, title_scale);

    if (is_paused) {
        set_text_color(240, 180, 60);
        draw_string(renderer, "[ " + S.paused + " ]",
                    cx + text_width(displayed, title_scale) + 16,
                    title_y + 8 * title_scale - 8, 1);
    }

    std::string pos_line = S.track + " " + std::to_string(current_track_idx + 1) +
                           "/" + std::to_string(midi_tracks.size());
    int pos_y = title_y + 8 * title_scale + 12;
    set_text_color(120, 170, 240);
    draw_string(renderer, pos_line, cx, pos_y, 1);

    std::string engine_line = "1: FLUID";
    engine_line += g_adl_available ? "  2: ADL"  : "  2: ADL (n/a)";
    engine_line += g_opn_available ? "  3: OPN2" : "  3: OPN2 (n/a)";
    set_text_color(g_adl_available || g_opn_available ? 180 : 100,
                   g_adl_available || g_opn_available ? 200 : 110,
                   g_adl_available || g_opn_available ? 240 : 130);
    int elw = text_width(engine_line, 1);
    draw_string(renderer, engine_line, w - elw - 24, pos_y, 1);

    int controls_y   = h - 24;
    int engine_row_y = controls_y - 66;

    int eq_x = cx;
    int eq_y = pos_y + 8 + 12;
    int eq_w = main_w - 48;
    int eq_h = (engine_row_y - 22) - eq_y;
    if (eq_h < 40) eq_h = 40;

    SDL_SetRenderDrawColor(renderer, 18, 22, 34, 255);
    SDL_Rect eq_bg = { eq_x, eq_y, eq_w, eq_h };
    SDL_RenderFillRect(renderer, &eq_bg);

    {
        int step = 24;
        int offset = (int)((t / 60) % (step * 2));
        SDL_SetRenderDrawColor(renderer, 90, 140, 220, 255);
        for (int i = -step*2; i < eq_w + step*2; i += step*2) {
            int px = eq_x + i + offset;
            if (px >= eq_x && px < eq_x + eq_w - 3) {
                SDL_Rect a = {px, eq_y, 3, 3};
                SDL_Rect b = {px, eq_y + eq_h - 3, 3, 3};
                SDL_RenderFillRect(renderer, &a);
                SDL_RenderFillRect(renderer, &b);
            }
        }
        for (int j = -step*2; j < eq_h + step*2; j += step*2) {
            int py = eq_y + j + offset;
            if (py >= eq_y && py < eq_y + eq_h - 3) {
                SDL_Rect a = {eq_x, py, 3, 3};
                SDL_Rect b = {eq_x + eq_w - 3, py, 3, 3};
                SDL_RenderFillRect(renderer, &a);
                SDL_RenderFillRect(renderer, &b);
            }
        }
    }

    SDL_SetRenderDrawColor(renderer, 30, 36, 54, 255);
    for (int i = 1; i < 4; ++i) {
        int gy = eq_y + eq_h * i / 4;
        SDL_RenderDrawLine(renderer, eq_x + 1, gy, eq_x + eq_w - 2, gy);
    }

    draw_corner_marks(renderer, eq_x, eq_y, eq_w, eq_h, t);
    draw_equalizer(renderer, eq_x + 6, eq_y + 6, eq_w - 12, eq_h - 12);

    set_text_color(90, 140, 210);
    draw_string(renderer, S.sound_engine, cx, engine_row_y, 1);
    set_text_color(240, 245, 255);

    std::string sf_display;
    switch (current_engine) {
        case ENGINE_FLUIDSYNTH:
            sf_display = soundfonts.empty() ? "(no sf)" : get_clean_name(soundfonts[current_sf_idx]);
            break;
        case ENGINE_ADLMIDI:
            if (!g_adl_available) sf_display = "(libADLMIDI.dll not found)";
            else if (adl_banks.empty()) sf_display = "(embedded bank)";
            else sf_display = get_clean_name(adl_banks[g_adl_current_bank % (int)adl_banks.size()]);
            break;
        case ENGINE_OPNMIDI:
            if (!g_opn_available) sf_display = "(libOPNMIDI.dll not found)";
            else if (opn_banks.empty()) sf_display = "(no bank)";
            else sf_display = get_clean_name(opn_banks[g_opn_current_bank % (int)opn_banks.size()]);
            break;
    }
    draw_string(renderer, sf_display, cx, engine_row_y + 16, 1);

    set_text_color(100, 120, 150);
    const std::string hints[] = {
        S.hint_nav, S.hint_lr, S.hint_pause, S.hint_lang,
        std::string("TAB: PLAYLIST"), std::string("P: PAK"),
        std::string("H: HELP"), S.hint_exit
    };
    int total_hint_w = 0;
    for (auto& hs : hints) total_hint_w += text_width(hs, 1) + 20;
    total_hint_w -= 20;
    int hx = main_x + (main_w - total_hint_w) / 2;
    if (hx < main_x + 12) hx = main_x + 12;
    for (auto& hs : hints) {
        draw_string(renderer, hs, hx, controls_y, 1);
        hx += text_width(hs, 1) + 20;
    }

    set_text_color(60, 75, 100);
    std::string ver = "MKLIUKANG1 v1.1";
    int vw = text_width(ver, 1);
    draw_string(renderer, ver, w - vw - 12, h - 16, 1);

    if (g_show_playlist) {
        for (int i = 0; i < 3; ++i) {
            float phase = t * 0.002f + i * 0.7f;
            float intensity = (sinf(phase) + 1.0f) * 0.5f;
            Uint8 R = (Uint8)(40 + intensity * 40);
            Uint8 G = (Uint8)(90 + intensity * 60);
            Uint8 B = (Uint8)(180 + intensity * 60);
            SDL_SetRenderDrawColor(renderer, R, G, B, 200);
            SDL_Rect bar = { main_x + 2 + i * 4, h / 3, 2, h / 3 };
            SDL_RenderFillRect(renderer, &bar);
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (g_show_pak_browser) {
        draw_pak_browser(renderer, w, h, t);
    }

    if (g_ctx_active) {
        draw_context_menu(renderer);
    }

    if (g_help_active) {
        draw_help_window(renderer, w, h);
    }

    SDL_RenderPresent(renderer);
}

// ==================================================================
//                       MAIN
// ==================================================================
int main(int, char**) {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow(
        "MKLIUKANG1 Player",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1000, 500,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { SDL_Quit(); return 1; }
    SDL_SetWindowMinimumSize(window, 700, 400);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            "Failed to create SDL renderer", window);
        SDL_DestroyWindow(window); SDL_Quit();
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    try_load_adlmidi();
    try_load_opnmidi();
    loadSettings();
    vgm_init();

    {
        std::vector<char> pak = load_embedded_pak();
        if (pak.empty() || !g_pack.load_from_memory(pak.data(), pak.size())) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
                "resources.pak not found or corrupted", window);
            SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
            return 1;
        }
    }

    midi_tracks = g_pack.list_by_prefix("midi/");
    soundfonts  = g_pack.list_by_prefix("soundfonts/");
    adl_banks   = g_pack.list_by_prefix("banks/adl/");
    opn_banks   = g_pack.list_by_prefix("banks/opn/");

    g_all_pak_names.clear();
    for (auto& e : g_pack.entries()) g_all_pak_names.push_back(e.name);
    std::sort(g_all_pak_names.begin(), g_all_pak_names.end());
    std::cout << "[pak] total entries: " << g_all_pak_names.size() << "\n";

    if (midi_tracks.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            "No tracks in pak", window);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
        return 1;
    }

    build_folder_index();
    build_display_rows();

    g_temp_dir = unpack_pak_to_temp();
    g_pack.release_data();

    init_equalizer();
    init_engine(current_engine);
    open_audio_device();
    play_track(window, 0);

    bool      running = true;
    SDL_Event event;
    int       prev_status = -1;
    uint32_t  next_frame = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            // ============= HELP WINDOW =============
            if (g_help_active) {
                if (event.type == SDL_KEYDOWN) {
                    SDL_Keycode k = event.key.keysym.sym;
                    if (k == SDLK_ESCAPE || k == SDLK_h) g_help_active = false;
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    int ww, hh;
                    SDL_GetWindowSize(window, &ww, &hh);
                    int dw = std::min(ww - 60, 820);
                    int dh = std::min(hh - 40, 720);
                    int dx = (ww - dw) / 2, dy = (hh - dh) / 2;
                    int btnW = 200, btnH = 34;
                    int btnX = dx + (dw - btnW) / 2;
                    int btnY = dy + dh - 48;
                    int mx = event.button.x, my = event.button.y;
                    bool onBtn = (mx >= btnX && mx < btnX + btnW &&
                                  my >= btnY && my < btnY + btnH);
                    bool outside = (mx < dx || mx >= dx + dw || my < dy || my >= dy + dh);
                    if (onBtn || outside) g_help_active = false;
                }
                continue;
            }

            // ============= CONTEXT MENU =============
            if (g_ctx_active) {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    int item = ctx_hit_item(event.button.x, event.button.y);
                    g_ctx_active = false;
                    if (item >= 0) {
                        switch (item) {
                            case CTX_PLAY_PAUSE:
                                if (current_engine == ENGINE_FLUIDSYNTH) {
                                    if (player && fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
                                        fluid_player_stop(player); is_paused = true;
                                    } else if (player) { fluid_player_play(player); is_paused = false; }
                                } else if (current_engine == ENGINE_ADLMIDI && p_adl_pause && adl_player) {
                                    if (is_paused) { p_adl_pause(adl_player, 0); is_paused = false; }
                                    else           { p_adl_pause(adl_player, 1); is_paused = true; }
                                } else if (current_engine == ENGINE_OPNMIDI && p_opn_pause && opn_player) {
                                    if (is_paused) { p_opn_pause(opn_player, 0); is_paused = false; }
                                    else           { p_opn_pause(opn_player, 1); is_paused = true; }
                                }
                                break;
                            case CTX_NEXT: next_track(window); break;
                            case CTX_PREV: prev_track(window); break;
                            case CTX_OPEN: {
                                std::string path = openMidiDialogW();
                                if (!path.empty()) {
                                    std::string entry = "@EXT@" + path;
                                    auto it = std::find(midi_tracks.begin(), midi_tracks.end(), entry);
                                    if (it == midi_tracks.end()) {
                                        midi_tracks.push_back(entry);
                                        build_folder_index();
                                        build_display_rows();
                                        it = std::find(midi_tracks.begin(), midi_tracks.end(), entry);
                                    }
                                    if (it != midi_tracks.end()) {
                                        int idx = (int)std::distance(midi_tracks.begin(), it);
                                        play_track(window, idx);
                                    }
                                }
                                break;
                            }
                            case CTX_VGM_OPN2:
                            case CTX_VGM_OPL3:
                            case CTX_VGM_OPL2:
                            case CTX_VGM_YM2151: {
                                if (current_track_idx < 0 ||
                                    current_track_idx >= (int)midi_tracks.size()) break;

                                int chip = 0;
                                std::string suffix = ".vgm";
                                std::string toolNeeded = "";

                                if (item == CTX_VGM_OPN2) {
                                    chip = 0;
                                    suffix = "_opn2.vgm";
                                    if (!vgm_opn2_available()) toolNeeded = "midi2vgm_opn2.exe";
                                } else if (item == CTX_VGM_OPL3) {
                                    chip = 1;
                                    suffix = "_opl3.vgm";
                                    if (!vgm_adl_available()) toolNeeded = "mid2vgm_adl.exe";
                                } else if (item == CTX_VGM_OPL2) {
                                    chip = 2;
                                    suffix = "_opl2.vgm";
                                    if (!vgm_adl_available()) toolNeeded = "mid2vgm_adl.exe";
                                } else {
                                    chip = 3;
                                    suffix = "_ym2151.vgm";
                                    if (!vgm_adl_available()) toolNeeded = "mid2vgm_adl.exe";
                                }

                                if (!toolNeeded.empty()) {
                                    std::string msg = toolNeeded + " not found next to the player.";
                                    SDL_ShowSimpleMessageBox(
                                        SDL_MESSAGEBOX_ERROR, "VGM export",
                                        msg.c_str(), window);
                                    break;
                                }

                                std::string base = get_clean_name(midi_tracks[current_track_idx]);
                                std::string out = vgm_save_dialog(base + suffix);
                                if (out.empty()) break;

                                std::string rel = midi_tracks[current_track_idx];
                                std::string midiPath;
                                if (rel.rfind("@EXT@", 0) == 0) midiPath = rel.substr(5);
                                else                           midiPath = g_temp_dir + "/" + rel;

                                std::string err;
                                if (vgm_export(midiPath, out, chip, err)) {
                                    SDL_ShowSimpleMessageBox(
                                        SDL_MESSAGEBOX_INFORMATION, "VGM export",
                                        ("Saved: " + out).c_str(), window);
                                } else {
                                    SDL_ShowSimpleMessageBox(
                                        SDL_MESSAGEBOX_ERROR, "VGM export failed",
                                        err.c_str(), window);
                                }
                                break;
                            }
                            case CTX_SAVE: saveSettings(); break;
                            case CTX_HELP: g_help_active = true; break;
                            case CTX_EXIT: running = false; break;
                        }
                    }
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                    g_ctx_active = false;
                }
                else if (event.type == SDL_MOUSEMOTION) {
                    g_ctx_selected = ctx_hit_item(event.motion.x, event.motion.y);
                }
                else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                    g_ctx_active = false;
                }
                continue;
            }

            // ============= PAK BROWSER (обрабатывает все свои события) =============
            if (g_show_pak_browser) {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    int ww, hh;
                    SDL_GetWindowSize(window, &ww, &hh);
                    int bx = 40, by = 40;
                    int bw = ww - 80, bh = hh - 80;
                    int list_y = by + 30;
                    int list_h = bh - 30 - 22;
                    int mx = event.button.x, my = event.button.y;
                    if (mx >= bx && mx < bx + bw && my >= list_y && my < list_y + list_h) {
                        int row = (my - list_y) / g_pak_row_h + g_pak_scroll;
                        if (row >= 0 && row < (int)g_all_pak_names.size()) {
                            const std::string& name = g_all_pak_names[row];
                            if (name.rfind("midi/", 0) == 0) {
                                auto it = std::find(midi_tracks.begin(), midi_tracks.end(), name);
                                if (it != midi_tracks.end()) {
                                    int idx = (int)std::distance(midi_tracks.begin(), it);
                                    play_track(window, idx);
                                    g_show_pak_browser = false;
                                }
                            }
                        }
                    }
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                    g_show_pak_browser = false;
                }
                else if (event.type == SDL_MOUSEWHEEL) {
                    g_pak_scroll -= event.wheel.y * 3;
                    int ww, hh;
                    SDL_GetWindowSize(window, &ww, &hh);
                    int list_h = (hh - 80) - 30 - 22;
                    int visible = list_h / g_pak_row_h;
                    int max_scroll = std::max(0, (int)g_all_pak_names.size() - visible);
                    if (g_pak_scroll < 0) g_pak_scroll = 0;
                    if (g_pak_scroll > max_scroll) g_pak_scroll = max_scroll;
                }
                else if (event.type == SDL_KEYDOWN) {
                    SDL_Keycode k = event.key.keysym.sym;
                    if (k == SDLK_ESCAPE || k == SDLK_p) g_show_pak_browser = false;
                    else if (k == SDLK_l) g_lang = (g_lang == LANG_EN) ? LANG_RU : LANG_EN;
                }
                continue;
            }

            // ============= ОСНОВНОЕ ОКНО =============
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x, my = event.button.y;
                if (g_show_playlist) {
                    int ww, hh;
                    SDL_GetWindowSize(window, &ww, &hh);
                    int row = playlist_hit_test(mx, my, 0, 0, hh, 26);
                    if (row >= 0) {
                        const auto& dr = g_display_rows[row];
                        if (dr.type == DisplayRow::FOLDER) {
                            g_folders[dr.folder_idx].expanded = !g_folders[dr.folder_idx].expanded;
                            build_display_rows();
                        } else {
                            play_track(window, dr.track_idx);
                        }
                    }
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                g_ctx_active = true;
                g_ctx_x = event.button.x;
                g_ctx_y = event.button.y;
                g_ctx_selected = -1;
            }
            else if (event.type == SDL_MOUSEWHEEL) {
                if (g_show_playlist && event.wheel.y != 0) {
                    int mx, my;
                    SDL_GetMouseState(&mx, &my);
                    if (mx < g_playlist_width) {
                        g_playlist_scroll -= event.wheel.y * 3;
                        int ww, hh;
                        SDL_GetWindowSize(window, &ww, &hh);
                        int visible = (hh - 26) / g_playlist_row_h;
                        int max_scroll = std::max(0, (int)g_display_rows.size() - visible);
                        if (g_playlist_scroll < 0) g_playlist_scroll = 0;
                        if (g_playlist_scroll > max_scroll) g_playlist_scroll = max_scroll;
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE) running = false;
                else if (key == SDLK_p) g_show_pak_browser = !g_show_pak_browser;
                else if (key == SDLK_h) g_help_active = true;
                else if (key == SDLK_TAB) {
                    g_show_playlist = !g_show_playlist;
                }
                else if (key == SDLK_SPACE) {
                    if (current_engine == ENGINE_FLUIDSYNTH) {
                        if (player && fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
                            fluid_player_stop(player); is_paused = true;
                        } else if (player) {
                            fluid_player_play(player); is_paused = false;
                        }
                    } else if (current_engine == ENGINE_ADLMIDI && p_adl_pause && adl_player) {
                        if (is_paused) { p_adl_pause(adl_player, 0); is_paused = false; }
                        else           { p_adl_pause(adl_player, 1); is_paused = true; }
                    } else if (current_engine == ENGINE_OPNMIDI && p_opn_pause && opn_player) {
                        if (is_paused) { p_opn_pause(opn_player, 0); is_paused = false; }
                        else           { p_opn_pause(opn_player, 1); is_paused = true; }
                    }
                }
                else if (key == SDLK_DOWN) next_track(window);
                else if (key == SDLK_UP)   prev_track(window);
                else if (key == SDLK_l) {
                    g_lang = (g_lang == LANG_EN) ? LANG_RU : LANG_EN;
                }
                else if (key == SDLK_1) {
                    if (current_engine != ENGINE_FLUIDSYNTH) {
                        init_engine(ENGINE_FLUIDSYNTH);
                        play_track(window, current_track_idx);
                    }
                }
                else if (key == SDLK_2) {
                    if (g_adl_available && current_engine != ENGINE_ADLMIDI) {
                        init_engine(ENGINE_ADLMIDI);
                        play_track(window, current_track_idx);
                    }
                }
                else if (key == SDLK_3) {
                    if (g_opn_available && current_engine != ENGINE_OPNMIDI) {
                        init_engine(ENGINE_OPNMIDI);
                        play_track(window, current_track_idx);
                    }
                }
                else if (key == SDLK_RIGHT || key == SDLK_LEFT) {
                    if (current_engine == ENGINE_FLUIDSYNTH && !soundfonts.empty()) {
                        int n = (int)soundfonts.size();
                        int old_idx  = current_sf_idx;
                        int old_sfid = active_sf_id;
                        if (key == SDLK_RIGHT) current_sf_idx = (current_sf_idx + 1) % n;
                        else                   current_sf_idx = (current_sf_idx - 1 + n) % n;

                        if (g_audio_dev) SDL_LockAudioDevice(g_audio_dev);
                        std::string sf_path = g_temp_dir + "/" + soundfonts[current_sf_idx];
                        int new_id = fluid_synth_sfload(synth, sf_path.c_str(), 1);
                        if (new_id == FLUID_FAILED) {
                            current_sf_idx = old_idx;
                            active_sf_id   = old_sfid;
                        } else {
                            if (old_sfid != -1) fluid_synth_sfunload(synth, old_sfid, 1);
                            active_sf_id = new_id;
                        }
                        if (g_audio_dev) SDL_UnlockAudioDevice(g_audio_dev);
                    }
                    else if (current_engine == ENGINE_ADLMIDI && !adl_banks.empty() && g_adl_available) {
                        int n = (int)adl_banks.size();
                        if (key == SDLK_RIGHT) g_adl_current_bank = (g_adl_current_bank + 1) % n;
                        else                   g_adl_current_bank = (g_adl_current_bank - 1 + n) % n;

                        if (g_audio_dev) SDL_LockAudioDevice(g_audio_dev);
                        if (adl_player && p_adl_openBankFile) {
                            std::string bank_path = g_temp_dir + "/" + adl_banks[g_adl_current_bank];
                            p_adl_openBankFile(adl_player, bank_path.c_str());
                        }
                        if (g_audio_dev) SDL_UnlockAudioDevice(g_audio_dev);
                    }
                    else if (current_engine == ENGINE_OPNMIDI && !opn_banks.empty() && g_opn_available) {
                        int n = (int)opn_banks.size();
                        if (key == SDLK_RIGHT) g_opn_current_bank = (g_opn_current_bank + 1) % n;
                        else                   g_opn_current_bank = (g_opn_current_bank - 1 + n) % n;

                        if (g_audio_dev) SDL_LockAudioDevice(g_audio_dev);
                        if (opn_player && p_opn_openBankFile) {
                            std::string bank_path = g_temp_dir + "/" + opn_banks[g_opn_current_bank];
                            p_opn_openBankFile(opn_player, bank_path.c_str());
                        }
                        if (g_audio_dev) SDL_UnlockAudioDevice(g_audio_dev);
                    }
                }
            }
        }

        if (current_engine == ENGINE_FLUIDSYNTH && player) {
            int cur = fluid_player_get_status(player);
            if (!is_paused && cur == FLUID_PLAYER_DONE && prev_status != FLUID_PLAYER_DONE) {
                SDL_Delay(50);
                next_track(window);
            }
            prev_status = cur;
        }

        bool playing = !is_paused;
        update_equalizer(playing);
        draw_interface(renderer, SDL_GetTicks());

        next_frame += 16;
        uint32_t now = SDL_GetTicks();
        if (next_frame > now) SDL_Delay(next_frame - now);
        else                  next_frame = now;
    }

    saveSettings();

    if (g_audio_dev) {
        SDL_PauseAudioDevice(g_audio_dev, 1);
        SDL_CloseAudioDevice(g_audio_dev);
        g_audio_dev = 0;
    }
    shutdown_engines();
    unload_midi_dlls();

    for (auto& kv : g_glyph_cache) if (kv.second) SDL_DestroyTexture(kv.second);
    g_glyph_cache.clear();
    if (g_bg_tex) SDL_DestroyTexture(g_bg_tex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    std::error_code ec;
    fs::remove_all(g_temp_dir, ec);

    SDL_Quit();
    return 0;
}
