#include "SDFViewport3D.h"
#include <cmath>

SDFViewport3D::SDFViewport3D(juce::AudioProcessorValueTreeState& a)
    : apvts(a)
{
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(false);
    openGLContext.attachTo(*this);
    startTimerHz(30);
}

SDFViewport3D::~SDFViewport3D()
{
    stopTimer();
    openGLContext.detach();
}

void SDFViewport3D::timerCallback()
{
    if (!isDragging)
        cameraAzimuth += 0.006f;  // doubled since timer is 30Hz (was 60Hz)
    openGLContext.triggerRepaint();
}

// Mouse interaction
void SDFViewport3D::mouseDown(const juce::MouseEvent& e)
{
    isDragging = true;
    lastMousePos = e.position;
}

void SDFViewport3D::mouseDrag(const juce::MouseEvent& e)
{
    auto delta = e.position - lastMousePos;
    cameraAzimuth += delta.x * 0.008f;
    cameraPitch = juce::jlimit(0.05f, 1.4f, cameraPitch - delta.y * 0.008f);
    lastMousePos = e.position;
}

void SDFViewport3D::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
}

void SDFViewport3D::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    cameraDistance = juce::jlimit(1.5f, 6.0f, cameraDistance - wheel.deltaY * 0.5f);
}

void SDFViewport3D::setVoxelSDF(std::shared_ptr<const VoxelSDF> sdf)
{
    std::lock_guard<std::mutex> lock(voxelMutex);
    pendingVoxelSDF = sdf;
    voxelNeedsUpload = true;
}

void SDFViewport3D::resized() {}

// OpenGL
void SDFViewport3D::newOpenGLContextCreated()
{
    createShader();
    createQuad();
}

void SDFViewport3D::createQuad()
{
    float vertices[] = {
        -1.f, -1.f,
         1.f, -1.f,
        -1.f,  1.f,
         1.f,  1.f
    };

    juce::gl::glGenVertexArrays(1, &vao);
    juce::gl::glBindVertexArray(vao);

    juce::gl::glGenBuffers(1, &vbo);
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(vertices), vertices, juce::gl::GL_STATIC_DRAW);

    juce::gl::glEnableVertexAttribArray(0);
    juce::gl::glVertexAttribPointer(0, 2, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 2 * sizeof(float), nullptr);

    juce::gl::glBindVertexArray(0);
}

void SDFViewport3D::createShader()
{
    shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);

    if (!shader->addVertexShader(getVertexShader()) ||
        !shader->addFragmentShader(getFragmentShader()) ||
        !shader->link())
    {
        // Capture error BEFORE reset clears it
        auto err = shader->getLastError();
        DBG("Shader compile error: " + err);
        shader.reset();
    }
}

