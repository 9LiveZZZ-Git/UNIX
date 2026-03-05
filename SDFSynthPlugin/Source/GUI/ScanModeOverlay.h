#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DSP/ContourExtractor.h"
#include "DSP/WavetableGenerator.h"
#include "SDFLookAndFeel.h"
#include <cmath>
#include <memory>

// Animated 2D diagram overlay showing what each scan mode is doing.
// Sits in the bottom-right corner of the 3D viewport.
class ScanModeOverlay : public juce::Component, public juce::Timer
{
public:
    ScanModeOverlay() { startTimerHz(30); }
    ~ScanModeOverlay() override { stopTimer(); }

    void setScanMode(int m) { scanMode = m; }
    void setContour(std::shared_ptr<const std::vector<ContourPoint>> c) { contour = std::move(c); }
    void setScanHeight(float h) { scanHeight = h; }
    void setTopoMorph(float m) { topoMorph = m; }
    void setScanRadius(float r) { scanRadius = r; }
    void setPlayheadPhase(float p) { playheadPhase = p; }
    void setDistScale(float d) { distScale = d; }
    void setAlgorithmData(std::shared_ptr<const ScanAlgorithmData> d) { algData = std::move(d); }

    void timerCallback() override
    {
        animPhase += 1.f / 30.f;
        if (animPhase > 1000.f) animPhase -= 1000.f;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Semi-transparent background
        g.setColour(juce::Colour(0xCC0a1414));
        g.fillRoundedRectangle(bounds, 4.f);
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, 4.f, 1.f);

        auto area = bounds.reduced(4.f);

        // Mode label
        static const char* modeNames[] = { "CONTOUR", "MARCH", "ACOUSTIC", "GRAIN", "SPECTRAL", "LISSAJOUS" };
        int m = juce::jlimit(0, 5, scanMode);
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.6f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), juce::Font::bold));
        g.drawText(modeNames[m], area.removeFromTop(10.f), juce::Justification::centred);
        area.removeFromTop(2.f);

        switch (scanMode)
        {
            case 0: drawContourMode(g, area); break;
            case 1: drawMarchMode(g, area); break;
            case 2: drawAcousticMode(g, area); break;
            case 3: drawGrainMode(g, area); break;
            case 4: drawSpectralMode(g, area); break;
            case 5: drawTraverseMode(g, area); break;
        }
    }

