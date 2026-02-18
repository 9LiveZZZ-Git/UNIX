#include "TextureSystem.h"

void TextureSystem::loadFromImage(TextureSlot& slot, const juce::Image& img)
{
    if (!img.isValid()) return;

    std::lock_guard<std::mutex> lock(uploadMutex);

    // Extract pixel data on the calling thread (message thread is fine)
    slot.sourceImage = img;
    slot.width = img.getWidth();
    slot.height = img.getHeight();
    slot.pixelData.resize(static_cast<size_t>(slot.width * slot.height * 4));

    juce::Image::BitmapData bmp(img, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < slot.height; ++y)
    {
        for (int x = 0; x < slot.width; ++x)
        {
            auto pixel = bmp.getPixelColour(x, y);
            size_t idx = static_cast<size_t>((y * slot.width + x) * 4);
            slot.pixelData[idx]     = pixel.getRed();
            slot.pixelData[idx + 1] = pixel.getGreen();
            slot.pixelData[idx + 2] = pixel.getBlue();
            slot.pixelData[idx + 3] = pixel.getAlpha();
        }
    }

    // Mark for GL upload (will happen on GL thread)
    slot.needsUpload = true;
    slot.loaded = true;
}

void TextureSystem::clearSlot(TextureSlot& slot)
{
    std::lock_guard<std::mutex> lock(uploadMutex);
    // GL texture will be cleaned up in releaseGL or processGLUploads
    slot.loaded = false;
    slot.needsUpload = false;
    slot.sourceImage = {};
    slot.pixelData.clear();
    slot.width = 0;
    slot.height = 0;
    // Note: textureId is left for GL thread to clean up
}

void TextureSystem::clearAll()
{
    clearSlot(colorTex);
    clearSlot(normalTex);
    clearSlot(roughTex);
    clearSlot(dispTex);
    clearSlot(aoTex);
    clearSlot(emitTex);
}

void TextureSystem::loadAllFromImages(const juce::Image& color, const juce::Image& normal,
                                       const juce::Image& rough, const juce::Image& disp,
                                       const juce::Image& ao, const juce::Image& emit)
{
    loadFromImage(colorTex, color);
    loadFromImage(normalTex, normal);
    loadFromImage(roughTex, rough);
    loadFromImage(dispTex, disp);
    loadFromImage(aoTex, ao);
    loadFromImage(emitTex, emit);
}

void TextureSystem::uploadSlotToGL(TextureSlot& slot)
{
    if (slot.textureId == 0)
        juce::gl::glGenTextures(1, &slot.textureId);

    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, slot.textureId);
    juce::gl::glTexImage2D(juce::gl::GL_TEXTURE_2D, 0, juce::gl::GL_RGBA,
                           slot.width, slot.height, 0,
                           juce::gl::GL_RGBA, juce::gl::GL_UNSIGNED_BYTE,
                           slot.pixelData.data());
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_WRAP_S, juce::gl::GL_REPEAT);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_WRAP_T, juce::gl::GL_REPEAT);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MIN_FILTER, juce::gl::GL_LINEAR);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MAG_FILTER, juce::gl::GL_LINEAR);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, 0);

    slot.needsUpload = false;
}

void TextureSystem::processGLUploads()
{
    std::lock_guard<std::mutex> lock(uploadMutex);

    auto processSlot = [this](TextureSlot& slot) {
        if (!slot.loaded && slot.textureId != 0)
        {
            juce::gl::glDeleteTextures(1, &slot.textureId);
            slot.textureId = 0;
        }
        if (slot.needsUpload && slot.loaded && !slot.pixelData.empty())
            uploadSlotToGL(slot);
    };

    processSlot(colorTex);
    processSlot(normalTex);
    processSlot(roughTex);
    processSlot(dispTex);
    processSlot(aoTex);
    processSlot(emitTex);
}

void TextureSystem::bindToShader(juce::OpenGLShaderProgram& shader, int startUnit)
{
    auto bindSlot = [&](TextureSlot& slot, const char* samplerName, const char* flagName, int unit)
    {
        juce::gl::glActiveTexture(static_cast<GLenum>(juce::gl::GL_TEXTURE0 + unit));
        if (slot.loaded && slot.textureId != 0)
        {
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, slot.textureId);
            if (auto u = shader.getUniformIDFromName(samplerName))
                juce::gl::glUniform1i(u, unit);
            if (auto u = shader.getUniformIDFromName(flagName))
                juce::gl::glUniform1i(u, 1);
        }
        else
        {
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, 0);
            if (auto u = shader.getUniformIDFromName(flagName))
                juce::gl::glUniform1i(u, 0);
        }
    };

    bindSlot(colorTex,  "uTex",      "uHasTex",      startUnit);
    bindSlot(normalTex, "uTexNorm",  "uHasTexNorm",  startUnit + 1);
    bindSlot(roughTex,  "uTexRough", "uHasTexRough", startUnit + 2);
    bindSlot(dispTex,   "uTexDisp",  "uHasTexDisp",  startUnit + 3);
    bindSlot(aoTex,     "uTexAO",    "uHasTexAO",    startUnit + 4);
    bindSlot(emitTex,   "uTexEmit",  "uHasTexEmit",  startUnit + 5);
}

void TextureSystem::releaseGL()
{
    auto freeSlot = [](TextureSlot& slot) {
        if (slot.textureId != 0)
        {
            juce::gl::glDeleteTextures(1, &slot.textureId);
            slot.textureId = 0;
        }
    };
    freeSlot(colorTex);
    freeSlot(normalTex);
    freeSlot(roughTex);
    freeSlot(dispTex);
    freeSlot(aoTex);
    freeSlot(emitTex);
}

bool TextureSystem::hasAnyTexture() const
{
    return colorTex.loaded || normalTex.loaded || roughTex.loaded
        || dispTex.loaded || aoTex.loaded || emitTex.loaded;
}
