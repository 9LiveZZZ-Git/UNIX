#pragma once
#include <juce_opengl/juce_opengl.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cstdint>
#include <mutex>

struct TextureSlot
{
    GLuint textureId = 0;
    bool loaded = false;
    juce::Image sourceImage;
    std::vector<uint8_t> pixelData; // RGBA for CPU sampling
    int width = 0, height = 0;

    // Deferred GL upload: image data stored here, uploaded on GL thread
    bool needsUpload = false;
};

class TextureSystem
{
public:
    TextureSystem() = default;

    TextureSlot colorTex, normalTex, roughTex, dispTex, aoTex, emitTex;

    // Queue an image for loading (safe to call from any thread)
    void loadFromImage(TextureSlot& slot, const juce::Image& img);
    void clearSlot(TextureSlot& slot);
    void clearAll();

    void loadAllFromImages(const juce::Image& color, const juce::Image& normal,
                           const juce::Image& rough, const juce::Image& disp,
                           const juce::Image& ao, const juce::Image& emit);

    // Must be called from GL thread (inside renderOpenGL) before bindToShader
    void processGLUploads();

    // Bind textures to shader (call from GL thread)
    void bindToShader(juce::OpenGLShaderProgram& shader, int startUnit = 0);

    // Release GL resources (call from GL thread / openGLContextClosing)
    void releaseGL();

    bool hasAnyTexture() const;

private:
    void uploadSlotToGL(TextureSlot& slot);
    std::mutex uploadMutex;
};
