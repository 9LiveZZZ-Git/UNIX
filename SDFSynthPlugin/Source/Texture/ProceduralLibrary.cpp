#include "ProceduralLibrary.h"
#include <cmath>
#include <algorithm>

juce::StringArray ProceduralLibrary::getPresetNames()
{
    return { "Checkerboard", "Brick", "Brushed Metal", "Concrete",
             "Organic", "Marble", "Lava", "Circuit Board",
             "Wood Grain", "Crystal", "Rust", "Scales",
             "Plasma", "Ice", "Obsidian", "Alien" };
}

juce::StringArray ProceduralLibrary::getPresetKeys()
{
    return { "checkerboard", "brick", "metal", "concrete",
             "organic", "marble", "lava", "circuit",
             "wood", "crystal", "rust", "scales",
             "plasma", "ice", "obsidian", "alien" };
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

    for (int y = 0; y < sz; ++y)
    {
        for (int x = 0; x < sz; ++x)
        {
            float dL = src.getPixelColour((x - 1 + sz) % sz, y).getRed() / 255.f;
            float dR = src.getPixelColour((x + 1) % sz, y).getRed() / 255.f;
            float dU = src.getPixelColour(x, (y - 1 + sz) % sz).getRed() / 255.f;
            float dD = src.getPixelColour(x, (y + 1) % sz).getRed() / 255.f;
            float nx = dL - dR, ny = dU - dD, nz = 1.f;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= len; ny /= len; nz /= len;
            dst.setPixelColour(x, y, juce::Colour::fromRGBA(
                static_cast<uint8_t>((nx * 0.5f + 0.5f) * 255),
                static_cast<uint8_t>((ny * 0.5f + 0.5f) * 255),
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
            tex.colorMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(40 + hot * 215), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(10 + hot * 140), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(5 + hot * 20), 0, 255))));
            uint8_t dv = static_cast<uint8_t>(std::clamp(static_cast<int>(40 + n * 200), 0, 255));
            tex.dispMap.setPixelAt(x, y, juce::Colour(dv, dv, dv));
            uint8_t rv = static_cast<uint8_t>(std::clamp(static_cast<int>(200 - hot * 180), 0, 255));
            tex.roughMap.setPixelAt(x, y, juce::Colour(rv, rv, rv));
            uint8_t av = static_cast<uint8_t>(std::clamp(static_cast<int>(120 + hot * 130), 0, 255));
            tex.aoMap.setPixelAt(x, y, juce::Colour(av, av, av));

            // Emissive: orange-red crack glow
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(hot * 255), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(hot * hot * 180), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(hot * hot * hot * 40), 0, 255))));
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
                for (int y = gy * cs; y < (gy + 1) * cs; ++y)
                    for (int dx = -1; dx <= 1; ++dx)
                        if (cx + dx >= 0 && cx + dx < sz && y >= 0 && y < sz)
                            tex.colorMap.setPixelAt(cx + dx, y, juce::Colour(0x1a, 0x6b, 0x3a));
            }

            // Horizontal traces
            if (seededRand(static_cast<float>(gx + 100), static_cast<float>(gy)) > 0.5f)
            {
                int cy = gy * cs + cs / 2;
                for (int x = gx * cs; x < (gx + 1) * cs; ++x)
                    for (int dy = -1; dy <= 1; ++dy)
                        if (x >= 0 && x < sz && cy + dy >= 0 && cy + dy < sz)
                            tex.colorMap.setPixelAt(x, cy + dy, juce::Colour(0x1a, 0x6b, 0x3a));
            }

            // LED dots (color + emissive)
            if (seededRand(static_cast<float>(gx), static_cast<float>(gy + 100)) > 0.8f)
            {
                int cx = gx * cs + cs / 2;
                int cy = gy * cs + cs / 2;
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                    {
                        int px = cx + dx, py = cy + dy;
                        if (px >= 0 && px < sz && py >= 0 && py < sz)
                        {
                            tex.colorMap.setPixelAt(px, py, juce::Colour(0x3a, 0xff, 0x7a));
                            int gr = static_cast<int>(180 + seededRand(static_cast<float>(gx + 1), static_cast<float>(gy)) * 75);
                            tex.emitMap.setPixelAt(px, py, juce::Colour(
                                0, static_cast<uint8_t>(std::min(gr, 255)), 0));
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
            float u = x / static_cast<float>(sz) * 5.f;
            float v = y / static_cast<float>(sz) * 5.f;
            float f1, f2;
            voronoi2D(u, v, f1, f2);
            float edge = f2 - f1;
            float facet = f1;

            float t = std::clamp(facet * 1.5f, 0.f, 1.f);
            int r = static_cast<int>(180 + t * 75);
            int g = static_cast<int>(200 + t * 55);
            int b = static_cast<int>(230 + t * 25);
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
            float edgeGlow = std::max(0.f, 1.f - edge * 6.f);
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 100), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 180), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(edgeGlow * 255), 0, 255))));
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

            int r = static_cast<int>(15 + flow * 35 + detail * 30);
            int g = static_cast<int>(12 + flow * 20 + detail * 15);
            int b = static_cast<int>(20 + flow * 45 + detail * 25);
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
            float flowEdge = std::abs(std::sin(swirl * 16.f));
            flowEdge = std::max(0.f, flowEdge - 0.85f) * 6.f;
            tex.emitMap.setPixelAt(x, y, juce::Colour(
                static_cast<uint8_t>(std::clamp(static_cast<int>(flowEdge * 60), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(flowEdge * 20), 0, 255)),
                static_cast<uint8_t>(std::clamp(static_cast<int>(flowEdge * 80), 0, 255))));
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
