// vgm_export.cpp — экспорт в VGM
//
// OPN2 (chip=0)  → midi2vgm_opn2.exe (libOPNMIDI)
//   Синтаксис: --chips 2 xg.wopn input.mid
//
// OPL3/OPL2/YM2151 (chip=1/2/3) → mid2vgm_adl.exe (bakemono)
//   Синтаксис: mid2vgm.exe -N in.mid out.vgm
//   ISC-инструменты в папке isc/ рядом с exe
#include "vgm_export.h"

#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>

namespace fs = std::filesystem;

static std::string g_path_opn2;
static std::string g_path_adl;
static bool g_found_opn2 = false;
static bool g_found_adl  = false;

static std::string getExeDir() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
}

static std::string getTempDir() {
    char buf[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, buf);
    return std::string(buf);
}

void vgm_init() {
    std::string dir = getExeDir();

    const char* opn2_names[] = { "midi2vgm_opn2.exe", "midi2vgm.exe" };
    for (const char* n : opn2_names) {
        fs::path p = fs::path(dir) / n;
        if (fs::exists(p)) { g_path_opn2 = p.string(); g_found_opn2 = true; break; }
    }

    const char* adl_names[] = { "mid2vgm_adl.exe", "mid2vgm.exe" };
    for (const char* n : adl_names) {
        fs::path p = fs::path(dir) / n;
        if (fs::exists(p)) { g_path_adl = p.string(); g_found_adl = true; break; }
    }

    fs::path iscDir = fs::path(dir) / "isc";
    bool isc_ok = fs::exists(iscDir);

    std::cout << "[VGM] OPN2 tool: " << (g_found_opn2 ? g_path_opn2 : "(not found)") << "\n";
    std::cout << "[VGM] ADL tool:  " << (g_found_adl  ? g_path_adl  : "(not found)") << "\n";
    std::cout << "[VGM] ISC dir:   " << (isc_ok ? iscDir.string() : "(not found)") << "\n";
}

bool vgm_opn2_available() { return g_found_opn2; }
bool vgm_adl_available()  { return g_found_adl;  }

std::string vgm_save_dialog(const std::string& suggestedName) {
    char filename[MAX_PATH] = {};
    strncpy(filename, suggestedName.c_str(), MAX_PATH - 1);

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "VGM files\0*.vgm;*.vgz\0All Files\0*.*\0\0";
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = "Save VGM";
    ofn.lpstrDefExt = "vgm";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameA(&ofn)) return "";
    return std::string(filename);
}

static bool runProcess(const std::string& exe,
                       const std::string& args,
                       const std::string& workDir,
                       std::string& errorMessage) {
    std::string cmdLine = "\"" + exe + "\" " + args;
    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    const char* wd = workDir.empty() ? nullptr : workDir.c_str();

    BOOL ok = CreateProcessA(
        exe.c_str(), cmdBuf.data(),
        nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, wd,
        &si, &pi);

    if (!ok) {
        errorMessage = "CreateProcess failed, code " + std::to_string(GetLastError());
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) {
        errorMessage = "Converter exited with code " + std::to_string(exitCode);
        return false;
    }
    return true;
}

// ----------------------------------------------------------------
// OPN2 через libOPNMIDI midi2vgm
// ----------------------------------------------------------------
static bool vgm_export_opn2(const std::string& midiPath,
                            const std::string& outputPath,
                            std::string& errorMessage) {
    bool gzip = false;
    {
        std::string low = outputPath;
        for (auto& c : low) c = (char)tolower((unsigned char)c);
        if (low.size() >= 4 && low.substr(low.size() - 4) == ".vgz") gzip = true;
    }

    std::string tmpBase = getTempDir() + "MKLK1Player_vgm_opn2_" +
                          std::to_string(GetCurrentProcessId());
    std::error_code ec;
    fs::create_directories(tmpBase, ec);
    if (ec) { errorMessage = "Cannot create temp dir"; return false; }

    fs::path bankSrc = fs::path(getExeDir()) / "xg.wopn";
    fs::path bankDst = fs::path(tmpBase) / "xg.wopn";
    if (!fs::exists(bankSrc)) {
        errorMessage = "xg.wopn not found next to the player";
        fs::remove_all(tmpBase, ec);
        return false;
    }
    try {
        fs::copy_file(bankSrc, bankDst, fs::copy_options::overwrite_existing);
    } catch (std::exception& e) {
        errorMessage = std::string("Copy bank failed: ") + e.what();
        fs::remove_all(tmpBase, ec);
        return false;
    }

    fs::path inMidi = fs::path(tmpBase) / "input.mid";
    try {
        fs::copy_file(midiPath, inMidi, fs::copy_options::overwrite_existing);
    } catch (std::exception& e) {
        errorMessage = std::string("Copy MIDI failed: ") + e.what();
        fs::remove_all(tmpBase, ec);
        return false;
    }

    std::string args = "--chips 2";
    if (gzip) args += " -z";
    args += " xg.wopn input.mid";

    std::string err;
    if (!runProcess(g_path_opn2, args, tmpBase, err)) {
        errorMessage = err;
        fs::remove_all(tmpBase, ec);
        return false;
    }

    fs::path result;
    fs::path outVgz = fs::path(tmpBase) / "input.mid.vgz";
    fs::path outVgm = fs::path(tmpBase) / "input.mid.vgm";
    if      (fs::exists(outVgz)) result = outVgz;
    else if (fs::exists(outVgm)) result = outVgm;
    else {
        errorMessage = "midi2vgm did not produce an output file";
        fs::remove_all(tmpBase, ec);
        return false;
    }

    try {
        fs::copy_file(result, outputPath, fs::copy_options::overwrite_existing);
    } catch (std::exception& e) {
        errorMessage = std::string("Copy VGM failed: ") + e.what();
        fs::remove_all(tmpBase, ec);
        return false;
    }

    fs::remove_all(tmpBase, ec);
    return true;
}

// ----------------------------------------------------------------
// ADL через bakemono mid2vgm
// chipFlag: "-8" = OPL3, "-10" = OPL2, "-0" = YM2151
// ----------------------------------------------------------------
static bool vgm_export_adl(const std::string& midiPath,
                           const std::string& outputPath,
                           const std::string& chipFlag,
                           std::string& errorMessage) {
    std::string tmpBase = getTempDir() + "MKLK1Player_vgm_adl_" +
                          std::to_string(GetCurrentProcessId());
    std::error_code ec;
    fs::create_directories(tmpBase, ec);
    if (ec) { errorMessage = "Cannot create temp dir"; return false; }

    fs::path inMidi  = fs::path(tmpBase) / "in.mid";
    fs::path outVgm  = fs::path(tmpBase) / "out.vgm";

    try {
        fs::copy_file(midiPath, inMidi, fs::copy_options::overwrite_existing);
    } catch (std::exception& e) {
        errorMessage = std::string("Copy MIDI failed: ") + e.what();
        fs::remove_all(tmpBase, ec);
        return false;
    }

    // Флаг чипа передаётся СТРОКОЙ: "-8", "-10", "-0"
    std::string args = chipFlag + " in.mid out.vgm";

    std::string err;
    if (!runProcess(g_path_adl, args, tmpBase, err)) {
        errorMessage = err;
        fs::remove_all(tmpBase, ec);
        return false;
    }

    if (!fs::exists(outVgm)) {
        errorMessage = "mid2vgm did not produce out.vgm";
        fs::remove_all(tmpBase, ec);
        return false;
    }

    try {
        fs::copy_file(outVgm, outputPath, fs::copy_options::overwrite_existing);
    } catch (std::exception& e) {
        errorMessage = std::string("Copy VGM failed: ") + e.what();
        fs::remove_all(tmpBase, ec);
        return false;
    }

    fs::remove_all(tmpBase, ec);
    return true;
}

bool vgm_export(const std::string& midiPath,
                const std::string& outputPath,
                int chip,
                std::string& errorMessage) {
    if (chip == 0) {
        if (!g_found_opn2) { errorMessage = "midi2vgm_opn2.exe not found"; return false; }
        return vgm_export_opn2(midiPath, outputPath, errorMessage);
    }

    if (!g_found_adl) { errorMessage = "mid2vgm_adl.exe not found"; return false; }

    std::string chipFlag;
    if      (chip == 1) chipFlag = "-8";    // OPL3
    else if (chip == 2) chipFlag = "-10";   // OPL2
    else if (chip == 3) chipFlag = "-0";    // YM2151
    else { errorMessage = "Unknown chip"; return false; }

    return vgm_export_adl(midiPath, outputPath, chipFlag, errorMessage);
}
