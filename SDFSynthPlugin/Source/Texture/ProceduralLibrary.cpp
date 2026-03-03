#include "ProceduralLibrary.h"
#include <cmath>
#include <algorithm>

juce::StringArray ProceduralLibrary::getPresetNames()
{
    return { "Checkerboard", "Brick", "Brushed Metal", "Concrete",
             "Organic", "Marble", "Lava", "Circuit Board",
             "Wood Grain", "Crystal", "Rust", "Scales",
             "Plasma", "Ice", "Obsidian", "Alien",
             "Nebula", "Aurora" };
}

juce::StringArray ProceduralLibrary::getPresetKeys()
{
    return { "checkerboard", "brick", "metal", "concrete",
             "organic", "marble", "lava", "circuit",
             "wood", "crystal", "rust", "scales",
             "plasma", "ice", "obsidian", "alien",
             "nebula", "aurora" };
}

float ProceduralLibrary::seededRand(float x, float y)
{
    float n = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
    return n - std::floor(n);
}

float ProceduralLibrary::noise2D(float x, float y)
{
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix, fy = y - iy;
    auto sf = [](float t) { return t * t * (3.f - 2.f * t); };
    float a = seededRand(ix, iy), b = seededRand(ix + 1, iy);
    float c = seededRand(ix, iy + 1), d = seededRand(ix + 1, iy + 1);
    return a + (b - a) * sf(fx) + (c - a) * sf(fy) + (a - b - c + d) * sf(fx) * sf(fy);
}

float ProceduralLibrary::fbm(float x, float y, int octaves)
{
    float val = 0.f, amp = 0.5f, freq = 1.f;
    for (int i = 0; i < octaves; ++i)
    {
        val += amp * noise2D(x * freq, y * freq);
        freq *= 2.f;
        amp *= 0.5f;
    }
    return val;
}

