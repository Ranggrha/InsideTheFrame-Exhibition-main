#version 330 core

out vec4 FragColor;

in vec2  TexCoord;
in vec3  FragPos;
in vec3  Normal;

uniform int  surfaceType;
uniform vec3 viewPos;

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

float hash(vec2 p) {
    p = fract(p * vec2(127.1, 311.7));
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

    float tint      = grainSeed;
    vec3  baseLight = vec3(0.55, 0.32, 0.14);
    vec3  baseDark  = vec3(0.22, 0.10, 0.04);
    vec3  plankBase = mix(baseDark, baseLight, tint * 0.6 + 0.2);

    vec3 woodColor = mix(plankBase * 0.75, plankBase * 1.15,
                         grain * 0.7 + grain2 * 0.3);

    float edgeX  = smoothstep(0.97, 1.0, local.x);
    float edgeY  = smoothstep(0.93, 1.0, local.y);
    float groove = max(edgeX, edgeY);
    woodColor    = mix(woodColor, vec3(0.06, 0.03, 0.01), groove * 0.9);
    woodColor   *= 1.0 + smoothNoise(uv * 60.0) * 0.04;

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

    float crackNoise  = fbm(st * vec2(9.0, 5.0));
    float crack       = smoothstep(0.68, 0.70, crackNoise);
    plasterColor      = mix(plasterColor, vec3(0.50, 0.48, 0.44), crack * 0.55);

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

void main() {
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

    vec3 lighting = vec3(0.0);

    // Ambient global rendah
    lighting += albedo * vec3(0.92, 0.90, 0.86) * 0.04;

    // =====================================================
    // LAMPU STRIP MEMANJANG DI TENGAH LANGIT-LANGIT
    // 7 point light berjajar dari Z=-18 sampai Z=18
    // =====================================================
    vec3  stripColor = vec3(1.0, 0.97, 0.90);
    float stripY     = 5.85;
    float stripX     = 0.0;

    float zPositions[7];
    zPositions[0] = -18.0;
    zPositions[1] = -12.0;
    zPositions[2] =  -6.0;
    zPositions[3] =   0.0;
    zPositions[4] =   6.0;
    zPositions[5] =  12.0;
    zPositions[6] =  18.0;

    for (int i = 0; i < 7; i++) {
        vec3  lPos  = vec3(stripX, stripY, zPositions[i]);
        vec3  lDir  = normalize(lPos - FragPos);
        float dist  = length(lPos - FragPos);
        float atten = 1.0 / (1.0 + 0.09 * dist + 0.012 * dist * dist);

        float diff    = max(dot(norm, lDir), 0.0);
        vec3  diffuse = diff * stripColor * albedo * 1.6;

        vec3  halfway   = normalize(lDir + viewDir);
        float shininess = mix(8.0, 128.0, 1.0 - roughness);
        float spec      = pow(max(dot(norm, halfway), 0.0), shininess);
        vec3  specular  = spec * stripColor * (0.3 - roughness * 0.25) * 1.6;

        lighting += (diffuse + specular) * atten;
    }

    // =====================================================
    // LAMPU DINDING KIRI & KANAN (amber hangat)
    // =====================================================
    PointLight wall1;
    wall1.position  = vec3(-9.0, 3.5, 0.0);
    wall1.color     = vec3(1.0, 0.75, 0.40);
    wall1.intensity = 1.0;
    wall1.constant  = 1.0;
    wall1.linear    = 0.14;
    wall1.quadratic = 0.07;

    PointLight wall2;
    wall2.position  = vec3(9.0, 3.5, 0.0);
    wall2.color     = vec3(1.0, 0.75, 0.40);
    wall2.intensity = 1.0;
    wall2.constant  = 1.0;
    wall2.linear    = 0.14;
    wall2.quadratic = 0.07;

    // =====================================================
    // ACCENT BIRU DINGIN DI UJUNG BELAKANG
    // =====================================================
    PointLight accent;
    accent.position  = vec3(0.0, 0.8, -18.0);
    accent.color     = vec3(0.45, 0.55, 1.0);
    accent.intensity = 0.6;
    accent.constant  = 1.0;
    accent.linear    = 0.22;
    accent.quadratic = 0.12;

    lighting += calcPointLight(wall1,  norm, FragPos, viewDir, albedo, roughness);
    lighting += calcPointLight(wall2,  norm, FragPos, viewDir, albedo, roughness);
    lighting += calcPointLight(accent, norm, FragPos, viewDir, albedo, roughness);

    // Tone mapping Reinhard + gamma correction 2.2
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}