void SDFViewport3D::renderOpenGL()
{
    // Process deferred texture/skybox uploads on GL thread
    textureSystem.processGLUploads();
    skyboxSystem.processGLUpload();

    // Upload voxel 3D texture if pending
    {
        std::lock_guard<std::mutex> lock(voxelMutex);
        if (voxelNeedsUpload && pendingVoxelSDF && pendingVoxelSDF->res > 0)
        {
            if (voxelTexture3D == 0)
                juce::gl::glGenTextures(1, &voxelTexture3D);

            int r = pendingVoxelSDF->res;
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_3D, voxelTexture3D);
            juce::gl::glTexImage3D(juce::gl::GL_TEXTURE_3D, 0, juce::gl::GL_R32F,
                                   r, r, r, 0, juce::gl::GL_RED, juce::gl::GL_FLOAT,
                                   pendingVoxelSDF->data());
            juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_3D, juce::gl::GL_TEXTURE_MIN_FILTER, juce::gl::GL_LINEAR);
            juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_3D, juce::gl::GL_TEXTURE_MAG_FILTER, juce::gl::GL_LINEAR);
            juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_3D, juce::gl::GL_TEXTURE_WRAP_S, juce::gl::GL_CLAMP_TO_EDGE);
            juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_3D, juce::gl::GL_TEXTURE_WRAP_T, juce::gl::GL_CLAMP_TO_EDGE);
            juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_3D, juce::gl::GL_TEXTURE_WRAP_R, juce::gl::GL_CLAMP_TO_EDGE);
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_3D, 0);

            currentVoxelSDF = pendingVoxelSDF;
            voxelNeedsUpload = false;
        }
    }

    if (!shader)
        return;

    auto desktopScale = static_cast<float>(openGLContext.getRenderingScale());
    auto width = static_cast<float>(getWidth()) * desktopScale;
    auto height = static_cast<float>(getHeight()) * desktopScale;

    juce::gl::glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    shader->use();

    // Resolution
    if (auto u = shader->getUniformIDFromName("uRes"))
        juce::gl::glUniform2f(u, width, height);

    // Time
    static auto startTime = juce::Time::getMillisecondCounterHiRes();
    float time = static_cast<float>((juce::Time::getMillisecondCounterHiRes() - startTime) / 1000.0);
    if (auto u = shader->getUniformIDFromName("uTime"))
        juce::gl::glUniform1f(u, time);

    // Camera
    if (auto u = shader->getUniformIDFromName("uCa"))
        juce::gl::glUniform1f(u, cameraAzimuth);
    if (auto u = shader->getUniformIDFromName("uCp"))
        juce::gl::glUniform1f(u, cameraPitch);
    if (auto u = shader->getUniformIDFromName("uCd"))
        juce::gl::glUniform1f(u, cameraDistance);

    // SDF params from APVTS
    auto setInt = [&](const char* name, const char* paramId) {
        if (auto u = shader->getUniformIDFromName(name))
            juce::gl::glUniform1i(u, static_cast<int>(apvts.getRawParameterValue(paramId)->load()));
    };
    auto setFloat = [&](const char* name, const char* paramId) {
        if (auto u = shader->getUniformIDFromName(name))
        {
            float val = getModulatedValue ? getModulatedValue(paramId)
                                          : apvts.getRawParameterValue(paramId)->load();
            juce::gl::glUniform1f(u, val);
        }
    };

    setInt("uS1", "shape1");
    setInt("uS2", "shape2");
    setInt("uOp", "operation");
    setFloat("uSz1", "size1");
    setFloat("uSz2", "size2");
    setFloat("uK", "smoothK");
    setFloat("uOX", "offsetX");
    setFloat("uOY", "offsetY");
    setFloat("uTw", "twist");
    setFloat("uScanY", "scanHeight");
    setFloat("uTopoMorph", "topoMorph");
    setInt("uScanMode", "scanMode");
    setFloat("uScanR", "scanRadius");

    // SuperFormula params
    setFloat("uSfM", "sfM");
    setFloat("uSfN1", "sfN1");
    setFloat("uSfN2", "sfN2");
    setFloat("uSfN3", "sfN3");

    // Onion shell
    {
        bool onion = apvts.getRawParameterValue("onionEnable")->load() > 0.5f;
        if (auto u = shader->getUniformIDFromName("uOnionEnable"))
            juce::gl::glUniform1i(u, onion ? 1 : 0);
        setFloat("uOnionThickness", "onionThickness");
    }

    // Stairs operation
    setFloat("uStairCount", "stairCount");

    // Bind textures from TextureSystem (units 0-5)
    textureSystem.bindToShader(*shader, 0);

    // Bind skybox (unit 6)
    skyboxSystem.bindToShader(*shader, 6);

    // Skybox params
    setFloat("uSkyboxExp", "skyboxExposure");
    {
        float rotDeg = apvts.getRawParameterValue("skyboxRotation")->load();
        if (auto u = shader->getUniformIDFromName("uSkyboxRot"))
            juce::gl::glUniform1f(u, rotDeg * 3.14159f / 180.f);
    }
    setFloat("uSkyboxRefl", "skyboxReflect");
    setFloat("uSkyboxBlur", "skyboxBlur");

    // Bind voxel 3D texture on unit 7
    {
        bool hasVoxel = (voxelTexture3D != 0 && currentVoxelSDF != nullptr);
        juce::gl::glActiveTexture(juce::gl::GL_TEXTURE7);
        if (hasVoxel)
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_3D, voxelTexture3D);
        else
            juce::gl::glBindTexture(juce::gl::GL_TEXTURE_3D, 0);
        if (auto u = shader->getUniformIDFromName("uVoxelSDF"))
            juce::gl::glUniform1i(u, 7);
        if (auto u = shader->getUniformIDFromName("uHasVoxel"))
            juce::gl::glUniform1i(u, hasVoxel ? 1 : 0);
        juce::gl::glActiveTexture(juce::gl::GL_TEXTURE0);
    }

    setFloat("uTexScale", "texScale");
    setFloat("uNormInt", "normIntensity");
    setFloat("uRoughOff", "roughOffset");
    setFloat("uTexBlend", "texBlend");
    setFloat("uDispAmt", "dispAmt");
    setFloat("uTexBright", "texBright");
    setFloat("uAOInt", "aoIntensity");
    setFloat("uEmitInt", "emIntensity");

    // Draw fullscreen quad
    juce::gl::glBindVertexArray(vao);
    juce::gl::glDrawArrays(juce::gl::GL_TRIANGLE_STRIP, 0, 4);
    juce::gl::glBindVertexArray(0);
}

void SDFViewport3D::openGLContextClosing()
{
    textureSystem.releaseGL();
    skyboxSystem.releaseGL();
    if (voxelTexture3D) { juce::gl::glDeleteTextures(1, &voxelTexture3D); voxelTexture3D = 0; }
    if (vao) { juce::gl::glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { juce::gl::glDeleteBuffers(1, &vbo); vbo = 0; }
    shader.reset();
}

juce::String SDFViewport3D::getVertexShader()
{
    return R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";
}

juce::String SDFViewport3D::getFragmentShader()
{
    return shaderPreamble() + shaderSdfPrimitives() + shaderScene()
         + shaderDisplacement() + shaderNormals() + shaderShadowAndAO()
         + shaderTexturing() + shaderSkybox() + shaderBRDF() + shaderMain();
}

juce::String SDFViewport3D::shaderPreamble()
{
    return R"(
        #version 330 core
        out vec4 fragColor;

        uniform float uTime;
        uniform vec2 uRes;
        uniform int uS1, uS2, uOp;
        uniform float uSz1, uSz2, uK, uOX, uOY, uTw;
        uniform float uCa, uCp, uCd;
        uniform float uScanY, uTopoMorph;
        uniform int uScanMode;
        uniform float uScanR;

        uniform sampler2D uTex;
        uniform sampler2D uTexNorm;
        uniform sampler2D uTexRough;
        uniform sampler2D uTexDisp;
        uniform sampler2D uTexAO;
        uniform sampler2D uTexEmit;

        uniform int uHasTex, uHasTexNorm, uHasTexRough, uHasTexDisp;
        uniform int uHasTexAO, uHasTexEmit;
        uniform float uTexScale, uNormInt, uRoughOff, uTexBlend, uDispAmt, uTexBright;
        uniform float uAOInt, uEmitInt;

        // Skybox uniforms
        uniform sampler2D uSkybox;
        uniform float uHasSkybox;
        uniform float uSkyboxExp, uSkyboxRot, uSkyboxRefl, uSkyboxBlur;

        // Voxel SDF uniforms
        uniform sampler3D uVoxelSDF;
        uniform int uHasVoxel;

        // SuperFormula uniforms
        uniform float uSfM, uSfN1, uSfN2, uSfN3;

        // Onion shell uniforms
        uniform int uOnionEnable;
        uniform float uOnionThickness;

        // Stairs operation
        uniform float uStairCount;
    )";
}

juce::String SDFViewport3D::shaderSdfPrimitives()
{
    return R"(
        // SDF primitives
        float sdSphere(vec3 p, float r) { return length(p) - r; }
        float sdBox(vec3 p, float b) {
            vec3 q = abs(p) - b;
            return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
        }
        float sdTorus(vec3 p, float R, float r) {
            vec2 q = vec2(length(p.xz) - R, p.y);
            return length(q) - r;
        }
        float sdCyl(vec3 p, float r, float h) {
            float d = length(p.xz) - r;
            float dy = abs(p.y) - h;
            return min(max(d, dy), 0.0) + length(max(vec2(d, dy), 0.0));
        }
        float sdOct(vec3 p, float s) {
            return (abs(p.x) + abs(p.y) + abs(p.z) - s) * 0.57735;
        }

        float sdCapsule(vec3 p, float h, float r) {
            p.y -= clamp(p.y, 0.0, h);
            return length(p) - r;
        }
        float sdRoundBox(vec3 p, vec3 b, float r) {
            vec3 q = abs(p) - b;
            return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0) - r;
        }
        float sdHexPrism(vec3 p, vec2 h) {
            const vec3 k = vec3(-0.8660254, 0.5, 0.57735);
            p = abs(p);
            p.xz -= 2.0 * min(dot(k.xy, p.xz), 0.0) * k.xy;
            vec2 d = vec2(length(p.xz - vec2(clamp(p.x, -k.z*h.x, k.z*h.x), h.x)) * sign(p.z - h.x),
                          p.y - h.y);
            return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
        }
        float sdTorus82(vec3 p, vec2 t) {
            vec2 q = vec2(length(p.xz) - t.x, p.y);
            return length(q) - t.y;
        }
        float sdTorus88(vec3 p, vec2 t) {
            vec2 q = vec2(max(abs(p.x), abs(p.z)) - t.x, p.y);
            return max(abs(q.x), abs(q.y)) - t.y;
        }
        float sdSuperFormula(vec3 p, float m, float n1, float n2, float n3, float scale) {
            float r = length(p);
            if (r < 1e-8) return -scale;
            float theta = acos(clamp(p.y / r, -1.0, 1.0));
            float phi = atan(p.z, p.x);
            float a1 = abs(cos(m * theta / 4.0));
            float b1 = abs(sin(m * theta / 4.0));
            float r1 = pow(pow(a1, n2) + pow(b1, n3), -1.0/n1);
            float a2 = abs(cos(m * phi / 4.0));
            float b2 = abs(sin(m * phi / 4.0));
            float r2 = pow(pow(a2, n2) + pow(b2, n3), -1.0/n1);
            return r - r1 * r2 * scale;
        }

        float sampleVoxel(vec3 p) {
            vec3 uv = p * 0.5 + 0.5;
            return texture(uVoxelSDF, uv).r;
        }

        float eS(int s, vec3 p, float sz) {
            if (s == 5 && uHasVoxel == 1) return sampleVoxel(p);
            if (s == 0) return sdSphere(p, sz);
            if (s == 1) return sdBox(p, sz * 0.75);
            if (s == 2) return sdTorus(p, sz * 0.65, sz * 0.25);
            if (s == 3) return sdCyl(p, sz * 0.5, sz * 0.8);
            if (s == 4) return sdOct(p, sz);
            if (s == 6) return sdCapsule(p, sz, sz * 0.3);
            if (s == 7) return sdRoundBox(p, vec3(sz), sz * 0.15);
            if (s == 8) return sdHexPrism(p, vec2(sz, sz * 0.5));
            if (s == 9) return sdTorus82(p, vec2(sz, sz * 0.25));
            if (s == 10) return sdTorus88(p, vec2(sz, sz * 0.25));
            if (s == 11) return sdSuperFormula(p, uSfM, uSfN1, uSfN2, uSfN3, sz);
            return sdOct(p, sz);
        }
    )";
}

