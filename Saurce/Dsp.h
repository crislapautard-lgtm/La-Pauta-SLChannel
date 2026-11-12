#pragma once
// DSP puro (sin dependencias de JUCE): biquads, cadena de EQ y dinámica.
#include <algorithm>
#include <cmath>

namespace lp
{
constexpr double kPi = 3.14159265358979323846;

inline float toDb (float x) noexcept   { return 20.f * std::log10 (std::max (x, 1.0e-6f)); }
inline float fromDb (float d) noexcept { return std::pow (10.f, d / 20.f); }

//==============================================================================
struct Coefs { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

inline Coefs norm (double b0, double b1, double b2, double a0, double a1, double a2)
{
    return { (float) (b0 / a0), (float) (b1 / a0), (float) (b2 / a0),
             (float) (a1 / a0), (float) (a2 / a0) };
}

inline double limitF (double f, double fs) { return std::min (std::max (f, 10.0), fs * 0.45); }

inline Coefs makeHP1 (double fs, double f)
{
    const double K = std::tan (kPi * limitF (f, fs) / fs);
    return norm (1, -1, 0, K + 1, K - 1, 0);
}

inline Coefs makeHP2 (double fs, double f, double q)
{
    const double w = 2 * kPi * limitF (f, fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
    return norm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
}

inline Coefs makeLP2 (double fs, double f, double q)
{
    const double w = 2 * kPi * limitF (f, fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
    return norm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
}

inline Coefs makePeak (double fs, double f, double q, double dB)
{
    const double A = std::pow (10.0, dB / 40.0);
    const double w = 2 * kPi * limitF (f, fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
    return norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
}

inline Coefs makeLowShelf (double fs, double f, double q, double dB)
{
    const double A = std::pow (10.0, dB / 40.0);
    const double w = 2 * kPi * limitF (f, fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
    const double t = 2 * std::sqrt (A) * al;
    return norm (A * ((A + 1) - (A - 1) * c + t),
                 2 * A * ((A - 1) - (A + 1) * c),
                 A * ((A + 1) - (A - 1) * c - t),
                 (A + 1) + (A - 1) * c + t,
                 -2 * ((A - 1) + (A + 1) * c),
                 (A + 1) + (A - 1) * c - t);
}

inline Coefs makeHighShelf (double fs, double f, double q, double dB)
{
    const double A = std::pow (10.0, dB / 40.0);
    const double w = 2 * kPi * limitF (f, fs) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
    const double t = 2 * std::sqrt (A) * al;
    return norm (A * ((A + 1) + (A - 1) * c + t),
                 -2 * A * ((A - 1) + (A + 1) * c),
                 A * ((A + 1) + (A - 1) * c - t),
                 (A + 1) - (A - 1) * c + t,
                 2 * ((A - 1) - (A + 1) * c),
                 (A + 1) - (A - 1) * c - t);
}

struct Biquad
{
    Coefs c;
    float z1 = 0, z2 = 0;

    inline float process (float x) noexcept   // transposed direct form II
    {
        const float y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

//==============================================================================
struct EqParams
{
    bool  hpfOn = false, lpfOn = false;
    float hpf = 20.f, lpf = 12000.f;
    float hfG = 0.f,  hfF = 8000.f;  bool hfBell = false;
    float hmfG = 0.f, hmfF = 3000.f, hmfQ = 1.f; bool hmfX3 = false;
    float lmfG = 0.f, lmfF = 600.f,  lmfQ = 1.f; bool lmfD3 = false;
    float lfG = 0.f,  lfF = 100.f;   bool lfBell = false;
};

struct EqChain
{
    Biquad hp1[2], hp2[2], lpf[2], hf[2], hmf[2], lmf[2], lf[2];
    EqParams p;

    void reset()
    {
        for (int c = 0; c < 2; ++c)
            hp1[c].reset(), hp2[c].reset(), lpf[c].reset(), hf[c].reset(),
            hmf[c].reset(), lmf[c].reset(), lf[c].reset();
    }

    void setup (const EqParams& e, double fs)
    {
        p = e;
        const auto cHp1 = makeHP1 (fs, e.hpf);
        const auto cHp2 = makeHP2 (fs, e.hpf, 0.7071);        // HPF total: 18 dB/oct
        const auto cLp  = makeLP2 (fs, e.lpf, 0.7071);        // LPF: 12 dB/oct
        const auto cHf  = e.hfBell ? makePeak (fs, e.hfF, 0.8, e.hfG)
                                   : makeHighShelf (fs, e.hfF, 0.8, e.hfG);
        const auto cHmf = makePeak (fs, e.hmfF * (e.hmfX3 ? 3.0 : 1.0), e.hmfQ, e.hmfG);
        const auto cLmf = makePeak (fs, e.lmfF / (e.lmfD3 ? 3.0 : 1.0), e.lmfQ, e.lmfG);
        const auto cLf  = e.lfBell ? makePeak (fs, e.lfF, 0.8, e.lfG)
                                   : makeLowShelf (fs, e.lfF, 0.8, e.lfG);

        for (int c = 0; c < 2; ++c)
        {
            hp1[c].c = cHp1; hp2[c].c = cHp2; lpf[c].c = cLp;
            hf[c].c = cHf;   hmf[c].c = cHmf; lmf[c].c = cLmf; lf[c].c = cLf;
        }
    }

    inline float process (int ch, float x) noexcept
    {
        if (p.hpfOn) { x = hp1[ch].process (x); x = hp2[ch].process (x); }
        if (p.lpfOn)   x = lpf[ch].process (x);
        x = lf[ch].process (x);
        x = lmf[ch].process (x);
        x = hmf[ch].process (x);
        x = hf[ch].process (x);
        return x;
    }
};

//==============================================================================
struct DynParams
{
    float compThr = 10.f, compRatio = 1.f, compRelS = 0.4f;
    bool  fastAtk = false;
    float gateThr = -30.f, gateRange = 0.f, gateRelS = 0.1f;
    bool  gateIsGate = false;   // false = expansor 2:1, true = gate
};

struct Dynamics
{
    double fs = 44100.0;
    float compGr = 0.f, gateGr = 0.f, gateLvl = 0.f;   // dB de reducción (positivos)

    void prepare (double sr) { fs = sr; reset(); }
    void reset() { compGr = gateGr = gateLvl = 0.f; }

    float tc (float ms) const { return std::exp (-1.f / (0.001f * ms * (float) fs)); }

    // x: audio (se modifica). sc: señal del detector (puede ser la misma que x).
    void process (float* const* x, const float* const* sc, int nch, int n, const DynParams& p)
    {
        const float atk       = tc (p.fastAtk ? 0.1f : 3.f);
        const float rel       = tc (std::max (p.compRelS, 0.01f) * 1000.f);
        const float gateClose = tc (std::max (p.gateRelS, 0.01f) * 1000.f);
        const float gateOpen  = tc (0.5f);
        const float lvlDecay  = tc (20.f);
        const float ratio     = p.compRatio >= 19.5f ? 1000.f : std::max (p.compRatio, 1.f);
        const float slope     = 1.f - 1.f / ratio;
        const float knee      = 4.f;

        for (int i = 0; i < n; ++i)
        {
            float lvl = 0.f;
            for (int c = 0; c < nch; ++c)
                lvl = std::max (lvl, std::fabs (sc[c][i]));

            // --- Expansor / Gate ---
            gateLvl = lvl > gateLvl ? lvl : lvl + (gateLvl - lvl) * lvlDecay;
            float gTarget = 0.f;
            if (p.gateRange > 0.01f)
            {
                const float ldb = toDb (gateLvl);
                if (p.gateIsGate) gTarget = ldb < p.gateThr ? p.gateRange : 0.f;
                else              gTarget = std::min (p.gateRange, std::max (0.f, p.gateThr - ldb));
            }
            const float gc = gTarget > gateGr ? gateClose : gateOpen;
            gateGr = gTarget + (gateGr - gTarget) * gc;

            // --- Compresor (detector después del gate) ---
            const float cdb  = toDb (lvl * fromDb (-gateGr));
            const float over = cdb - p.compThr;
            float red = 0.f;
            if (over > -knee * 0.5f)
            {
                if (over < knee * 0.5f)
                {
                    const float t = over + knee * 0.5f;
                    red = slope * t * t / (2.f * knee);
                }
                else
                    red = slope * over;
            }
            const float cc = red > compGr ? atk : rel;
            compGr = red + (compGr - red) * cc;

            const float g = fromDb (-(compGr + gateGr));
            for (int c = 0; c < nch; ++c)
                x[c][i] *= g;
        }
    }
};
} // namespace lp