juce::Image ProceduralLibrary::normalFromDisp(const juce::Image& disp)
{
    int sz = disp.getWidth();
    juce::Image normal(juce::Image::ARGB, sz, sz, true);
    juce::Image::BitmapData src(disp, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst(normal, juce::Image::BitmapData::writeOnly);

    // Helper to sample with wrapping
    auto sample = [&](int x, int y) -> float {
        return src.getPixelColour((x + sz) % sz, (y + sz) % sz).getRed() / 255.f;
    };

    for (int y = 0; y < sz; ++y)
    {
        for (int x = 0; x < sz; ++x)
        {
            // Scharr 3x3 operator (weighted, 8-neighbor sampling)
            // Horizontal gradient (Gx): [-3 0 3; -10 0 10; -3 0 3] / 32
            float gx = (-3.f * sample(x-1, y-1) + 3.f * sample(x+1, y-1)
                       -10.f * sample(x-1, y)   +10.f * sample(x+1, y)
                        -3.f * sample(x-1, y+1) + 3.f * sample(x+1, y+1)) / 32.f;
            // Vertical gradient (Gy): [-3 -10 -3; 0 0 0; 3 10 3] / 32
            float gy = (-3.f * sample(x-1, y-1) -10.f * sample(x, y-1) - 3.f * sample(x+1, y-1)
                        +3.f * sample(x-1, y+1) +10.f * sample(x, y+1) + 3.f * sample(x+1, y+1)) / 32.f;
            float nz = 1.f;
            float len = std::sqrt(gx * gx + gy * gy + nz * nz);
            gx /= len; gy /= len; nz /= len;
            dst.setPixelColour(x, y, juce::Colour::fromRGBA(
                static_cast<uint8_t>((gx * 0.5f + 0.5f) * 255),
                static_cast<uint8_t>((gy * 0.5f + 0.5f) * 255),
                static_cast<uint8_t>((nz * 0.5f + 0.5f) * 255),
                255));
        }
    }
    return normal;
}

ProceduralTexture ProceduralLibrary::generate(const juce::String& key, int size)
{
    if (key == "checkerboard") return generateCheckerboard(size);
    if (key == "brick")        return generateBrick(size);
    if (key == "metal")        return generateMetal(size);
    if (key == "concrete")     return generateConcrete(size);
    if (key == "organic")      return generateOrganic(size);
    if (key == "marble")       return generateMarble(size);
    if (key == "lava")         return generateLava(size);
    if (key == "circuit")      return generateCircuit(size);
    if (key == "wood")         return generateWood(size);
    if (key == "crystal")      return generateCrystal(size);
    if (key == "rust")         return generateRust(size);
    if (key == "scales")       return generateScales(size);
    if (key == "plasma")       return generatePlasma(size);
    if (key == "ice")          return generateIce(size);
    if (key == "obsidian")     return generateObsidian(size);
    if (key == "alien")        return generateAlien(size);
    if (key == "nebula")       return generateNebula(size);
    if (key == "aurora")       return generateAurora(size);
    return generateCheckerboard(size);
}

ProceduralTexture ProceduralLibrary::generateCheckerboard(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    int g = 8, cs = sz / g;
    for (int y = 0; y < sz; ++y)
    {
        for (int x = 0; x < sz; ++x)
        {
            bool white = ((x / cs) + (y / cs)) % 2 == 0;
            uint8_t cv = white ? 0xdd : 0x33;
            tex.colorMap.setPixelAt(x, y, juce::Colour(cv, cv, cv));
            uint8_t dv = white ? 200 : 40;
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = white ? 180 : 60;
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));

            // AO: dark at tile edges
            bool edge = (x % cs < 3) || (y % cs < 3);
            uint8_t ao = edge ? 0x99 : 0xff;
            tex.aoMap.setPixelAt(x, y, juce::Colour(ao, ao, ao));

            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateBrick(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    int bw = sz / 4, bh = sz / 8;
    float gap = sz * 0.012f;

    // Fill with mortar
    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            tex.colorMap.setPixelAt(x, y, juce::Colour(0x8a, 0x8a, 0x82));
            tex.dispMap.setPixelAt(x, y, juce::Colour(0x44, 0x44, 0x44));
            tex.roughMap.setPixelAt(x, y, juce::Colour(0xb0, 0xb0, 0xb0));
            tex.aoMap.setPixelAt(x, y, juce::Colour(0x77, 0x77, 0x77));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }

    // Draw bricks
    for (int r = 0; r < 8; ++r)
    {
        int off = (r % 2) ? bw / 2 : 0;
        for (int c = -1; c < 5; ++c)
        {
            int bx0 = c * bw + off + static_cast<int>(gap);
            int by0 = r * bh + static_cast<int>(gap);
            int bx1 = bx0 + bw - static_cast<int>(gap * 2);
            int by1 = by0 + bh - static_cast<int>(gap * 2);

            for (int y = by0; y < by1; ++y)
                for (int x = bx0; x < bx1; ++x)
                {
                    if (x < 0 || x >= sz || y < 0 || y >= sz) continue;
                    int rv = 155 + static_cast<int>((seededRand(static_cast<float>(x), static_cast<float>(y)) - 0.5f) * 30);
                    int gv = 72 + static_cast<int>((seededRand(static_cast<float>(x + 50), static_cast<float>(y)) - 0.5f) * 30);
                    int bv = 52 + static_cast<int>((seededRand(static_cast<float>(x), static_cast<float>(y + 50)) - 0.5f) * 30);
                    tex.colorMap.setPixelAt(x, y, juce::Colour(
                        static_cast<uint8_t>(std::clamp(rv, 0, 255)),
                        static_cast<uint8_t>(std::clamp(gv, 0, 255)),
                        static_cast<uint8_t>(std::clamp(bv, 0, 255))));
                    tex.dispMap.setPixelAt(x, y, juce::Colour(0x99, 0x99, 0x99));
                    tex.roughMap.setPixelAt(x, y, juce::Colour(0x88, 0x88, 0x88));
                    tex.aoMap.setPixelAt(x, y, juce::Colour(0xdd, 0xdd, 0xdd));
                }
        }
    }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateMetal(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
    {
        float v = 170.f + std::sin(y * 0.5f) * 15.f + (seededRand(0, static_cast<float>(y)) - 0.5f) * 24.f;
        uint8_t cv = static_cast<uint8_t>(std::clamp(static_cast<int>(v), 0, 255));
        float dv = 120.f + std::sin(y * 0.3f) * 30.f + (seededRand(3, static_cast<float>(y)) - 0.5f) * 20.f;
        uint8_t dvu = static_cast<uint8_t>(std::clamp(static_cast<int>(dv), 0, 255));
        float rv = 30.f + seededRand(2, static_cast<float>(y)) * 40.f;
        uint8_t rvu = static_cast<uint8_t>(std::clamp(static_cast<int>(rv), 0, 255));

        for (int x = 0; x < sz; ++x)
        {
            tex.colorMap.setPixelAt(x, y, juce::Colour(cv, static_cast<uint8_t>(std::min(255, cv + 5)),
                                                        static_cast<uint8_t>(std::min(255, cv + 12))));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dvu, dvu, dvu));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rvu, rvu, rvu));
            tex.aoMap.setPixelAt(x, y, juce::Colour(0xee, 0xee, 0xee));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateConcrete(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float n = fbm(x / static_cast<float>(sz) * 6.f, y / static_cast<float>(sz) * 6.f);
            uint8_t cv = static_cast<uint8_t>(std::clamp(static_cast<int>(140 + n * 60), 0, 255));
            tex.colorMap.setPixelAt(x, y, juce::Colour(cv, static_cast<uint8_t>(std::max(0, cv - 3)),
                                                        static_cast<uint8_t>(std::max(0, cv - 8))));
            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(80 + n * 120), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(160 + n * 80), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(180 + n * 60), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateOrganic(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float n = fbm(u * 4, v * 4, 6);
            float n2 = fbm(u * 8 + 5, v * 8 + 5, 4);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(30 + n * 80), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(90 + n * 100 + n2 * 30), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(80 + n * 70), 0, 255))));
            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(50 + n * 180), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(100 + n * 120), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(160 + n * 80), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Emissive: subtle bioluminescent spots
            float glow = std::max(0.f, fbm(u * 12 + 7, v * 12 + 7, 3) - 0.55f) * 4.f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(glow * 20), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(glow * 200), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(glow * 120), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateMarble(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float n = fbm(u * 3, v * 3, 5);
            float v0 = std::sin(u * 4 + n * 4) * 0.5f + 0.5f;
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(200 + v0 * 55), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(195 + v0 * 50), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(190 + v0 * 45), 0, 255))));
            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(100 + v0 * 80), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(20 + v0 * 40), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(210 + v0 * 40), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateLava(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float n = fbm(u * 3, v * 3, 6);
            float hot = std::max(0.f, n * 2.f - 0.5f);

            // RidgeFBM crack network for defined lava flow channels
            float crack = ridgeFbm(u * 4.f, v * 4.f, 5);
            float crackIntensity = std::clamp(crack * 1.5f - 0.3f, 0.f, 1.f);

            // Darker cooled surface between cracks, brighter in cracks
            float combined = std::max(hot, crackIntensity * 0.8f);
            float cooled = (1.f - crackIntensity) * 0.6f;

            int cr = static_cast<int>(20 + cooled * 30 + combined * 215);
            int cg = static_cast<int>(5 + cooled * 8 + combined * 140);
            int cb = static_cast<int>(3 + cooled * 5 + combined * 20);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(cr, 0, 255)),
                static_cast<uint8_t>(std::clamp(cg, 0, 255)),
                static_cast<uint8_t>(std::clamp(cb, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(40 + n * 160 + crackIntensity * 60), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(200 - combined * 180), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(120 + combined * 130), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Brighter emissive in cracks
            float emitHot = std::max(hot, crackIntensity);
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitHot * 255), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitHot * emitHot * 200), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitHot * emitHot * emitHot * 50), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateCircuit(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    int g = 16, cs = sz / g;

    // Base fills
    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            tex.colorMap.setPixelAt(x, y, juce::Colour(0x0a, 0x1a, 0x12));
            tex.dispMap.setPixelAt(x, y, juce::Colour(0x88, 0x88, 0x88));
            tex.roughMap.setPixelAt(x, y, juce::Colour(0x22, 0x22, 0x22));
            tex.aoMap.setPixelAt(x, y, juce::Colour(0xcc, 0xcc, 0xcc));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }

    // Draw traces and components
    for (int gy = 0; gy < g; ++gy)
    {
        for (int gx = 0; gx < g; ++gx)
        {
            // Vertical traces
            if (seededRand(static_cast<float>(gx), static_cast<float>(gy)) > 0.5f)
            {
                int cx = gx * cs + cs / 2;
                // Wider bus lines for some traces
                int hw = (seededRand(static_cast<float>(gx + 50), static_cast<float>(gy + 50)) > 0.7f) ? 2 : 1;
                for (int y = gy * cs; y < (gy + 1) * cs; ++y)
                    for (int dx = -hw; dx <= hw; ++dx)
                        if (cx + dx >= 0 && cx + dx < sz && y >= 0 && y < sz)
                            tex.colorMap.setPixelAt(cx + dx, y, juce::Colour(0x1a, 0x6b, 0x3a));
            }

            // Horizontal traces
            if (seededRand(static_cast<float>(gx + 100), static_cast<float>(gy)) > 0.5f)
            {
                int cy = gy * cs + cs / 2;
                int hw = (seededRand(static_cast<float>(gx + 150), static_cast<float>(gy + 150)) > 0.7f) ? 2 : 1;
                for (int x = gx * cs; x < (gx + 1) * cs; ++x)
                    for (int dy = -hw; dy <= hw; ++dy)
                        if (x >= 0 && x < sz && cy + dy >= 0 && cy + dy < sz)
                            tex.colorMap.setPixelAt(x, cy + dy, juce::Colour(0x1a, 0x6b, 0x3a));
            }

            // Diagonal traces (new)
            if (seededRand(static_cast<float>(gx + 300), static_cast<float>(gy + 300)) > 0.75f)
            {
                int x0 = gx * cs, y0 = gy * cs;
                for (int i = 0; i < cs; ++i)
                {
                    int px = x0 + i, py = y0 + i;
                    if (px >= 0 && px < sz && py >= 0 && py < sz)
                        tex.colorMap.setPixelAt(px, py, juce::Colour(0x1a, 0x6b, 0x3a));
                    if (px + 1 < sz && py >= 0 && py < sz)
                        tex.colorMap.setPixelAt(px + 1, py, juce::Colour(0x1a, 0x6b, 0x3a));
                }
            }

            // LED dots with multi-color variety (green, red, blue)
            if (seededRand(static_cast<float>(gx), static_cast<float>(gy + 100)) > 0.78f)
            {
                int cx = gx * cs + cs / 2;
                int cy = gy * cs + cs / 2;
                float ledType = seededRand(static_cast<float>(gx + 400), static_cast<float>(gy + 400));
                uint8_t lr, lg, lb, er, eg, eb;
                if (ledType > 0.66f) // green
                {
                    lr = 0x3a; lg = 0xff; lb = 0x7a;
                    er = 0; eg = static_cast<uint8_t>(180 + static_cast<int>(seededRand(static_cast<float>(gx + 1), static_cast<float>(gy)) * 75)); eb = 0;
                }
                else if (ledType > 0.33f) // red
                {
                    lr = 0xff; lg = 0x3a; lb = 0x2a;
                    er = static_cast<uint8_t>(180 + static_cast<int>(seededRand(static_cast<float>(gx + 2), static_cast<float>(gy)) * 75)); eg = 0x20; eb = 0;
                }
                else // blue
                {
                    lr = 0x3a; lg = 0x7a; lb = 0xff;
                    er = 0; eg = 0x30; eb = static_cast<uint8_t>(180 + static_cast<int>(seededRand(static_cast<float>(gx + 3), static_cast<float>(gy)) * 75));
                }
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                    {
                        int px = cx + dx, py = cy + dy;
                        if (px >= 0 && px < sz && py >= 0 && py < sz)
                        {
                            tex.colorMap.setPixelAt(px, py, juce::Colour(lr, lg, lb));
                            tex.emitMap.setPixelAt(px, py, juce::Colour(er, eg, eb));
                        }
                    }
            }

            // Raised components
            if (seededRand(static_cast<float>(gx + 200), static_cast<float>(gy)) > 0.6f)
            {
                for (int y = gy * cs; y < (gy + 1) * cs; ++y)
                    for (int x = gx * cs; x < (gx + 1) * cs; ++x)
                        if (x >= 0 && x < sz && y >= 0 && y < sz)
                            tex.dispMap.setPixelAt(x, y, juce::Colour(0xbb, 0xbb, 0xbb));
            }
        }
    }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

