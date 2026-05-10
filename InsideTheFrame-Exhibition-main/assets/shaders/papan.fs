// =============================================================================
//  papan.fs — Exhibition Board fragment shader
//
//  New features:
//   • Artwork texture  (useTexture=1 → sample artworkTex; 0 → white panel)
//   • Highlight mode   (highlighted=1 → gold border + brightness boost)
//   • Same PCF shadow + Blinn-Phong multi-light as room shader
//   • Tone mapping (Reinhard) + gamma 2.2
// =============================================================================
#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform vec3  viewPos;
uniform float time;
uniform int   shadowsEnabled;
uniform int   lightingQuality;
uniform sampler2D shadowMap;     // unit 1
uniform sampler2D artworkTex;    // unit 2 — artwork image
uniform int   useTexture;        // 1 = sample artworkTex, 0 = white panel
uniform int   highlighted;       // 1 = draw gold interaction highlight

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

#define MAX_LIGHTS 8
uniform PointLight lights[MAX_LIGHTS];
uniform int        numLights;

// ─── PCF Shadow (4-tap, matches ruangan.fs) ──────────────────────────────────
float calcShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias         = 0.005;
    vec2  texelSize    = vec2(1.0 / 512.0);
    float shadow = 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    return shadow / 4.0;
}

// ─── Blinn-Phong point light ─────────────────────────────────────────────────
vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos,
                    vec3 viewDir, vec3 albedo) {
    vec3  lightDir = normalize(light.position - fragPos);
    float dist     = length(light.position - fragPos);
    float atten    = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  halfway  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfway), 0.0), 48.0);
    vec3  diffuse  = diff * light.color * albedo * light.intensity;
    vec3  specular = spec * light.color * 0.2 * light.intensity;
    return (diffuse + specular) * atten;
}

// ─── Highlight border — UV-based gold frame ───────────────────────────────────
float highlightBorder(vec2 uv, float thickness) {
    vec2 edge = step(uv, vec2(thickness)) + step(vec2(1.0 - thickness), uv);
    return clamp(edge.x + edge.y, 0.0, 1.0);
}

void main() {
    // ── Albedo: artwork texture or white panel ───────────────────────────────
    vec3 albedo;
    if (useTexture == 1) {
        albedo = texture(artworkTex, TexCoord).rgb;
    } else {
        albedo = vec3(0.97, 0.97, 0.97);   // clean white panel
    }

    // ── Highlight: gold border + brightness boost ────────────────────────────
    if (highlighted == 1) {
        float border = highlightBorder(TexCoord, 0.035);
        // Animate border with a slow pulse
        float pulse  = 0.85 + 0.15 * sin(time * 3.0);
        vec3  gold   = vec3(1.0, 0.78, 0.0) * pulse;
        albedo = mix(albedo, gold, border * 0.9);
        albedo *= 1.12;   // slight brightness boost to stand out
    }

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // ── Ambient ──────────────────────────────────────────────────────────────
    vec3 lighting = albedo * 0.07;

    // ── Shadow ───────────────────────────────────────────────────────────────
    float shadow = 0.0;
    if (shadowsEnabled == 1) shadow = calcShadow(FragPosLightSpace);
    float litFactor = 1.0 - shadow * 0.6;

    // ── Point lights ─────────────────────────────────────────────────────────
    for (int i = 0; i < numLights; i++)
        lighting += calcPointLight(lights[i], norm, FragPos, viewDir, albedo) * litFactor;

    // ── Tone map + gamma ─────────────────────────────────────────────────────
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}
