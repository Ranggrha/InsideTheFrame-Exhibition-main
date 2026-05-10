// =============================================================================
//  ruangan.fs — Room fragment shader  (Collision & Texture Edition)
//
//  NEW vs previous version:
//   • worleyNoise()    — cellular/stone pattern for accent surfaces
//   • marbleTexture()  — fbm-distorted sine veins
//   • brickTexture()   — running-bond brick with mortar joints
//   • surfaceType 3    → marble accent panels (raised centre wall strip)
//   • surfaceType 4    → brick (back wall, below arch)
//   • All original features preserved (PCF shadow, multi-light, AO, emissive)
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
uniform int   surfaceType;      // 0=floor 1=wall 2=ceiling 3=marble 4=brick
uniform vec3  viewPos;
uniform float time;
uniform int   lightingQuality;
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

// =============================================================================
//  NOISE LIBRARY
// =============================================================================

float hash(vec2 p) {
    p  = fract(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return fract(p.x * p.y);
}

float smoothNoise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st) {
    float value = 0.0, amplitude = 0.5, frequency = 1.0;
    for (int i = 0; i < 5; i++) {
        value     += amplitude * smoothNoise(st * frequency);
        frequency *= 2.1;
        amplitude *= 0.5;
    }
    return value;
}

// ── Worley (cellular) noise — returns distance to nearest feature point ───────
float worleyNoise(vec2 uv) {
    vec2  id = floor(uv);
    vec2  ff = fract(uv);
    float minDist = 1.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 neighbor = vec2(float(x), float(y));
            // Feature point: pseudo-random position inside each cell
            vec2 point = vec2(hash(id + neighbor + vec2(0.3)),
                              hash(id + neighbor + vec2(0.7)));
            vec2 diff  = neighbor + point - ff;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

// =============================================================================
//  PROCEDURAL TEXTURES
// =============================================================================

// ── Original: Wood floor ──────────────────────────────────────────────────────
vec3 woodFloorTexture(vec2 uv) {
    vec2  plankUv  = uv * vec2(10.0, 2.0);
    float row      = floor(plankUv.y);
    float colShift = mod(row, 2.0) * 0.5;
    plankUv.x     += colShift;
    float plankCol = floor(plankUv.x);
    float plankRow = row;
    vec2  local    = fract(plankUv);
    float grainSeed = hash(vec2(plankCol, plankRow));
    vec2  grainUv   = vec2(uv.x * 20.0, uv.y * 200.0 + grainSeed * 50.0);
    float grain     = fbm(grainUv);
    float grain2    = smoothNoise(grainUv * vec2(0.5, 2.0));
    vec3  baseLight = vec3(0.55, 0.32, 0.14);
    vec3  baseDark  = vec3(0.22, 0.10, 0.04);
    vec3  plankBase = mix(baseDark, baseLight, grainSeed * 0.6 + 0.2);
    vec3  woodColor = mix(plankBase * 0.75, plankBase * 1.15, grain * 0.7 + grain2 * 0.3);
    float edgeX     = smoothstep(0.97, 1.0, local.x);
    float edgeY     = smoothstep(0.93, 1.0, local.y);
    woodColor = mix(woodColor, vec3(0.06, 0.03, 0.01), max(edgeX, edgeY) * 0.9);
    woodColor *= 1.0 + smoothNoise(uv * 60.0) * 0.04;
    return woodColor;
}

// ── Original: Plaster wall ───────────────────────────────────────────────────
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
    float crackNoise  = fbm(st * vec2(9.0, 5.0));
    float crack       = smoothstep(0.68, 0.70, crackNoise);
    plasterColor      = mix(plasterColor, vec3(0.50, 0.48, 0.44), crack * 0.55);
    float dirtyFactor = smoothstep(0.15, 0.0, uv.y);
    plasterColor      = mix(plasterColor, vec3(0.60, 0.55, 0.48), dirtyFactor * 0.35);
    return plasterColor;
}

// ── Original: Ceiling tiles ───────────────────────────────────────────────────
vec3 ceilingTexture(vec2 uv) {
    vec2  panelUv = uv * vec2(5.0, 2.5);
    float px      = smoothstep(0.96, 1.0, fract(panelUv.x));
    float py      = smoothstep(0.96, 1.0, fract(panelUv.y));
    float grid    = max(px, py);
    vec2  st      = uv * 3.0;
    float base    = fbm(st * 2.0) * 0.06;
    float fine    = smoothNoise(st * 14.0) * 0.04;
    float grain   = hash(uv * 600.0) * 0.02;
    vec3 ceilColor = vec3(0.97, 0.96, 0.93) + vec3(base + fine + grain - 0.03);
    ceilColor = mix(ceilColor, vec3(0.70, 0.68, 0.65), grid * 0.6);
    return ceilColor;
}

