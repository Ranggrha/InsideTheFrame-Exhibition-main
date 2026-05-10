#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

// =============================================================================
//  CollisionSystem.h — InsideTheFrame Virtual Exhibition
//  Part 1: Collision Detection System
//
//  Features:
//   • AABB broad-phase static colliders (walls, floor, ceiling, boards)
//   • Sphere-AABB narrow-phase resolution with wall-sliding response
//   • Ray-AABB intersection for E-key board interaction
//   • Fixed Y-lock so the player never leaves the floor
//
//  Performance targets (integrated GPU):
//   • resolvePlayer()  : < 0.5 ms  (≤ 20 static AABBs, simple math)
//   • raycastBoards()  : < 0.1 ms  (on key-press only, not every frame)
//
//  Usage (main.cpp):
//    CollisionSystem gCollision;
//    gCollision.addStatic({ glm::vec3(-10.5f,-0.1f,-20.5f),
//                           glm::vec3(-9.5f,  6.1f, 20.5f) });  // left wall
//    ...
//    camera.Position = gCollision.resolvePlayer(camera.Position, 0.4f, 1.5f);
// =============================================================================

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <limits>
#include <algorithm>
#include <string>
#include <iostream>

// -----------------------------------------------------------------------------
//  AABB — Axis-Aligned Bounding Box
// -----------------------------------------------------------------------------
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(glm::vec3 mn, glm::vec3 mx) : min(mn), max(mx) {}

    // Returns true if a point is inside (or on the surface of) this AABB
    bool contains(glm::vec3 p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }

    // Closest point on (or inside) the AABB to point p
    glm::vec3 closestPoint(glm::vec3 p) const {
        return glm::clamp(p, min, max);
    }

    // Center and half-extents
    glm::vec3 center()  const { return (min + max) * 0.5f; }
    glm::vec3 extents() const { return (max - min) * 0.5f; }
};

// -----------------------------------------------------------------------------
//  RaycastHit
// -----------------------------------------------------------------------------
struct RaycastHit {
    int   objectIndex = -1;   // index into the queried array, or -1 = no hit
    float distance    = std::numeric_limits<float>::max();
    glm::vec3 hitPoint { 0.0f };
};

// =============================================================================
//  CollisionSystem
// =============================================================================
class CollisionSystem {
public:
    // ── Static colliders (walls, floor, ceiling, partition, boards) ──────────
    std::vector<AABB> staticColliders;

    // ── Settings ─────────────────────────────────────────────────────────────
    float playerFloorY   = 1.5f;   // camera eye height above floor
    float playerRadius   = 0.4f;   // horizontal capsule radius
    bool  debugEnabled   = false;  // toggled by G key

    // ------------------------------------------------------------------
    //  addStatic — register an AABB that the player cannot enter
    // ------------------------------------------------------------------
    void addStatic(AABB box) {
        staticColliders.push_back(box);
    }

    // ------------------------------------------------------------------
    //  setupRoomColliders — pre-populate the exhibition room geometry
    //  Room bounds: X [-10, 10], Y [0, 6], Z [-20, 20]
    //  Partition:   thin box along X=0 the full Z length, Y [0,4]
    // ------------------------------------------------------------------
    void setupRoomColliders() {
        // ── Floor (thin slab so player can't fall through) ────────────
        addStatic({ glm::vec3(-10.5f, -0.5f, -20.5f),
                    glm::vec3( 10.5f,  0.05f,  20.5f) });

        // ── Ceiling ───────────────────────────────────────────────────
        addStatic({ glm::vec3(-10.5f,  5.9f, -20.5f),
                    glm::vec3( 10.5f,  7.0f,  20.5f) });

        // ── Left wall ─────────────────────────────────────────────────
        addStatic({ glm::vec3(-11.0f, -0.5f, -20.5f),
                    glm::vec3( -9.8f,  7.0f,  20.5f) });

        // ── Right wall ────────────────────────────────────────────────
        addStatic({ glm::vec3(  9.8f, -0.5f, -20.5f),
                    glm::vec3( 11.0f,  7.0f,  20.5f) });

        // ── Back wall (Z = -20) ───────────────────────────────────────
        addStatic({ glm::vec3(-10.5f, -0.5f, -21.0f),
                    glm::vec3( 10.5f,  7.0f, -19.8f) });

        // ── Front wall (Z = +20) ──────────────────────────────────────
        addStatic({ glm::vec3(-10.5f, -0.5f,  19.8f),
                    glm::vec3( 10.5f,  7.0f,  21.0f) });

        // ── Centre partition board (the main Board object) ────────────
        // Model: position=(0,1.8,0), scale=(0.15, 3.6, 28)
        // World extents: X[-0.075, 0.075], Y[0, 3.6], Z[-14, 14]
        addStatic({ glm::vec3(-0.3f,  0.0f, -14.0f),
                    glm::vec3( 0.3f,  3.7f,  14.0f) });

        std::cout << "[Collision] Room colliders registered: "
                  << staticColliders.size() << " AABBs\n";
    }

