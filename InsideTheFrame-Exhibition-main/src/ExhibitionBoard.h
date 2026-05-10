#ifndef EXHIBITION_BOARD_H
#define EXHIBITION_BOARD_H

// =============================================================================
//  ExhibitionBoard.h — InsideTheFrame Virtual Exhibition
//  Part 2: Multiple Exhibition Boards with Artwork Textures
//
//  Each board is a thin flat panel placed on the side walls.
//  It carries:
//   • World-space position, rotation (Y-axis angle), scale
//   • Artwork texture (loaded by TextureManager)
//   • Label + description string (printed on interaction)
//   • AABB collision bounds (registered with CollisionSystem)
//   • "highlighted" state toggled by E-key raycasting
//
//  VAO layout (per vertex): pos(3) + normal(3) + uv(2)  — 8 floats
// =============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

#include "CollisionSystem.h"   // for AABB

class ExhibitionBoard {
public:
    unsigned int VAO = 0, VBO = 0;

    // World placement
    glm::vec3 position    { 0.0f };
    float     rotationY   = 0.0f;        // degrees, for wall-facing boards
    glm::vec3 scale       { 1.0f, 1.0f, 1.0f };

    // Artwork
    GLuint textureId      = 0;
    bool   hasTexture     = false;

    // Interaction metadata
    std::string label       = "Artwork";
    std::string description = "No description provided.";
    bool        highlighted = false;

    // Collision AABB (world space — set after positioning)
    AABB colliderBounds;

    // ── Constructor ──────────────────────────────────────────────────────────
    ExhibitionBoard() { initGeometry(); }

    ~ExhibitionBoard() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    // ── Setup ─────────────────────────────────────────────────────────────────
    // Call after setting position/rotationY/scale.
    // boardWidth / boardHeight are in local space (before scale).
    void setup(glm::vec3 pos, float rotY, glm::vec3 sc,
               GLuint texId, bool hasTex,
               const std::string& lbl, const std::string& desc)
    {
        position    = pos;
        rotationY   = rotY;
        scale       = sc;
        textureId   = texId;
        hasTexture  = hasTex;
        label       = lbl;
        description = desc;
        computeColliderBounds();
    }

    // Compute world-space AABB around the board (conservative — padded slightly)
    // For boards rotated ±90° around Y (side-wall panels), local X (width) maps
    // to world Z and local Z (depth 0.15) maps to world X, so we swap those extents.
    void computeColliderBounds() {
        // Half-extents in local space (width=X, height=Y, depth=Z)
        float halfW = glm::abs(scale.x) * 0.5f + 0.05f;  // wide dimension
        float halfH = glm::abs(scale.y) * 0.5f + 0.05f;  // vertical
        float halfD = glm::abs(scale.z) * 0.5f + 0.05f;  // thin depth

        glm::vec3 he;
        // rotationY = ±90° → local X becomes world Z, local Z becomes world X
        if (glm::abs(glm::abs(rotationY) - 90.0f) < 1.0f) {
            he = glm::vec3(halfD, halfH, halfW);  // world: X=depth, Y=height, Z=width
        } else {
            he = glm::vec3(halfW, halfH, halfD);
        }
        colliderBounds.min = position - he;
        colliderBounds.max = position + he;
    }

    // Returns the model matrix for this board
    glm::mat4 modelMatrix() const {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        m = glm::rotate(m, glm::radians(rotationY), glm::vec3(0, 1, 0));
        m = glm::scale(m, scale);
        return m;
    }

    // ── Draw ─────────────────────────────────────────────────────────────────
    // Shader uniforms: model, useTexture, highlighted, boardFaceAspect, artworkTex
    void draw(unsigned int shaderProgram) const {
        glm::mat4 model = modelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
                           1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"),
                    hasTexture ? 1 : 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "highlighted"),
                    highlighted ? 1 : 0);

        // Front face aspect = local X extent / local Y extent.
        // After rotY=±90°, local X maps to world ±Z (along wall = the width),
        // local Y stays vertical, local Z becomes the depth into/out of wall.
        float faceAspect = (scale.y > 0.001f) ? (scale.x / scale.y) : 1.0f;
        glUniform1f(glGetUniformLocation(shaderProgram, "boardFaceAspect"), faceAspect);

        // Always bind artworkTex to unit 2 so textureSize() in the shader works
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, hasTexture ? textureId : 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "artworkTex"), 2);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

