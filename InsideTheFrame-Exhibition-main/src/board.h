#ifndef BOARD_H
#define BOARD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Board {
public:
    unsigned int VAO, VBO;

    Board() {
        float vertices[] = {
            // Sisi Depan
            -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            // Sisi Belakang
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            // Sisi Kiri
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            // Sisi Kanan
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            // Sisi Bawah
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
            // Sisi Atas
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void Draw(unsigned int shaderProgram, glm::vec3 position, glm::vec3 scale) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, scale);

        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
};

#endif