    // ------------------------------------------------------------------
    //  addBoardCollider — call for each ExhibitionBoard
    // ------------------------------------------------------------------
    void addBoardCollider(AABB box) {
        addStatic(box);
    }

    // ------------------------------------------------------------------
    //  resolvePlayer — push player position out of any overlapping AABB
    //
    //  Strategy:
    //    1. Lock Y to playerFloorY (no flying / falling)
    //    2. For each static collider, check sphere-AABB overlap
    //    3. If overlapping, compute penetration and push player out
    //    4. Wall sliding: only zero-out velocity along collision normal,
    //       allowing movement parallel to the wall
    //
    //  Call AFTER processing keyboard input, BEFORE using camera.Position.
    // ------------------------------------------------------------------
    glm::vec3 resolvePlayer(glm::vec3 desiredPos, float radius = 0.4f) {
        // Hard lock Y (no collision needed — just clamp)
        desiredPos.y = playerFloorY;

        // Iterate static colliders (broad phase = all of them, N < 25)
        for (const AABB& box : staticColliders) {
            // Quick AABB range check (broad phase reject)
            float pad = radius + 0.01f;
            if (desiredPos.x < box.min.x - pad || desiredPos.x > box.max.x + pad ||
                desiredPos.y < box.min.y - pad || desiredPos.y > box.max.y + pad ||
                desiredPos.z < box.min.z - pad || desiredPos.z > box.max.z + pad)
                continue;

            // Closest point on AABB to the player sphere centre
            glm::vec3 closest = box.closestPoint(desiredPos);
            glm::vec3 delta   = desiredPos - closest;
            float     dist2   = glm::dot(delta, delta);

            if (dist2 < radius * radius && dist2 > 1e-8f) {
                // Penetration depth and push-out direction
                float dist  = glm::sqrt(dist2);
                float depth = radius - dist;
                glm::vec3 pushDir = delta / dist;   // normalised away from AABB

                // Push player out of the wall
                desiredPos += pushDir * (depth + 0.002f);

                // Re-lock Y in case push introduced vertical drift
                desiredPos.y = playerFloorY;
            }
        }

        return desiredPos;
    }

    // ------------------------------------------------------------------
    //  raycastAABB — Slab method ray-AABB intersection
    //  Returns true + distance if ray hits box within [0, maxDist]
    // ------------------------------------------------------------------
    static bool raycastAABB(glm::vec3 origin, glm::vec3 dir,
                            const AABB& box,
                            float maxDist, float& outDist)
    {
        float tmin = 0.0f, tmax = maxDist;

        for (int axis = 0; axis < 3; ++axis) {
            float d = dir[axis];
            float o = origin[axis];
            float bmin = box.min[axis];
            float bmax = box.max[axis];

            if (glm::abs(d) < 1e-8f) {
                // Ray parallel to slab
                if (o < bmin || o > bmax) return false;
            } else {
                float t1 = (bmin - o) / d;
                float t2 = (bmax - o) / d;
                if (t1 > t2) std::swap(t1, t2);
                tmin = glm::max(tmin, t1);
                tmax = glm::min(tmax, t2);
                if (tmin > tmax) return false;
            }
        }

        outDist = tmin;
        return true;
    }

    // ------------------------------------------------------------------
    //  raycastBoards — cast a ray against a list of board AABBs
    //  Returns the index of the closest hit board, or -1 if none
    //  maxInteractDistance controls how far the player can "reach"
    // ------------------------------------------------------------------
    static int raycastBoards(glm::vec3 origin, glm::vec3 dir,
                             const std::vector<AABB>& boardBoxes,
                             float maxInteractDistance = 5.0f)
    {
        int   bestIdx  = -1;
        float bestDist = maxInteractDistance;

        for (int i = 0; i < (int)boardBoxes.size(); ++i) {
            float dist;
            if (raycastAABB(origin, dir, boardBoxes[i], bestDist, dist)) {
                bestIdx  = i;
                bestDist = dist;
            }
        }
        return bestIdx;
    }
};

#endif // COLLISION_SYSTEM_H
