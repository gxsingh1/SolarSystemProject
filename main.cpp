#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include "Shader.h"
#include "Sphere.h"
#include "stb_image.h"

// Settings
const unsigned int SCR_WIDTH = 1900;
const unsigned int SCR_HEIGHT = 1080;

// Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float fov = 70.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Struct for Planets
struct Planet {
    Sphere sphere;
    unsigned int textureID;
    float orbitRadius;
    float orbitSpeed;
    float size;
};

// Load a texture
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Failed to load texture at path: " << path << std::endl;
    }
    stbi_image_free(data);

    return textureID;
}

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= static_cast<float>(yoffset);
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

void processInput(GLFWwindow* window)
{
    float cameraSpeed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Build shader
    Shader planetShader("simpleVS.vs", "simpleFS.fs");

    // Create Sun
    Sphere sunSphere(1.0f, 50, 50);
    unsigned int sunTexture = loadTexture("resources/2k_sun.jpg");

    // Planets
    std::vector<Planet> planets;

    planets.push_back({ Sphere(0.2f, 50, 50), loadTexture("resources/2k_mercury.jpg"), 2.0f, 365.25f / 88.0f, 0.2f }); // Mercury
    planets.push_back({ Sphere(0.35f, 50, 50), loadTexture("resources/2k_venus.jpg"), 2.8f, 365.25f / 225.0f, 0.35f }); // Venus
    planets.push_back({ Sphere(0.5f, 50, 50), loadTexture("resources/earth2k.jpg"), 4.0f, 1.0f, 0.5f }); // Earth
    planets.push_back({ Sphere(0.4f, 50, 50), loadTexture("resources/2k_mars.jpg"), 5.5f, 365.25f / 687.0f, 0.4f }); // Mars
    planets.push_back({ Sphere(0.8f, 50, 50), loadTexture("resources/2k_jupiter.jpg"), 7.5f, 365.25f / 4332.0f, 0.8f }); // Jupiter
    planets.push_back({ Sphere(0.7f, 50, 50), loadTexture("resources/2k_saturn.jpg"), 9.0f, 365.25f / 10759.0f, 0.7f }); // Saturn
    planets.push_back({ Sphere(0.5f, 50, 50), loadTexture("resources/2k_uranus.jpg"), 10.5f, 365.25f / 30688.0f, 0.5f }); // Uranus
    planets.push_back({ Sphere(0.5f, 50, 50), loadTexture("resources/2k_neptune.jpg"), 12.0f, 365.25f / 60190.0f, 0.5f }); // Neptune

    // Moon (special)
    Sphere moonSphere(0.15f, 50, 50);
    unsigned int moonTexture = loadTexture("resources/2k_moon.jpg");

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        planetShader.Use();

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        planetShader.setMat4("view", view);

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        planetShader.setMat4("projection", projection);

        float time = glfwGetTime();

        // Draw Sun
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glm::mat4 model = glm::mat4(1.0f);
        planetShader.setMat4("model", model);
        sunSphere.Draw();

        // Draw Planets
        for (size_t i = 0; i < planets.size(); ++i)
        {
            Planet& planet = planets[i];

            glBindTexture(GL_TEXTURE_2D, planet.textureID);

            glm::mat4 model = glm::mat4(1.0f);
            float x = planet.orbitRadius * cos(time * planet.orbitSpeed);
            float z = planet.orbitRadius * sin(time * planet.orbitSpeed);
            model = glm::translate(model, glm::vec3(x, 0.0f, z));
            model = glm::scale(model, glm::vec3(planet.size));
            model = glm::rotate(model, time * planet.orbitSpeed, glm::vec3(0.0f, 1.0f, 0.0f));

            planetShader.setMat4("model", model);
            planet.sphere.Draw();

            // Draw Moon orbiting Earth
            if (i == 2) // Earth index
            {
                glBindTexture(GL_TEXTURE_2D, moonTexture);

                glm::mat4 moonModel = glm::mat4(1.0f);
                moonModel = glm::translate(moonModel, glm::vec3(x, 0.0f, z)); // Move to Earth
                float moonOrbitRadius = 0.4f;
                float moonX = moonOrbitRadius * cos(time * (365.25f / 27.0f));
                float moonZ = moonOrbitRadius * sin(time * (365.25f / 27.0f));
                moonModel = glm::translate(moonModel, glm::vec3(moonX, 0.0f, moonZ));
                moonModel = glm::scale(moonModel, glm::vec3(0.15f));

                planetShader.setMat4("model", moonModel);
                moonSphere.Draw();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