juce::String SDFViewport3D::shaderScene()
{
    return R"(
        float smin(float a, float b, float k) {
            if (k < 0.001) return min(a, b);
            float h = max(k - abs(a - b), 0.0) / k;
            return min(a, b) - h * h * h * k / 6.0;
        }

        float opSmoothIntersection(float d1, float d2, float k) {
            float h = clamp(0.5 - 0.5*(d2-d1)/k, 0.0, 1.0);
            return mix(d2, d1, h) + k*h*(1.0-h);
        }
        float opSmoothSubtraction(float d1, float d2, float k) {
            float h = clamp(0.5 - 0.5*(d2+d1)/k, 0.0, 1.0);
            return mix(d2, -d1, h) + k*h*(1.0-h);
        }
        float opChamferUnion(float a, float b, float r) {
            return min(min(a, b), (a - r + b) * 0.7071);
        }
        float opChamferIntersection(float a, float b, float r) {
            return max(max(a, b), (a + r + b) * 0.7071);
        }
        float opChamferSubtraction(float a, float b, float r) {
            return opChamferIntersection(a, -b, r);
        }
        float opStairsUnion(float a, float b, float r, float n) {
            float s = r / n;
            float u = b - r;
            return min(min(a, b), 0.5*(u + a + abs(mod(u - a + s, 2.0*s) - s)));
        }
        float opPipe(float a, float b, float r) {
            return length(vec2(a, b)) - r;
        }

        float scene(vec3 p) {
            vec3 q = p;
            if (uTw > 0.01) {
                float c = cos(uTw * q.y), s = sin(uTw * q.y);
                q.xz = mat2(c, -s, s, c) * q.xz;
            }
            float d1 = eS(uS1, q, uSz1);
            float d2 = eS(uS2, q - vec3(uOX, uOY, 0.0), uSz2);
            float d;
            if (uOp == 0) d = smin(d1, d2, uK);
            else if (uOp == 1) d = min(d1, d2);
            else if (uOp == 2) d = max(d1, d2);
            else if (uOp == 3) d = max(d1, -d2);
            else if (uOp == 4) d = opSmoothIntersection(d1, d2, uK);
            else if (uOp == 5) d = opSmoothSubtraction(d1, d2, uK);
            else if (uOp == 6) d = opChamferUnion(d1, d2, uK);
            else if (uOp == 7) d = opChamferIntersection(d1, d2, uK);
            else if (uOp == 8) d = opChamferSubtraction(d1, d2, uK);
            else if (uOp == 9) d = opStairsUnion(d1, d2, uK, uStairCount);
            else if (uOp == 10) d = opPipe(d1, d2, uK);
            else d = min(d1, d2);

            // Onion shell
            if (uOnionEnable == 1) d = abs(d) - uOnionThickness;

            return d;
        }
    )";
}

juce::String SDFViewport3D::shaderDisplacement()
{
    return R"(
        // Triplanar displacement (caller passes p * uTexScale)
        float triplanarDisp(vec3 p) {
            vec3 n = normalize(p + vec3(0.001));
            vec3 w = abs(n); w = w / (w.x + w.y + w.z + 0.001);
            float tx = texture(uTexDisp, p.yz * 0.5 + 0.5).r;
            float ty = texture(uTexDisp, p.xz * 0.5 + 0.5).r;
            float tz = texture(uTexDisp, p.xy * 0.5 + 0.5).r;
            return tx * w.x + ty * w.y + tz * w.z;
        }

        float sceneDisp(vec3 p) {
            float d = scene(p);
            if (uHasTexDisp == 1) {
                float disp = triplanarDisp(p * uTexScale);
                d -= disp * uDispAmt;
            }
            return d;
        }

        float sceneClipped(vec3 p) {
            float shape = sceneDisp(p);
            float clipPlane = uScanY - p.y;
            return max(shape, clipPlane);
        }
    )";
}

juce::String SDFViewport3D::shaderNormals()
{
    return R"(
        vec3 calcN(vec3 p) {
            // Tetrahedron normal estimation (4 SDF evals instead of 6)
            // eps=0.001 matches reference; 0.003 was too large for thin torus tube (r=0.0875)
            vec2 e = vec2(0.001, -0.001);
            return normalize(
                e.xyy * sceneDisp(p + e.xyy) +
                e.yyx * sceneDisp(p + e.yyx) +
                e.yxy * sceneDisp(p + e.yxy) +
                e.xxx * sceneDisp(p + e.xxx));
        }
    )";
}

