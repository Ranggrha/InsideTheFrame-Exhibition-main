#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Camera.h"
#include "Board.h" 
#include "Shader.h" 

Camera camera(glm::vec3(0.0f, 1.5f, 18.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

bool isFullscreen = false;
bool f11KeyPressed = false;
int windowPosX, windowPosY, windowWidth, windowHeight;

bool isCursorLocked = true;
bool tabKeyPressed = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!isCursorLocked) {
        firstMouse = true;
        return; 
    }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyPressed) {
            isCursorLocked = !isCursorLocked;
            if (isCursorLocked)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            tabKeyPressed = true;
        }
    } else {
        tabKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!f11KeyPressed) {
            if (!isFullscreen) {
                glfwGetWindowPos(window, &windowPosX, &windowPosY);
                glfwGetWindowSize(window, &windowWidth, &windowHeight);
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                isFullscreen = true;
            } else {
                glfwSetWindowMonitor(window, NULL, windowPosX, windowPosY, windowWidth, windowHeight, 0);
                isFullscreen = false;
            }
            f11KeyPressed = true;
        }
    } else {
        f11KeyPressed = false;
    }

    if (isCursorLocked) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

// ================= SHADER UNTUK WARNA SOLID (PAPAN FOTO) =================
const char* colorVertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 ourColor;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "}\n";

const char* colorFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 ourColor;\n"
    "void main() {\n"
    "   FragColor = vec4(ourColor, 1.0f);\n"
    "}\n";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "InsideTheFrame - Virtual Exhibition", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return -1; }
    glEnable(GL_DEPTH_TEST);

    // ================= LOAD SHADER RUANGAN DARI FILE =================
    Shader roomShader("../assets/shaders/ruangan.vs", "../assets/shaders/ruangan.fs");

    // ================= COMPILE SHADER PAPAN (TETAP INLINE) =================
    unsigned int cVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(cVS, 1, &colorVertexShaderSource, NULL); glCompileShader(cVS);
    unsigned int cFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(cFS, 1, &colorFragmentShaderSource, NULL); glCompileShader(cFS);
    unsigned int colorShader = glCreateProgram();
    glAttachShader(colorShader, cVS); glAttachShader(colorShader, cFS); glLinkProgram(colorShader);
    glDeleteShader(cVS); glDeleteShader(cFS);

    float roomVertices[] = {
        // LANTAI (6 Titik)
        -10.0f, 0.0f, -20.0f,   0.0f, 10.0f,
         10.0f, 0.0f, -20.0f,  10.0f, 10.0f,
         10.0f, 0.0f,  20.0f,  10.0f,  0.0f,
         10.0f, 0.0f,  20.0f,  10.0f,  0.0f,
        -10.0f, 0.0f,  20.0f,   0.0f,  0.0f,
        -10.0f, 0.0f, -20.0f,   0.0f, 10.0f,

        // TEMBOK KIRI & KANAN (12 Titik)
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

        // TEMBOK BELAKANG U TERBALIK (18 Titik)
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

        // TEMBOK DEPAN (18 Titik)
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

        // SEKAT VERTIKAL ATAP (12 Titik)
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

        // ATAP (18 Titik)
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

    Board exhibitionBoard;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        if (currentHeight == 0) currentHeight = 1; 
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)currentWidth / (float)currentHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 roomModel = glm::mat4(1.0f);

        // ================= RENDER RUANGAN =================
        roomShader.use();
        roomShader.setMat4("projection", projection);
        roomShader.setMat4("view",       view);
        roomShader.setMat4("model",      roomModel);
        roomShader.setVec3("viewPos",    camera.Position);

        glBindVertexArray(roomVAO);

        roomShader.setInt("surfaceType", 0); // Lantai
        glDrawArrays(GL_TRIANGLES, 0, 6);

        roomShader.setInt("surfaceType", 1); // Tembok
        glDrawArrays(GL_TRIANGLES, 6, 60);

        roomShader.setInt("surfaceType", 2); // Atap
        glDrawArrays(GL_TRIANGLES, 66, 18);

        // ================= RENDER PAPAN (SATU, PUTIH, TIPIS) =================
        glUseProgram(colorShader);
        glUniformMatrix4fv(glGetUniformLocation(colorShader, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(colorShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        exhibitionBoard.Draw(colorShader, glm::vec3(0.0f, 1.8f, 0.0f), glm::vec3(0.15f, 3.6f, 28.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &roomVAO);
    glDeleteBuffers(1, &roomVBO);
    glDeleteProgram(colorShader);
    glfwTerminate();
    return 0;
}