// ============================================================
// New noise primitives
// ============================================================

void ProceduralLibrary::voronoi2D(float x, float y, float& f1, float& f2)
{
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix, fy = y - iy;
    f1 = 8.f; f2 = 8.f;
    for (int j = -1; j <= 1; ++j)
        for (int i = -1; i <= 1; ++i)
        {
            float cx = ix + i, cy = iy + j;
            float px = seededRand(cx, cy);
            float py = seededRand(cx + 71.f, cy + 37.f);
            float dx = (i + px) - fx, dy = (j + py) - fy;
            float d = dx * dx + dy * dy;
            if (d < f1) { f2 = f1; f1 = d; }
            else if (d < f2) { f2 = d; }
        }
    f1 = std::sqrt(f1);
    f2 = std::sqrt(f2);
}

float ProceduralLibrary::ridgeFbm(float x, float y, int octaves)
{
    float val = 0.f, amp = 0.5f, freq = 1.f, prev = 1.f;
    for (int i = 0; i < octaves; ++i)
    {
        float n = noise2D(x * freq, y * freq);
        n = 1.f - std::abs(n * 2.f - 1.f);
        n = n * n;
        val += n * amp * prev;
        prev = n;
        freq *= 2.f;
        amp *= 0.5f;
    }
    return val;
}

