#ifndef LIGHTING_MANAGER_H
#define LIGHTING_MANAGER_H

// =============================================================================
//  LightingManager.h — Advanced Lighting System for InsideTheFrame Exhibition
//  Provides: GPU quality detection, shadow FBO, light structs, falloff presets
// =============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <cstring>   // strstr

// ---------------------------------------------------------------------------
//  Quality tiers — auto-detected from GPU caps at startup
// ---------------------------------------------------------------------------
enum class LightingQuality {
    MINIMAL = 0,  // Single directional, no shadows, 2 lights
    LOW     = 1,  // Basic Phong, no shadows,        4 lights
    MEDIUM  = 2,  // No shadows, baked AO,            6 lights  ← target for iGPU
    HIGH    = 3,  // 512×512 shadows + PCF,           8 lights
    ULTRA   = 4   // 1024×1024 shadows + PCF + bloom, 8 lights
};

// ---------------------------------------------------------------------------
//  Light-falloff presets (attenuation coefficients)
// ---------------------------------------------------------------------------
enum class LightPreset {
    CANDLELIGHT  = 0,   // Tight warm pool  — linear=0.35  quad=0.44
    FLUORESCENT  = 1,   // Even wash        — linear=0.09  quad=0.012
    DAYLIGHT     = 2    // Wide coverage    — linear=0.022 quad=0.0019
};

// ---------------------------------------------------------------------------
//  PointLight — uploaded as a uniform array to shaders
// ---------------------------------------------------------------------------
struct PointLight {
    glm::vec3 position  = glm::vec3(0.0f);
    glm::vec3 color     = glm::vec3(1.0f);
    float intensity     = 1.0f;
    float constant      = 1.0f;
    float linear        = 0.09f;
    float quadratic     = 0.032f;
};

// ---------------------------------------------------------------------------
//  DirectionalLight — sun / ambient direction with time-of-day variation
// ---------------------------------------------------------------------------
struct DirectionalLight {
    glm::vec3 direction = glm::normalize(glm::vec3(0.3f, -1.0f, 0.5f));
    glm::vec3 color     = glm::vec3(1.0f, 0.98f, 0.92f);
    float     intensity = 0.05f;   // kept low for gallery feel
};

// ---------------------------------------------------------------------------
//  Falloff coefficients lookup
// ---------------------------------------------------------------------------
static inline void applyFalloffPreset(PointLight& light, LightPreset preset) {
    switch (preset) {
        case LightPreset::CANDLELIGHT:
            light.constant  = 1.0f;  light.linear = 0.35f;  light.quadratic = 0.44f;  break;
        case LightPreset::FLUORESCENT:
            light.constant  = 1.0f;  light.linear = 0.09f;  light.quadratic = 0.012f; break;
        case LightPreset::DAYLIGHT:
            light.constant  = 1.0f;  light.linear = 0.022f; light.quadratic = 0.0019f; break;
    }
}

// ---------------------------------------------------------------------------
//  Apply falloff preset to ALL lights in an array
// ---------------------------------------------------------------------------
static inline void applyFalloffToAll(PointLight* lights, int count, LightPreset preset) {
    for (int i = 0; i < count; ++i)
        applyFalloffPreset(lights[i], preset);
}

// ---------------------------------------------------------------------------
//  Max active lights per quality tier
// ---------------------------------------------------------------------------
static inline int maxLightsForQuality(LightingQuality q) {
    switch (q) {
        case LightingQuality::MINIMAL: return 2;
        case LightingQuality::LOW:     return 4;
        case LightingQuality::MEDIUM:  return 6;
        case LightingQuality::HIGH:    return 8;
        case LightingQuality::ULTRA:   return 8;
        default:                       return 4;
    }
}

// ---------------------------------------------------------------------------
//  Shadow map resolution per quality tier
// ---------------------------------------------------------------------------
static inline int shadowResForQuality(LightingQuality q) {
    if (q == LightingQuality::ULTRA) return 1024;
    if (q == LightingQuality::HIGH)  return 512;
    return 0;   // no shadows
}

// ---------------------------------------------------------------------------
//  Name strings (for console output)
// ---------------------------------------------------------------------------
static inline const char* qualityName(LightingQuality q) {
    switch (q) {
        case LightingQuality::MINIMAL: return "MINIMAL";
        case LightingQuality::LOW:     return "LOW";
        case LightingQuality::MEDIUM:  return "MEDIUM";
        case LightingQuality::HIGH:    return "HIGH";
        case LightingQuality::ULTRA:   return "ULTRA";
        default:                       return "UNKNOWN";
    }
}
static inline const char* presetName(LightPreset p) {
    switch (p) {
        case LightPreset::CANDLELIGHT: return "CANDLELIGHT";
        case LightPreset::FLUORESCENT: return "FLUORESCENT";
        case LightPreset::DAYLIGHT:    return "DAYLIGHT";
        default:                       return "UNKNOWN";
    }
}

