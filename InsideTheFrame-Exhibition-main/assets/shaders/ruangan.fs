// =============================================================================
//  ruangan.fs — Room fragment shader  (Advanced Lighting Edition)
//
//  Features:
//   • Procedural textures  (wood floor, plaster wall, ceiling tiles)
//   • Multi-light array    up to 8 PointLights (capped by numLights)
//   • PCF shadow mapping   4-tap kernel from 512×512 depth FBO
//   • Dynamic directional  time-based sun colour/intensity (60 s demo cycle)
//   • Light falloff        coefficient-driven (preset set from CPU)
//   • Baked AO approx      analytic corner darkening — zero texture cost
//   • Emissive ceiling     strip-light glow approximation
//   • Tone mapping         Reinhard + gamma 2.2
//
//  Compatibility: OpenGL 3.3 Core Profile (GLSL 330)
// =============================================================================
#version 330 core

out vec4 FragColor;

in vec2  TexCoord;
in vec3  FragPos;
in vec3  Normal;
in vec4  FragPosLightSpace;

// ─── Uniforms ─────────────────────────────────────────────────────────────────
uniform int   surfaceType;      // 0=floor 1=wall 2=ceiling
uniform vec3  viewPos;
uniform float time;             // seconds, wraps at 3600
uniform int   lightingQuality;  // 0=MINIMAL … 4=ULTRA
uniform int   shadowsEnabled;   // 0 or 1

uniform sampler2D shadowMap;    // bound to texture unit 1

// ── Multi-light array ──────────────────────────────────────────────────────────
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
uniform int        numLights;   // actual count this frame

// ─── Procedural noise helpers ─────────────────────────────────────────────────
float hash(vec2 p) {
    p  = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

float smoothNoise(vec2 st) {
    vec2  i = floor(st);
    vec2  f = fract(st);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2  u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 5; i++) {
        value     += amplitude * smoothNoise(st * frequency);
        frequency *= 2.1;
        amplitude *= 0.5;
    }
    return value;
}

// ─── Procedural textures ──────────────────────────────────────────────────────
vec3 woodFloorTexture(vec2 uv) {
    vec2  plankUv  = uv * vec2(10.0, 2.0);
    float row      = floor(plankUv.y);
    float colShift = mod(row, 2.0) * 0.5;
    plankUv.x += colShift;

    float plankCol = floor(plankUv.x);
    float plankRow = row;
    vec2  local    = fract(plankUv);

    float grainSeed = hash(vec2(plankCol, plankRow));
    vec2  grainUv   = vec2(uv.x * 20.0, uv.y * 200.0 + grainSeed * 50.0);
    float grain     = fbm(grainUv);
    float grain2    = smoothNoise(grainUv * vec2(0.5, 2.0));

    vec3 baseLight  = vec3(0.55, 0.32, 0.14);
    vec3 baseDark   = vec3(0.22, 0.10, 0.04);
    vec3 plankBase  = mix(baseDark, baseLight, grainSeed * 0.6 + 0.2);

    vec3 woodColor  = mix(plankBase * 0.75, plankBase * 1.15,
                          grain * 0.7 + grain2 * 0.3);

    float edgeX   = smoothstep(0.97, 1.0, local.x);
    float edgeY   = smoothstep(0.93, 1.0, local.y);
    woodColor     = mix(woodColor, vec3(0.06, 0.03, 0.01), max(edgeX, edgeY) * 0.9);
    woodColor    *= 1.0 + smoothNoise(uv * 60.0) * 0.04;

    return woodColor;
}

vec3 plasterWallTexture(vec2 uv) {
    vec2  st    = uv * vec2(4.0, 2.0);
    float base  = fbm(st * 3.5);
    float fine  = smoothNoise(st * 18.0);
    float micro = hash(uv * 800.0) * 0.03;

    vec3 colorA = vec3(0.95, 0.93, 0.89);
    vec3 colorB = vec3(0.80, 0.78, 0.74);
    vec3 colorC = vec3(0.68, 0.65, 0.60);

    vec3 plasterColor = mix(colorA, colorB, base * 0.6);
    plasterColor      = mix(plasterColor, colorC, fine * 0.25);
    plasterColor     += micro;

    float crackNoise = fbm(st * vec2(9.0, 5.0));
    float crack      = smoothstep(0.68, 0.70, crackNoise);
    plasterColor     = mix(plasterColor, vec3(0.50, 0.48, 0.44), crack * 0.55);

    float dirtyFactor = smoothstep(0.15, 0.0, uv.y);
    plasterColor      = mix(plasterColor, vec3(0.60, 0.55, 0.48), dirtyFactor * 0.35);

    return plasterColor;
}

