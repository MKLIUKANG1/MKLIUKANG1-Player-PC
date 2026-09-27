// vgm_export.h — экспорт треков в VGM
#pragma once
#include <string>

void vgm_init();

// libOPNMIDI midi2vgm — OPN2 (YM2612)
bool vgm_opn2_available();

// bakemono mid2vgm — OPL3 / OPL2 / YM2151 / OPLL
bool vgm_adl_available();

std::string vgm_save_dialog(const std::string& suggestedName);

// chip:
//   0 = OPN2 (YM2612)   — midi2vgm_opn2.exe
//   1 = OPL3 (YMF262)   — mid2vgm_adl.exe -8   (ADL)
//   2 = OPL2 (YM3812)   — mid2vgm_adl.exe -10  (ADL)
//   3 = YM2151          — mid2vgm_adl.exe -0   (Fluid)
bool vgm_export(const std::string& midiPath,
                const std::string& outputPath,
                int chip,
                std::string& errorMessage);
