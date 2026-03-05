#include "ADSRDisplay.h"
#include <cmath>

ADSRDisplay::ADSRDisplay(juce::AudioProcessorValueTreeState& a)
    : apvts(a)
{
    attack  = apvts.getRawParameterValue("attack")->load();
    decay   = apvts.getRawParameterValue("decay")->load();
    sustain = apvts.getRawParameterValue("sustain")->load();
    release = apvts.getRawParameterValue("release")->load();

    apvts.addParameterListener("attack", this);
    apvts.addParameterListener("decay", this);
    apvts.addParameterListener("sustain", this);
    apvts.addParameterListener("release", this);
}

ADSRDisplay::~ADSRDisplay()
{
    apvts.removeParameterListener("attack", this);
    apvts.removeParameterListener("decay", this);
    apvts.removeParameterListener("sustain", this);
    apvts.removeParameterListener("release", this);
}

void ADSRDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "attack")       attack = newValue;
    else if (parameterID == "decay")   decay = newValue;
    else if (parameterID == "sustain") sustain = newValue;
    else if (parameterID == "release") release = newValue;

    juce::MessageManager::callAsync([this]() { repaint(); });
}

ADSRDisplay::EnvPoints ADSRDisplay::computePoints(juce::Rectangle<float> area) const
{
    EnvPoints pts;
    float w = area.getWidth();
    float h = area.getHeight();
    float x0 = area.getX();
    float y0 = area.getY();

    // Time proportions: A, D, sustain-hold(20% fixed), R share the width
    float totalTime = attack + decay + release;
    float sustainHoldFrac = 0.20f;
    float timeFrac = 1.f - sustainHoldFrac;

    float minPx = 12.f;
    float aW, dW, rW, sW;

    if (totalTime < 0.001f)
    {
        aW = dW = rW = minPx;
        sW = w - aW - dW - rW;
    }
    else
    {
        aW = std::max(minPx, (attack / totalTime) * timeFrac * w);
        dW = std::max(minPx, (decay / totalTime) * timeFrac * w);
        rW = std::max(minPx, (release / totalTime) * timeFrac * w);
        sW = sustainHoldFrac * w;

        // Normalize so they fit
        float sum = aW + dW + sW + rW;
        if (sum > w)
        {
            float scale = w / sum;
            aW *= scale;
            dW *= scale;
            sW *= scale;
            rW *= scale;
        }
    }

    pts.start      = { x0, y0 + h };
    pts.attackPeak  = { x0 + aW, y0 };
    pts.decayEnd    = { x0 + aW + dW, y0 + h * (1.f - sustain) };
    pts.sustainEnd  = { x0 + aW + dW + sW, y0 + h * (1.f - sustain) };
    pts.releaseEnd  = { x0 + aW + dW + sW + rW, y0 + h };

    return pts;
}

ADSRDisplay::DragTarget ADSRDisplay::hitTest(juce::Point<float> pos, const EnvPoints& pts) const
{
    float tol = 10.f;
    if (pos.getDistanceFrom(pts.attackPeak) < tol) return Attack;
    if (pos.getDistanceFrom(pts.decayEnd) < tol) return Decay;
    if (pos.getDistanceFrom(pts.releaseEnd) < tol) return Release;
    return None;
}

juce::Rectangle<float> ADSRDisplay::getModHandleRect() const
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    float hw = 28.f, hh = 13.f;
    return { bounds.getRight() - hw - 2.f, bounds.getBottom() - hh - 2.f, hw, hh };
}

juce::Point<int> ADSRDisplay::getModHandleCentre() const
{
    return getModHandleRect().getCentre().toInt();
}