private:
    // ── Geometry (unit box with UVs and face normals) ─────────────────────
    void initGeometry() {
        // Format per vertex: pos(3) + normal(3) + uv(2)
        // Each face is 2 triangles = 6 verts, 6 faces = 36 verts total
        // UVs: front face = [0,1]x[0,1], others get dummy UVs
        float v[] = {
            // FRONT  (normal  0, 0,+1)
            -0.5f,-0.5f, 0.5f,  0,0,1,  0.0f,0.0f,
             0.5f,-0.5f, 0.5f,  0,0,1,  1.0f,0.0f,
             0.5f, 0.5f, 0.5f,  0,0,1,  1.0f,1.0f,
             0.5f, 0.5f, 0.5f,  0,0,1,  1.0f,1.0f,
            -0.5f, 0.5f, 0.5f,  0,0,1,  0.0f,1.0f,
            -0.5f,-0.5f, 0.5f,  0,0,1,  0.0f,0.0f,
            // BACK   (normal  0, 0,-1)
             0.5f,-0.5f,-0.5f,  0,0,-1, 0.0f,0.0f,
            -0.5f,-0.5f,-0.5f,  0,0,-1, 1.0f,0.0f,
            -0.5f, 0.5f,-0.5f,  0,0,-1, 1.0f,1.0f,
            -0.5f, 0.5f,-0.5f,  0,0,-1, 1.0f,1.0f,
             0.5f, 0.5f,-0.5f,  0,0,-1, 0.0f,1.0f,
             0.5f,-0.5f,-0.5f,  0,0,-1, 0.0f,0.0f,
            // LEFT   (normal -1, 0, 0)
            -0.5f,-0.5f,-0.5f, -1,0,0,  0.0f,0.0f,
            -0.5f,-0.5f, 0.5f, -1,0,0,  1.0f,0.0f,
            -0.5f, 0.5f, 0.5f, -1,0,0,  1.0f,1.0f,
            -0.5f, 0.5f, 0.5f, -1,0,0,  1.0f,1.0f,
            -0.5f, 0.5f,-0.5f, -1,0,0,  0.0f,1.0f,
            -0.5f,-0.5f,-0.5f, -1,0,0,  0.0f,0.0f,
            // RIGHT  (normal +1, 0, 0)
             0.5f,-0.5f, 0.5f,  1,0,0,  0.0f,0.0f,
             0.5f,-0.5f,-0.5f,  1,0,0,  1.0f,0.0f,
             0.5f, 0.5f,-0.5f,  1,0,0,  1.0f,1.0f,
             0.5f, 0.5f,-0.5f,  1,0,0,  1.0f,1.0f,
             0.5f, 0.5f, 0.5f,  1,0,0,  0.0f,1.0f,
             0.5f,-0.5f, 0.5f,  1,0,0,  0.0f,0.0f,
            // BOTTOM (normal  0,-1, 0)
            -0.5f,-0.5f,-0.5f,  0,-1,0, 0.0f,0.0f,
             0.5f,-0.5f,-0.5f,  0,-1,0, 1.0f,0.0f,
             0.5f,-0.5f, 0.5f,  0,-1,0, 1.0f,1.0f,
             0.5f,-0.5f, 0.5f,  0,-1,0, 1.0f,1.0f,
            -0.5f,-0.5f, 0.5f,  0,-1,0, 0.0f,1.0f,
            -0.5f,-0.5f,-0.5f,  0,-1,0, 0.0f,0.0f,
            // TOP    (normal  0,+1, 0)
            -0.5f, 0.5f, 0.5f,  0,1,0,  0.0f,0.0f,
             0.5f, 0.5f, 0.5f,  0,1,0,  1.0f,0.0f,
             0.5f, 0.5f,-0.5f,  0,1,0,  1.0f,1.0f,
             0.5f, 0.5f,-0.5f,  0,1,0,  1.0f,1.0f,
            -0.5f, 0.5f,-0.5f,  0,1,0,  0.0f,1.0f,
            -0.5f, 0.5f, 0.5f,  0,1,0,  0.0f,0.0f,
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

        GLsizei stride = 8 * sizeof(float);
        // attrib 0: position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // attrib 1: normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // attrib 2: uv
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }
};