// ── NEW: Marble accent panels (surfaceType 3) ─────────────────────────────────
// Inspired by Carrara marble: white base, grey veins distorted with fbm
vec3 marbleTexture(vec2 uv) {
    vec2  st      = uv * vec2(3.0, 5.0);
    // Distort the vein coordinate with FBM for organic flow
    float distort = fbm(st * 1.5) * 2.0;
    float vein    = sin((st.x + st.y + distort) * 3.14159);
    vein          = abs(vein);
    vein          = pow(vein, 2.5);   // sharpen veins

    // Secondary fine veins
    float vein2   = sin((st.x * 1.7 - st.y * 0.9 + fbm(st * 3.0)) * 6.28);
    vein2         = abs(vein2);
    vein2         = pow(vein2, 6.0) * 0.4;

    vec3 baseColor = vec3(0.96, 0.94, 0.92);   // warm white
    vec3 veinColor = vec3(0.42, 0.40, 0.42);   // cool grey vein
    vec3 darkVein  = vec3(0.22, 0.20, 0.24);   // dark accent

    vec3 marble = mix(baseColor, veinColor, vein * 0.55);
    marble      = mix(marble,    darkVein,  vein2);
    // Subtle worley dimple pattern for fine crystal texture
    float crystal = worleyNoise(uv * 18.0) * 0.08;
    marble        = marble * (0.94 + crystal);
    return marble;
}

// ── NEW: Brick wall (surfaceType 4) ──────────────────────────────────────────
// Running-bond pattern with mortar joints and surface variation
vec3 brickTexture(vec2 uv) {
    vec2  brickUv  = uv * vec2(8.0, 4.0);
    float row      = floor(brickUv.y);
    float offset   = mod(row, 2.0) * 0.5;   // running bond offset
    brickUv.x     += offset;

    vec2  local    = fract(brickUv);

    // Mortar joint: thin gap at edges
    float mortarX  = smoothstep(0.90, 0.95, local.x) + smoothstep(0.05, 0.0, local.x);
    float mortarY  = smoothstep(0.88, 0.93, local.y) + smoothstep(0.07, 0.0, local.y);
    float mortar   = clamp(mortarX + mortarY, 0.0, 1.0);

    // Per-brick colour variation
    float brickId  = hash(floor(brickUv));
    vec3  brickA   = vec3(0.72, 0.32, 0.20);   // warm terracotta
    vec3  brickB   = vec3(0.55, 0.22, 0.14);   // dark brick
    vec3  brickC   = vec3(0.80, 0.40, 0.26);   // light brick
    vec3  brickCol = mix(brickA, brickB, brickId);
    brickCol       = mix(brickCol, brickC, smoothNoise(brickUv * 2.0) * 0.35);

    // Surface roughness (noise overlay)
    float rough    = fbm(uv * 20.0) * 0.12;
    brickCol      *= (0.88 + rough);

    // Worley divot detail
    float divot    = 1.0 - worleyNoise(uv * 12.0) * 0.2;
    brickCol      *= divot;

    vec3 mortarCol = vec3(0.75, 0.72, 0.68) + fbm(uv * 35.0) * 0.06;
    return mix(brickCol, mortarCol, mortar);
}

// =============================================================================
//  LIGHTING HELPERS
// =============================================================================

float calcShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords      = projCoords * 0.5 + 0.5;
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

vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos,
                    vec3 viewDir, vec3 albedo, float roughness) {
    vec3  lightDir  = normalize(light.position - fragPos);
    float dist      = length(light.position - fragPos);
    float atten     = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    float diff      = max(dot(norm, lightDir), 0.0);
    vec3  halfway   = normalize(lightDir + viewDir);
    float shininess = mix(8.0, 64.0, 1.0 - roughness);
    float spec      = pow(max(dot(norm, halfway), 0.0), shininess);
    vec3  diffuse   = diff * light.color * albedo * light.intensity;
    vec3  specular  = spec * light.color * (0.15 - roughness * 0.1) * light.intensity;
    return (diffuse + specular) * atten;
}