void ADSRDisplay::writeParam(const juce::String& id, float value)
{
    if (auto* param = apvts.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

// Generate exponential curve segment using multi-step polyline (gnarly3 technique)
// curvature > 0 = concave up (attack), curvature < 0 = concave down (decay/release)
static void addCurveSegment(juce::Path& path,
                            juce::Point<float> from, juce::Point<float> to,
                            float curvature, int steps = 24)
{
    for (int i = 1; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(steps);

        // Apply exponential shaping to t
        float shaped;
        if (curvature > 0.f)
            shaped = std::pow(t, 2.5f); // concave up: slow start, fast end (attack)
        else if (curvature < 0.f)
            shaped = 1.f - std::pow(1.f - t, 2.5f); // concave down: fast start, slow end (decay/release)
        else
            shaped = t; // linear

        float px = from.x + (to.x - from.x) * t;
        float py = from.y + (to.y - from.y) * shaped;
        path.lineTo(px, py);
    }
}

void ADSRDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.f);

    // Background
    g.setColour(SDFLookAndFeel::panelBg);
    g.fillRoundedRectangle(bounds, 8.f);
    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 8.f, 1.f);

    // Reserve space for labels + values at bottom
    float labelRowH = 22.f;
    auto inner = bounds.reduced(6.f, 3.f);
    auto labelArea = inner.removeFromBottom(labelRowH);
    inner.removeFromBottom(1.f); // tiny gap
    auto drawArea = inner;

    if (drawArea.getHeight() < 10.f || drawArea.getWidth() < 40.f)
        return;

    auto pts = computePoints(drawArea);

    // --- Build envelope path using multi-segment polyline ---
    juce::Path envPath;
    envPath.startNewSubPath(pts.start);
    addCurveSegment(envPath, pts.start, pts.attackPeak, 1.f);   // concave up
    addCurveSegment(envPath, pts.attackPeak, pts.decayEnd, -1.f); // concave down
    envPath.lineTo(pts.sustainEnd);                                // flat hold
    addCurveSegment(envPath, pts.sustainEnd, pts.releaseEnd, -1.f); // concave down

    // --- Filled area under curve ---
    juce::Path fillPath(envPath);
    fillPath.lineTo(drawArea.getRight(), drawArea.getBottom());
    fillPath.lineTo(drawArea.getX(), drawArea.getBottom());
    fillPath.closeSubPath();

    // Vertical gradient fill
    g.setGradientFill(juce::ColourGradient(
        SDFLookAndFeel::tertiaryAccent.withAlpha(0.22f), drawArea.getX(), drawArea.getY(),
        SDFLookAndFeel::tertiaryAccent.withAlpha(0.04f), drawArea.getX(), drawArea.getBottom(),
        false));
    g.fillPath(fillPath);

    // --- Envelope line ---
    // Glow
    g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.1f));
    g.strokePath(envPath, juce::PathStrokeType(5.f));
    // Main line
    g.setColour(SDFLookAndFeel::tertiaryAccent);
    g.strokePath(envPath, juce::PathStrokeType(1.5f));

    // --- Dashed vertical segment dividers ---
    g.setColour(SDFLookAndFeel::borderColour.withAlpha(0.2f));
    auto drawDash = [&](float x)
    {
        for (float dy = drawArea.getY(); dy < drawArea.getBottom(); dy += 5.f)
            g.drawLine(x, dy, x, std::min(dy + 2.5f, drawArea.getBottom()), 0.5f);
    };
    drawDash(pts.attackPeak.x);
    drawDash(pts.decayEnd.x);
    drawDash(pts.sustainEnd.x);

    // --- Control points (ring style with filled center on drag + glow) ---
    auto drawPoint = [&](juce::Point<float> p, bool active)
    {
        float r = active ? 7.f : 6.f;
        // Glow ring on active
        if (active)
        {
            g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.20f));
            float gr = r + 4.f;
            g.fillEllipse(p.x - gr, p.y - gr, gr * 2.f, gr * 2.f);
        }
        // Outer ring
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(active ? 1.f : 0.65f));
        g.fillEllipse(p.x - r, p.y - r, r * 2.f, r * 2.f);
        // Inner fill
        float ir = r - 1.8f;
        g.setColour(active ? SDFLookAndFeel::tertiaryAccent.withAlpha(0.3f) : SDFLookAndFeel::panelBg);
        g.fillEllipse(p.x - ir, p.y - ir, ir * 2.f, ir * 2.f);
    };

    drawPoint(pts.attackPeak, currentDrag == Attack);
    drawPoint(pts.decayEnd, currentDrag == Decay);
    drawPoint(pts.releaseEnd, currentDrag == Release);

    // --- Sustain level line (horizontal dashed) ---
    if (sustain > 0.01f && sustain < 0.99f)
    {
        float sy = pts.decayEnd.y;
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.12f));
        for (float dx = drawArea.getX(); dx < drawArea.getRight(); dx += 6.f)
            g.drawLine(dx, sy, std::min(dx + 3.f, drawArea.getRight()), sy, 0.5f);
    }

    // --- Segment labels and values ---
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(8.f), juce::Font::bold));

    float aMid = (pts.start.x + pts.attackPeak.x) * 0.5f;
    float dMid = (pts.attackPeak.x + pts.decayEnd.x) * 0.5f;
    float sMid = (pts.decayEnd.x + pts.sustainEnd.x) * 0.5f;
    float rMid = (pts.sustainEnd.x + pts.releaseEnd.x) * 0.5f;

    auto drawSegLabel = [&](float cx, const juce::String& label, const juce::String& value)
    {
        float lw = 28.f;
        // Label
        g.setColour(SDFLookAndFeel::mutedText.withAlpha(0.6f));
        g.drawText(label, juce::Rectangle<float>(cx - lw * 0.5f, labelArea.getY(), lw, 10.f),
                   juce::Justification::centred);
        // Value
        g.setColour(SDFLookAndFeel::tertiaryAccent.withAlpha(0.5f));
        g.drawText(value, juce::Rectangle<float>(cx - lw * 0.5f, labelArea.getY() + 10.f, lw, 10.f),
                   juce::Justification::centred);
    };

    auto fmtTime = [](float t) -> juce::String
    {
        if (t >= 1.f) return juce::String(t, 1) + "s";
        return juce::String(static_cast<int>(t * 1000.f)) + "ms";
    };

    drawSegLabel(aMid, "A", fmtTime(attack));
    drawSegLabel(dMid, "D", fmtTime(decay));
    drawSegLabel(sMid, "S", juce::String(static_cast<int>(sustain * 100.f)) + "%");
    drawSegLabel(rMid, "R", fmtTime(release));

    // --- MOD drag handle ---
    auto modRect = getModHandleRect();
    g.setColour(SDFLookAndFeel::secondaryAccent.withAlpha(modDragging ? 0.9f : 0.4f));
    g.fillRoundedRectangle(modRect, 5.f);
    g.setColour(SDFLookAndFeel::bgColour);
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), SDFLookAndFeel::scaled(7.f), juce::Font::bold));
    g.drawText("MOD", modRect, juce::Justification::centred);
}

