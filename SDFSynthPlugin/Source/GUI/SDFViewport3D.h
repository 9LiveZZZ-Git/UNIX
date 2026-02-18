#pragma once
#include <juce_opengl/juce_opengl.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Texture/TextureSystem.h"
#include "GUI/SkyboxSystem.h"

class SDFViewport3D : public juce::Component,
                       public juce::OpenGLRenderer,
                       public juce::Timer
{
public:
    SDFViewport3D(juce::AudioProcessorValueTreeState& apvts);
    ~SDFViewport3D() override;

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Component
    void paint(juce::Graphics&) override {}
    void resized() override;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // Timer for auto-rotate
    void timerCallback() override;

    // Texture system access (owned here since we have the GL context)
    TextureSystem textureSystem;
    SkyboxSystem skyboxSystem;
    juce::OpenGLContext& getOpenGLContext() { return openGLContext; }

private:
    juce::OpenGLContext openGLContext;
    juce::AudioProcessorValueTreeState& apvts;

    // Shader
    std::unique_ptr<juce::OpenGLShaderProgram> shader;

    // Fullscreen quad
    GLuint vao = 0, vbo = 0;

    // Camera
    float cameraAzimuth = 0.8f;
    float cameraPitch = 0.4f;
    float cameraDistance = 2.8f;
    bool isDragging = false;
    juce::Point<float> lastMousePos;

    void createShader();
    void createQuad();

    static juce::String getVertexShader();
    static juce::String getFragmentShader();
};
