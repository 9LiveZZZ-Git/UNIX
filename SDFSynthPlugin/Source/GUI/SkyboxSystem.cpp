#include "SkyboxSystem.h"
#include <algorithm>
#include <cstring>

juce::StringArray SkyboxSystem::getPresetNames()
{
    return { "Void", "Dark Space", "Sunset", "Studio", "Nebula", "Blue Hour" };
}

float SkyboxSystem::smoothstep(float edge0, float edge1, float x)
{
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float SkyboxSystem::mix(float a, float b, float t)
{
    return a + (b - a) * t;
}

float SkyboxSystem::seededRand(float x, float y)
{
    float n = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
    return n - std::floor(n);
}

float SkyboxSystem::noise2D(float x, float y)
{
    float ix = std::floor(x), iy = std::floor(y);
    float fx = x - ix, fy = y - iy;
    auto sf = [](float t) { return t * t * (3.f - 2.f * t); };
    float a = seededRand(ix, iy), b = seededRand(ix + 1, iy);
    float c = seededRand(ix, iy + 1), d = seededRand(ix + 1, iy + 1);
    return a + (b - a) * sf(fx) + (c - a) * sf(fy) + (a - b - c + d) * sf(fx) * sf(fy);
}

float SkyboxSystem::fbm(float x, float y, int octaves)
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

// === Procedural Skybox Generators ===

void SkyboxSystem::generateVoid(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // Faint radial gradient from center for subtle ambient illumination
            float cu = u - 0.5f, cv = v - 0.5f;
            float dist = std::sqrt(cu * cu + cv * cv);
            float radial = std::max(0.f, 1.f - dist * 2.2f) * 0.025f;

            rgb[idx]     = 0.015f + radial * 0.6f;
            rgb[idx + 1] = 0.025f + radial * 0.8f;
            rgb[idx + 2] = 0.03f  + radial;

            // Extremely sparse dim stars for depth
            float star = seededRand(u * 800.f, v * 400.f);
            if (star > 0.9993f)
            {
                float brightness = (star - 0.9993f) * 1400.f * 0.15f;
                rgb[idx]     += brightness * 0.7f;
                rgb[idx + 1] += brightness * 0.8f;
                rgb[idx + 2] += brightness;
            }
        }
}

void SkyboxSystem::generateDarkSpace(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // Base: near black
            float r = 0.005f, g = 0.008f, b = 0.015f;

            // Denser stars with color temperature variation
            float star = seededRand(u * 500.f, v * 250.f);
            if (star > 0.996f)
            {
                float brightness = (star - 0.996f) * 250.f;
                // Star color temperature: warm or cool
                float temp = seededRand(u * 300.f + 50.f, v * 150.f + 50.f);
                if (temp > 0.6f) // cool blue-white
                {
                    r += brightness * 0.7f;
                    g += brightness * 0.85f;
                    b += brightness * 1.0f;
                }
                else if (temp > 0.3f) // neutral white
                {
                    r += brightness * 0.9f;
                    g += brightness * 0.9f;
                    b += brightness * 0.9f;
                }
                else // warm orange-red
                {
                    r += brightness * 1.0f;
                    g += brightness * 0.7f;
                    b += brightness * 0.4f;
                }
            }

            // Boosted blue nebula wash
            float neb = fbm(u * 3.f + 2.f, v * 2.f, 4) * 0.1f;
            b += neb;
            g += neb * 0.3f;

            // Second warmer nebula layer
            float neb2 = fbm(u * 4.f + 7.f, v * 3.f + 5.f, 4) * 0.06f;
            r += neb2 * 0.8f;
            g += neb2 * 0.3f;
            b += neb2 * 0.15f;

            rgb[idx] = r;
            rgb[idx + 1] = g;
            rgb[idx + 2] = b;
        }
}

