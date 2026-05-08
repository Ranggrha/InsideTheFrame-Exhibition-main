// =============================================================================
//  main.cpp — InsideTheFrame Virtual Exhibition
//  Advanced Lighting System Edition
//
//  Key bindings:
//    WASD        — Move camera
//    Mouse       — Look around
//    TAB         — Toggle cursor lock
//    F11         — Toggle fullscreen
//    L           — Cycle light falloff preset (Candlelight → Fluorescent → Daylight)
//    Q           — Lower lighting quality one step (debug)
//    ESC         — Quit
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

#include "Camera.h"
#include "Board.h"
#include "Shader.h"
#include "LightingManager.h"

// ─── Camera & window state ────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 1.5f, 18.0f));
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool  firstMouse = true;
float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

bool isFullscreen   = false;
bool f11KeyPressed  = false;
int  windowPosX, windowPosY, windowWidth, windowHeight;

bool isCursorLocked = true;
bool tabKeyPressed  = false;

// ─── Key-press state for debounced bindings ───────────────────────────────────
bool lKeyPressed = false;
bool qKeyPressed = false;

// ─── Lighting manager (global for callback access) ────────────────────────────
LightingManager gLighting;

// =============================================================================
//  Callbacks
// =============================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!isCursorLocked) { firstMouse = true; return; }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// =============================================================================
//  Input
// =============================================================================
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // TAB — toggle cursor
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyPressed) {
            isCursorLocked = !isCursorLocked;
            glfwSetInputMode(window, GLFW_CURSOR,
                isCursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            tabKeyPressed = true;
        }
    } else { tabKeyPressed = false; }

    // F11 — fullscreen toggle
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!f11KeyPressed) {
            if (!isFullscreen) {
                glfwGetWindowPos(window, &windowPosX, &windowPosY);
                glfwGetWindowSize(window, &windowWidth, &windowHeight);
                GLFWmonitor*      monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode   = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0,
                                     mode->width, mode->height, mode->refreshRate);
                isFullscreen = true;
            } else {
                glfwSetWindowMonitor(window, NULL, windowPosX, windowPosY,
                                     windowWidth, windowHeight, 0);
                isFullscreen = false;
            }
            f11KeyPressed = true;
        }
    } else { f11KeyPressed = false; }

    // L — cycle light falloff preset
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!lKeyPressed) { gLighting.cyclePreset(); lKeyPressed = true; }
    } else { lKeyPressed = false; }

    // Q — lower quality one step (debug)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        if (!qKeyPressed) { gLighting.cycleQualityDown(); qKeyPressed = true; }
    } else { qKeyPressed = false; }

    // WASD movement
    if (isCursorLocked) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    }
}

// =============================================================================
//  buildLightArray — define all exhibition lights, capped by quality
// =============================================================================
//
//  Light layout:
//    [0-6]  : 7 ceiling strip lights (Z = -18 … +18, step 6)  — white warm
//    [7]    : Left wall amber accent
//    [8]    : Right wall amber accent
//    [9]    : Cool blue rear accent
//  Only first numLights (per quality) are uploaded.
//
static const int TOTAL_LIGHTS = 10;

void buildLightArray(PointLight* lights, LightPreset preset) {
    // ── 7 ceiling strip lights ─────────────────────────────────────────────
    float stripZ[7] = { -18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f };
    for (int i = 0; i < 7; i++) {
        lights[i].position  = glm::vec3(0.0f, 5.85f, stripZ[i]);
        lights[i].color     = glm::vec3(1.0f, 0.97f, 0.90f);   // warm white
        lights[i].intensity = 1.6f;
    }

    // ── Left wall amber ───────────────────────────────────────────────────
    lights[7].position  = glm::vec3(-9.0f, 3.5f, 0.0f);
    lights[7].color     = glm::vec3(1.0f,  0.75f, 0.40f);
    lights[7].intensity = 1.0f;

    // ── Right wall amber ──────────────────────────────────────────────────
    lights[8].position  = glm::vec3( 9.0f, 3.5f, 0.0f);
    lights[8].color     = glm::vec3(1.0f,  0.75f, 0.40f);
    lights[8].intensity = 1.0f;

    // ── Rear cool accent ──────────────────────────────────────────────────
    lights[9].position  = glm::vec3(0.0f, 0.8f, -18.0f);
    lights[9].color     = glm::vec3(0.45f, 0.55f, 1.0f);
    lights[9].intensity = 0.6f;

    // Apply falloff preset to all lights
    applyFalloffToAll(lights, TOTAL_LIGHTS, preset);
}