// =============================================================================
//  createExhibitionBoards — factory for 8 side-wall gallery boards
//
//  Layout: 4 frames per wall, evenly spaced.
//  Room Z range [-20, +20]. Usable zone (clear of front/back walls): ~±17.
//  4 frames → spacing = 34 / 3 ≈ 11.33  → Z positions: -17, -5.67, +5.67, +17
//  Rounded to clean values:              → Z = -15, -5, +5, +15
//
//  Wall X positions:
//   Left wall  geometry is at X = -10.  Panel center (depth 0.15) → X = -9.925
//   Right wall geometry is at X = +10.  Panel center              → X = +9.925
//
//  rotY =  90° → panel faces +X (inward from left wall)
//  rotY = -90° → panel faces -X (inward from right wall)
// =============================================================================
inline std::vector<ExhibitionBoard> createExhibitionBoards(
        const std::vector<GLuint>& textureIds)
{
    struct BoardDef {
        glm::vec3   pos;
        float       rotY;
        glm::vec3   sc;
        int         texIdx;
        const char* label;
        const char* desc;
    };

    // Uniform frame size: 3.0 wide × 2.2 tall × 0.15 deep
    // Z positions for 4 evenly-spaced frames per wall
    // spacing = (15 - (-15)) / 3 = 10  →  Z = -15, -5, +5, +15
    static const BoardDef defs[] = {
        // ── Left wall (X = -9.925, rotY = +90°, faces inward) ────────────────
        { {-9.925f, 2.0f, -15.0f},  90.0f, {3.0f, 2.2f, 0.15f}, 0,
          "Luminous Void",
          "Oil on canvas — exploring the tension between darkness and emerging light." },

        { {-9.925f, 2.0f,  -5.0f},  90.0f, {3.0f, 2.2f, 0.15f}, 1,
          "Urban Fragments",
          "Mixed media — deconstructed city textures layered with archival photographs." },

        { {-9.925f, 2.0f,   5.0f},  90.0f, {3.0f, 2.2f, 0.15f}, 2,
          "Threshold",
          "Digital print — liminal spaces and the geometry of transition." },

        { {-9.925f, 2.0f,  15.0f},  90.0f, {3.0f, 2.2f, 0.15f}, 3,
          "Ephemeral Forms",
          "Ink and resin — transient patterns frozen in amber-coloured resin sheets." },

        // ── Right wall (X = +9.925, rotY = -90°, faces inward) ───────────────
        { { 9.925f, 2.0f, -15.0f}, -90.0f, {3.0f, 2.2f, 0.15f}, 4,
          "Resonance",
          "Acrylic on board — sound waves rendered as colour fields." },

        { { 9.925f, 2.0f,  -5.0f}, -90.0f, {3.0f, 2.2f, 0.15f}, 5,
          "Soft Architecture",
          "Photography — built environments softened by long exposure and mist." },

        { { 9.925f, 2.0f,   5.0f}, -90.0f, {3.0f, 2.2f, 0.15f}, 6,
          "Chromatic Field",
          "Watercolour on paper — gradients as emotional landscape." },

        { { 9.925f, 2.0f,  15.0f}, -90.0f, {3.0f, 2.2f, 0.15f}, 7,
          "Silent Geometry",
          "Graphite on panel — architectural silence distilled into pure form." },
    };

    static const int NUM_BOARDS = 8;
    std::vector<ExhibitionBoard> boards(NUM_BOARDS);
    for (int i = 0; i < NUM_BOARDS; ++i) {
        const BoardDef& d = defs[i];
        bool hasTex = (d.texIdx < (int)textureIds.size());
        GLuint tex  = hasTex ? textureIds[d.texIdx] : 0;
        boards[i].setup(d.pos, d.rotY, d.sc, tex, hasTex, d.label, d.desc);
    }
    return boards;
}

#endif // EXHIBITION_BOARD_H