void SkyboxSystem::generateSunset(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // Warm-to-cool gradient: deep blue top -> warm orange horizon -> dark ground
            float skyT = smoothstep(0.3f, 0.55f, v);
            float r = mix(0.02f, 1.8f, skyT) * (v < 0.6f ? 1.f : 0.1f);
            float g = mix(0.01f, 0.6f, smoothstep(0.35f, 0.55f, v)) * (v < 0.6f ? 1.f : 0.05f);
            float b = mix(0.15f, 0.2f, smoothstep(0.2f, 0.5f, v)) * (v < 0.6f ? 1.f : 0.05f);

            // Upper sky: add cool blue tint
            float upperBlue = std::max(0.f, 1.f - v * 3.f) * 0.12f;
            b += upperBlue;

            // Sun disc glow near horizon (Gaussian hot spot at u=0.3, v=0.52)
            float sunDu = u - 0.3f, sunDv = v - 0.52f;
            float sunDist2 = sunDu * sunDu * 4.f + sunDv * sunDv * 40.f;
            float sunGlow = std::exp(-sunDist2 * 8.f) * 2.5f;
            r += sunGlow * 1.0f;
            g += sunGlow * 0.5f;
            b += sunGlow * 0.1f;

            // Primary cloud layer
            float cloud = fbm(u * 6.f + 1.f, v * 3.f, 5) * 0.3f;
            r += cloud * 0.4f;
            g += cloud * 0.15f;

            // Second FBM pass for cloud shadows
            float cloudShadow = fbm(u * 8.f + 3.f, v * 4.f + 2.f, 4) * 0.15f;
            float shadowMask = smoothstep(0.3f, 0.55f, v);
            r -= cloudShadow * shadowMask * 0.3f;
            g -= cloudShadow * shadowMask * 0.2f;

            rgb[idx] = std::max(0.f, r);
            rgb[idx + 1] = std::max(0.f, g);
            rgb[idx + 2] = std::max(0.f, b);
        }
}

void SkyboxSystem::generateStudio(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // Neutral gray gradient: bright top -> dark bottom
            float brightness = mix(0.6f, 0.15f, v);

            // Subtle fill light variation
            float fill = 0.05f * std::sin(v * 3.14159f);
            brightness += fill;

            // Soft rim light hotspot (key light at upper-right)
            float klDu = u - 0.75f, klDv = v - 0.2f;
            float klDist2 = klDu * klDu + klDv * klDv;
            float keyLight = std::exp(-klDist2 * 6.f) * 0.15f;
            brightness += keyLight;

            // Subtle warm/cool split between left and right halves
            float warmCool = (u - 0.5f) * 0.04f;
            rgb[idx]     = brightness + warmCool;           // warmer right
            rgb[idx + 1] = brightness;
            rgb[idx + 2] = brightness * 1.05f - warmCool;  // cooler left
        }
}

void SkyboxSystem::generateNebula(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // More FBM layers for richer cloud depth
            float n1 = fbm(u * 4.f, v * 3.f, 6);
            float n2 = fbm(u * 6.f + 5.f, v * 4.f + 3.f, 5);
            float n3 = fbm(u * 3.f + 10.f, v * 2.f + 7.f, 4);
            float n4 = fbm(u * 8.f + 2.f, v * 6.f + 1.f, 5);
            float n5 = fbm(u * 2.f + 15.f, v * 3.f + 12.f, 4);

            // Purple-teal-magenta space clouds with extra depth
            float r = n1 * 0.4f + n3 * 0.3f + n4 * 0.08f;
            float g = n2 * 0.2f + n1 * 0.15f + n5 * 0.1f;
            float b = n1 * 0.5f + n2 * 0.3f + n4 * 0.1f;

            // Add magenta highlights
            float mag = std::max(0.f, n3 - 0.4f) * 2.f;
            r += mag * 0.6f;
            b += mag * 0.4f;

            // Golden/amber highlights in bright regions
            float amber = std::max(0.f, n5 - 0.45f) * 2.5f;
            r += amber * 0.5f;
            g += amber * 0.35f;

            // Bright emission knots (localized bright spots in the gas)
            float knot = std::max(0.f, n4 * n1 - 0.2f) * 4.f;
            r += knot * 0.3f;
            g += knot * 0.15f;
            b += knot * 0.4f;

            // Sparse stars
            float star = seededRand(u * 400.f, v * 200.f);
            if (star > 0.997f)
            {
                float brightness = (star - 0.997f) * 333.f;
                r += brightness;
                g += brightness;
                b += brightness;
            }

            rgb[idx] = r;
            rgb[idx + 1] = g;
            rgb[idx + 2] = b;
        }
}