// =============================================================================
//  uploadSharedUniforms — upload lights + shadow + time to a shader
// =============================================================================
void uploadSharedUniforms(const Shader& shader,
                          const PointLight* lights,
                          float currentTime,
                          const ShadowMap& shadowMap) {
    shader.setFloat("time", currentTime);
    shader.setInt("shadowsEnabled",  gLighting.shadowsOn ? 1 : 0);
    shader.setInt("lightingQuality", (int)gLighting.quality);
    shader.setInt("shadowMap",       1);   // texture unit 1

    // Upload light-space matrix
    shader.setMat4("lightSpaceMatrix", shadowMap.lightSpaceMatrix);

    // Upload active lights via LightingManager helper
    gLighting.uploadLights(shader.ID, lights, TOTAL_LIGHTS);
}

// =============================================================================
//  main
// =============================================================================
int main() {
    // ── GLFW initialisation ──────────────────────────────────────────────────
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "InsideTheFrame - Virtual Exhibition", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);   // VSync — keeps GPU from spinning at 2000 FPS

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    // Slight polygon offset on shadow pass (helps with acne on slope surfaces)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    // ── Detect GPU quality ───────────────────────────────────────────────────
    gLighting.detectQuality();
    gLighting.initShadows();

    // Shadow light source — primary ceiling strip at centre
    glm::vec3 shadowLightPos(0.0f, 5.85f, 0.0f);
    gLighting.shadowMap.updateLightMatrix(shadowLightPos);

    // ── Load shaders ─────────────────────────────────────────────────────────
    Shader roomShader(  "../assets/shaders/ruangan.vs", "../assets/shaders/ruangan.fs");
    Shader boardShader( "../assets/shaders/papan.vs",   "../assets/shaders/papan.fs");
    Shader shadowShader("../assets/shaders/shadow.vs",  "../assets/shaders/shadow.fs");

    // ── Build light array ────────────────────────────────────────────────────
    PointLight lights[TOTAL_LIGHTS];
    buildLightArray(lights, gLighting.preset);

    // ── Room geometry (VAO/VBO) ───────────────────────────────────────────────
    float roomVertices[] = {
        // LANTAI (6 titik)
        -10.0f, 0.0f, -20.0f,   0.0f, 10.0f,
         10.0f, 0.0f, -20.0f,  10.0f, 10.0f,
         10.0f, 0.0f,  20.0f,  10.0f,  0.0f,
         10.0f, 0.0f,  20.0f,  10.0f,  0.0f,
        -10.0f, 0.0f,  20.0f,   0.0f,  0.0f,
        -10.0f, 0.0f, -20.0f,   0.0f, 10.0f,

        // TEMBOK KIRI & KANAN (12 titik)
        -10.0f, 0.0f,  20.0f,   0.0f, 0.0f,
        -10.0f, 0.0f, -20.0f,  10.0f, 0.0f,
        -10.0f, 4.0f, -20.0f,  10.0f, 2.0f,
        -10.0f, 4.0f, -20.0f,  10.0f, 2.0f,
        -10.0f, 4.0f,  20.0f,   0.0f, 2.0f,
        -10.0f, 0.0f,  20.0f,   0.0f, 0.0f,
         10.0f, 0.0f, -20.0f,   0.0f, 0.0f,
         10.0f, 0.0f,  20.0f,  10.0f, 0.0f,
         10.0f, 4.0f,  20.0f,  10.0f, 2.0f,
         10.0f, 4.0f,  20.0f,  10.0f, 2.0f,
         10.0f, 4.0f, -20.0f,   0.0f, 2.0f,
         10.0f, 0.0f, -20.0f,   0.0f, 0.0f,

        // TEMBOK BELAKANG U TERBALIK (18 titik)
        -10.0f, 0.0f, -20.0f,   0.0f, 0.0f,
        -4.0f,  0.0f, -20.0f,   2.0f, 0.0f,
        -4.0f,  4.0f, -20.0f,   2.0f, 2.0f,
        -4.0f,  4.0f, -20.0f,   2.0f, 2.0f,
        -10.0f, 4.0f, -20.0f,   0.0f, 2.0f,
        -10.0f, 0.0f, -20.0f,   0.0f, 0.0f,
        -4.0f,  0.0f, -20.0f,   2.0f, 0.0f,
         4.0f,  0.0f, -20.0f,   6.0f, 0.0f,
         4.0f,  6.0f, -20.0f,   6.0f, 3.0f,
         4.0f,  6.0f, -20.0f,   6.0f, 3.0f,
        -4.0f,  6.0f, -20.0f,   2.0f, 3.0f,
        -4.0f,  0.0f, -20.0f,   2.0f, 0.0f,
         4.0f,  0.0f, -20.0f,   6.0f, 0.0f,
         10.0f, 0.0f, -20.0f,  10.0f, 0.0f,
         10.0f, 4.0f, -20.0f,  10.0f, 2.0f,
         10.0f, 4.0f, -20.0f,  10.0f, 2.0f,
         4.0f,  4.0f, -20.0f,   6.0f, 2.0f,
         4.0f,  0.0f, -20.0f,   6.0f, 0.0f,

        // TEMBOK DEPAN (18 titik)
        -10.0f, 0.0f,  20.0f,  10.0f, 0.0f,
        -4.0f,  0.0f,  20.0f,   6.0f, 0.0f,
        -4.0f,  4.0f,  20.0f,   6.0f, 2.0f,
        -4.0f,  4.0f,  20.0f,   6.0f, 2.0f,
        -10.0f, 4.0f,  20.0f,  10.0f, 2.0f,
        -10.0f, 0.0f,  20.0f,  10.0f, 0.0f,
         4.0f,  0.0f,  20.0f,   4.0f, 0.0f,
         10.0f, 0.0f,  20.0f,   0.0f, 0.0f,
         10.0f, 4.0f,  20.0f,   0.0f, 2.0f,
         10.0f, 4.0f,  20.0f,   0.0f, 2.0f,
         4.0f,  4.0f,  20.0f,   4.0f, 2.0f,
         4.0f,  0.0f,  20.0f,   4.0f, 0.0f,
        -4.0f,  3.0f,  20.0f,   6.0f, 1.5f,
         4.0f,  3.0f,  20.0f,   4.0f, 1.5f,
         4.0f,  6.0f,  20.0f,   4.0f, 3.0f,
         4.0f,  6.0f,  20.0f,   4.0f, 3.0f,
        -4.0f,  6.0f,  20.0f,   6.0f, 3.0f,
        -4.0f,  3.0f,  20.0f,   6.0f, 1.5f,

        // SEKAT VERTIKAL ATAP (12 titik)
        -4.0f,  4.0f, -20.0f,   0.0f, 0.0f,
        -4.0f,  4.0f,  20.0f,  10.0f, 0.0f,
        -4.0f,  6.0f,  20.0f,  10.0f, 1.0f,
        -4.0f,  6.0f,  20.0f,  10.0f, 1.0f,
        -4.0f,  6.0f, -20.0f,   0.0f, 1.0f,
        -4.0f,  4.0f, -20.0f,   0.0f, 0.0f,
         4.0f,  4.0f, -20.0f,  10.0f, 0.0f,
         4.0f,  4.0f,  20.0f,   0.0f, 0.0f,
         4.0f,  6.0f,  20.0f,   0.0f, 1.0f,
         4.0f,  6.0f,  20.0f,   0.0f, 1.0f,
         4.0f,  6.0f, -20.0f,  10.0f, 1.0f,
         4.0f,  4.0f, -20.0f,  10.0f, 0.0f,

        // ATAP (18 titik)
        -10.0f, 4.0f, -20.0f,   0.0f, 10.0f,
        -4.0f,  4.0f, -20.0f,   2.0f, 10.0f,
        -4.0f,  4.0f,  20.0f,   2.0f,  0.0f,
        -4.0f,  4.0f,  20.0f,   2.0f,  0.0f,
        -10.0f, 4.0f,  20.0f,   0.0f,  0.0f,
        -10.0f, 4.0f, -20.0f,   0.0f, 10.0f,
         4.0f,  4.0f, -20.0f,   2.0f, 10.0f,
         10.0f, 4.0f, -20.0f,   0.0f, 10.0f,
         10.0f, 4.0f,  20.0f,   0.0f,  0.0f,
         10.0f, 4.0f,  20.0f,   0.0f,  0.0f,
         4.0f,  4.0f,  20.0f,   2.0f,  0.0f,
         4.0f,  4.0f, -20.0f,   2.0f, 10.0f,
        -4.0f,  6.0f, -20.0f,   0.0f, 10.0f,
         4.0f,  6.0f, -20.0f,   4.0f, 10.0f,
         4.0f,  6.0f,  20.0f,   4.0f,  0.0f,
         4.0f,  6.0f,  20.0f,   4.0f,  0.0f,
        -4.0f,  6.0f,  20.0f,   0.0f,  0.0f,
        -4.0f,  6.0f, -20.0f,   0.0f, 10.0f,
    };

    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glGenBuffers(1, &roomVBO);
    glBindVertexArray(roomVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roomVertices), roomVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    Board exhibitionBoard;

    // ── FPS diagnostic ────────────────────────────────────────────────────────
    float  fpsPrintTimer = 0.0f;
    int    frameCount    = 0;

    std::cout << "\n[InsideTheFrame] Controls:\n"
              << "  WASD + Mouse : Navigate\n"
              << "  L            : Cycle light preset (Candlelight / Fluorescent / Daylight)\n"
              << "  Q            : Lower lighting quality (debug)\n"
              << "  TAB          : Toggle cursor\n"
              << "  F11          : Fullscreen\n"
              << "  ESC          : Quit\n\n";

    // ==========================================================================
    //  RENDER LOOP
    // ==========================================================================
    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        deltaTime  = currentTime - lastFrame;
        lastFrame  = currentTime;

        fpsPrintTimer += deltaTime;
        frameCount++;
        if (fpsPrintTimer >= 5.0f) {
            float fps = frameCount / fpsPrintTimer;
            std::cout << "[FPS] " << fps
                      << "  quality=" << qualityName(gLighting.quality)
                      << "  preset=" << presetName(gLighting.preset) << "\n";
            fpsPrintTimer = 0.0f;
            frameCount    = 0;
        }

        processInput(window);

        // Rebuild lights if preset changed (cheap rebuild each frame is fine
        // since buildLightArray is trivial — no GL calls)
        buildLightArray(lights, gLighting.preset);

        // ── Matrices ──────────────────────────────────────────────────────────
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1;

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)currentWidth / (float)currentHeight,
            0.1f, 100.0f);
        glm::mat4 view      = camera.GetViewMatrix();
        glm::mat4 roomModel = glm::mat4(1.0f);
        glm::mat4 boardModel;
        {
            boardModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.8f, 0.0f));
            boardModel = glm::scale(boardModel, glm::vec3(0.15f, 3.6f, 28.0f));
        }

        // ══════════════════════════════════════════════════════════════════════
        //  PASS 1 — Shadow map (depth-only, if shadows enabled)
        // ══════════════════════════════════════════════════════════════════════
        if (gLighting.shadowsOn) {
            gLighting.shadowMap.bindForWriting();
            glDisable(GL_POLYGON_OFFSET_FILL);   // no offset on shadow pass

            shadowShader.use();
            shadowShader.setMat4("lightSpaceMatrix", gLighting.shadowMap.lightSpaceMatrix);

            // Render room geometry
            shadowShader.setMat4("model", roomModel);
            glBindVertexArray(roomVAO);
            glDrawArrays(GL_TRIANGLES, 0, 84);   // floor+walls (66) + ceiling(18) = 84... total
            glBindVertexArray(0);

            // Render board (uses its own VAO via Board class)
            shadowShader.setMat4("model", boardModel);
            glBindVertexArray(exhibitionBoard.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            // Restore
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, currentWidth, currentHeight);
            glEnable(GL_POLYGON_OFFSET_FILL);
        }

        // ══════════════════════════════════════════════════════════════════════
        //  PASS 2 — Main render
        // ══════════════════════════════════════════════════════════════════════
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind shadow depth texture to unit 1
        gLighting.shadowMap.bindDepthTexture(1);

        // ── Render room surfaces ──────────────────────────────────────────────
        roomShader.use();
        roomShader.setMat4("projection",      projection);
        roomShader.setMat4("view",            view);
        roomShader.setMat4("model",           roomModel);
        roomShader.setVec3("viewPos",         camera.Position);
        uploadSharedUniforms(roomShader, lights, currentTime, gLighting.shadowMap);

        glBindVertexArray(roomVAO);

        roomShader.setInt("surfaceType", 0);   // Lantai
        glDrawArrays(GL_TRIANGLES, 0, 6);

        roomShader.setInt("surfaceType", 1);   // Tembok (walls + sekat)
        glDrawArrays(GL_TRIANGLES, 6, 60);

        roomShader.setInt("surfaceType", 2);   // Atap
        glDrawArrays(GL_TRIANGLES, 66, 18);

        glBindVertexArray(0);

        // ── Render exhibition board ───────────────────────────────────────────
        boardShader.use();
        boardShader.setMat4("projection",      projection);
        boardShader.setMat4("view",            view);
        boardShader.setMat4("model",           boardModel);
        boardShader.setVec3("viewPos",         camera.Position);
        uploadSharedUniforms(boardShader, lights, currentTime, gLighting.shadowMap);

        glBindVertexArray(exhibitionBoard.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &roomVAO);
    glDeleteBuffers(1, &roomVBO);
    gLighting.shadowMap.cleanup();
    glfwTerminate();
    return 0;
}