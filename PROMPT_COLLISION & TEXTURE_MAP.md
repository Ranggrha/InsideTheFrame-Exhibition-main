# 🎮 InsideTheFrame Exhibition - Collision & Texture System

**Version**: 1.0  
**Date**: May 10, 2026  
**Project**: InsideTheFrame - Virtual Exhibition (OpenGL 3.3, C++17)  
**Focus**: Collision Detection + Advanced Texture Mapping (Device-Friendly)

---

## 📚 Table of Contents

1. [Overview](#overview)
2. [Part 1: Collision Detection System](#part-1-collision-detection-system)
3. [Part 2: Advanced Texture Mapping System](#part-2-advanced-texture-mapping-system)
4. [Integration & Architecture](#integration--architecture)
5. [Implementation Timeline](#implementation-timeline)
6. [Performance Benchmarks](#performance-benchmarks)

---

## Overview

This prompt combines two advanced systems for the InsideTheFrame Virtual Exhibition:

| System | Focus | Performance Budget | Memory Budget |
|--------|-------|-------------------|----------------|
| **Collision** | Player movement, interactions, raycasting | < 2ms | 70 KB |
| **Textures** | Atlasing, compression, artwork display | < 2ms | 64 MB |

**Total Frame Budget** (60 FPS target):
```
Collision:      2 ms
Textures:       2 ms
Rendering:      12 ms
─────────────────────
Total:          16 ms (60 FPS)
```

---

---

# PART 1: COLLISION DETECTION SYSTEM

## 🎯 Objective
Implement efficient **collision detection** for the InsideTheFrame Virtual Exhibition to enable:
- **Player-environment collision** (walking against walls, floor)
- **Object interaction detection** (click/interact with exhibition boards)
- **Light-volume collision** (lights only affect nearby areas)

All while maintaining **60 FPS performance** on integrated graphics.

---

## 📋 Requirements

### **Functional Goals**
1. ✅ **Boundary Collision** - Player cannot walk through walls/floor/ceiling
2. ✅ **Object Interaction** - Raycasting for mouse/click detection
3. ✅ **Light Volume Culling** - Only active lights in camera view
4. ✅ **Dynamic Object Handling** - Moving boards/furniture support
5. ✅ **Smooth Movement** - No jittering or getting stuck

### **Performance Constraints**
- **CPU Budget**: < 2ms per frame for collision calculations
- **Memory**: < 5 MB for collision data structures
- **Accuracy**: Millimeter-level precision for tight spaces
- **Target FPS**: Maintain 60 FPS during collision-heavy scenes
- **Scalability**: Support adding 20+ interactive objects

### **Device Targets**
- ✅ Desktop + Laptop + Integrated GPU
- ✅ Mobile devices (future expansion)

---

## 🔧 Technical Approach

### **1.1 Spatial Partitioning (Broad Phase)**

**Goal**: Quickly eliminate impossible collisions before expensive calculations.

#### **Option A: Bounding Volume Hierarchy (BVH)** - Recommended for medium-complexity scenes
```cpp
struct AABBNode {
    glm::vec3 minBounds;    // Axis-Aligned Bounding Box min
    glm::vec3 maxBounds;    // AABB max
    std::vector<uint32_t> objectIndices;  // If leaf node
    AABBNode* left;         // If internal node
    AABBNode* right;        // If internal node
    bool isLeaf;
};

class CollisionBVH {
    AABBNode* root;
    
    void build(const std::vector<Object>& objects);
    std::vector<uint32_t> queryPoint(glm::vec3 point);
    std::vector<uint32_t> querySphere(glm::vec3 center, float radius);
    std::vector<uint32_t> queryRay(glm::vec3 origin, glm::vec3 direction);
};
```

#### **Option B: Grid-based Spatial Hashing** - For larger open spaces (RECOMMENDED)
```cpp
class SpatialGrid {
    static const float CELL_SIZE = 2.0f;  // 2-unit cells
    std::unordered_map<uint64_t, std::vector<uint32_t>> grid;
    
    uint64_t hashCell(glm::vec3 pos);
    std::vector<uint32_t> getNearby(glm::vec3 pos, float radius);
};
```

**Recommendation**: **Use Grid-based** for this exhibition (mostly static room, few dynamic boards)

---

### **1.2 Collision Shapes (Narrow Phase)**

**Keep it simple for performance**:

#### **Shape Types:**
```cpp
enum CollisionShape {
    SHAPE_SPHERE,        // Point, moving player
    SHAPE_AABB,          // Axis-aligned boxes (walls, boards)
    SHAPE_CAPSULE,       // Player collision (cylinder with rounded ends)
    SHAPE_RAY            // Mouse raycasting
};

struct CollisionObject {
    uint32_t id;
    CollisionShape shapeType;
    glm::vec3 position;
    float rotation;        // For oriented boxes
    
    union {
        struct {
            float radius;
        } sphere;
        
        struct {
            glm::vec3 extents;  // Half-size (width, height, depth)
        } aabb;
        
        struct {
            float radius;
            float height;
        } capsule;
    } shape;
    
    bool isStatic;         // Static = no updates needed each frame
    bool isInteractable;   // Can be clicked/interacted with
};
```

#### **Collision Detection Functions:**
```cpp
// Sphere vs AABB (Player vs Wall)
bool sphereAABBCollision(glm::vec3 spherePos, float radius, 
                        glm::vec3 aabbMin, glm::vec3 aabbMax,
                        glm::vec3& contactPoint, glm::vec3& normal);

// Ray vs AABB (Mouse click on object)
bool rayAABBIntersection(glm::vec3 rayOrigin, glm::vec3 rayDir,
                        glm::vec3 aabbMin, glm::vec3 aabbMax,
                        float& outDistance);

// Sphere vs Sphere (Object-to-object)
bool sphereSphereCollision(glm::vec3 pos1, float r1,
                          glm::vec3 pos2, float r2);

// Capsule vs AABB (More accurate player collision)
bool capsuleAABBCollision(glm::vec3 capsulePos, float radius, float height,
                         glm::vec3 aabbMin, glm::vec3 aabbMax,
                         glm::vec3& contactPoint, glm::vec3& normal);
```

---

### **1.3 Player Collision Response**

**Smooth movement without getting stuck**:

```cpp
class PlayerCollisionController {
    glm::vec3 playerPos;
    float playerRadius = 0.4f;  // Player width
    float playerHeight = 1.8f;  // Player height
    
    glm::vec3 tryMove(glm::vec3 desiredPos, const std::vector<CollisionObject>& obstacles) {
        // Swept sphere: check if any collisions along path
        glm::vec3 movement = desiredPos - playerPos;
        float moveDistance = glm::length(movement);
        
        if (moveDistance < 0.001f) return playerPos;  // Too small
        
        glm::vec3 moveDir = glm::normalize(movement);
        float maxDist = moveDistance;
        
        // Check all nearby obstacles
        for (const auto& obstacle : obstacles) {
            glm::vec3 contact, normal;
            float distance;
            
            if (sphereAABBCollision(playerPos + moveDir * maxDist, 
                                   playerRadius, 
                                   obstacle.getBounds().min,
                                   obstacle.getBounds().max,
                                   contact, normal)) {
                // Collision detected: move up to collision point + margin
                maxDist = std::max(0.0f, distance - 0.05f);  // 5cm margin
            }
        }
        
        glm::vec3 newPos = playerPos + moveDir * maxDist;
        playerPos = newPos;
        return newPos;
    }
};
```

---

### **1.4 Object Interaction (Raycasting)**

**Click detection for exhibition boards**:

```cpp
class InteractionManager {
    struct RaycastHit {
        uint32_t objectId;
        float distance;
        glm::vec3 hitPoint;
        glm::vec3 normal;
    };
    
    // Cast ray from camera through mouse position
    RaycastHit raycastFromCamera(glm::vec2 mousePos, 
                                 const glm::mat4& projection,
                                 const glm::mat4& view,
                                 int viewportWidth, int viewportHeight) {
        // Convert screen coords to NDC (-1 to 1)
        glm::vec2 ndc = (mousePos / glm::vec2(viewportWidth, viewportHeight)) * 2.0f - 1.0f;
        
        // Unproject to world space
        glm::vec4 rayClip = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);
        glm::vec4 rayEye = glm::inverse(projection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
        glm::vec3 rayOrigin = camera.Position;
        
        // Test against all interactable objects
        RaycastHit closestHit;
        closestHit.distance = std::numeric_limits<float>::max();
        
        for (const auto& obj : interactableObjects) {
            float dist;
            if (rayAABBIntersection(rayOrigin, rayWorld, 
                                   obj.getBounds().min, 
                                   obj.getBounds().max, 
                                   dist)) {
                if (dist < closestHit.distance) {
                    closestHit.objectId = obj.id;
                    closestHit.distance = dist;
                    closestHit.hitPoint = rayOrigin + rayWorld * dist;
                }
            }
        }
        
        return closestHit;
    }
};
```

---

### **1.5 Scene Setup (Collision Geometry)**

**Define room collision structure**:

```cpp
class CollisionEnvironment {
    std::vector<CollisionObject> staticColliders;
    
    void setupRoomCollision() {
        // Floor
        staticColliders.push_back({
            .shapeType = SHAPE_AABB,
            .position = glm::vec3(0, -0.1f, 0),
            .shape.aabb.extents = glm::vec3(10, 0.1f, 20),
            .isStatic = true,
            .isInteractable = false
        });
        
        // Walls (4)
        // Left wall
        staticColliders.push_back({
            .position = glm::vec3(-10.5f, 2, 0),
            .shape.aabb.extents = glm::vec3(0.5f, 2, 20),
            .isStatic = true
        });
        
        // Right wall
        staticColliders.push_back({
            .position = glm::vec3(10.5f, 2, 0),
            .shape.aabb.extents = glm::vec3(0.5f, 2, 20),
            .isStatic = true
        });
        
        // Front wall
        staticColliders.push_back({
            .position = glm::vec3(0, 2, 20.5f),
            .shape.aabb.extents = glm::vec3(10, 2, 0.5f),
            .isStatic = true
        });
        
        // Back wall
        staticColliders.push_back({
            .position = glm::vec3(0, 2, -20.5f),
            .shape.aabb.extents = glm::vec3(10, 2, 0.5f),
            .isStatic = true
        });
        
        // Ceiling
        staticColliders.push_back({
            .position = glm::vec3(0, 4.1f, 0),
            .shape.aabb.extents = glm::vec3(10, 0.1f, 20),
            .isStatic = true
        });
        
        // Center partition
        staticColliders.push_back({
            .position = glm::vec3(0, 1, 0),
            .shape.aabb.extents = glm::vec3(0.2f, 1, 20),
            .isStatic = true
        });
    }
};
```

---

### **1.6 Performance Optimization Techniques**

#### **A. Broad Phase Optimization**
```cpp
// Use spatial grid for fast nearby queries
spatialGrid.update(allDynamicObjects);

// Only test collisions for nearby pairs
std::vector<CollisionPair> broadPhasePairs = spatialGrid.getCollisionCandidates();
std::vector<CollisionPair> narrowPhasePairs;

for (const auto& pair : broadPhasePairs) {
    if (detailedCollisionTest(pair)) {
        narrowPhasePairs.push_back(pair);
    }
}
```

#### **B. Caching & Invalidation**
```cpp
struct CollisionCache {
    uint32_t frameId;
    std::vector<CollisionPair> lastFrameCollisions;
    
    void update(const std::vector<CollisionPair>& newCollisions) {
        if (frameId == currentFrame) return;
        lastFrameCollisions = newCollisions;
        frameId = currentFrame;
    }
};
```

#### **C. Fixed Timestep for Physics**
```cpp
const float FIXED_DT = 1.0f / 60.0f;
float accumulator = 0;

while (accumulator >= FIXED_DT) {
    updateCollisions(FIXED_DT);
    accumulator -= FIXED_DT;
}
```

---

## 📝 Part 1 Implementation Steps

### **Phase 1: Spatial Partitioning** (3-4 days)
1. Implement spatial grid structure
2. Add update/query functions
3. Integrate with scene objects
4. Test with 10-20 objects

### **Phase 2: Collision Detection** (4-5 days)
1. Implement sphere-AABB collision
2. Implement ray-AABB intersection
3. Add capsule-AABB for player
4. Unit tests for each function

### **Phase 3: Player Collision Response** (3-4 days)
1. Implement swept sphere movement
2. Handle wall/floor sliding
3. Prevent clipping issues
4. Test smooth movement

### **Phase 4: Object Interaction** (2-3 days)
1. Implement raycasting from camera
2. Add mouse click detection
3. Highlight interactable objects
4. Trigger interaction callbacks

### **Phase 5: Optimization & Testing** (3-4 days)
1. Profile collision calculations
2. Optimize hot paths
3. Test on integrated GPU
4. Add debug visualization

---

## 🔢 Part 1 Performance Metrics

### **Expected Timings** (per frame at 60 FPS = 16.67ms):
```
Spatial grid update:     < 0.5 ms
Broad phase queries:     < 0.3 ms
Narrow phase collision:  < 1.0 ms
Player response:         < 0.3 ms
Raycasting (on click):   < 0.1 ms
─────────────────────────────────
Total collision budget:  < 2.2 ms
```

### **Memory Usage:**
```
Spatial grid (1000 cells):    ~50 KB
Collision objects (50):       ~10 KB
Broadphase pairs (100):       ~3 KB
Cache data:                   ~5 KB
─────────────────────────────────
Total collision memory:       ~70 KB
```

---

## 📊 Part 1 Quality Levels

### **ULTRA** (High precision, slower):
- Capsule-AABB collision for player
- Continuous collision detection (CCD)
- 20 nearby object tests per frame
- Raycasting with precise hit info

### **HIGH** (Balanced):
- Sphere-AABB collision for player
- Discrete collision detection
- 15 nearby object tests
- Standard raycasting

### **MEDIUM** (Fast):
- Simple sphere collision
- Only static geometry collisions
- 8 nearby object tests
- Basic raycasting

### **LOW** (Minimal):
- AABB collision only
- Only major walls/floor
- No object interaction
- No raycasting

---

### **✅ Part 1 Testing Checklist**

- [ ] Player can walk through entire room without clipping
- [ ] Player slides smoothly along walls (no jittering)
- [ ] Cannot walk through furniture/boards
- [ ] Mouse raycasting correctly identifies clicked objects
- [ ] Performance stays < 2ms on integrated GPU
- [ ] No memory leaks in collision data
- [ ] Debug visualization shows correct shapes
- [ ] Works with dynamic moving objects
- [ ] Handles edge cases (tight corners, stairs)

---

---

# PART 2: ADVANCED TEXTURE MAPPING SYSTEM

## 🎯 Objective
Implement a comprehensive **texture mapping system** for InsideTheFrame Virtual Exhibition combining:
- **Procedural textures** (already excellent - enhance further)
- **Image-based textures** (load high-quality artwork & materials)
- **Dynamic texture atlasing** (optimize memory & draw calls)
- **Mipmapping & compression** (device-friendly performance)

All while maintaining **device compatibility** and **sub-2ms texture operations**.

---

## 📋 Requirements

### **Functional Goals**
1. ✅ **Procedural Texture System** - Enhanced Perlin noise, better patterns
2. ✅ **Image Texture Loading** - PNG/JPG support with automatic compression
3. ✅ **Texture Atlasing** - Batch multiple textures into atlas
4. ✅ **Dynamic Texture Streaming** - Load/unload based on LOD
5. ✅ **Material System** - Define properties (roughness, metallic, etc.)
6. ✅ **Exhibition Artwork Display** - Load artwork images onto boards

### **Performance Constraints**
- **GPU Memory**: Max 128 MB texture data on integrated GPU
- **Load Time**: Texture loading < 100ms per image
- **Shader Ops**: Texture sampling + procedural gen < 100 ALU ops
- **Draw Calls**: Batch textures to < 10 draw calls per render
- **VRAM Cache**: Mipmap levels reduce memory by 33%

### **Device Targets**
- ✅ Desktop + Laptop + Integrated GPU (128 MB texture budget)
- ✅ Mobile (future - 64 MB texture budget)

---

## 🔧 Technical Approach

### **2.1 Texture Format & Compression Strategy**

#### **Format Selection Table:**
```
Texture Type        | Format          | Size Reduction | Use Case
────────────────────┼─────────────────┼────────────────┼──────────────────────
Diffuse (Color)     | BC1 (DXT1)     | 6:1 (8bpp)     | Wood, walls, fabric
Normal Map          | BC5 (DXT5)     | 2:1 (16bpp)    | Surface details
Material (PBR)      | BC4 (DXT3)     | 4:1 (16bpp)    | Roughness/Metallic
Artwork (Photos)    | BC7 (High qual) | 4:1 (8bpp)     | Exhibition artwork
Procedural (Noise)  | RGBA8          | None           | Real-time generation
```

#### **Compression Implementation:**
```cpp
enum TextureFormat {
    FORMAT_RGB,         // Uncompressed 8-bit
    FORMAT_RGBA,        // Uncompressed 8-bit
    FORMAT_BC1,         // DXT1 compression (6:1 ratio)
    FORMAT_BC3,         // DXT5 compression (2:1 ratio)
    FORMAT_BC4,         // Single channel (4:1 ratio)
    FORMAT_BC5,         // Two channel normal map (2:1 ratio)
    FORMAT_BC6H,        // HDR (8:1 ratio)
    FORMAT_BC7,         // High quality (8:1 ratio)
    FORMAT_ETC2,        // Mobile (6:1 ratio)
    FORMAT_ASTC,        // Mobile (varies)
};

class TextureLoader {
    struct TextureData {
        uint32_t width, height;
        TextureFormat format;
        std::vector<uint8_t> pixelData;
        std::vector<std::vector<uint8_t>> mipmaps;  // Pre-computed levels
    };
    
    TextureData loadAndCompress(const char* filePath, 
                               TextureFormat targetFormat) {
        // 1. Load image (STB Image already in project)
        int width, height, channels;
        uint8_t* imageData = stbi_load(filePath, &width, &height, &channels, 4);
        
        // 2. Compress using libsquish or similar
        TextureData result;
        result.pixelData = compressImage(imageData, width, height, targetFormat);
        result.format = targetFormat;
        result.width = width;
        result.height = height;
        
        // 3. Generate mipmaps
        result.mipmaps = generateMipmaps(result.pixelData, width, height, targetFormat);
        
        free(imageData);
        return result;
    }
};
```

---

### **2.2 Texture Atlasing System**

**Goal**: Combine multiple textures into single large texture → fewer draw calls

#### **Atlas Structure:**
```cpp
struct TextureAtlas {
    uint32_t atlasID;
    uint32_t width, height;           // 2048x2048 or 4096x4096
    uint32_t nextX, nextY;            // Packing position
    std::vector<uint32_t> textureIds; // Which textures are in this atlas
    
    struct Entry {
        uint32_t textureId;
        uint16_t atlasX, atlasY;       // Position in atlas
        uint16_t width, height;        // Size
        float uvOffsetX, uvOffsetY;    // Normalized coords
        float uvScaleX, uvScaleY;
    };
    
    std::unordered_map<uint32_t, Entry> entries;
};

class AtlasManager {
    std::vector<TextureAtlas> atlases;
    const uint32_t ATLAS_SIZE = 2048;  // 2048x2048
    const uint32_t ATLAS_PADDING = 4;  // Border padding for filtering
    
    TextureAtlas& createNewAtlas() {
        atlases.emplace_back();
        atlases.back().atlasID = atlases.size() - 1;
        return atlases.back();
    }
    
    uint32_t addTextureToAtlas(uint32_t textureId, uint32_t width, uint32_t height) {
        // Try to fit in existing atlas
        for (auto& atlas : atlases) {
            if (canFit(atlas, width, height)) {
                Entry entry = packIntoAtlas(atlas, textureId, width, height);
                atlas.entries[textureId] = entry;
                return atlas.atlasID;
            }
        }
        
        // Create new atlas
        auto& newAtlas = createNewAtlas();
        Entry entry = packIntoAtlas(newAtlas, textureId, width, height);
        newAtlas.entries[textureId] = entry;
        return newAtlas.atlasID;
    }
};
```

#### **Shader Integration:**
```glsl
// Vertex shader
layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 atlasInfo;  // atlasID, offsetX, offsetY

out vec2 texCoordOut;
out uint atlasID;

void main() {
    texCoordOut = texCoords * uv_scale + vec2(atlasInfo.y, atlasInfo.z);
    atlasID = uint(atlasInfo.x);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

// Fragment shader
uniform sampler2D textureAtlases[4];  // Multiple atlases

void main() {
    vec4 color = texture(textureAtlases[atlasID], texCoordOut);
    FragColor = color;
}
```

---

### **2.3 Procedural Texture Enhancement**

**Expand on existing system with more variety**:

#### **Enhanced Noise Functions:**
```glsl
// Current: Simple Perlin Noise
// Enhancement: Add Worley noise for cellular patterns

float worleyNoise(vec2 uv) {
    vec2 id = floor(uv);
    vec2 ff = fract(uv);
    
    float minDist = 1.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = random2D(id + neighbor);
            vec2 diff = neighbor + point - ff;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

// Tileable noise (connects edges for seamless textures)
float tileableNoise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    
    float a = hash(i);
    float b = hash(i + vec2(1, 0));
    float c = hash(i + vec2(0, 1));
    float d = hash(i + vec2(1, 1));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    float result = mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
    return result;
}
```

#### **Texture Pattern Library:**
```cpp
enum ProceduralPattern {
    PATTERN_WOOD,           // Current - wood grain
    PATTERN_MARBLE,         // Veins + noise
    PATTERN_CONCRETE,       // Rough aggregate
    PATTERN_FABRIC,         // Woven pattern
    PATTERN_TILES,          // Grid + grout
    PATTERN_BRICKS,         // Running bond pattern
    PATTERN_NOISE_FBM,      // Generic Perlin FBM
};

struct ProceduralTexture {
    ProceduralPattern pattern;
    glm::vec3 colorA, colorB;  // Blend colors
    float scale;               // Detail scale
    float roughness;           // Micro detail
    int octaves;               // FBM complexity
};
```

---

### **2.4 Material System (PBR-Lite)**

**Physically-based rendering properties with minimal overhead**:

```cpp
struct Material {
    uint32_t textureId;           // Diffuse/Albedo
    uint32_t normalMapId;         // Normal map
    uint32_t roughnessMapId;      // Roughness texture
    uint32_t metallicMapId;       // Metallic texture
    
    glm::vec3 baseColor;          // If no texture
    float roughness;              // 0=smooth, 1=rough
    float metallic;               // 0=dielectric, 1=metal
    float specularIntensity;      // 0-1
    
    ProceduralTexture procedural; // Optional procedural overlay
};

class MaterialLibrary {
    std::unordered_map<std::string, Material> materials;
    
    void createPresets() {
        // Wood floor
        materials["wood_floor"] = {
            .baseColor = glm::vec3(0.55f, 0.32f, 0.14f),
            .roughness = 0.6f,
            .metallic = 0.0f
        };
        
        // Plaster wall
        materials["plaster_wall"] = {
            .baseColor = glm::vec3(0.95f, 0.93f, 0.89f),
            .roughness = 0.8f,
            .metallic = 0.0f
        };
        
        // Metal frame
        materials["metal_frame"] = {
            .baseColor = glm::vec3(0.8f, 0.8f, 0.8f),
            .roughness = 0.2f,
            .metallic = 1.0f
        };
    }
};
```

---

### **2.5 Exhibition Artwork Display**

**Load & display artwork on boards**:

```cpp
class ExhibitionBoard {
    uint32_t boardId;
    Material artworkMaterial;
    uint32_t artworkTextureId;
    
    void loadArtwork(const char* imagePath) {
        // Load artwork image
        int width, height, channels;
        uint8_t* imageData = stbi_load(imagePath, &width, &height, &channels, 4);
        
        // Create texture
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
        
        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Set filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Store reference
        artworkTextureId = textureId;
        
        // Create material
        artworkMaterial = {
            .textureId = textureId,
            .roughness = 0.3f,
            .metallic = 0.0f
        };
        
        stbi_image_free(imageData);
    }
};
```

---

### **2.6 Streaming & LOD System**

**Load textures based on distance/importance**:

```cpp
class TextureStreamingManager {
    enum LODLevel {
        LOD_ULTRA,    // Full quality (1024x1024)
        LOD_HIGH,     // 80% quality (512x512)
        LOD_MEDIUM,   // 50% quality (256x256)
        LOD_LOW,      // 25% quality (128x128)
        LOD_UNLOADED  // Not in memory
    };
    
    struct StreamingTexture {
        uint32_t textureId;
        std::string filePath;
        LODLevel currentLOD;
        float priority;           // 0-1: importance
        float distanceToCamera;
    };
    
    std::vector<StreamingTexture> streamingTextures;
    const uint32_t MEMORY_BUDGET = 64 * 1024 * 1024;  // 64 MB
    
    void updateLOD(float deltaTime) {
        // Update distances
        for (auto& tex : streamingTextures) {
            tex.distanceToCamera = glm::length(tex.position - camera.Position);
        }
        
        // Sort by priority & distance
        std::sort(streamingTextures.begin(), streamingTextures.end(),
            [](const auto& a, const auto& b) {
                return (a.priority / (a.distanceToCamera + 1.0f)) > 
                       (b.priority / (b.distanceToCamera + 1.0f));
            });
        
        // Load/unload based on memory budget
        uint32_t usedMemory = 0;
        for (auto& tex : streamingTextures) {
            LODLevel desiredLOD = getLODForDistance(tex.distanceToCamera);
            uint32_t requiredMemory = getMemoryForLOD(desiredLOD);
            
            if (usedMemory + requiredMemory <= MEMORY_BUDGET) {
                setTextureLOD(tex, desiredLOD);
                usedMemory += requiredMemory;
            } else {
                setTextureLOD(tex, LOD_LOW);
            }
        }
    }
};
```

---

## 📝 Part 2 Implementation Steps

### **Phase 1: Image Loading & Compression** (4-5 days)
1. Integrate texture compression library
2. Implement TextureLoader class
3. Test loading PNG/JPG files
4. Verify compression quality

### **Phase 2: Texture Atlasing** (5-6 days)
1. Implement strip packing algorithm
2. Create AtlasManager class
3. Update shaders for atlas coords
4. Batch draw calls

### **Phase 3: Procedural Enhancement** (3-4 days)
1. Add Worley noise
2. Add new pattern types
3. Create pattern library
4. Update material system

### **Phase 4: Material & PBR-Lite** (4-5 days)
1. Implement Material struct
2. Create MaterialLibrary
3. Update rendering code
4. Test visual quality

### **Phase 5: Artwork Display** (2-3 days)
1. Add artwork loading to ExhibitionBoard
2. Implement dynamic texture swapping
3. Test with sample artwork

### **Phase 6: Streaming & Testing** (4-5 days)
1. Implement LOD system
2. Profile memory usage
3. Optimize hot paths
4. Test on integrated GPU

---

## 🔢 Part 2 Performance Expectations

### **Memory Usage** (64 MB budget):
```
Atlased textures:     48 MB (3 × 2048x2048 atlases)
Mipmaps (auto):       16 MB (included in above)
Material data:        0.1 MB
Artwork textures:     12 MB (3 × 1024x1024 artwork)
─────────────────────────────
Total:                ~60-64 MB
```

### **Load Times**:
```
Load PNG (512x512):   5-10 ms
Compress to BC1:      20-30 ms
Create atlas:         50-100 ms
Generate mipmaps:     10-15 ms
─────────────────────────────
Per image total:      85-155 ms
```

### **Shader Performance**:
```
Texture sample:       ~5 cycles
Normal mapping:       ~3 cycles
Material calc:        ~8 cycles
PBR lighting:         ~15 cycles
─────────────────────────────
Fragment shader:      ~30 cycles / pixel
```

---

## 📊 Part 2 Quality Presets

### **ULTRA** (High-end GPU):
- 4096x4096 atlases
- BC7 compression
- Full mipmap chains
- Real-time artwork compression

### **HIGH** (Mid-range GPU):
- 2048x2048 atlases
- BC1/BC3 compression
- Pre-compressed artwork

### **MEDIUM** (Integrated GPU):
- 1024x1024 atlases
- BC1 compression only
- 3-level mipmaps
- Artwork 512x512

### **LOW** (Budget device):
- 512x512 atlases
- No compression
- 2-level mipmaps
- Artwork 256x256

---

### **✅ Part 2 Testing Checklist**

- [ ] Textures load without crashes
- [ ] Compression looks visually acceptable
- [ ] Atlas packing fills space efficiently
- [ ] No texture bleeding at atlas borders
- [ ] Mipmaps render correctly at distance
- [ ] Artwork displays on exhibition boards
- [ ] Performance stays < 2ms on integrated GPU
- [ ] Memory usage within budget
- [ ] No visual tearing or artifacts
- [ ] Smooth LOD transitions

---

---

# INTEGRATION & ARCHITECTURE

## 🏗️ System Integration

### **Data Flow:**
```
Input (Keyboard/Mouse)
    ↓
[Collision System] → Player movement, Object interaction
    ↓
[Texture System] → Load, prepare materials
    ↓
[Rendering] → Combine all & draw to screen
```

### **Shared Data Structures:**

```cpp
struct SceneObject {
    uint32_t id;
    glm::vec3 position;
    glm::mat4 modelMatrix;
    
    // Collision
    CollisionObject collider;
    
    // Rendering
    uint32_t meshId;
    Material material;
    GLuint textureAtlasId;
};

struct RenderContext {
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 cameraPosition;
    
    GLuint textureAtlas;
};
```

---

## 📋 Unified Quality System

**Single enum controls both systems:**

```cpp
enum QualityPreset {
    ULTRA,      // High-end desktop/GPU
    HIGH,       // Mid-range gaming GPU
    MEDIUM,     // Integrated GPU (Intel 630)
    LOW,        // Budget integrated GPU
    MINIMAL     // Mobile/very low-end
};

struct QualityConfig {
    // Collision
    bool continuousCollisionDetection;
    int maxNearbyObjects;
    
    // Texture
    TextureQuality textureQuality;
    uint32_t maxTextureResolution;
    uint32_t atlasSize;
    
    // Rendering
    int targetFPS;
    float renderScale;
};
```

---

## 🔄 Main Loop Integration

```cpp
int main() {
    // Initialize
    initializeOpenGL();
    scene.load();
    
    // Setup systems
    CollisionSystem collisionSystem;
    TextureSystem textureSystem;
    
    // Auto-detect quality
    QualityPreset quality = detectGPUCapability();
    QualityConfig config = getQualityConfig(quality);
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float deltaTime = calculateDeltaTime();
        
        // ==== UPDATE PHASE ====
        handleInput();
        collisionSystem.updatePlayerMovement(deltaTime);
        collisionSystem.updateObjectInteractions();
        textureSystem.updateLOD(deltaTime);
        
        // ==== RENDER PHASE ====
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        for (const auto& obj : scene.objects) {
            if (isVisible(obj, renderCtx.viewMatrix)) {
                renderObject(obj, renderCtx, config);
            }
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    cleanup();
    return 0;
}
```

---

---

# IMPLEMENTATION TIMELINE

## 📅 Total: 5-7 weeks (1.5-2 months)

### **Weeks 1-2: Collision Detection**
- [ ] Spatial grid implementation
- [ ] Collision shapes & detection
- [ ] Player movement response
- [ ] Integration with existing camera

### **Weeks 3-4: Texture Mapping (Phase 1-3)**
- [ ] Image loading & compression
- [ ] Texture atlasing system
- [ ] Procedural texture enhancement
- [ ] Material system

### **Week 5: Object Interaction & Artwork**
- [ ] Raycasting for mouse picking
- [ ] Exhibition board artwork loading
- [ ] Interaction callbacks

### **Week 6: Streaming & LOD**
- [ ] Texture streaming & LOD
- [ ] Memory management
- [ ] Dynamic texture swapping

### **Week 7: Optimization & Polish**
- [ ] Performance profiling
- [ ] Quality preset tuning
- [ ] Debug visualization
- [ ] Documentation

---

---

# PERFORMANCE BENCHMARKS

## 🖥️ Target Frame Times (60 FPS = 16.67ms)

### **Desktop (GTX 1080 or better):**
```
Collision:     0.1 ms
Textures:      0.1 ms
Rendering:    10.0 ms
─────────────────────────────
Total:        10.2 ms (39% headroom)
```

### **Mid-range GPU (GTX 960, RX 470):**
```
Collision:     0.5 ms
Textures:      0.3 ms
Rendering:    12.0 ms
─────────────────────────────
Total:        12.8 ms (23% headroom)
```

### **Integrated GPU (Intel HD 630):**
```
Collision:     0.8 ms
Textures:      0.6 ms
Rendering:    12.5 ms
─────────────────────────────
Total:        13.9 ms (17% headroom)
```

### **Low-end (UHD 610, Vega 3):**
```
Collision:     1.0 ms
Textures:      1.0 ms
Rendering:    13.0 ms
─────────────────────────────
Total:        15.0 ms (10% headroom)
```

---

## 💾 Memory Footprint

```
Collision System:     ~100 KB
Texture System:       ~65 MB
Scene Objects:        ~5 MB
─────────────────────────────
Total VRAM:           ~70 MB

System RAM:           ~150-200 MB
```

---

---

**Created**: May 10, 2026  
**Project**: InsideTheFrame - Virtual Exhibition  
**Status**: Ready for Implementation  
**Compatibility**: OpenGL 3.3+, C++17, All GPU Tiers
