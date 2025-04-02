#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "fluid_sim.h"
#include <vector>
#include <iostream>

FluidSim sim(1000);

// Gravity control
float gravityAngle = 270.0f; // Down by default (in degrees)
float gravityStrength = 9.8f; // Default gravity strength
const float ANGLE_INCREMENT = 5.0f; // Rotation speed in degrees

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Shader sources
unsigned int shaderProgram;
unsigned int VAO, VBO;

// Vertex Shader
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        gl_PointSize = 5.0;
    }
)";

// Fragment Shader
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color = vec4(0.0f, 0.5f, 1.0f, 1.0f); // Default blue color
    void main() {
        FragColor = color;
    }
)";

void setupShaders() {
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

void renderParticles() {
    // Get particles from simulation
    const auto& particles = sim.getParticles();

    // Create a vector of positions from particle data
    std::vector<float> positions;
    positions.reserve(particles.size() * 2); // x, y for each particle

    for (const auto& p : particles) {
        positions.push_back(p.position.x*2.0 - 1);
        positions.push_back(p.position.y*2.0 - 1);
    }

    // Bind buffers and update data
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);

    // Set vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Render particles
    glUseProgram(shaderProgram);
    glDrawArrays(GL_POINTS, 0, particles.size());

    // Unbind
    // glBindVertexArray(0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void renderGravityVector() {
    // Create a directional indicator for gravity
    float angleRad = gravityAngle * (3.14159f / 180.0f);
    float startX = 0.0f;
    float startY = 0.0f;
    float endX = 0.3f * std::cos(angleRad);
    float endY = 0.3f * std::sin(angleRad);
    
    float vertices[] = {
        startX, startY,
        endX, endY
    };
    
    // Use a separate VAO/VBO for the gravity vector
    unsigned int vectorVAO, vectorVBO;
    glGenVertexArrays(1, &vectorVAO);
    glGenBuffers(1, &vectorVBO);
    
    glBindVertexArray(vectorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vectorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Change color to red for the gravity vector
    glUseProgram(shaderProgram);
    int colorLocation = glGetUniformLocation(shaderProgram, "color");
    glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    
    // Draw the line
    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 2);
    
    // Reset color to default for particles
    glUniform4f(colorLocation, 0.0f, 0.5f, 1.0f, 1.0f);
    
    // Cleanup
    glDeleteVertexArrays(1, &vectorVAO);
    glDeleteBuffers(1, &vectorVBO);
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
    // Gravity direction controls
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        gravityAngle += ANGLE_INCREMENT;
        if(gravityAngle >= 360.0f) gravityAngle -= 360.0f;
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        gravityAngle -= ANGLE_INCREMENT;
        if(gravityAngle < 0.0f) gravityAngle += 360.0f;
    }
    
    // Gravity strength controls
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        gravityStrength += 0.1f;
        std::cout << "Gravity Strength: " << gravityStrength << std::endl;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        gravityStrength = std::max(0.0f, gravityStrength - 0.1f);
        std::cout << "Gravity Strength: " << gravityStrength << std::endl;
    }
    
    // Convert angle to radians and update simulation gravity
    float angleRad = gravityAngle * (3.14159f / 180.0f);
    float gravityX = gravityStrength * std::cos(angleRad);
    float gravityY = gravityStrength * std::sin(angleRad);
    sim.setGravity(glm::vec2(gravityX, gravityY));
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    // Set GLFW to use OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fluid Sim", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set the OpenGL context version
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLAD after creating the context
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the initial viewport size
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Set the clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Setup shaders and buffers
    setupShaders();
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Run for 20s

    while (!glfwWindowShouldClose(window)) {
        // Input
        processInput(window);

        // Redering code here
        glClear(GL_COLOR_BUFFER_BIT);

        sim.step(0.01f);
        renderParticles();
        renderGravityVector();

        // check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}