juce::String SDFViewport3D::shaderShadowAndAO()
{
    return R"(
        // IQ's improved C1-continuous soft shadow (eliminates banding on curved surfaces)
        float shadow(vec3 ro, vec3 rd) {
            float r = 1.0;
            float t = 0.02;
            float ph = 1e10;
            for (int i = 0; i < 32; i++) {
                float h = sceneDisp(ro + rd * t);
                float y = h * h / (2.0 * ph);
                float d = sqrt(h * h - y * y);
                r = min(r, 8.0 * d / max(0.0, t - y));
                ph = h;
                t += clamp(h, 0.02, 0.25);
                if (h < 0.001 || t > 5.0) break;
            }
            return clamp(r, 0.0, 1.0);
        }

        // SDF-based ambient occlusion (5 evals, conservative distances safe for thin geometry)
        float calcAO(vec3 p, vec3 n) {
            float occ = 0.0;
            float sca = 1.0;
            for (int i = 0; i < 5; i++) {
                float h = 0.002 + 0.015 * float(i);  // max reach 0.062 (safe for torus tube r=0.0875)
                float d = sceneDisp(p + n * h);
                occ += max(h - d, 0.0) * sca;
                sca *= 0.85;
            }
            return clamp(1.0 - 2.0 * occ, 0.0, 1.0);
        }
    )";
}

juce::String SDFViewport3D::shaderTexturing()
{
    return R"(
        // Triplanar texture
        vec3 triplanarTex(sampler2D tex, vec3 p, vec3 n) {
            vec3 w = abs(n); w = w / (w.x + w.y + w.z + 0.001);
            vec3 tx = texture(tex, p.yz * 0.5 + 0.5).rgb;
            vec3 ty = texture(tex, p.xz * 0.5 + 0.5).rgb;
            vec3 tz = texture(tex, p.xy * 0.5 + 0.5).rgb;
            return tx * w.x + ty * w.y + tz * w.z;
        }

        // Normal map perturbation
        vec3 triplanarNorm(vec3 p, vec3 n) {
            vec3 w = abs(n); w = w / (w.x + w.y + w.z + 0.001);
            vec3 tx = texture(uTexNorm, p.yz * 0.5 + 0.5).rgb * 2.0 - 1.0;
            vec3 ty = texture(uTexNorm, p.xz * 0.5 + 0.5).rgb * 2.0 - 1.0;
            vec3 tz = texture(uTexNorm, p.xy * 0.5 + 0.5).rgb * 2.0 - 1.0;
            vec3 nx = vec3(0.0, tx.yx) + vec3(n.x, 0.0, 0.0);
            vec3 ny = vec3(ty.x, 0.0, ty.y) + vec3(0.0, n.y, 0.0);
            vec3 nz = vec3(tz.xy, 0.0) + vec3(0.0, 0.0, n.z);
            return normalize(nx * w.x + ny * w.y + nz * w.z);
        }
    )";
}

juce::String SDFViewport3D::shaderSkybox()
{
    return R"(
        // Equirectangular UV from direction
        vec2 equirectUV(vec3 rd) {
            float phi = atan(rd.z, rd.x);
            float theta = asin(clamp(rd.y, -1.0, 1.0));
            return vec2(phi / (2.0 * 3.14159) + 0.5,
                        theta / 3.14159 + 0.5);
        }

        // Rotate xz by skybox rotation angle
        vec3 rotateSkybox(vec3 d) {
            float cr = cos(uSkyboxRot), sr = sin(uSkyboxRot);
            return vec3(cr * d.x - sr * d.z, d.y, sr * d.x + cr * d.z);
        }
    )";
}

juce::String SDFViewport3D::shaderBRDF()
{
    return R"(
        // Cook-Torrance GGX BRDF functions
        float D_GGX(float NdotH, float a2) {
            float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
            return a2 / max(3.14159 * d * d, 0.0001);
        }
        float G_Smith(float NdotV, float NdotL, float alpha) {
            // Schlick-GGX: k = alpha/2 for analytic lights (alpha = roughness^2)
            float k = alpha * 0.5;
            float g1 = NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
            float g2 = NdotL / max(NdotL * (1.0 - k) + k, 0.0001);
            return g1 * g2;
        }
        vec3 F_Schlick(float VdotH, vec3 F0) {
            return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
        }

        // ACES Filmic Tone Mapping
        vec3 ACESFilm(vec3 x) {
            float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
            return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
        }
    )";
}