void SkyboxSystem::generateBlueHour(float* rgb, int w, int h)
{
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float u = static_cast<float>(x) / w;
            float v = static_cast<float>(y) / h;
            int idx = (y * w + x) * 3;

            // Deep blue gradient with pale horizon glow
            float r = mix(0.01f, 0.12f, smoothstep(0.35f, 0.55f, v));
            float g = mix(0.02f, 0.18f, smoothstep(0.3f, 0.55f, v));
            float b = mix(0.08f, 0.35f, smoothstep(0.2f, 0.6f, v));

            // Warmer horizon glow tint
            float horizonGlow = std::exp(-std::pow((v - 0.52f) * 8.f, 2.f));
            r += horizonGlow * 0.22f;
            g += horizonGlow * 0.2f;
            b += horizonGlow * 0.2f;

            // Subtle cloud silhouettes near horizon using FBM
            float cloudV = smoothstep(0.4f, 0.55f, v) * smoothstep(0.6f, 0.5f, v);
            float cloud = fbm(u * 8.f + 3.f, v * 4.f, 5) * cloudV * 0.06f;
            r -= cloud;
            g -= cloud;
            b -= cloud * 0.7f;

            // Faint star field in upper sky region
            if (v < 0.35f)
            {
                float starFade = smoothstep(0.35f, 0.1f, v);
                float star = seededRand(u * 600.f, v * 300.f);
                if (star > 0.997f)
                {
                    float brightness = (star - 0.997f) * 333.f * starFade * 0.4f;
                    r += brightness * 0.8f;
                    g += brightness * 0.9f;
                    b += brightness;
                }
            }

            // Dark below horizon
            if (v > 0.6f)
            {
                float dark = smoothstep(0.6f, 0.8f, v);
                r *= (1.f - dark * 0.9f);
                g *= (1.f - dark * 0.9f);
                b *= (1.f - dark * 0.85f);
            }

            rgb[idx] = std::max(0.f, r);
            rgb[idx + 1] = std::max(0.f, g);
            rgb[idx + 2] = std::max(0.f, b);
        }
}

// === Deferred Upload Pattern ===
// loadPreset() and loadHDR() store pixel data; processGLUpload() uploads on GL thread.

void SkyboxSystem::uploadToGL(const float* rgb, int w, int h)
{
    if (textureId == 0)
        juce::gl::glGenTextures(1, &textureId);

    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
    juce::gl::glTexImage2D(juce::gl::GL_TEXTURE_2D, 0,
                           juce::gl::GL_RGB16F, w, h, 0,
                           juce::gl::GL_RGB, juce::gl::GL_FLOAT, rgb);
    juce::gl::glGenerateMipmap(juce::gl::GL_TEXTURE_2D);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_WRAP_S, juce::gl::GL_REPEAT);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_WRAP_T, juce::gl::GL_CLAMP_TO_EDGE);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MIN_FILTER, juce::gl::GL_LINEAR_MIPMAP_LINEAR);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MAG_FILTER, juce::gl::GL_LINEAR);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, 0);

    texWidth = w;
    texHeight = h;
    loaded = true;
}

void SkyboxSystem::loadPreset(int presetIndex)
{
    constexpr int W = 512, H = 256;
    std::vector<float> rgbData(W * H * 3);

    using GenFunc = void(*)(float*, int, int);
    GenFunc generators[] = {
        generateVoid, generateDarkSpace, generateSunset,
        generateStudio, generateNebula, generateBlueHour
    };

    if (presetIndex < 0 || presetIndex > 5)
        presetIndex = 0;

    generators[presetIndex](rgbData.data(), W, H);

    // Store for deferred GL upload
    std::lock_guard<std::mutex> lock(uploadMutex);
    pendingRGB = std::move(rgbData);
    pendingW = W;
    pendingH = H;
    needsUpload = true;
    needsClear = false;
}