// =============================================================================
//  ShadowMap — Creates and manages a depth-only FBO for shadow mapping
// =============================================================================
class ShadowMap {
public:
    GLuint FBO         = 0;
    GLuint depthTex    = 0;
    int    width       = 512;
    int    height      = 512;
    bool   initialized = false;

    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);

    // Construct with desired resolution (call after OpenGL context is ready)
    bool init(int res) {
        if (res <= 0) return false;
        width  = res;
        height = res;

        // Create depth texture
        glGenTextures(1, &depthTex);
        glBindTexture(GL_TEXTURE_2D, depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Clamp to border so fragments outside frustum = not in shadow
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // Create FBO
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, depthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "[ShadowMap] ERROR: FBO incomplete (status="
                      << status << ")\n";
            return false;
        }

        initialized = true;
        std::cout << "[ShadowMap] Initialized " << width << "x" << height << "\n";
        return true;
    }

    // Compute light-space matrix from a top-down directional perspective
    // covering the exhibition room (-10..10 X, 0..6 Y, -20..20 Z)
    void updateLightMatrix(const glm::vec3& lightPos) {
        float nearP = 0.1f, farP = 60.0f;
        glm::mat4 lightProj = glm::ortho(-12.0f, 12.0f, -22.0f, 22.0f, nearP, farP);
        glm::mat4 lightView = glm::lookAt(
            lightPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        lightSpaceMatrix = lightProj * lightView;
    }

    void bindForWriting() const {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, width, height);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void bindDepthTexture(GLuint textureUnit = 1) const {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, depthTex);
    }

    void cleanup() {
        if (depthTex) glDeleteTextures(1, &depthTex);
        if (FBO)      glDeleteFramebuffers(1, &FBO);
        depthTex = 0; FBO = 0; initialized = false;
    }
};

// =============================================================================
//  LightingManager — GPU detection + uniform upload helpers
// =============================================================================
class LightingManager {
public:
    LightingQuality quality   = LightingQuality::MEDIUM;
    LightPreset     preset    = LightPreset::FLUORESCENT;
    ShadowMap       shadowMap;
    bool            shadowsOn = false;

    // ------------------------------------------------------------------
    //  detectQuality() — call AFTER gladLoadGL, reads GL caps
    // ------------------------------------------------------------------
    LightingQuality detectQuality() {
        GLint maxTexSize    = 0;
        GLint maxTexUnits   = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE,         &maxTexSize);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,  &maxTexUnits);

        // Check for shadow extension support
        bool hasShadowExt = false;
        const char* extStr = (const char*)glGetString(GL_EXTENSIONS);
        // In core profile GL_EXTENSIONS is not available via glGetString,
        // but GL_ARB_depth_texture is core since GL 1.4 — always available in 3.3
        // We check via glGetStringi loop instead for safety
        GLint numExt = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
        for (GLint i = 0; i < numExt; ++i) {
            const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
            if (ext && (strstr(ext, "ARB_shadow") || strstr(ext, "EXT_shadow"))) {
                hasShadowExt = true;
                break;
            }
        }
        // GL 3.3 guarantees depth textures — treat as always available
        hasShadowExt = true;

        LightingQuality detected;
        if (maxTexSize >= 8192 && maxTexUnits >= 32) {
            detected = LightingQuality::ULTRA;
        } else if (maxTexSize >= 4096 && maxTexUnits >= 16) {
            detected = LightingQuality::HIGH;
        } else if (maxTexSize >= 2048 && maxTexUnits >= 8 && hasShadowExt) {
            detected = LightingQuality::MEDIUM;
        } else if (maxTexSize >= 1024) {
            detected = LightingQuality::LOW;
        } else {
            detected = LightingQuality::MINIMAL;
        }

        quality = detected;
        std::cout << "[Lighting] Detected quality: " << qualityName(quality)
                  << "  (maxTexSize=" << maxTexSize
                  << ", maxTexUnits=" << maxTexUnits << ")\n";
        return detected;
    }

    // ------------------------------------------------------------------
    //  initShadows() — create FBO after quality detected
    // ------------------------------------------------------------------
    bool initShadows() {
        int res = shadowResForQuality(quality);
        if (res > 0 && shadowMap.init(res)) {
            shadowsOn = true;
            return true;
        }
        shadowsOn = false;
        return false;
    }

    // ------------------------------------------------------------------
    //  Upload point lights to a shader (prefix e.g. "lights[0].position")
    // ------------------------------------------------------------------
    void uploadLights(GLuint shaderID, const PointLight* lights, int count) const {
        int activeCount = std::min(count, maxLightsForQuality(quality));
        glUniform1i(glGetUniformLocation(shaderID, "numLights"), activeCount);
        for (int i = 0; i < activeCount; ++i) {
            std::string base = "lights[" + std::to_string(i) + "].";
            glUniform3fv(glGetUniformLocation(shaderID, (base+"position").c_str()),  1, &lights[i].position[0]);
            glUniform3fv(glGetUniformLocation(shaderID, (base+"color").c_str()),     1, &lights[i].color[0]);
            glUniform1f(glGetUniformLocation(shaderID,  (base+"intensity").c_str()),    lights[i].intensity);
            glUniform1f(glGetUniformLocation(shaderID,  (base+"constant").c_str()),     lights[i].constant);
            glUniform1f(glGetUniformLocation(shaderID,  (base+"linear").c_str()),       lights[i].linear);
            glUniform1f(glGetUniformLocation(shaderID,  (base+"quadratic").c_str()),    lights[i].quadratic);
        }
    }

    // ------------------------------------------------------------------
    //  Cycle helpers for key bindings
    // ------------------------------------------------------------------
    void cyclePreset() {
        int next = ((int)preset + 1) % 3;
        preset = (LightPreset)next;
        std::cout << "[Lighting] Light preset: " << presetName(preset) << "\n";
    }

    void cycleQualityDown() {
        int cur = (int)quality;
        if (cur > 0) cur--;
        quality = (LightingQuality)cur;
        std::cout << "[Lighting] Quality lowered to: " << qualityName(quality) << "\n";
        // Note: shadow FBO remains alive; shadowsOn flag gates its use
        shadowsOn = (shadowResForQuality(quality) > 0) && shadowMap.initialized;
    }
};

#endif // LIGHTING_MANAGER_H
