// =============================================================================
//  papan.fs — Exhibition Board fragment shader (Lit version)
//
//  Shares the same PointLight array and shadow map as the room shader,
//  giving the board consistent lighting with the rest of the exhibition.
// =============================================================================
#version 330 core

out vec4 FragColor;

in vec3 VertColor;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

uniform vec3  viewPos;
uniform float time;
uniform int   shadowsEnabled;
uniform sampler2D shadowMap;

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

// ─── PCF shadow (same kernel as ruangan.fs) ──────────────────────────────────
float calcShadow(vec4 fragPosLightSpace) {
    vec3  projCoords   = fragPosLightSpace.xyz / fragPosLightSpace.w * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias         = 0.005;
    vec2  texelSize    = vec2(1.0 / 512.0);
    float shadow       = 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    return shadow / 4.0;
}

// ─── Blinn-Phong point light ──────────────────────────────────────────────────
vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos,
                    vec3 viewDir, vec3 albedo) {
    vec3  lightDir = normalize(light.position - fragPos);
    float dist     = length(light.position - fragPos);
    float atten    = 1.0 / (light.constant
                          + light.linear    * dist
                          + light.quadratic * dist * dist);

    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * light.color * albedo * light.intensity;

    vec3  halfway  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfway), 0.0), 32.0);
    vec3  specular = spec * light.color * 0.25 * light.intensity;

    return (diffuse + specular) * atten;
}

void main() {
    // Board albedo — pure white tinted with vertex colour
    vec3  albedo    = VertColor;
    vec3  norm      = normalize(Normal);
    vec3  viewDir   = normalize(viewPos - FragPos);

    // Ambient
    vec3 lighting   = albedo * 0.05;

    // Shadow
    float shadow    = 0.0;
    if (shadowsEnabled == 1)
        shadow = calcShadow(FragPosLightSpace);
    float litFactor = 1.0 - shadow * 0.6;

    // Point lights
    for (int i = 0; i < numLights; i++) {
        lighting += calcPointLight(lights[i], norm, FragPos, viewDir, albedo) * litFactor;
    }

    // Tone map + gamma
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}
