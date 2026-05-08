#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Enum untuk opsi arah pergerakan kamera
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Nilai default kamera
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  3.5f;
const float SENSITIVITY =  0.1f;

class Camera {
public:
    // Atribut Kamera
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Sudut Euler
    float Yaw;
    float Pitch;

    // Opsi Kamera
    float MovementSpeed;
    float MouseSensitivity;

    // Constructor: Dijalankan saat objek kamera pertama kali dibuat
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Mengembalikan matriks View menggunakan fungsi lookAt dari GLM
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Memproses input dari keyboard (WASD)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        
        // Memaksa pergerakan hanya di sumbu X dan Z (agar tidak bisa terbang saat melihat ke atas)
        glm::vec3 walkDirection = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));

        if (direction == FORWARD)
            Position += walkDirection * velocity;
        if (direction == BACKWARD)
            Position -= walkDirection * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // Memproses input dari pergerakan mouse
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Membatasi leher agar tidak bisa berputar 360 derajat ke belakang
        if (constrainPitch) {
            if (Pitch > 89.0f)  Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        // Perbarui vektor arah Front, Right, dan Up menggunakan matematika Euler
        updateCameraVectors();
    }

private:
    // Menghitung ulang vektor arah berdasarkan sudut Yaw dan Pitch yang baru
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        
        // Menghitung ulang vektor Kanan (Right) dan Atas (Up)
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif