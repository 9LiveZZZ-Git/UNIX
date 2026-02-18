#pragma once
#include <juce_opengl/juce_opengl.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <cmath>
#include <mutex>

class SkyboxSystem
{
public:
    SkyboxSystem() = default;

    static juce::StringArray getPresetNames();

    // Generate a procedural skybox preset (safe from any thread)
    void loadPreset(int presetIndex);

    // Load a custom .hdr file (safe from any thread)
    bool loadHDR(const juce::File& file);

    // Clear the current skybox
    void clear();

    // Must be called from GL thread before bindToShader
    void processGLUpload();

    // Bind skybox texture to a given texture unit (GL thread only)
    void bindToShader(juce::OpenGLShaderProgram& shader, int textureUnit);

    // Release GL resources (GL thread only)
    void releaseGL();

    bool isLoaded() const { return loaded; }

private:
    GLuint textureId = 0;
    bool loaded = false;
    int texWidth = 0, texHeight = 0;

    // Deferred upload data
    std::vector<float> pendingRGB;
    int pendingW = 0, pendingH = 0;
    bool needsUpload = false;
    bool needsClear = false;
    std::mutex uploadMutex;

    // Procedural generators
    static void generateVoid(float* rgb, int w, int h);
    static void generateDarkSpace(float* rgb, int w, int h);
    static void generateSunset(float* rgb, int w, int h);
    static void generateStudio(float* rgb, int w, int h);
    static void generateNebula(float* rgb, int w, int h);
    static void generateBlueHour(float* rgb, int w, int h);

    void uploadToGL(const float* rgb, int w, int h);

    static float seededRand(float x, float y);
    static float noise2D(float x, float y);
    static float fbm(float x, float y, int octaves = 5);
    static float smoothstep(float edge0, float edge1, float x);
    static float mix(float a, float b, float t);
};