private:
    static constexpr float PI = 3.14159265f;
    static constexpr float TWO_PI = 6.28318530f;

    // --- Helpers ---
    void drawShapeOutline(juce::Graphics& g, juce::Rectangle<float> area, float alpha = 0.5f) const
    {
        if (!contour || contour->empty()) return;

        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.32f;

        juce::Path path;
        bool started = false;
        for (size_t i = 0; i < contour->size(); ++i)
        {
            auto& pt = (*contour)[i];
            if (!pt.valid) continue;
            float px = cx + pt.x * scale;
            float py = cy - pt.z * scale;
            if (!started) { path.startNewSubPath(px, py); started = true; }
            else path.lineTo(px, py);
        }
        path.closeSubPath();

        g.setColour(juce::Colour(0x1900ffff));
        g.fillPath(path);
        g.setColour(SDFLookAndFeel::primaryAccent.withAlpha(alpha));
        g.strokePath(path, juce::PathStrokeType(1.f));
    }

    float getContourRadius(float angle) const
    {
        if (!contour || contour->empty()) return 0.5f;
        while (angle < 0.f) angle += TWO_PI;
        while (angle >= TWO_PI) angle -= TWO_PI;
        float idx = (angle / TWO_PI) * static_cast<float>(contour->size());
        int i0 = static_cast<int>(idx) % static_cast<int>(contour->size());
        auto& pt = (*contour)[static_cast<size_t>(i0)];
        return pt.valid ? pt.r : 0.5f;
    }

    // --- Mode 0: Contour cross-section with topoMorph blend indicator ---
    void drawContourMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        drawShapeOutline(g, area, 0.6f);
        if (!contour || contour->empty()) return;

        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.32f;

        // Draw faint radial lines
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.1f));
        for (int i = 0; i < 16; ++i)
        {
            float angle = static_cast<float>(i) / 16.f * TWO_PI;
            float r = getContourRadius(angle) * scale;
            g.drawLine(cx, cy, cx + std::cos(angle) * r, cy - std::sin(angle) * r, 0.5f);
        }

        // TopoMorph blend indicator: inner ring = SDF sampling, outer = profile
        if (topoMorph > 0.01f && topoMorph < 0.99f)
        {
            float sdfMix = std::max(0.f, (1.f - topoMorph) * 2.f);
            // Inner ring (SDF contribution)
            g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(sdfMix * 0.3f));
            float innerR = scale * 0.3f;
            g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f, 0.5f);
        }

        // MRI mode indicator for topoMorph > 0.5
        if (topoMorph > 0.5f)
        {
            g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.2f));
            g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
            g.drawText("MRI", area.getX(), area.getY(), area.getWidth(), 10.f, juce::Justification::centredRight);
        }

        // Animated sweep line
        float sweepAngle = playheadPhase >= 0.f ? playheadPhase * TWO_PI : std::fmod(animPhase * 1.5f, TWO_PI);
        float sweepR = getContourRadius(sweepAngle) * scale;
        float sx = cx + std::cos(sweepAngle) * sweepR;
        float sy = cy - std::sin(sweepAngle) * sweepR;
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.7f));
        g.drawLine(cx, cy, sx, sy, 1.5f);
        g.setColour(SDFLookAndFeel::tertiaryAccent);
        g.fillEllipse(sx - 3.f, sy - 3.f, 6.f, 6.f);

        // Center dot
        g.setColour(SDFLookAndFeel::primaryAccent);
        g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);

        // Scan height label
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.4f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(7.f), 0));
        g.drawText("Y=" + juce::String(scanHeight, 2), area.removeFromBottom(10.f), juce::Justification::centred);
    }

    // --- Mode 1: Ray March with SDF proximity coloring ---
    void drawMarchMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        drawShapeOutline(g, area, 0.3f);

        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.32f;

        // Use exact values from DSP when available
        int numRays = algData ? algData->numRays : (1 + static_cast<int>(topoMorph * 7.f));
        float maxRange = algData ? algData->maxRange : (scanRadius * 2.f);
        float goldenAngle = 2.39996322f;

        // Adaptive Tukey taper matching DSP
        float taper = 0.15f + 0.1f * (static_cast<float>(numRays) / 8.f);

        // Animated progress
        float cycleTime = 2.f;
        float progress = std::fmod(animPhase / cycleTime, 1.f);
        float activeRayF = progress * numRays;
        int activeRay = static_cast<int>(activeRayF);
        float marchFrac = activeRayF - activeRay;

        // Visual max range scaled to area
        float maxR = juce::jmin(maxRange / scanRadius, 2.f) * scale * 0.5f;

        for (int i = 0; i < numRays; ++i)
        {
            float angle = static_cast<float>(i) * goldenAngle;
            float hitR = getContourRadius(angle) * scale;

            float dx = std::cos(angle);
            float dy = -std::sin(angle);

            bool isActive = (i == activeRay % numRays);
            bool isDone = (i < activeRay % numRays) || (activeRay >= numRays);

            // Draw ray line with gradient: brighter near surface
            if (isDone || isActive)
            {
                float drawR = isDone ? maxR : (marchFrac * maxR);
                int segments = 8;
                for (int s = 0; s < segments; ++s)
                {
                    float t0 = static_cast<float>(s) / segments;
                    float t1 = static_cast<float>(s + 1) / segments;
                    if (t1 * maxR > drawR) t1 = drawR / maxR;
                    if (t0 * maxR > drawR) break;

                    // Brightness: closer to contour = brighter
                    float midT = (t0 + t1) * 0.5f;
                    float distFromSurface = std::abs(midT * maxR - hitR) / maxR;
                    float brightness = std::exp(-distFromSurface * distFromSurface * 8.f);

                    // Tukey window taper visualization
                    float windowAlpha = 1.f;
                    if (midT < taper) windowAlpha = 0.4f + 0.6f * (midT / taper);
                    else if (midT > 1.f - taper) windowAlpha = 0.4f + 0.6f * ((1.f - midT) / taper);

                    float alpha = (isDone ? 0.15f : 0.4f) + brightness * (isDone ? 0.4f : 0.5f);
                    alpha *= windowAlpha;

                    auto col = SDFLookAndFeel::primaryAccent.interpolatedWith(
                        SDFLookAndFeel::tertiaryAccent, brightness);
                    g.setColour(col.withAlpha(alpha));
                    g.drawLine(cx + dx * t0 * maxR, cy + dy * t0 * maxR,
                               cx + dx * t1 * maxR, cy + dy * t1 * maxR, isDone ? 1.f : 1.5f);
                }
            }
            else
            {
                g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.1f));
                g.drawLine(cx, cy, cx + dx * maxR, cy + dy * maxR, 0.5f);
            }

            if (isActive)
            {
                float marchR = marchFrac * maxR;
                float mx = cx + dx * marchR;
                float my = cy + dy * marchR;
                g.setColour(SDFLookAndFeel::tertiaryAccent);
                g.fillEllipse(mx - 3.f, my - 3.f, 6.f, 6.f);
            }

            if (isDone)
            {
                float hx = cx + dx * hitR;
                float hy = cy + dy * hitR;
                g.setColour(SDFLookAndFeel::primaryAccent.withAlpha(0.7f));
                g.fillEllipse(hx - 2.f, hy - 2.f, 4.f, 4.f);
            }
        }

        // Center origin
        g.setColour(SDFLookAndFeel::secondaryAccent);
        g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);

        // Ray count label
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.4f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
        g.drawText(juce::String(numRays) + " rays", area.removeFromBottom(8.f), juce::Justification::centred);
    }

    // --- Mode 2: Acoustic bouncing with multi-ray + energy visualization ---
    void drawAcousticMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        drawShapeOutline(g, area, 0.3f);

        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.32f;

        int maxBounces = algData ? algData->maxBounces : (1 + static_cast<int>(std::floor(topoMorph * 5.f)));

        // Show multiple representative rays (8 of the 64)
        int dispRays = 8;
        float goldenAngle = 2.39996322f;

        // Animated: cycle through rays
        float cycleTime = 4.f;
        float progress = std::fmod(animPhase / cycleTime, 1.f);
        int activeDispRay = static_cast<int>(progress * dispRays) % dispRays;

        for (int ray = 0; ray < dispRays; ++ray)
        {
            float baseAngle = ray * goldenAngle;
            float dirX = std::cos(baseAngle) * 0.7f;
            float dirY = std::sin(baseAngle) * 0.7f;
            float len = std::sqrt(dirX * dirX + dirY * dirY);
            dirX /= len; dirY /= len;

            float posX = 0.f, posY = 0.f;
            float energy = 1.f;
            bool isActiveRay = (ray == activeDispRay);

            for (int b = 0; b < maxBounces; ++b)
            {
                float bestT = 0.5f;
                for (float t = 0.05f; t < 3.f; t += 0.02f)
                {
                    float tx = posX + dirX * t;
                    float ty = posY + dirY * t;
                    float angle = std::atan2(ty, tx);
                    if (angle < 0.f) angle += TWO_PI;
                    float surfR = getContourRadius(angle);
                    if (std::sqrt(tx * tx + ty * ty) >= surfR) { bestT = t; break; }
                }

                float hitX = posX + dirX * bestT;
                float hitY = posY + dirY * bestT;

                float alpha = energy * (isActiveRay ? 0.6f : 0.15f);
                auto col = isActiveRay ? SDFLookAndFeel::secondaryAccent : SDFLookAndFeel::secondaryAccent.withAlpha(0.5f);
                g.setColour(col.withAlpha(alpha));
                g.drawLine(cx + posX * scale, cy - posY * scale,
                           cx + hitX * scale, cy - hitY * scale,
                           isActiveRay ? 1.2f : 0.5f);

                if (isActiveRay)
                {
                    g.setColour(SDFLookAndFeel::primaryAccent.withAlpha(energy * 0.8f));
                    float dotSize = 2.f + energy * 2.f;
                    g.fillEllipse(cx + hitX * scale - dotSize * 0.5f, cy - hitY * scale - dotSize * 0.5f,
                                  dotSize, dotSize);
                }

                // Reflect
                float normLen = std::sqrt(hitX * hitX + hitY * hitY);
                float nx = (normLen > 0.001f) ? hitX / normLen : 0.f;
                float ny = (normLen > 0.001f) ? hitY / normLen : 1.f;
                float dot = dirX * nx + dirY * ny;
                dirX -= 2.f * dot * nx;
                dirY -= 2.f * dot * ny;
                posX = hitX - nx * 0.02f;
                posY = hitY - ny * 0.02f;

                // Surface-dependent energy decay (matching DSP)
                energy *= 0.7f + 0.2f * std::abs(dot);
            }
        }

        // Origin mic dot
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.5f));
        g.fillEllipse(cx - 2.f, cy - 2.f, 4.f, 4.f);

        // Labels
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.4f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
        g.drawText("64 rays " + juce::String(maxBounces) + "b", area.removeFromBottom(8.f), juce::Justification::centred);
    }

    // --- Mode 3: Granular with accurate grain count + freq visualization ---
    void drawGrainMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        drawShapeOutline(g, area, 0.3f);

        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float scale = juce::jmin(area.getWidth(), area.getHeight()) * 0.32f;

        // Use exact DSP values when available
        int numGrains = algData ? algData->numGrains
                                : (16 + static_cast<int>(topoMorph * 240.f));
        numGrains = juce::jmin(numGrains, 256);

        // For display, cap at reasonable visual density
        int dispGrains = juce::jmin(numGrains, 64);
        float goldenAngle = 2.39996322f;

        // Animated pulse
        float cycleTime = 2.f;
        float progress = std::fmod(animPhase / cycleTime, 1.f);
        int activeGrain = static_cast<int>(progress * dispGrains) % dispGrains;

        for (int i = 0; i < dispGrains; ++i)
        {
            float angle, freq, curvature;

            if (algData && i < static_cast<int>(algData->grainData.size()))
            {
                angle = algData->grainData[static_cast<size_t>(i)].angle;
                freq = algData->grainData[static_cast<size_t>(i)].freq;
                curvature = algData->grainData[static_cast<size_t>(i)].curvature;
            }
            else
            {
                angle = std::fmod(static_cast<float>(i) * goldenAngle, TWO_PI);
                float r = getContourRadius(angle);
                float rNext = getContourRadius(angle + 0.1f);
                float rPrev = getContourRadius(angle - 0.1f);
                curvature = std::abs(2.f * r - rNext - rPrev) * 20.f;
                freq = 2.f + std::abs(curvature) * 10.f;
            }

            float r = getContourRadius(angle);
            float px = cx + std::cos(angle) * r * scale;
            float py = cy - std::sin(angle) * r * scale;

            float curvNorm = juce::jlimit(0.f, 1.f, std::abs(curvature) * 0.5f);
            bool isActive = (i == activeGrain);
            float baseDotSize = 2.f + curvNorm * 3.f;
            float dotSize = isActive ? baseDotSize + 3.f : baseDotSize;
            float alpha = isActive ? 1.f : (0.3f + curvNorm * 0.3f);

            auto col = SDFLookAndFeel::primaryAccent.interpolatedWith(
                SDFLookAndFeel::secondaryAccent, curvNorm);
            g.setColour(col.withAlpha(alpha));
            g.fillEllipse(px - dotSize * 0.5f, py - dotSize * 0.5f, dotSize, dotSize);

            // Active grain: show frequency as small oscillation lines
            if (isActive)
            {
                float pulseR = baseDotSize + 6.f;
                g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.4f));
                g.drawEllipse(px - pulseR, py - pulseR, pulseR * 2.f, pulseR * 2.f, 1.f);

                // Frequency indication: small sine wave near grain
                float freqNorm = juce::jlimit(0.f, 1.f, freq / 80.f);
                int waveSteps = 8;
                float waveLen = 10.f + (1.f - freqNorm) * 6.f;
                float waveAmp = 3.f;
                float waveDir = angle + PI * 0.5f;
                for (int w = 0; w < waveSteps - 1; ++w)
                {
                    float t0 = static_cast<float>(w) / waveSteps;
                    float t1 = static_cast<float>(w + 1) / waveSteps;
                    float wx0 = px + std::cos(waveDir) * (t0 - 0.5f) * waveLen;
                    float wy0 = py - std::sin(waveDir) * (t0 - 0.5f) * waveLen;
                    float wx1 = px + std::cos(waveDir) * (t1 - 0.5f) * waveLen;
                    float wy1 = py - std::sin(waveDir) * (t1 - 0.5f) * waveLen;
                    float sineOff0 = std::sin(t0 * freqNorm * 6.f * PI) * waveAmp;
                    float sineOff1 = std::sin(t1 * freqNorm * 6.f * PI) * waveAmp;
                    float perpX = -std::sin(waveDir);
                    float perpY = -std::cos(waveDir);
                    g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.5f));
                    g.drawLine(wx0 + perpX * sineOff0, wy0 + perpY * sineOff0,
                               wx1 + perpX * sineOff1, wy1 + perpY * sineOff1, 1.f);
                }
            }
        }

        // Grain count label
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.4f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
        g.drawText(juce::String(numGrains) + " grains", area.removeFromBottom(8.f), juce::Justification::centred);
    }

    // --- Mode 4: Spectral — professional spectrogram with 512 height slices ---
    void drawSpectralMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        constexpr int totalBins = sdf::SPECTRO_HEIGHT_SLICES;
        int activeBins = algData ? algData->activeBins
                                 : (8 + static_cast<int>(topoMorph * static_cast<float>(totalBins - 8)));
        activeBins = juce::jlimit(1, totalBins, activeBins);

        float areaW = area.getWidth();
        float maxH = area.getHeight() * 0.9f;

        // Animated scan line
        float sweepFrac = std::fmod(animPhase * 0.8f, 1.f);
        int sweepBin = static_cast<int>(sweepFrac * activeBins);

        // Find max weight for normalization
        float maxWeight = 0.01f;
        if (algData)
        {
            for (int h = 0; h < totalBins; ++h)
                maxWeight = std::max(maxWeight, algData->harmonicWeights[static_cast<size_t>(h)]);
        }

        // With 512 bins, each bin may be sub-pixel. Render as pixel-width columns.
        float colW = std::max(1.f, areaW / static_cast<float>(activeBins));
        int visBins = std::min(activeBins, static_cast<int>(areaW));

        for (int k = 0; k < visBins; ++k)
        {
            // Map visible column to bin index
            int binIdx = static_cast<int>(static_cast<float>(k) / visBins * activeBins);
            binIdx = juce::jlimit(0, totalBins - 1, binIdx);

            float weight;
            if (algData)
            {
                weight = algData->harmonicWeights[static_cast<size_t>(binIdx)] / maxWeight;
            }
            else
            {
                float angle = static_cast<float>(binIdx) / totalBins * TWO_PI;
                weight = juce::jlimit(0.1f, 1.f, getContourRadius(angle) * 2.f);
            }

            // Raw DSP weight — no rolloff
            float height = weight * maxH;

            bool isSweep = (binIdx == sweepBin);
            if (isSweep) height = juce::jmin(height * 1.2f, maxH);

            float x = area.getX() + static_cast<float>(k) * colW;
            float y = area.getBottom() - height;

            // Professional gradient: deep blue -> cyan -> green
            float normK = static_cast<float>(binIdx) / static_cast<float>(activeBins);
            float hue = 0.55f - normK * 0.2f;
            float sat = 0.7f;
            float bri = isSweep ? 0.95f : 0.75f;
            float alpha = isSweep ? 0.95f : 0.85f;

            auto barCol = juce::Colour::fromHSV(hue, sat, bri, alpha);
            g.setColour(barCol);
            g.fillRect(x, y, colW, height);

            // Bright top edge
            if (height > 2.f)
            {
                g.setColour(barCol.brighter(0.4f).withAlpha(alpha));
                g.fillRect(x, y, colW, 1.f);
            }

            // Vertical gradient overlay for depth (only if columns wide enough)
            if (height > 4.f && colW > 1.5f)
            {
                juce::ColourGradient grad(barCol.brighter(0.2f), x, y,
                                           barCol.darker(0.3f), x, area.getBottom(), false);
                g.setGradientFill(grad);
                g.fillRect(x + 0.5f, y + 1.f, colW - 1.f, height - 1.f);
            }
        }

        // Dim inactive region (beyond activeBins)
        if (activeBins < totalBins)
        {
            float inactiveX = area.getX() + static_cast<float>(visBins) * colW;
            float inactiveW = area.getRight() - inactiveX;
            if (inactiveW > 0.f)
            {
                g.setColour(juce::Colour(0x20778899));
                g.fillRect(inactiveX, area.getY(), inactiveW, area.getHeight());
            }
        }

        // Baseline
        g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.5f));
        g.drawLine(area.getX(), area.getBottom(), area.getRight(), area.getBottom(), 0.5f);

        // Labels
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.35f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
        g.drawText("HEIGHT SLICES", area.getX(), area.getY(), area.getWidth(), 8.f, juce::Justification::centredRight);
        g.drawText(juce::String(activeBins) + "/" + juce::String(totalBins),
                   area.removeFromBottom(8.f), juce::Justification::centredLeft);
    }

    // --- Mode 5: Lissajous traverse with exact DSP ratios + Y-dimension ---
    void drawTraverseMode(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        float cx = area.getCentreX();
        float cy = area.getCentreY();
        float baseScale = juce::jmin(area.getWidth(), area.getHeight()) * 0.42f;

        // Use exact Lissajous ratios from DSP when available
        float a, b, c, delta;
        if (algData)
        {
            a = algData->lissA;
            b = algData->lissB;
            c = algData->lissC;
            delta = algData->lissDelta;
        }
        else
        {
            if (topoMorph < 0.5f)
            {
                float t = topoMorph * 2.f;
                a = 1.f; b = 1.f + t; c = t * 1.5f; delta = t * 0.5f;
            }
            else
            {
                float t = (topoMorph - 0.5f) * 2.f;
                a = 1.f + t; b = 2.f + t; c = 1.5f + t * 3.5f; delta = 0.5f + t * 1.f;
            }
        }

        float r = scanRadius;

        // Compute max contour extent for auto-scaling
        float maxContourR = 0.3f;
        if (contour && !contour->empty())
        {
            for (auto& pt : *contour)
            {
                if (pt.valid)
                    maxContourR = std::max(maxContourR, std::sqrt(pt.x * pt.x + pt.z * pt.z));
            }
        }

        // Auto-scale: fit both shape outline and Lissajous to overlay
        float maxExtent = std::max(maxContourR, r);
        float visScale = baseScale / std::max(maxExtent, 0.1f);

        // Draw shape outline with aligned scale
        if (contour && !contour->empty())
        {
            juce::Path shapePath;
            bool started = false;
            for (size_t i = 0; i < contour->size(); ++i)
            {
                auto& pt = (*contour)[i];
                if (!pt.valid) continue;
                float px = cx + pt.x * visScale;
                float py = cy - pt.z * visScale;
                if (!started) { shapePath.startNewSubPath(px, py); started = true; }
                else shapePath.lineTo(px, py);
            }
            shapePath.closeSubPath();
            g.setColour(juce::Colour(0x1900ffff));
            g.fillPath(shapePath);
            g.setColour(SDFLookAndFeel::primaryAccent.withAlpha(0.2f));
            g.strokePath(shapePath, juce::PathStrokeType(1.f));
        }

        // Draw the full Lissajous path with Y-dimension encoded as brightness
        juce::Path lissPath;
        int steps = 256;
        for (int i = 0; i < steps; ++i)
        {
            float t = static_cast<float>(i) / steps * TWO_PI;
            float x = std::sin(a * t + delta) * r;
            float z = std::sin(b * t) * r;
            float px = cx + x * visScale;
            float py = cy - z * visScale;
            if (i == 0) lissPath.startNewSubPath(px, py);
            else lissPath.lineTo(px, py);
        }
        lissPath.closeSubPath();

        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.08f));
        g.fillPath(lissPath);

        // Draw path segments with Y-height encoded as color brightness
        for (int i = 0; i < steps; ++i)
        {
            float t0 = static_cast<float>(i) / steps * TWO_PI;
            float t1 = static_cast<float>(i + 1) / steps * TWO_PI;
            float x0 = std::sin(a * t0 + delta) * r;
            float z0 = std::sin(b * t0) * r;
            float y0 = std::sin(c * t0) * r * 0.4f;
            float x1 = std::sin(a * t1 + delta) * r;
            float z1 = std::sin(b * t1) * r;

            float yNorm = (y0 + r * 0.4f) / (r * 0.8f + 0.001f);
            yNorm = juce::jlimit(0.f, 1.f, yNorm);
            float alpha = 0.2f + yNorm * 0.5f;
            float thick = 0.5f + yNorm * 1.f;

            g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(alpha));
            g.drawLine(cx + x0 * visScale, cy - z0 * visScale,
                       cx + x1 * visScale, cy - z1 * visScale, thick);
        }

        // Animated dot traveling along the path
        float dotT = std::fmod(animPhase * 1.2f, 1.f) * TWO_PI;
        float dotX = cx + std::sin(a * dotT + delta) * r * visScale;
        float dotZ = cy - std::sin(b * dotT) * r * visScale;
        float dotY = std::sin(c * dotT) * r * 0.4f;

        // Trail
        for (int t = 8; t >= 1; --t)
        {
            float trailT = dotT - static_cast<float>(t) * 0.04f;
            float tx = cx + std::sin(a * trailT + delta) * r * visScale;
            float ty = cy - std::sin(b * trailT) * r * visScale;
            float trailAlpha = (1.f - static_cast<float>(t) / 9.f) * 0.4f;
            float trailSize = 2.f + (1.f - static_cast<float>(t) / 9.f) * 2.f;
            g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(trailAlpha));
            g.fillEllipse(tx - trailSize * 0.5f, ty - trailSize * 0.5f, trailSize, trailSize);
        }

        // Main dot — size encodes Y height
        float dotYNorm = (dotY + r * 0.4f) / (r * 0.8f + 0.001f);
        dotYNorm = juce::jlimit(0.f, 1.f, dotYNorm);
        float mainDotSize = 4.f + dotYNorm * 4.f;
        g.setColour(SDFLookAndFeel::tertiaryAccent);
        g.fillEllipse(dotX - mainDotSize * 0.5f, dotZ - mainDotSize * 0.5f, mainDotSize, mainDotSize);
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.2f));
        g.fillEllipse(dotX - mainDotSize, dotZ - mainDotSize, mainDotSize * 2.f, mainDotSize * 2.f);

        // Ratio label
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.3f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(6.f), 0));
        g.drawText(juce::String(a, 1) + ":" + juce::String(b, 1) + ":" + juce::String(c, 1),
                   area.removeFromBottom(8.f), juce::Justification::centred);
    }

    std::shared_ptr<const std::vector<ContourPoint>> contour;
    std::shared_ptr<const ScanAlgorithmData> algData;
    int scanMode = 0;
    float scanHeight = 0.f;
    float topoMorph = 1.f;
    float scanRadius = 1.f;
    float distScale = 3.f;
    float playheadPhase = -1.f;
    float animPhase = 0.f;
};