juce::String SDFViewport3D::shaderMain()
{
    return juce::String(R"(
        void main() {
            vec2 uv = (gl_FragCoord.xy - uRes * 0.5) / uRes.y;
            vec3 ro = vec3(cos(uCa) * cos(uCp), sin(uCp), sin(uCa) * cos(uCp)) * uCd;
            vec3 ww = normalize(-ro);
            vec3 uu = normalize(cross(ww, vec3(0.0, 1.0, 0.0)));
            vec3 vv = cross(uu, ww);
            vec3 rd = normalize(uv.x * uu + uv.y * vv + 1.5 * ww);

            // Bounding sphere early-out (R=2.5)
            float t = 0.0;
            vec3 p;
            bool hit = false;
            float edgeFade = 1.0;
            float pixelSize = 1.0 / uRes.y;
            float minD = 1e10;
            vec3 minP = vec3(0.0);
            float minT = 0.0;
            float b_s = dot(ro, rd);
            float c_s = dot(ro, ro) - 2.5 * 2.5;
            float disc = b_s * b_s - c_s;
            if (disc >= 0.0) {
                t = max(-b_s - sqrt(disc), 0.0);
                // Standard sphere tracing (100 iterations — thin torus tube needs more to converge)
                for (int i = 0; i < 100; i++) {
                    p = ro + rd * t;
                    float d = sceneDisp(p);
                    if (d < minD) { minD = d; minP = p; minT = t; }
                    if (d < 0.0006) { hit = true; break; }
                    if (t > 10.0) break;
                    t += d;
                }
                // Edge AA: near-miss rays get soft blended
                if (!hit) {
                    float threshold = pixelSize * minT * 0.75;
                    if (minD < threshold) {
                        hit = true;
                        p = minP;
                        t = minT;
                        edgeFade = 1.0 - minD / threshold;
                    }
                }
            }

            // Ray-plane intersection for scan slice face (Contour mode only)
            bool isSliceFace = false;
            if (uScanMode == 0) {
                float tSlice = (abs(rd.y) > 0.0001) ? (uScanY - ro.y) / rd.y : -1.0;
                if (tSlice > 0.0 && tSlice < 10.0) {
                    vec3 sp = ro + rd * tSlice;
                    if (scene(sp) < 0.0) {
                        if (!hit || tSlice < t) {
                            isSliceFace = true;
                            p = sp;
                            t = tSlice;
                            hit = true;
                        }
                    }
                }
            }

            // Background: skybox or fallback (tone mapping applied at end via ACES)
            vec3 col;
            if (uHasSkybox > 0.5) {
                vec3 rotRd = rotateSkybox(rd);
                vec3 hdr = texture(uSkybox, equirectUV(rotRd)).rgb;
                col = hdr * uSkyboxExp;
            } else {
                col = vec3(0.015, 0.025, 0.03);
            }
            vec3 bgCol = col;

            if (hit) {
                vec3 n;
                if (isSliceFace) {
                    n = vec3(0.0, sign(ro.y - uScanY), 0.0);
                } else {
                    n = calcN(p);
                }

                // Normal map perturbation (skip for slice face)
                if (!isSliceFace && uHasTexNorm == 1) {
                    vec3 nPerturbed = triplanarNorm(p * uTexScale, n);
                    n = normalize(mix(n, nPerturbed, uNormInt));
                }

                // Three-light setup
                vec3 ld = normalize(vec3(1.0, 2.0, 1.5));
                vec3 ld2 = normalize(vec3(-1.0, 0.5, -1.0));
                vec3 ld3 = normalize(vec3(0.0, -0.5, 1.0));
                float diff = max(dot(n, ld), 0.0);
                float diff2 = max(dot(n, ld2), 0.0);
                float diff3 = max(dot(n, ld3), 0.0);

                // Roughness
                float roughness = 0.35;
                if (!isSliceFace && uHasTexRough == 1) {
                    roughness = clamp(triplanarTex(uTexRough, p * uTexScale, n).r + uRoughOff, 0.0, 1.0);
                } else if (!isSliceFace) {
                    roughness = clamp(0.35 + uRoughOff, 0.0, 1.0);
                }
                // Cook-Torrance GGX BRDF
                float alpha = roughness * roughness;
                float a2 = alpha * alpha;
                vec3 V = -rd;
                float NdotV = max(dot(n, V), 0.05);
                vec3 F0 = vec3(0.04); // dielectric
                float fres = pow(1.0 - NdotV, 4.0);

                // GGX specular on key light only (fill/rim are diffuse-only)
                vec3 h1 = normalize(ld + V);
                float NdotL1 = max(dot(n, ld), 0.0);
                float NdotH1 = max(dot(n, h1), 0.0);
                float VdotH1 = max(dot(V, h1), 0.0);
                vec3 spec1v = (NdotL1 > 0.0) ? (D_GGX(NdotH1, a2) * G_Smith(NdotV, NdotL1, alpha) * F_Schlick(VdotH1, F0)) / max(4.0 * NdotV * NdotL1, 0.001) : vec3(0.0);

                float sh = shadow(p + n * 0.02, ld);

                vec3 bc;

                if (isSliceFace) {
                    float intDist = abs(scene(p));
                    bc = mix(vec3(0.02, 0.18, 0.2), vec3(0.04, 0.35, 0.38), intDist * 8.0);
                    // Concentric distance rings (topographic cross-section)
                    float rings = sin(intDist * 80.0) * 0.5 + 0.5;
                    bc = mix(bc, vec3(0.06, 0.45, 0.5), rings * 0.25);
                    col = bc * (0.5 + diff * 0.35) + vec3(0.0, 0.3, 0.3) * 0.15;
                } else {
                    bc = vec3(0.08, 0.55, 0.6);
                    bc += vec3(0.0, 0.15, 0.15) * fres * 0.4;

                    // Color texture
                    if (uHasTex == 1) {
                        vec3 texCol = triplanarTex(uTex, p * uTexScale, n) * uTexBright;
                        bc = mix(bc, texCol, uTexBlend);
                    }

                    float ambient = 0.4;

                    // SDF-based ambient occlusion (geometry-aware)
                    float sdfAO = calcAO(p, n);
                    ambient *= mix(1.0, sdfAO, uAOInt);

                    // Texture AO (multiplicative on top of SDF AO)
                    if (uHasTexAO == 1) {
                        float texAO = triplanarTex(uTexAO, p * uTexScale, n).r;
                        ambient *= texAO;
                    }

                    // Energy conservation: reduce diffuse by Fresnel
                    vec3 kD = vec3(1.0) - F_Schlick(NdotV, F0) * 0.5;
                    col = bc * kD * (ambient + diff * 0.5 * sh + diff2 * 0.15 + diff3 * 0.1)
                        + vec3(0.5, 0.9, 1.0) * spec1v * NdotL1 * sh * 0.5;

                    // Emissive
                    if (uHasTexEmit == 1) {
                        vec3 emitCol = triplanarTex(uTexEmit, p * uTexScale, n) * uEmitInt;
                        col += emitCol;
                    }

                    // Skybox image-based ambient lighting
                    if (uHasSkybox > 0.5) {
                        vec3 ambDir = rotateSkybox(n);
                        vec3 envAmbient = texture(uSkybox, equirectUV(ambDir)).rgb * uSkyboxExp * 0.15;
                        col += bc * envAmbient;
                    }

                    // Skybox Fresnel reflections
                    if (uHasSkybox > 0.5 && uSkyboxRefl > 0.001) {
                        vec3 reflDir = rotateSkybox(reflect(rd, n));
                        vec3 envCol = texture(uSkybox, equirectUV(reflDir)).rgb * uSkyboxExp;
                        float fresRefl = pow(1.0 - max(dot(n, -rd), 0.0), 5.0);
                        float reflAmount = mix(0.04, 1.0, fresRefl) * uSkyboxRefl;
                        reflAmount *= (1.0 - roughness * uSkyboxBlur);
                        col = mix(col, envCol, reflAmount);
                    }
                }
    )") + R"(
                // Per-mode scan visualization (3D on surface)
                float sliceDist = abs(p.y - uScanY);
                float axR = length(p.xz);
                float pAngle = atan(p.z, p.x);

                if (uScanMode == 0) {
                    float sliceGlow = exp(-sliceDist * 25.0) * 0.6;
                    float edgeGlow = exp(-sliceDist * 8.0) * 0.15;
                    col += vec3(0.0, 0.9, 0.85) * sliceGlow + vec3(0.0, 0.4, 0.35) * edgeGlow;

                } else if (uScanMode == 1) {
                    int numRays = 1 + int(uTopoMorph * 7.0);
                    float ga = 2.39996;
                    float beamSum = 0.0;
                    for (int r = 0; r < 8; r++) {
                        if (r >= numRays) break;
                        float rayAngle = float(r) * ga;
                        float angDist = pAngle - rayAngle;
                        angDist = angDist - 6.28318 * floor((angDist + 3.14159) / 6.28318);
                        float beam = exp(-angDist * angDist * 800.0);
                        beam *= exp(-sliceDist * 6.0);
                        beam *= smoothstep(0.0, uScanR * 2.0, axR);
                        beamSum += beam;
                    }
                    float rangeRing = exp(-abs(axR - uScanR * 2.0) * 20.0) * exp(-sliceDist * 8.0) * 0.3;
                    float originDist = length(p - vec3(0.0, uScanY, 0.0));
                    float originGlow = exp(-originDist * 10.0) * 0.6;
                    col += vec3(0.0, 0.9, 0.85) * beamSum * 0.8;
                    col += vec3(0.3, 1.0, 0.95) * (rangeRing + originGlow);

                } else if (uScanMode == 2) {
                    vec3 micPos = vec3(0.0, uScanY, 0.0);
                    float micDist = length(p - micPos);
                    float micGlow = exp(-micDist * 12.0) * 0.8;
                    int numBounces = 1 + int(uTopoMorph * 5.0);
                    float raySum = 0.0;
                    float bounceSum = 0.0;
                    for (int r = 0; r < 12; r++) {
                        float ft = float(r) / 12.0;
                        float fphi = acos(1.0 - 2.0 * ft);
                        float ftheta = 2.39996 * float(r);
                        vec3 rayDir = vec3(sin(fphi) * cos(ftheta), sin(fphi) * sin(ftheta), cos(fphi));
                        vec3 toP = p - micPos;
                        float along = dot(toP, rayDir);
                        vec3 closest = micPos + rayDir * max(along, 0.0);
                        float lineDist = length(p - closest);
                        raySum += exp(-lineDist * 18.0) * 0.15;
                        for (int b = 0; b < 6; b++) {
                            if (b >= numBounces) break;
                            float bR = 0.2 + float(b) * 0.18;
                            float bRingDist = abs(axR - bR);
                            float bRing = exp(-(bRingDist * bRingDist * 100.0 + sliceDist * sliceDist * 30.0));
                            bounceSum += bRing * (1.0 - float(b) * 0.14) * 0.08;
                        }
                    }
                    col += vec3(0.2, 0.7, 1.0) * raySum;
                    col += vec3(0.5, 0.9, 1.0) * bounceSum;
                    col += vec3(0.7, 0.95, 1.0) * micGlow;

                } else if (uScanMode == 3) {
                    float ga = 2.39996;
                    int numDots = 16 + int(uTopoMorph * 176.0);
                    numDots = min(numDots, 64);
                    float dotSum = 0.0;
                    float brightDot = 0.0;
                    for (int g = 0; g < 64; g++) {
                        if (g >= numDots) break;
                        float gAngle = mod(float(g) * ga, 6.28318) - 3.14159;
                        float angDist = pAngle - gAngle;
                        angDist = angDist - 6.28318 * floor((angDist + 3.14159) / 6.28318);
                        float yDist = p.y - uScanY;
                        float gR = 0.4;
                        vec3 gP = vec3(cos(gAngle) * gR, uScanY, sin(gAngle) * gR);
                        float gD = abs(scene(gP));
                        float sharpness = mix(30.0, 100.0, smoothstep(0.0, 0.3, gD));
                        float spot = exp(-(angDist * angDist * sharpness + yDist * yDist * 40.0));
                        dotSum += spot;
                        brightDot = max(brightDot, spot);
                    }
                    float pulse = 0.85 + 0.15 * sin(uTime * 3.0 + dotSum * 5.0);
                    col += vec3(0.0, 0.9, 0.85) * dotSum * 0.5 * pulse;
                    col += vec3(0.4, 1.0, 0.95) * brightDot * 0.4;

                } else if (uScanMode == 4) {
                    int numRings = 8 + int(uTopoMorph * 24.0);
                    numRings = min(numRings, 32);
                    float ringSum = 0.0;
                    for (int h = 0; h < 32; h++) {
                        if (h >= numRings) break;
                        float hy = uScanY - 0.4 + (float(h) / float(numRings - 1)) * 0.8;
                        float ringY = abs(p.y - hy);
                        float ringR = abs(axR - uScanR);
                        float ringDist = sqrt(ringY * ringY + ringR * ringR);
                        float brightness = 1.0 / sqrt(float(h + 1));
                        ringSum += exp(-ringDist * 18.0) * brightness;
                    }
                    float pulse = 0.9 + 0.1 * sin(uTime * 2.0);
                    col += vec3(0.8, 0.3, 0.9) * ringSum * 0.5 * pulse;
                    col += vec3(0.9, 0.5, 1.0) * ringSum * 0.15;

                } else if (uScanMode == 5) {
                    float a_l, b_l, c_l, delta_l;
                    if (uTopoMorph < 0.5) {
                        float tm = uTopoMorph * 2.0;
                        a_l = 1.0; b_l = 1.0 + tm; c_l = tm * 1.5; delta_l = tm * 0.5;
                    } else {
                        float tm = (uTopoMorph - 0.5) * 2.0;
                        a_l = 1.0 + tm; b_l = 2.0 + tm; c_l = 1.5 + tm * 3.5; delta_l = 0.5 + tm;
                    }
                    float minDist = 100.0;
                    for (int s = 0; s < 64; s++) {
                        float lt = (float(s) / 64.0) * 6.28318;
                        vec3 lp = vec3(sin(a_l * lt + delta_l) * uScanR, uScanY + sin(c_l * lt) * uScanR * 0.4, sin(b_l * lt) * uScanR);
                        minDist = min(minDist, length(p - lp));
                    }
                    float pathGlow = exp(-minDist * 12.0) * 0.4;
                    float dotT = mod(uTime * 1.2, 6.28318);
                    vec3 dotPos = vec3(sin(a_l * dotT + delta_l) * uScanR, uScanY + sin(c_l * dotT) * uScanR * 0.4, sin(b_l * dotT) * uScanR);
                    float dotDist = length(p - dotPos);
                    float dotGlow = exp(-dotDist * 15.0) * 1.5;
                    float trailGlow = 0.0;
                    for (int tr = 1; tr <= 8; tr++) {
                        float trT = dotT - float(tr) * 0.08;
                        vec3 trP = vec3(sin(a_l * trT + delta_l) * uScanR, uScanY + sin(c_l * trT) * uScanR * 0.4, sin(b_l * trT) * uScanR);
                        float trDist = length(p - trP);
                        trailGlow += exp(-trDist * 12.0) * (1.0 - float(tr) * 0.1);
                    }
                    col += vec3(0.95, 0.55, 0.1) * pathGlow;
                    col += vec3(1.0, 0.75, 0.2) * dotGlow;
                    col += vec3(0.9, 0.45, 0.05) * trailGlow * 0.15;
                }
            }

            // Edge AA: blend near-miss pixels with background
            if (edgeFade < 1.0)
                col = mix(bgCol, col, edgeFade);

            // Ground grid
            if (rd.y < -0.005) {
                float tg = (-0.85 - ro.y) / rd.y;
                if (tg > 0.0 && tg < 12.0) {
                    vec3 gp = ro + rd * tg;
                    float gx = abs(fract(gp.x * 2.0) - 0.5);
                    float gz = abs(fract(gp.z * 2.0) - 0.5);
                    col += vec3(0.0, 0.12, 0.12) * (1.0 - smoothstep(0.0, 0.025, min(gx, gz))) * 0.12 * exp(-tg * 0.25);
                }
            }

            // ACES filmic tone mapping + sRGB gamma + vignette + dither
            col = ACESFilm(col);
            col = pow(col, vec3(1.0 / 2.2));
            col *= 1.0 - 0.12 * dot(uv, uv);
            float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
            col += (dither - 0.5) / 255.0;
            fragColor = vec4(col, 1.0);
        }
    )";
}
