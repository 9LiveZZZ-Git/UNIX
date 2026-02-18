#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

struct ProceduralTexture
{
    juce::Image colorMap, normalMap, roughMap, dispMap, aoMap, emitMap;
};

class ProceduralLibrary
{
public:
    static juce::StringArray getPresetNames();
    static juce::StringArray getPresetKeys();
    static ProceduralTexture generate(const juce::String& key, int size = 256);

private:
    // Noise primitives
    static float seededRand(float x, float y);
    static float noise2D(float x, float y);
    static float fbm(float x, float y, int octaves = 5);
    static void voronoi2D(float x, float y, float& f1, float& f2);
    static float ridgeFbm(float x, float y, int octaves = 5);
    static float warpedFbm(float x, float y, float warpAmt = 2.f, int octaves = 5);

    // Original 8
    static ProceduralTexture generateCheckerboard(int sz);
    static ProceduralTexture generateBrick(int sz);
    static ProceduralTexture generateMetal(int sz);
    static ProceduralTexture generateConcrete(int sz);
    static ProceduralTexture generateOrganic(int sz);
    static ProceduralTexture generateMarble(int sz);
    static ProceduralTexture generateLava(int sz);
    static ProceduralTexture generateCircuit(int sz);

    // New 8
    static ProceduralTexture generateWood(int sz);
    static ProceduralTexture generateCrystal(int sz);
    static ProceduralTexture generateRust(int sz);
    static ProceduralTexture generateScales(int sz);
    static ProceduralTexture generatePlasma(int sz);
    static ProceduralTexture generateIce(int sz);
    static ProceduralTexture generateObsidian(int sz);
    static ProceduralTexture generateAlien(int sz);

    static juce::Image normalFromDisp(const juce::Image& disp);
};