vec3 ceilingTexture(vec2 uv) {
    vec2  st      = uv * 3.0;
    vec2  panelUv = uv * vec2(5.0, 2.5);
    float px      = smoothstep(0.96, 1.0, fract(panelUv.x));
    float py      = smoothstep(0.96, 1.0, fract(panelUv.y));
    float grid    = max(px, py);

    float base  = fbm(st * 2.0) * 0.06;
    float fine  = smoothNoise(st * 14.0) * 0.04;
    float grain = hash(uv * 600.0) * 0.02;

    vec3 ceilColor = vec3(0.97, 0.96, 0.93)
                   + vec3(base + fine + grain - 0.03);
    ceilColor = mix(ceilColor, vec3(0.70, 0.68, 0.65), grid * 0.6);

    return ceilColor;
}

// ─── PCF Shadow Calculation ───────────────────────────────────────────────────
// 4-tap kernel — cheap on integrated GPUs, smooth enough for soft shadows
float calcShadow(vec4 fragPosLightSpace) {
    // Perspective divide (ortho → no-op, but kept for generality)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;

    // Outside shadow frustum = not in shadow
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias         = 0.005;   // prevents shadow acne

    float shadow    = 0.0;
    vec2  texelSize = vec2(1.0 / 512.0);   // matches FBO resolution

    // 4-tap PCF (2x2 jittered — very cheap)
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0,  1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2(-1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow += texture(shadowMap, projCoords.xy + vec2( 1.0, -1.0) * texelSize).r < (currentDepth - bias) ? 1.0 : 0.0;
    shadow /= 4.0;

    return shadow;
}

// ─── Point light shading (Blinn-Phong) ───────────────────────────────────────
vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos,
                    vec3 viewDir, vec3 albedo, float roughness) {
    vec3  lightDir  = normalize(light.position - fragPos);
    float dist      = length(light.position - fragPos);
    float atten     = 1.0 / (light.constant
                           + light.linear    * dist
                           + light.quadratic * dist * dist);

    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * light.color * albedo * light.intensity;

    vec3  halfway   = normalize(lightDir + viewDir);
    float shininess = mix(8.0, 64.0, 1.0 - roughness);
    float spec      = pow(max(dot(norm, halfway), 0.0), shininess);
    vec3  specular  = spec * light.color * (0.15 - roughness * 0.1) * light.intensity;

    return (diffuse + specular) * atten;
}

// ─── Baked AO approximation (analytic — zero texture cost) ───────────────────
float calcBakedAO(vec3 fragPos, int surface) {
    float ao = 1.0;
    if (surface == 0) {
        // Floor — darken near walls (X and Z edges of room)
        float distX = min(abs(fragPos.x + 10.0), abs(fragPos.x - 10.0));
        float distZ = min(abs(fragPos.z + 20.0), abs(fragPos.z - 20.0));
        float edgeAO = min(distX, distZ);
        ao = mix(0.45, 1.0, smoothstep(0.0, 2.5, edgeAO));
    } else if (surface == 1) {
        // Walls — darken near floor (bottom edge)
        float heightFactor = clamp(fragPos.y / 1.5, 0.0, 1.0);
        ao = mix(0.55, 1.0, heightFactor);
        // Also darken near ceiling junction
        float ceilFactor = clamp((4.2 - fragPos.y) / 0.8, 0.0, 1.0);
        ao = min(ao, mix(0.70, 1.0, ceilFactor));
    } else {
        // Ceiling — darken near edges
        float distX = min(abs(fragPos.x + 10.0), abs(fragPos.x - 10.0));
        float distZ = min(abs(fragPos.z + 20.0), abs(fragPos.z - 20.0));
        ao = mix(0.60, 1.0, smoothstep(0.0, 3.0, min(distX, distZ)));
    }
    return ao;
}

