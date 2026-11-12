#pragma once
// IDs de parámetros y presets de fábrica de La Pauta SL Channel.
#include <utility>
#include <vector>

namespace ID
{
    constexpr auto inGain = "inGain";     constexpr auto outGain = "outGain";
    constexpr auto phase  = "phase";      constexpr auto analog  = "analog";

    constexpr auto hpfOn = "hpfOn";       constexpr auto hpf = "hpf";
    constexpr auto lpfOn = "lpfOn";       constexpr auto lpf = "lpf";

    constexpr auto hfGain  = "hfGain";    constexpr auto hfFreq  = "hfFreq";  constexpr auto hfBell = "hfBell";
    constexpr auto hmfGain = "hmfGain";   constexpr auto hmfFreq = "hmfFreq"; constexpr auto hmfQ = "hmfQ"; constexpr auto hmfX3 = "hmfX3";
    constexpr auto lmfGain = "lmfGain";   constexpr auto lmfFreq = "lmfFreq"; constexpr auto lmfQ = "lmfQ"; constexpr auto lmfD3 = "lmfD3";
    constexpr auto lfGain  = "lfGain";    constexpr auto lfFreq  = "lfFreq";  constexpr auto lfBell = "lfBell";

    constexpr auto eqByp = "eqByp";       constexpr auto eqSc = "eqSc";       // eqSc = FLT/EQ -> sidechain dinámica
    constexpr auto dynByp = "dynByp";     constexpr auto dynPost = "dynPost"; // dynPost = CH OUT (dinámica después del EQ)

    constexpr auto compThr = "compThr";   constexpr auto compRatio = "compRatio";
    constexpr auto compRel = "compRel";   constexpr auto fastAtk = "fastAtk";

    constexpr auto gateThr = "gateThr";   constexpr auto gateRange = "gateRange";
    constexpr auto gateRel = "gateRel";   constexpr auto gateMode = "gateMode";
}

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;   // sobre los valores por defecto
};

// Puntos de partida: ajusta siempre a oído según la fuente.
inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        { "Init", {} },

        { "Vocal - Lead Presence", {
            { ID::hpfOn, 1 }, { ID::hpf, 90 },
            { ID::lmfGain, -3 }, { ID::lmfFreq, 400 }, { ID::lmfQ, 1.5f },
            { ID::hmfGain, 2.5f }, { ID::hmfFreq, 4500 }, { ID::hmfQ, 1.2f },
            { ID::hfGain, 3 }, { ID::hfFreq, 10000 },
            { ID::compThr, -8 }, { ID::compRatio, 4 }, { ID::compRel, 0.3f } } },

        { "Vocal - Air & Smooth", {
            { ID::hpfOn, 1 }, { ID::hpf, 80 },
            { ID::lmfGain, -2 }, { ID::lmfFreq, 300 },
            { ID::hmfGain, 1.5f }, { ID::hmfFreq, 3000 },
            { ID::hfGain, 4 }, { ID::hfFreq, 12000 },
            { ID::compThr, -6 }, { ID::compRatio, 3 }, { ID::compRel, 0.5f }, { ID::analog, 1 } } },

        { "Kick - Punch", {
            { ID::hpfOn, 1 }, { ID::hpf, 30 },
            { ID::lfGain, 4 }, { ID::lfFreq, 60 }, { ID::lfBell, 1 },
            { ID::lmfGain, -5 }, { ID::lmfFreq, 350 }, { ID::lmfQ, 1.5f },
            { ID::hmfGain, 3 }, { ID::hmfFreq, 4000 },
            { ID::compThr, -6 }, { ID::compRatio, 4 }, { ID::compRel, 0.2f },
            { ID::gateThr, -20 }, { ID::gateRange, 30 }, { ID::gateRel, 0.2f }, { ID::gateMode, 1 } } },

        { "Snare - Crack", {
            { ID::hpfOn, 1 }, { ID::hpf, 100 },
            { ID::lfGain, 2 }, { ID::lfFreq, 150 },
            { ID::lmfGain, -3 }, { ID::lmfFreq, 500 },
            { ID::hmfGain, 4 }, { ID::hmfFreq, 5000 },
            { ID::hfGain, 2 }, { ID::hfFreq, 10000 },
            { ID::compThr, -8 }, { ID::compRatio, 4 }, { ID::compRel, 0.2f },
            { ID::gateThr, -15 }, { ID::gateRange, 25 }, { ID::gateRel, 0.15f }, { ID::gateMode, 1 } } },

        { "Bass - Solid", {
            { ID::hpfOn, 1 }, { ID::hpf, 30 },
            { ID::lfGain, 3 }, { ID::lfFreq, 80 },
            { ID::lmfGain, -3 }, { ID::lmfFreq, 250 },
            { ID::hmfGain, 2.5f }, { ID::hmfFreq, 1500 },
            { ID::compThr, -10 }, { ID::compRatio, 4 }, { ID::compRel, 0.4f } } },

        { "Acoustic Guitar - Bright", {
            { ID::hpfOn, 1 }, { ID::hpf, 80 },
            { ID::lfGain, -2 }, { ID::lfFreq, 120 },
            { ID::lmfGain, -2 }, { ID::lmfFreq, 300 },
            { ID::hmfGain, 2 }, { ID::hmfFreq, 3500 },
            { ID::hfGain, 3 }, { ID::hfFreq, 12000 },
            { ID::compThr, -6 }, { ID::compRatio, 2 }, { ID::compRel, 0.4f } } },

        { "Drum Bus - Glue", {
            { ID::hpfOn, 1 }, { ID::hpf, 30 },
            { ID::lfGain, 2 }, { ID::lfFreq, 80 },
            { ID::hfGain, 2 }, { ID::hfFreq, 10000 },
            { ID::compThr, -6 }, { ID::compRatio, 4 }, { ID::compRel, 0.1f }, { ID::analog, 1 } } },

        { "Mix Bus - Glue", {
            { ID::lfGain, 1 }, { ID::lfFreq, 60 },
            { ID::hfGain, 1.5f }, { ID::hfFreq, 14000 },
            { ID::compThr, -4 }, { ID::compRatio, 2 }, { ID::compRel, 0.4f }, { ID::analog, 1 } } },

        { "Piano - Natural", {
            { ID::hpfOn, 1 }, { ID::hpf, 40 },
            { ID::lmfGain, -2 }, { ID::lmfFreq, 400 },
            { ID::hfGain, 2 }, { ID::hfFreq, 12000 },
            { ID::compThr, -3 }, { ID::compRatio, 2 }, { ID::compRel, 0.8f } } },

        { "Toms / Room - Tight Gate", {
            { ID::hpfOn, 1 }, { ID::hpf, 60 },
            { ID::lmfGain, -4 }, { ID::lmfFreq, 300 },
            { ID::compThr, -6 }, { ID::compRatio, 3 }, { ID::compRel, 0.3f },
            { ID::gateThr, -22 }, { ID::gateRange, 30 }, { ID::gateRel, 0.3f }, { ID::gateMode, 1 } } },

        { "Cleanup - Rumble & Hiss", {
            { ID::hpfOn, 1 }, { ID::hpf, 120 },
            { ID::lpfOn, 1 }, { ID::lpf, 10000 },
            { ID::lmfGain, -3 }, { ID::lmfFreq, 250 } } },
    };
    return presets;
}