float ProceduralLibrary::warpedFbm(float x, float y, float warpAmt, int octaves)
{
    float qx = fbm(x, y, octaves);
    float qy = fbm(x + 5.2f, y + 1.3f, octaves);
    return fbm(x + warpAmt * qx, y + warpAmt * qy, octaves);
}

// ============================================================
// New texture generators
// ============================================================

ProceduralTexture ProceduralLibrary::generateWood(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float cx = u - 0.5f, cy = v - 0.5f;
            float dist = std::sqrt(cx * cx + cy * cy) * 12.f;
            float warp = fbm(u * 6, v * 6, 4) * 1.5f;
            float ring = std::sin(dist + warp) * 0.5f + 0.5f;
            float grain = noise2D(u * 40, v * 2) * 0.15f;

            float t = std::clamp(ring + grain, 0.f, 1.f);
            int r = static_cast<int>(160 + t * 65);
            int g = static_cast<int>(100 + t * 50);
            int b = static_cast<int>(50 + t * 30);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(90 + t * 80 + grain * 200), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(140 + ring * 80), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(200 + grain * 200), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateCrystal(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float uf = x / static_cast<float>(sz);
            float vf = y / static_cast<float>(sz);
            float u = uf * 5.f;
            float v = vf * 5.f;
            float f1, f2;
            voronoi2D(u, v, f1, f2);
            float edge = f2 - f1;
            float facet = f1;

            // More saturated blue-white palette
            float t = std::clamp(facet * 1.5f, 0.f, 1.f);
            int r = static_cast<int>(160 + t * 60);
            int g = static_cast<int>(190 + t * 50);
            int b = static_cast<int>(240 + t * 15);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(60 + facet * 400), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(10 + (1.f - edge) * 60), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(140 + edge * 200), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Edge glow + internal refraction highlights
            float edgeGlow = std::max(0.f, 1.f - edge * 6.f);
            // Refraction: bright specular spots inside facets
            float refract = seededRand(u * 2.3f + 0.7f, v * 2.3f + 1.3f);
            float refractSpot = std::max(0.f, refract - 0.88f) * 8.f;
            refractSpot *= (1.f - std::clamp(edge * 3.f, 0.f, 1.f)); // only inside facets

            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 100 + refractSpot * 200), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 180 + refractSpot * 220), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 255 + refractSpot * 255), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateRust(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float rust = warpedFbm(u * 4, v * 4, 1.8f, 5);
            float detail = fbm(u * 12, v * 12, 4);
            float f1, f2;
            voronoi2D(u * 8, v * 8, f1, f2);
            float pitting = std::max(0.f, 0.3f - f1) * 3.f;

            float rustAmt = std::clamp(rust * 1.4f - 0.1f, 0.f, 1.f);
            int r = static_cast<int>((1.f - rustAmt) * 140 + rustAmt * 180);
            int g = static_cast<int>((1.f - rustAmt) * 145 + rustAmt * 80);
            int b = static_cast<int>((1.f - rustAmt) * 150 + rustAmt * 30);
            r = static_cast<int>(r + detail * 30 - 15);
            g = static_cast<int>(g + detail * 20 - 10);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            float d = 120 + rust * 60 - pitting * 100 + detail * 40;
            uint8_t dvu = static_cast<uint8_t>(std::clamp(static_cast<int>(d), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dvu, dvu, dvu));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(30 + rustAmt * 200), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(180 - pitting * 200 + detail * 40), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            tex.emitMap.setPixelAt(x, y, juce::Colours::black);
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateScales(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    float scaleFreq = 8.f;
    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz) * scaleFreq;
            float v = y / static_cast<float>(sz) * scaleFreq;
            int row = static_cast<int>(std::floor(v));
            if (row % 2 != 0) u += 0.5f;
            float cu = u - std::floor(u) - 0.5f;
            float cv = v - std::floor(v) - 0.5f;
            float scaleD = std::sqrt(cu * cu * 4.f + (cv - 0.15f) * (cv - 0.15f) * 6.f);
            float scaleEdge = std::clamp(1.f - scaleD * 2.f, 0.f, 1.f);
            scaleEdge = scaleEdge * scaleEdge;

            float n = noise2D(u * 3, v * 3) * 0.15f;
            float t = scaleEdge + n;
            int r = static_cast<int>(20 + t * 60);
            int g = static_cast<int>(60 + t * 120 + n * 80);
            int b = static_cast<int>(50 + t * 100);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(40 + scaleEdge * 180), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(20 + (1.f - scaleEdge) * 160), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(120 + scaleEdge * 135), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            float sheen = std::max(0.f, scaleEdge - 0.6f) * 2.5f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(sheen * 30), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(sheen * 80), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(sheen * 60), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generatePlasma(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float p1 = std::sin(u * 10.f + fbm(u * 3, v * 3, 3) * 3.f);
            float p2 = std::sin(v * 12.f + fbm(u * 4 + 2, v * 4, 3) * 2.5f);
            float p3 = std::sin((u + v) * 8.f + fbm(u * 2 + 5, v * 2 + 5, 4) * 2.f);
            float p = (p1 + p2 + p3) / 3.f * 0.5f + 0.5f;

            float phase = p * 6.283185f;
            int r = static_cast<int>(128 + std::sin(phase) * 127);
            int g = static_cast<int>(128 + std::sin(phase + 2.094f) * 127);
            int b = static_cast<int>(128 + std::sin(phase + 4.189f) * 127);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(100 + p * 100), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            tex.roughMap.setPixelAt(x, y, juce::Colour(60, 60, 60));
            tex.aoMap.setPixelAt(x, y, juce::Colour(0xee, 0xee, 0xee));
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r / 2, 0, 255)),
                static_cast<uint8_t>(std::clamp(g / 2, 0, 255)),
                static_cast<uint8_t>(std::clamp(b / 2, 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateIce(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float f1, f2;
            voronoi2D(u * 6, v * 6, f1, f2);
            float crack = std::max(0.f, 1.f - (f2 - f1) * 8.f);
            float frost = fbm(u * 8, v * 8, 5) * 0.4f;
            float surface = ridgeFbm(u * 3, v * 3, 4) * 0.3f;

            float t = f1 * 0.8f + frost;
            int r = static_cast<int>(200 + t * 40 - crack * 40);
            int g = static_cast<int>(215 + t * 30 - crack * 20);
            int b = static_cast<int>(235 + t * 20);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            float d = 140 + surface * 200 - crack * 100;
            uint8_t dvu = static_cast<uint8_t>(std::clamp(static_cast<int>(d), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dvu, dvu, dvu));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(15 + frost * 300 + crack * 80), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(200 - crack * 120), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            float crackGlow = crack * 0.6f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(crackGlow * 80), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(crackGlow * 140), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(crackGlow * 255), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateObsidian(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float swirl = warpedFbm(u * 3, v * 3, 2.5f, 5);
            float detail = fbm(u * 10, v * 10, 4) * 0.2f;
            float flow = std::sin(swirl * 8.f) * 0.5f + 0.5f;

            // Subtle iridescent color shift using sine-based hue rotation
            float hueShift = swirl * 6.f + u * 2.f;
            float iridR = std::sin(hueShift) * 0.5f + 0.5f;
            float iridG = std::sin(hueShift + 2.094f) * 0.5f + 0.5f;
            float iridB = std::sin(hueShift + 4.189f) * 0.5f + 0.5f;
            float iridAmt = flow * 0.12f; // subtle

            int r = static_cast<int>(15 + flow * 35 + detail * 30 + iridR * iridAmt * 60);
            int g = static_cast<int>(12 + flow * 20 + detail * 15 + iridG * iridAmt * 40);
            int b = static_cast<int>(20 + flow * 45 + detail * 25 + iridB * iridAmt * 50);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(110 + swirl * 40 + detail * 60), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(8 + detail * 60), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(220 + detail * 30), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // More dramatic flow edge emissive (brighter purple/blue at edges)
            float flowEdge = std::abs(std::sin(swirl * 16.f));
            flowEdge = std::max(0.f, flowEdge - 0.8f) * 5.f;
            float flowEdge2 = std::abs(std::sin(swirl * 12.f + 1.f));
            flowEdge2 = std::max(0.f, flowEdge2 - 0.85f) * 4.f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>((flowEdge * 80 + flowEdge2 * 40)), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>((flowEdge * 25 + flowEdge2 * 15)), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>((flowEdge * 120 + flowEdge2 * 80)), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

ProceduralTexture ProceduralLibrary::generateAlien(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);
            float w1 = warpedFbm(u * 3, v * 3, 3.f, 5);
            float w2 = warpedFbm(u * 5 + 10, v * 5 + 10, 2.f, 4);
            float f1, f2;
            voronoi2D(u * 4 + w1 * 2, v * 4 + w1 * 2, f1, f2);
            float vein = std::max(0.f, 1.f - (f2 - f1) * 5.f);
            float membrane = std::clamp(f1 * 2.f, 0.f, 1.f);

            float t = w2 * 0.6f + membrane * 0.4f;
            int r = static_cast<int>(40 + t * 80 + vein * 60);
            int g = static_cast<int>(80 + t * 60 - vein * 30);
            int b = static_cast<int>(50 + w1 * 100 + vein * 40);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            float d = 80 + membrane * 120 - vein * 60 + w2 * 40;
            uint8_t dvu = static_cast<uint8_t>(std::clamp(static_cast<int>(d), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dvu, dvu, dvu));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(30 + membrane * 80 + vein * 40), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(140 + membrane * 80 - vein * 60), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));
            float veinGlow = vein * vein * 1.5f;
            float spotGlow = std::max(0.f, w1 - 0.65f) * 5.f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(veinGlow * 100 + spotGlow * 200), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(veinGlow * 255 + spotGlow * 80), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(veinGlow * 80 + spotGlow * 150), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

// ============================================================
// Nebula texture — cosmic gas clouds with filamentary structure
// ============================================================

ProceduralTexture ProceduralLibrary::generateNebula(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);

            // Layered cloud structure
            float ridge = ridgeFbm(u * 3.f, v * 3.f, 6);
            float warp = warpedFbm(u * 2.f, v * 2.f, 2.5f, 5);
            float detail = fbm(u * 8.f + 3.f, v * 8.f + 7.f, 4);

            // Purple/magenta/teal color blend
            float r_f = ridge * 0.45f + warp * 0.25f + detail * 0.08f;
            float g_f = warp * 0.12f + detail * 0.15f + ridge * 0.08f;
            float b_f = ridge * 0.35f + warp * 0.4f + detail * 0.1f;

            // Magenta hotspots
            float hot = std::max(0.f, ridge - 0.55f) * 3.f;
            r_f += hot * 0.5f;
            b_f += hot * 0.3f;

            // Teal wisps
            float teal = std::max(0.f, warp - 0.5f) * 2.f;
            g_f += teal * 0.3f;
            b_f += teal * 0.25f;

            // Star hotspots
            float star = seededRand(u * 600.f, v * 600.f);
            if (star > 0.997f)
            {
                float brightness = (star - 0.997f) * 333.f;
                r_f += brightness * 0.9f;
                g_f += brightness * 0.85f;
                b_f += brightness;
            }

            // Dark base: mostly the gas is dim
            int r = static_cast<int>(r_f * 255.f);
            int g = static_cast<int>(g_f * 255.f);
            int b = static_cast<int>(b_f * 255.f);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(80 + ridge * 100 + warp * 60), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(120 + detail * 100), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(160 + ridge * 80), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Strong nebula glow in filaments
            float glowR = std::clamp(ridge * 0.6f + hot * 0.8f, 0.f, 1.f);
            float glowG = std::clamp(teal * 0.4f + hot * 0.1f, 0.f, 1.f);
            float glowB = std::clamp(ridge * 0.4f + teal * 0.5f + hot * 0.4f, 0.f, 1.f);
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(glowR * 200), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(glowG * 160), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(glowB * 220), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}

// ============================================================
// Aurora texture — shimmering curtain-like vertical bands
// ============================================================

ProceduralTexture ProceduralLibrary::generateAurora(int sz)
{
    ProceduralTexture tex;
    tex.colorMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.dispMap  = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.roughMap = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.aoMap    = juce::Image(juce::Image::ARGB, sz, sz, true);
    tex.emitMap  = juce::Image(juce::Image::ARGB, sz, sz, true);

    for (int y = 0; y < sz; ++y)
        for (int x = 0; x < sz; ++x)
        {
            float u = x / static_cast<float>(sz), v = y / static_cast<float>(sz);

            // Vertical curtain bands with sine-wave distortion
            float warpX = fbm(u * 2.f + 1.f, v * 4.f + 3.f, 4) * 0.6f;
            float warpY = fbm(u * 3.f + 5.f, v * 2.f + 7.f, 3) * 0.3f;
            float curtain = std::sin((u + warpX) * 12.f) * 0.5f + 0.5f;
            float flow = fbm((u + warpX) * 5.f, (v + warpY) * 8.f, 5);

            // Vertical fade: strongest in upper portion
            float vertFade = std::clamp(1.f - v * 1.2f, 0.f, 1.f);
            vertFade = vertFade * vertFade;

            // Aurora intensity
            float intensity = curtain * flow * vertFade;
            intensity = std::clamp(intensity * 2.f, 0.f, 1.f);

            // Color cycling: green -> cyan -> purple based on position
            float phase = u * 4.f + warpX * 2.f + v * 0.5f;
            float sinP = std::sin(phase * 3.14159f);
            float cosP = std::cos(phase * 3.14159f);

            float r_f = std::max(0.f, -sinP) * 0.5f * intensity + intensity * 0.05f;
            float g_f = std::max(0.f, cosP) * 0.8f * intensity + intensity * 0.25f;
            float b_f = (std::max(0.f, sinP) * 0.4f + std::max(0.f, -cosP) * 0.3f) * intensity + intensity * 0.1f;

            // Bright edge glow at curtain edges
            float edgeDist = std::abs(curtain - 0.5f) * 2.f;
            float edgeGlow = std::max(0.f, 1.f - edgeDist * 3.f) * vertFade;

            // Dark sky base
            r_f += 0.02f;
            g_f += 0.03f;
            b_f += 0.05f;

            int r = static_cast<int>(r_f * 255.f);
            int g = static_cast<int>(g_f * 255.f);
            int b = static_cast<int>(b_f * 255.f);
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(r, 0, 255)),
                static_cast<uint8_t>(std::clamp(g, 0, 255)),
                static_cast<uint8_t>(std::clamp(b, 0, 255))));

            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(100 + intensity * 80 + flow * 40), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(100 + (1.f - intensity) * 120), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(180 + intensity * 60), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Emissive: bright aurora glow + edge highlights
            float emitR = r_f * 0.6f + edgeGlow * 0.3f;
            float emitG = g_f * 0.8f + edgeGlow * 0.6f;
            float emitB = b_f * 0.5f + edgeGlow * 0.4f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitR * 255), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitG * 255), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(emitB * 255), 0, 255))));
        }
    tex.normalMap = normalFromDisp(tex.dispMap);
    return tex;
}