// ─── Dynamic directional light (time-of-day, 60-second demo cycle) ───────────
// Returns (rgb color) already multiplied by intensity
vec3 calcDirectionalLight(float t, vec3 norm) {
    // t in [0,1] over 60 seconds
    float tod = mod(t / 60.0, 1.0);

    // Sun colour: warm sunrise/sunset ↔ cool neutral noon
    vec3 sunriseColor = vec3(1.0,  0.65, 0.25);  // deep amber
    vec3 noonColor    = vec3(1.0,  0.98, 0.92);  // near-white
    vec3 sunsetColor  = vec3(0.95, 0.40, 0.15);  // red-orange

    vec3 sunColor;
    if (tod < 0.5)
        sunColor = mix(sunriseColor, noonColor,    smoothstep(0.15, 0.45, tod));
    else
        sunColor = mix(noonColor,    sunsetColor,  smoothstep(0.50, 0.85, tod));

    // Intensity: fades in/out at cycle edges (simulates twilight)
    float sunIntensity = smoothstep(0.0, 0.2, tod) * (1.0 - smoothstep(0.8, 1.0, tod));
    sunIntensity *= 0.07;   // keep subtle — this is an indoor gallery

    // Direction sweeps around Y axis (subtle, mostly top-down)
    float angle  = tod * 3.14159 * 2.0;
    vec3  sunDir = normalize(vec3(sin(angle) * 0.4, 0.9, cos(angle) * 0.4));

    float diff = max(dot(norm, sunDir), 0.0);
    return sunColor * diff * sunIntensity;
}

// ─── Emissive ceiling strip glow ──────────────────────────────────────────────
// Analytic emissive brightening near the 7 ceiling strip positions —
// makes the area directly above each lamp feel self-luminous.
vec3 calcEmissiveCeiling(vec3 fragPos, vec3 albedo) {
    if (surfaceType != 2) return vec3(0.0);  // only ceiling

    vec3  stripColor = vec3(1.0, 0.97, 0.90);
    float stripX     = 0.0;
    float stripY     = 5.85;

    float zPos[7];
    zPos[0] = -18.0; zPos[1] = -12.0; zPos[2] = -6.0; zPos[3] = 0.0;
    zPos[4] =  6.0;  zPos[5] =  12.0; zPos[6] = 18.0;

    float glow = 0.0;
    for (int i = 0; i < 7; i++) {
        float dx  = fragPos.x - stripX;
        float dz  = fragPos.z - zPos[i];
        float d2  = dx*dx + dz*dz;
        glow += exp(-d2 * 0.8);   // Gaussian falloff
    }
    glow = clamp(glow, 0.0, 1.0);
    return stripColor * albedo * glow * 0.9;
}

// =============================================================================
//  main()
// =============================================================================
void main() {
    // ── Surface albedo & roughness from procedural textures ──────────────────
    vec3  albedo;
    float roughness;

    if (surfaceType == 0) {
        albedo    = woodFloorTexture(TexCoord);
        roughness = 0.35;
    } else if (surfaceType == 1) {
        albedo    = plasterWallTexture(TexCoord);
        roughness = 0.85;
    } else {
        albedo    = ceilingTexture(TexCoord);
        roughness = 0.90;
    }

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // ── Baked AO ─────────────────────────────────────────────────────────────
    float ao = calcBakedAO(FragPos, surfaceType);

    // ── Ambient (global fill — kept low for gallery drama) ────────────────────
    vec3 lighting = albedo * vec3(0.92, 0.90, 0.86) * 0.04 * ao;

    // ── Dynamic directional (time-of-day sun bleed through skylights) ─────────
    lighting += calcDirectionalLight(time, norm) * albedo * ao;

    // ── Shadow factor (HIGH/ULTRA only) ──────────────────────────────────────
    float shadow = 0.0;
    if (shadowsEnabled == 1) {
        shadow = calcShadow(FragPosLightSpace);
    }
    float litFactor = 1.0 - shadow * 0.6;   // max 60% shadow (never pitch black)

    // ── Multi-light point lights ──────────────────────────────────────────────
    for (int i = 0; i < numLights; i++) {
        vec3 contrib = calcPointLight(lights[i], norm, FragPos, viewDir, albedo, roughness);
        lighting += contrib * litFactor * ao;
    }

    // ── Emissive ceiling strips (self-luminous glow) ──────────────────────────
    lighting += calcEmissiveCeiling(FragPos, albedo);

    // ── Tone mapping (Reinhard) + gamma correction 2.2 ───────────────────────
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}