void ADSRDisplay::mouseDown(const juce::MouseEvent& e)
{
    // Check mod handle first
    if (getModHandleRect().contains(e.position))
    {
        modDragging = true;
        modDragPos = e.getEventRelativeTo(getParentComponent()).getPosition();
        if (onModDragStarted) onModDragStarted();
        return;
    }

    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    auto inner = bounds.reduced(6.f, 3.f);
    inner.removeFromBottom(23.f); // label row
    auto pts = computePoints(inner);
    currentDrag = hitTest(e.position, pts);
}

void ADSRDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (modDragging)
    {
        modDragPos = e.getEventRelativeTo(getParentComponent()).getPosition();
        if (onModDragging) onModDragging(modDragPos);
        getParentComponent()->repaint();
        return;
    }

    if (currentDrag == None) return;

    auto bounds = getLocalBounds().toFloat().reduced(2.f);
    auto inner = bounds.reduced(6.f, 3.f);
    inner.removeFromBottom(23.f);
    auto drawArea = inner;
    auto pts = computePoints(drawArea);
    auto pos = e.position;

    if (currentDrag == Attack)
    {
        // Map x position to attack time proportionally
        float xRel = std::clamp(pos.x - drawArea.getX(), 0.f, drawArea.getWidth() * 0.5f);
        float frac = xRel / drawArea.getWidth();
        float newAttack = std::clamp(frac * 4.f, 0.001f, 2.f);
        writeParam("attack", newAttack);
    }
    else if (currentDrag == Decay)
    {
        // Vertical = sustain level
        float yFrac = std::clamp((pos.y - drawArea.getY()) / drawArea.getHeight(), 0.f, 1.f);
        writeParam("sustain", 1.f - yFrac);

        // Horizontal = decay time
        float xRel = std::clamp(pos.x - pts.attackPeak.x, 0.f, drawArea.getWidth() * 0.5f);
        float frac = xRel / drawArea.getWidth();
        writeParam("decay", std::clamp(frac * 4.f, 0.001f, 2.f));
    }
    else if (currentDrag == Release)
    {
        float xRel = std::clamp(pos.x - pts.sustainEnd.x, 0.f, drawArea.getWidth() * 0.5f);
        float frac = xRel / drawArea.getWidth();
        writeParam("release", std::clamp(frac * 10.f, 0.001f, 5.f));
    }
}

void ADSRDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (modDragging)
    {
        modDragging = false;
        modDragPos = e.getEventRelativeTo(getParentComponent()).getPosition();
        if (onModDragEnded) onModDragEnded(modDragPos);
        getParentComponent()->repaint();
        return;
    }
    currentDrag = None;
}