bool SkyboxSystem::loadHDR(const juce::File& file)
{
    auto stream = file.createInputStream();
    if (!stream)
        return false;

    // Parse Radiance .hdr RGBE format
    juce::String line;
    int width = 0, height = 0;

    // Read header
    while (!(line = stream->readNextLine()).isEmpty())
    {
        if (line.startsWith("-Y"))
        {
            // Parse "-Y height +X width"
            auto tokens = juce::StringArray::fromTokens(line, " ", "");
            if (tokens.size() >= 4)
            {
                height = tokens[1].getIntValue();
                width = tokens[3].getIntValue();
            }
            break;
        }
    }

    if (width <= 0 || height <= 0 || width > 8192 || height > 4096)
        return false;

    std::vector<float> rgbData(width * height * 3);

    // Read scanlines (handles both RLE and non-RLE)
    for (int y = 0; y < height; ++y)
    {
        std::vector<uint8_t> scanline(width * 4);

        // Check for new-style RLE
        uint8_t header[4];
        if (stream->read(header, 4) != 4)
            return false;

        if (header[0] == 2 && header[1] == 2 && ((header[2] << 8) | header[3]) == width)
        {
            // New-style RLE: each channel is RLE-encoded separately
            for (int ch = 0; ch < 4; ++ch)
            {
                int ptr = 0;
                while (ptr < width)
                {
                    uint8_t code;
                    if (stream->read(&code, 1) != 1) return false;

                    if (code > 128)
                    {
                        int count = code - 128;
                        uint8_t val;
                        if (stream->read(&val, 1) != 1) return false;
                        for (int i = 0; i < count && ptr < width; ++i, ++ptr)
                            scanline[ptr * 4 + ch] = val;
                    }
                    else
                    {
                        int count = code;
                        for (int i = 0; i < count && ptr < width; ++i, ++ptr)
                        {
                            uint8_t val;
                            if (stream->read(&val, 1) != 1) return false;
                            scanline[ptr * 4 + ch] = val;
                        }
                    }
                }
            }
        }
        else
        {
            // Old-style: first 4 bytes are the first pixel
            scanline[0] = header[0];
            scanline[1] = header[1];
            scanline[2] = header[2];
            scanline[3] = header[3];
            // Read remaining pixels
            if (width > 1)
                stream->read(scanline.data() + 4, (width - 1) * 4);
        }

        // Convert RGBE to float RGB
        for (int x = 0; x < width; ++x)
        {
            int si = x * 4;
            int di = (y * width + x) * 3;
            uint8_t e = scanline[si + 3];
            if (e == 0)
            {
                rgbData[di] = rgbData[di + 1] = rgbData[di + 2] = 0.f;
            }
            else
            {
                float scale = std::ldexp(1.f, static_cast<int>(e) - 128 - 8);
                rgbData[di]     = scanline[si]     * scale;
                rgbData[di + 1] = scanline[si + 1] * scale;
                rgbData[di + 2] = scanline[si + 2] * scale;
            }
        }
    }

    // Store for deferred GL upload
    std::lock_guard<std::mutex> lock(uploadMutex);
    pendingRGB = std::move(rgbData);
    pendingW = width;
    pendingH = height;
    needsUpload = true;
    needsClear = false;
    return true;
}

void SkyboxSystem::clear()
{
    std::lock_guard<std::mutex> lock(uploadMutex);
    pendingRGB.clear();
    pendingW = 0;
    pendingH = 0;
    needsUpload = false;
    needsClear = true;
}

void SkyboxSystem::processGLUpload()
{
    std::lock_guard<std::mutex> lock(uploadMutex);

    if (needsClear)
    {
        if (textureId != 0)
        {
            juce::gl::glDeleteTextures(1, &textureId);
            textureId = 0;
        }
        loaded = false;
        texWidth = 0;
        texHeight = 0;
        needsClear = false;
    }

    if (needsUpload && !pendingRGB.empty())
    {
        uploadToGL(pendingRGB.data(), pendingW, pendingH);
        pendingRGB.clear();
        needsUpload = false;
    }
}

void SkyboxSystem::releaseGL()
{
    if (textureId != 0)
    {
        juce::gl::glDeleteTextures(1, &textureId);
        textureId = 0;
    }
    loaded = false;
}

void SkyboxSystem::bindToShader(juce::OpenGLShaderProgram& shader, int textureUnit)
{
    juce::gl::glActiveTexture(static_cast<GLenum>(juce::gl::GL_TEXTURE0 + textureUnit));

    if (loaded && textureId != 0)
    {
        juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
        if (auto u = shader.getUniformIDFromName("uSkybox"))
            juce::gl::glUniform1i(u, textureUnit);
        if (auto u = shader.getUniformIDFromName("uHasSkybox"))
            juce::gl::glUniform1f(u, 1.0f);
    }
    else
    {
        juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, 0);
        if (auto u = shader.getUniformIDFromName("uHasSkybox"))
            juce::gl::glUniform1f(u, 0.0f);
    }
}
