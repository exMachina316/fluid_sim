#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "fluid_sim.h"
#include <vector>
#include <iostream>

FluidSim sim;

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Shader sources
unsigned int densityShaderProgram, densityVAO, densityVBO;
unsigned int velocityShaderProgram, velocityVAO, velocityVBO;

// Visualization toggle
bool showVelocityVectors = false;

// Additional shader for density field visualization
const char *densityVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in float aDensity;
    out float density;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        density = aDensity;
    }
)";

const char *densityFragmentShaderSource = R"(
    #version 330 core
    in float density;
    out vec4 FragColor;
    void main() {
        // Colormap for density visualization (blue to red)
        vec3 color = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), density);
        FragColor = vec4(color, min(density * 2.0, 0.8));
    }
)";

// Velocity vector visualization shaders
const char *velocityVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 color;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        color = aColor;
    }
)";

const char *velocityFragmentShaderSource = R"(
    #version 330 core
    in vec3 color;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(color, 0.8);
    }
)";

void setupShaders()
{
    // Compile density field shaders
    unsigned int densityVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(densityVertexShader, 1, &densityVertexShaderSource, NULL);
    glCompileShader(densityVertexShader);

    // Check for shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(densityVertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(densityVertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::DENSITY_VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    unsigned int densityFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(densityFragmentShader, 1, &densityFragmentShaderSource, NULL);
    glCompileShader(densityFragmentShader);

    // Check for shader compilation errors
    glGetShaderiv(densityFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(densityFragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::DENSITY_FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link density shaders
    densityShaderProgram = glCreateProgram();
    glAttachShader(densityShaderProgram, densityVertexShader);
    glAttachShader(densityShaderProgram, densityFragmentShader);
    glLinkProgram(densityShaderProgram);

    // Check for linking errors
    glGetProgramiv(densityShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(densityShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::DENSITY_PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(densityVertexShader);
    glDeleteShader(densityFragmentShader);

    // Compile velocity vector shaders
    unsigned int velocityVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(velocityVertexShader, 1, &velocityVertexShaderSource, NULL);
    glCompileShader(velocityVertexShader);

    // Check for shader compilation errors
    glGetShaderiv(velocityVertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(velocityVertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VELOCITY_VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    unsigned int velocityFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(velocityFragmentShader, 1, &velocityFragmentShaderSource, NULL);
    glCompileShader(velocityFragmentShader);

    // Check for shader compilation errors
    glGetShaderiv(velocityFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(velocityFragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VELOCITY_FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link velocity shaders
    velocityShaderProgram = glCreateProgram();
    glAttachShader(velocityShaderProgram, velocityVertexShader);
    glAttachShader(velocityShaderProgram, velocityFragmentShader);
    glLinkProgram(velocityShaderProgram);

    // Check for linking errors
    glGetProgramiv(velocityShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(velocityShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VELOCITY_PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(velocityVertexShader);
    glDeleteShader(velocityFragmentShader);

    // Create density field buffers
    glGenVertexArrays(1, &densityVAO);
    glGenBuffers(1, &densityVBO);

    // Create velocity vector buffers
    glGenVertexArrays(1, &velocityVAO);
    glGenBuffers(1, &velocityVBO);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

// Mouse position tracking
double lastMouseX = 0.0, lastMouseY = 0.0;
bool mouseLeftPressed = false;
bool mouseRightPressed = false;
bool firstMouse = true;

// Mouse callback function
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }

    // Calculate mouse movement
    double deltaX = xpos - lastMouseX;
    double deltaY = ypos - lastMouseY;

    // Store current position for next frame
    lastMouseX = xpos;
    lastMouseY = ypos;

    // If mouse is pressed, apply force to fluid
    if (mouseLeftPressed)
    {
        // Convert screen coordinates to simulation space (-1,1)
        float normX = (2.0f * xpos / SCR_WIDTH) - 1.0f;
        float normY = 1.0f - (2.0f * ypos / SCR_HEIGHT);

        // Convert normalized coordinates to grid coordinates
        int gridX = static_cast<int>((normX + 1.0f) * 0.5f * GRID_SIZE_X);
        int gridY = static_cast<int>((normY + 1.0f) * 0.5f * GRID_SIZE_Y);

        // Add velocity in the direction of mouse movement
        float velocityScaleFactor = 10.0f;
        float dx = deltaX / SCR_WIDTH * velocityScaleFactor;
        float dy = -deltaY / SCR_HEIGHT * velocityScaleFactor; // Invert Y for screen coordinates

        // Apply velocity and density to a small area around the cursor
        for (int i = -3; i <= 3; i++)
        {
            for (int j = -3; j <= 3; j++)
            {
                int x = gridX + i;
                int y = gridY + j;
                if (x >= 0 && x < GRID_SIZE_X && y >= 0 && y < GRID_SIZE_Y)
                {
                    // Use the public methods we added
                    sim.addVelocity(x, y, dx, dy);
                }
            }
        }
    } else if (mouseRightPressed)
    {
        // Convert screen coordinates to simulation space (-1,1)
        float normX = (2.0f * xpos / SCR_WIDTH) - 1.0f;
        float normY = 1.0f - (2.0f * ypos / SCR_HEIGHT);

        // Convert normalized coordinates to grid coordinates
        int gridX = static_cast<int>((normX + 1.0f) * 0.5f * GRID_SIZE_X);
        int gridY = static_cast<int>((normY + 1.0f) * 0.5f * GRID_SIZE_Y);

        // Apply density to a small area around the cursor
        for (int i = -3; i <= 3; i++)
        {
            for (int j = -3; j <= 3; j++)
            {
                int x = gridX + i;
                int y = gridY + j;
                if (x >= 0 && x < GRID_SIZE_X && y >= 0 && y < GRID_SIZE_Y)
                {
                    // Use the public methods we added
                    sim.addDensity(x, y, 1.0f);
                }
            }
        }
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mouseLeftPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mouseLeftPressed = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS)
        {
            mouseRightPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mouseRightPressed = false;
        }
    }
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Reset simulation with R key
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        sim = FluidSim();
        std::cout << "Simulation reset" << std::endl;
    }

    // Toggle velocity vector visualization with V key
    static bool vKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !vKeyPressed)
    {
        showVelocityVectors = !showVelocityVectors;
        std::cout << "Velocity vectors: " << (showVelocityVectors ? "ON" : "OFF") << std::endl;
        vKeyPressed = true;
    }
    else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_RELEASE)
    {
        vKeyPressed = false;
    }
}

int main()
{
    if (!glfwInit())
    {
        return -1;
    }

    // Set GLFW to use OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fluid Sim", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set the OpenGL context version
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Initialize GLAD after creating the context
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the initial viewport size
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Set the clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Setup shaders and buffers
    setupShaders();

    // Add initial density and velocity for benchmarking
    int centerX = sim.getWidth() / 2;
    int centerY = sim.getHeight() / 2;
    for (int i = -5; i <= 5; i++)
    {
        for (int j = -5; j <= 5; j++)
        {
            if (i * i + j * j < 25)
            { // circular area
                sim.addDensity(centerX + i, centerY + j, 10.0f);
                sim.addVelocity(centerX + i, centerY + j, 0.0f, 2.0f);
            }
        }
    }

    // Run for 10s for benchmarking
    double startTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)/* && (glfwGetTime() - startTime < 10.0)*/)
    {
        // Input
        processInput(window);

        // Rendering code here
        glClear(GL_COLOR_BUFFER_BIT);

        // Enable blending for transparent fluid rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Step the simulation
        sim.step(0.01f);

        // Render the density field as a grid of quads
        int width = sim.getWidth();
        int height = sim.getHeight();
        std::vector<float> densityVertices;

        // Create a vertex for each density cell (with normalized coordinates)
        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                float x = (float)i / width * 2.0f - 1.0f;
                float y = (float)j / height * 2.0f - 1.0f;
                float cellWidth = 2.0f / width;
                float cellHeight = 2.0f / height;
                float density = sim.getDensity(i, j);

                // Skip cells with no density for efficiency
                if (density < 0.01f)
                    continue;

                // Create a quad for this cell
                // Triangle 1
                densityVertices.push_back(x);
                densityVertices.push_back(y);
                densityVertices.push_back(density);

                densityVertices.push_back(x + cellWidth);
                densityVertices.push_back(y);
                densityVertices.push_back(density);

                densityVertices.push_back(x);
                densityVertices.push_back(y + cellHeight);
                densityVertices.push_back(density);

                // Triangle 2
                densityVertices.push_back(x + cellWidth);
                densityVertices.push_back(y);
                densityVertices.push_back(density);

                densityVertices.push_back(x + cellWidth);
                densityVertices.push_back(y + cellHeight);
                densityVertices.push_back(density);

                densityVertices.push_back(x);
                densityVertices.push_back(y + cellHeight);
                densityVertices.push_back(density);
            }
        }

        // Render density field if we have any vertices
        if (!densityVertices.empty())
        {
            glBindVertexArray(densityVAO);
            glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
            glBufferData(GL_ARRAY_BUFFER, densityVertices.size() * sizeof(float), densityVertices.data(), GL_DYNAMIC_DRAW);

            // Position attribute
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // Density attribute
            glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // Draw the density field
            glUseProgram(densityShaderProgram);
            glDrawArrays(GL_TRIANGLES, 0, densityVertices.size() / 3);
        }

        // Render velocity vectors if enabled
        if (showVelocityVectors)
        {
            std::vector<float> velocityVertices;
            
            // Create velocity vector lines
            for (int i = 0; i < width; i += 2) // Sample every 2nd cell for better visibility
            {
                for (int j = 0; j < height; j += 2)
                {
                    glm::vec2 normalizedVel = sim.getNormalizedVelocity(i, j);
                    float magnitude = sim.getVelocityMagnitude(i, j);
                    
                    // Skip cells with very low velocity
                    if (magnitude < 0.1f)
                        continue;
                        
                    glm::vec3 color = sim.getVelocityColor(i, j);
                    
                    // Convert grid coordinates to normalized screen coordinates
                    float centerX = (float)i / width * 2.0f - 1.0f + (1.0f / width);
                    float centerY = (float)j / height * 2.0f - 1.0f + (1.0f / height);
                    
                    // Scale the vector for visualization
                    float scale = 0.05f; // Adjust this to make vectors longer/shorter
                    float endX = centerX + normalizedVel.x * scale;
                    float endY = centerY + normalizedVel.y * scale;
                    
                    // Line from center of cell to direction of velocity
                    velocityVertices.push_back(centerX);
                    velocityVertices.push_back(centerY);
                    velocityVertices.push_back(color.r);
                    velocityVertices.push_back(color.g);
                    velocityVertices.push_back(color.b);
                    
                    velocityVertices.push_back(endX);
                    velocityVertices.push_back(endY);
                    velocityVertices.push_back(color.r);
                    velocityVertices.push_back(color.g);
                    velocityVertices.push_back(color.b);
                }
            }
            
            if (!velocityVertices.empty())
            {
                // Upload velocity vector data
                glBindVertexArray(velocityVAO);
                glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
                glBufferData(GL_ARRAY_BUFFER, velocityVertices.size() * sizeof(float), velocityVertices.data(), GL_DYNAMIC_DRAW);
                
                // Position attribute
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
                glEnableVertexAttribArray(0);
                
                // Color attribute
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
                glEnableVertexAttribArray(1);
                
                // Draw the velocity vectors
                glUseProgram(velocityShaderProgram);
                glLineWidth(2.0f);
                glDrawArrays(GL_LINES, 0, velocityVertices.size() / 5);
            }
        }

        // check and call events and swap the buffers
        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &densityVAO);
    glDeleteBuffers(1, &densityVBO);
    glDeleteProgram(densityShaderProgram);
    
    glDeleteVertexArrays(1, &velocityVAO);
    glDeleteBuffers(1, &velocityVBO);
    glDeleteProgram(velocityShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}