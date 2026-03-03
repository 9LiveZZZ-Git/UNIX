#include "WaveformScope.h"
#include "SDFLookAndFeel.h"
#include <cmath>

WaveformScope::WaveformScope()
{
    startTimerHz(30);
}

WaveformScope::~WaveformScope()
{
    stopTimer();
}

void WaveformScope::timerCallback()
{
    repaint();
}

void WaveformScope::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(SDFLookAndFeel::bgColour);
    g.fillRect(bounds);

    // Border frame
    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.reduced(1.f), 6.f, 1.f);

    auto scopeArea = bounds.reduced(8.f);
    auto heightBarArea = scopeArea.removeFromRight(8.f);
    auto mriBarArea = scopeArea.removeFromBottom(20.f);

    drawGrid(g, scopeArea);
    drawWaveform(g, scopeArea);
    drawPlayhead(g, scopeArea);
    drawHeightBar(g, heightBarArea);

    // Scan mode label (top-left of scope)
    {
        static const juce::StringArray modeNames = { "CONTOUR", "MARCH", "ACOUSTIC", "GRAIN", "SPECTRAL", "TRAVERSE" };
        juce::String modeName = (scanMode >= 0 && scanMode < modeNames.size()) ? modeNames[scanMode] : "?";
        g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(0.5f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(9.f), juce::Font::bold));
        g.drawText(modeName, static_cast<int>(scopeArea.getX()) + 4,
                   static_cast<int>(scopeArea.getY()) + 2, 80, 12, juce::Justification::centredLeft);
    }

    if (scanMode == 0)
    {
        drawMRIIndicator(g, mriBarArea);

        if (topoMorph > 0.3f && contour != nullptr)
            drawMiniCrossSection(g, juce::Rectangle<float>(
                scopeArea.getRight() - 78.f, scopeArea.getY() + 4.f, 70.f, 70.f));
    }
}

void WaveformScope::drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xff122020));
    int divisions = 8;
    for (int i = 1; i < divisions; ++i)
    {
        float xFrac = static_cast<float>(i) / divisions;
        float yFrac = static_cast<float>(i) / divisions;
        g.drawHorizontalLine(static_cast<int>(area.getY() + yFrac * area.getHeight()),
                             area.getX(), area.getRight());
        g.drawVerticalLine(static_cast<int>(area.getX() + xFrac * area.getWidth()),
                           area.getY(), area.getBottom());
    }

    // Center zero line
    g.setColour(juce::Colour(0xff2a4a4a));
    float centerY = area.getCentreY();
    g.drawHorizontalLine(static_cast<int>(centerY), area.getX(), area.getRight());

    // Axis labels
    g.setColour(juce::Colour(0xff446666));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), SDFLookAndFeel::scaled(9.f), 0));
    g.drawText("+1", static_cast<int>(area.getX()), static_cast<int>(area.getY()),
               20, 12, juce::Justification::left);
    g.drawText("0", static_cast<int>(area.getX()), static_cast<int>(centerY - 6),
               20, 12, juce::Justification::left);
    g.drawText("-1", static_cast<int>(area.getX()), static_cast<int>(area.getBottom() - 12),
               20, 12, juce::Justification::left);
}

void WaveformScope::drawWaveform(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (wavetable == nullptr) return;

    int w = static_cast<int>(area.getWidth());
    if (w <= 0) return;
    float halfH = area.getHeight() * 0.5f;
    float centerY = area.getCentreY();

    // Interpolated sample lookup
    auto lerpSample = [&](int pixel) -> float {
        float fIdx = (static_cast<float>(pixel) / w) * sdf::TABLE_SIZE;
        int i0 = static_cast<int>(fIdx) % sdf::TABLE_SIZE;
        int i1 = (i0 + 1) % sdf::TABLE_SIZE;
        float frac = fIdx - std::floor(fIdx);
        return (*wavetable)[i0] + ((*wavetable)[i1] - (*wavetable)[i0]) * frac;
    };

    // Filled area
    juce::Path fillPath;
    fillPath.startNewSubPath(area.getX(), centerY);
    for (int i = 0; i < w; ++i)
    {
        float sample = lerpSample(i);
        float px = area.getX() + static_cast<float>(i);
        float py = centerY - sample * halfH;
        fillPath.lineTo(px, py);
    }
    fillPath.lineTo(area.getRight(), centerY);
    fillPath.closeSubPath();
    g.setColour(juce::Colour(0x2800ffff));
    g.fillPath(fillPath);

    // Waveform line
    juce::Path linePath;
    for (int i = 0; i < w; ++i)
    {
        float sample = lerpSample(i);
        float px = area.getX() + static_cast<float>(i);
        float py = centerY - sample * halfH;
        if (i == 0) linePath.startNewSubPath(px, py);
        else linePath.lineTo(px, py);
    }
    g.setColour(SDFLookAndFeel::primaryAccent);
    g.strokePath(linePath, juce::PathStrokeType(2.f));
}

void WaveformScope::drawMiniCrossSection(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (contour == nullptr || contour->empty()) return;

    g.setColour(juce::Colour(0xff0a1414));
    g.fillRoundedRectangle(area, 3.f);
    g.setColour(SDFLookAndFeel::borderColour);
    g.drawRoundedRectangle(area, 3.f, 1.f);

    // Draw contour shape
    float cx = area.getCentreX();
    float cy = area.getCentreY();
    float scale = area.getWidth() * 0.3f;

    juce::Path contourPath;
    bool started = false;
    for (size_t i = 0; i < contour->size(); ++i)
    {
        auto& pt = (*contour)[i];
        if (!pt.valid) continue;
        float px = cx + pt.x * scale;
        float py = cy - pt.z * scale;
        if (!started) { contourPath.startNewSubPath(px, py); started = true; }
        else contourPath.lineTo(px, py);
    }
    contourPath.closeSubPath();

    g.setColour(juce::Colour(0x1900ffff));
    g.fillPath(contourPath);
    g.setColour(SDFLookAndFeel::primaryAccent.withAlpha(0.6f));
    g.strokePath(contourPath, juce::PathStrokeType(1.f));

    g.setColour(SDFLookAndFeel::mutedText);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), juce::Font::bold));
    g.drawText("CROSS-SEC", area.removeFromTop(10), juce::Justification::centred);
}

void WaveformScope::drawHeightBar(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xff0a1414));
    g.fillRect(area);

    float normalized = (scanHeight + 0.9f) / 1.8f;
    float indicatorY = area.getBottom() - normalized * area.getHeight();

    g.setColour(juce::Colour(0xff00aaaa));
    g.fillRect(area.getX(), indicatorY - 3.f, area.getWidth(), 6.f);
}

void WaveformScope::drawMRIIndicator(juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Colour barColour(0x00, 0xb4, 0xaa);
    g.setColour(barColour.withAlpha(topoMorph * 0.15f));
    g.fillRect(area);

    g.setColour(juce::Colour(0x00, 0xdc, 0xd2).withAlpha(0.2f + topoMorph * 0.4f));
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), SDFLookAndFeel::scaled(9.f), 0));
    juce::String text = "MRI SLICE Y=" + juce::String(scanHeight, 2);
    g.drawText(text, area, juce::Justification::centred);
}

void WaveformScope::drawPlayhead(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (playheadPhase < 0.f) return;

    float px = area.getX() + playheadPhase * area.getWidth();

    g.setColour(juce::Colour(0x3300ff88));
    g.drawVerticalLine(static_cast<int>(px), area.getY(), area.getBottom());

    if (wavetable != nullptr)
    {
        int tableIdx = static_cast<int>(playheadPhase * sdf::TABLE_SIZE) % sdf::TABLE_SIZE;
        float sample = (*wavetable)[tableIdx];
        float py = area.getCentreY() - sample * area.getHeight() * 0.5f;
        // Glow ring behind dot
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.20f));
        g.fillEllipse(px - 8.f, py - 8.f, 16.f, 16.f);
        // Dot
        g.setColour(SDFLookAndFeel::tertiaryAccent);
        g.fillEllipse(px - 4.f, py - 4.f, 8.f, 8.f);
    }
}