float calcBakedAO(vec3 fragPos, int surface) {
    float ao = 1.0;
    if (surface == 0) {
        float distX = min(abs(fragPos.x + 10.0), abs(fragPos.x - 10.0));
        float distZ = min(abs(fragPos.z + 20.0), abs(fragPos.z - 20.0));
        ao = mix(0.45, 1.0, smoothstep(0.0, 2.5, min(distX, distZ)));
    } else if (surface == 1 || surface == 3 || surface == 4) {
        float heightFactor = clamp(fragPos.y / 1.5, 0.0, 1.0);
        ao = mix(0.55, 1.0, heightFactor);
        float ceilFactor = clamp((4.2 - fragPos.y) / 0.8, 0.0, 1.0);
        ao = min(ao, mix(0.70, 1.0, ceilFactor));
    } else {
        float distX = min(abs(fragPos.x + 10.0), abs(fragPos.x - 10.0));
        float distZ = min(abs(fragPos.z + 20.0), abs(fragPos.z - 20.0));
        ao = mix(0.60, 1.0, smoothstep(0.0, 3.0, min(distX, distZ)));
    }
    return ao;
}

vec3 calcDirectionalLight(float t, vec3 norm) {
    float tod = mod(t / 60.0, 1.0);
    vec3 sunriseColor = vec3(1.0,  0.65, 0.25);
    vec3 noonColor    = vec3(1.0,  0.98, 0.92);
    vec3 sunsetColor  = vec3(0.95, 0.40, 0.15);
    vec3 sunColor;
    if (tod < 0.5)
        sunColor = mix(sunriseColor, noonColor,   smoothstep(0.15, 0.45, tod));
    else
        sunColor = mix(noonColor,   sunsetColor,  smoothstep(0.50, 0.85, tod));
    float sunIntensity = smoothstep(0.0, 0.2, tod) * (1.0 - smoothstep(0.8, 1.0, tod));
    sunIntensity *= 0.07;
    float angle  = tod * 3.14159 * 2.0;
    vec3  sunDir = normalize(vec3(sin(angle) * 0.4, 0.9, cos(angle) * 0.4));
    return sunColor * max(dot(norm, sunDir), 0.0) * sunIntensity;
}

vec3 calcEmissiveCeiling(vec3 fragPos, vec3 albedo) {
    if (surfaceType != 2) return vec3(0.0);
    vec3 stripColor = vec3(1.0, 0.97, 0.90);
    float zPos[7];
    zPos[0]=-18.0; zPos[1]=-12.0; zPos[2]=-6.0; zPos[3]=0.0;
    zPos[4]=6.0;   zPos[5]=12.0;  zPos[6]=18.0;
    float glow = 0.0;
    for (int i = 0; i < 7; i++) {
        float dx = fragPos.x;
        float dz = fragPos.z - zPos[i];
        glow += exp(-(dx*dx + dz*dz) * 0.8);
    }
    return stripColor * albedo * clamp(glow, 0.0, 1.0) * 0.9;
}

// =============================================================================
//  main()
// =============================================================================
void main() {
    // ── Surface albedo & roughness ───────────────────────────────────────────
    vec3  albedo;
    float roughness;

    if (surfaceType == 0) {
        albedo    = woodFloorTexture(TexCoord);
        roughness = 0.35;
    } else if (surfaceType == 1) {
        albedo    = plasterWallTexture(TexCoord);
        roughness = 0.85;
    } else if (surfaceType == 2) {
        albedo    = ceilingTexture(TexCoord);
        roughness = 0.90;
    } else if (surfaceType == 3) {
        albedo    = marbleTexture(TexCoord);
        roughness = 0.15;   // marble is polished
    } else {
        // surfaceType 4 = brick
        albedo    = brickTexture(TexCoord);
        roughness = 0.92;
    }

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // ── Baked AO ─────────────────────────────────────────────────────────────
    float ao = calcBakedAO(FragPos, surfaceType);

    // ── Ambient ───────────────────────────────────────────────────────────────
    vec3 lighting = albedo * vec3(0.92, 0.90, 0.86) * 0.04 * ao;

    // ── Directional (time-of-day) ─────────────────────────────────────────────
    lighting += calcDirectionalLight(time, norm) * albedo * ao;

    // ── Shadow ───────────────────────────────────────────────────────────────
    float shadow = 0.0;
    if (shadowsEnabled == 1) shadow = calcShadow(FragPosLightSpace);
    float litFactor = 1.0 - shadow * 0.6;

    // ── Multi-light ───────────────────────────────────────────────────────────
    for (int i = 0; i < numLights; i++)
        lighting += calcPointLight(lights[i], norm, FragPos, viewDir, albedo, roughness) * litFactor * ao;

    // ── Emissive ceiling strips ───────────────────────────────────────────────
    lighting += calcEmissiveCeiling(FragPos, albedo);

    // ── Marble specular gloss boost ───────────────────────────────────────────
    if (surfaceType == 3) {
        vec3 reflDir = reflect(-viewDir, norm);
        float gloss  = pow(max(dot(reflDir, norm), 0.0), 32.0) * 0.3;
        lighting    += vec3(gloss) * litFactor;
    }

    // ── Tone map + gamma ──────────────────────────────────────────────────────
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}