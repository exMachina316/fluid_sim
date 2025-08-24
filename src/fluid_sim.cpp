#include "fluid_sim.h"
#include <iostream>

FluidSim::FluidSim() : width(GRID_SIZE_X), height(GRID_SIZE_Y)
{
    // Initialize the grid
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            density[i][j] = 0.0f;
            velocityX[i][j] = 0.0f;
            velocityY[i][j] = 0.0f;
            prevDensity[i][j] = 0.0f;
            prevVelocityX[i][j] = 0.0f;
            prevVelocityY[i][j] = 0.0f;
            tempField1[i][j] = 0.0f;
            tempField2[i][j] = 0.0f;
        }
    }
}

void FluidSim::step(float dt)
{
    // Perform velocity and density steps
    velocityStep(dt);
    densityStep(dt);
}

// Add source terms to the density/velocity fields
void FluidSim::addSource(float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], float dt)
{
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            dest[i][j] += dt * source[i][j];
        }
    }
}

// Diffuse the field using Gauss-Seidel relaxation
void FluidSim::diffuse(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], float diff, float dt)
{
    float a = dt * diff * width * height;
    float cRecip = 1.0f / (1 + 4 * a);
    float omega = 1.5f; // Relaxation parameter for SOR

    // Successive Over-Relaxation
    for (int k = 0; k < 5; k++)
    { // 20 iterations for stability
        for (int i = 1; i < width - 1; i++)
        {
            for (int j = 1; j < height - 1; j++)
            {
                float newValue = (source[i][j] + a * (dest[i + 1][j] + dest[i - 1][j] + dest[i][j + 1] + dest[i][j - 1])) * cRecip;
                dest[i][j] = dest[i][j] + omega * (newValue - dest[i][j]);
            }
        }
        setBoundary(b, dest);
    }
}

// Semi-Lagrangian advection (original method)
void FluidSim::semiLagrangianAdvect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y],
                      const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt)
{
    float dt0 = dt * width;

    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            // Trace particle position backward
            float x = i - dt0 * u[i][j];
            float y = j - dt0 * v[i][j];

            // Clamp to grid bounds
            x = std::max(0.5f, std::min(width - 1.5f, x));
            y = std::max(0.5f, std::min(height - 1.5f, y));

            // Find grid cell indices
            int i0 = static_cast<int>(x);
            int i1 = i0 + 1;
            int j0 = static_cast<int>(y);
            int j1 = j0 + 1;

            // Bilinear interpolation weights
            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;

            // Bilinear interpolation
            dest[i][j] = s0 * (t0 * source[i0][j0] + t1 * source[i0][j1]) +
                         s1 * (t0 * source[i1][j0] + t1 * source[i1][j1]);
        }
    }
    setBoundary(b, dest);
}

// MacCormack advection method - more accurate, reduces numerical diffusion
void FluidSim::macCormackAdvect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y],
                                const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt)
{
    float dt0 = dt * width;
    
    // Step 1: Forward advection (predictor step)
    // Use semi-Lagrangian method to advect forward
    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            // Trace particle position backward
            float x = i - dt0 * u[i][j];
            float y = j - dt0 * v[i][j];

            // Clamp to grid bounds
            x = std::max(0.5f, std::min(width - 1.5f, x));
            y = std::max(0.5f, std::min(height - 1.5f, y));

            // Find grid cell indices
            int i0 = static_cast<int>(x);
            int i1 = i0 + 1;
            int j0 = static_cast<int>(y);
            int j1 = j0 + 1;

            // Bilinear interpolation weights
            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;

            // Bilinear interpolation - store in tempField1
            tempField1[i][j] = s0 * (t0 * source[i0][j0] + t1 * source[i0][j1]) +
                               s1 * (t0 * source[i1][j0] + t1 * source[i1][j1]);
        }
    }
    setBoundary(b, tempField1);
    
    // Step 2: Backward advection (corrector step)
    // Advect the result from step 1 backward in time
    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            // Trace particle position forward (opposite direction)
            float x = i + dt0 * u[i][j];
            float y = j + dt0 * v[i][j];

            // Clamp to grid bounds
            x = std::max(0.5f, std::min(width - 1.5f, x));
            y = std::max(0.5f, std::min(height - 1.5f, y));

            // Find grid cell indices
            int i0 = static_cast<int>(x);
            int i1 = i0 + 1;
            int j0 = static_cast<int>(y);
            int j1 = j0 + 1;

            // Bilinear interpolation weights
            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;

            // Bilinear interpolation - store in tempField2
            tempField2[i][j] = s0 * (t0 * tempField1[i0][j0] + t1 * tempField1[i0][j1]) +
                               s1 * (t0 * tempField1[i1][j0] + t1 * tempField1[i1][j1]);
        }
    }
    setBoundary(b, tempField2);
    
    // Step 3: Calculate error and apply correction
    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            // Calculate the error between original and round-trip advection
            float error = source[i][j] - tempField2[i][j];
            
            // Apply MacCormack correction: result = forward_advection + 0.5 * error
            dest[i][j] = tempField1[i][j] + 0.5f * error;
            
            // Optional: clamp to prevent overshoots (helps with stability)
            // Find min/max in the neighborhood for clamping
            float minVal = source[i][j];
            float maxVal = source[i][j];
            
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    int ni = i + di;
                    int nj = j + dj;
                    if (ni >= 0 && ni < width && nj >= 0 && nj < height) {
                        minVal = std::min(minVal, source[ni][nj]);
                        maxVal = std::max(maxVal, source[ni][nj]);
                    }
                }
            }
            
            // Clamp the result to prevent overshoots
            dest[i][j] = std::max(minVal, std::min(maxVal, dest[i][j]));
        }
    }
    setBoundary(b, dest);
}

// Main advection method - can switch between different advection schemes
void FluidSim::advect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y],
                      const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt)
{
    // Available advection methods (uncomment desired method):
    // 
    // RK4 - Uses 4th order Runge-Kutta integration for particle tracing
    //       Highest accuracy, preserves fine details, but computationally expensive
    rk4Advect(b, dest, source, u, v, dt);                    
    
    // MacCormack - Two-step predictor-corrector method with error correction
    //              Good balance of accuracy and performance, reduces numerical diffusion
    // macCormackAdvect(b, dest, source, u, v, dt);          
    
    // Semi-Lagrangian - Basic backward particle tracing with bilinear interpolation
    //                   Fastest but most diffusive, good for real-time applications
    // semiLagrangianAdvect(b, dest, source, u, v, dt);      
}

// Helper method for bilinear interpolation
float FluidSim::bilinearInterpolate(const float field[GRID_SIZE_X][GRID_SIZE_Y], float x, float y) const
{
    // Clamp to grid bounds
    x = std::max(0.5f, std::min(width - 1.5f, x));
    y = std::max(0.5f, std::min(height - 1.5f, y));

    // Find grid cell indices
    int i0 = static_cast<int>(x);
    int i1 = i0 + 1;
    int j0 = static_cast<int>(y);
    int j1 = j0 + 1;

    // Bilinear interpolation weights
    float s1 = x - i0;
    float s0 = 1 - s1;
    float t1 = y - j0;
    float t0 = 1 - t1;

    // Bilinear interpolation
    return s0 * (t0 * field[i0][j0] + t1 * field[i0][j1]) +
           s1 * (t0 * field[i1][j0] + t1 * field[i1][j1]);
}

// Helper method to get velocity at arbitrary position
glm::vec2 FluidSim::getVelocityAt(const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float x, float y) const
{
    return glm::vec2(bilinearInterpolate(u, x, y), bilinearInterpolate(v, x, y));
}

// RK4 advection method - highest accuracy, uses 4th order Runge-Kutta integration
void FluidSim::rk4Advect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y],
                         const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt)
{
    float dt0 = dt * width;

    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            float x = static_cast<float>(i);
            float y = static_cast<float>(j);
            
            // RK4 integration to find particle's original position
            // k1: initial velocity at current position
            glm::vec2 k1 = getVelocityAt(u, v, x, y) * (-dt0);
            
            // k2: velocity at midpoint using k1
            glm::vec2 pos2 = glm::vec2(x, y) + k1 * 0.5f;
            glm::vec2 k2 = getVelocityAt(u, v, pos2.x, pos2.y) * (-dt0);
            
            // k3: velocity at midpoint using k2
            glm::vec2 pos3 = glm::vec2(x, y) + k2 * 0.5f;
            glm::vec2 k3 = getVelocityAt(u, v, pos3.x, pos3.y) * (-dt0);
            
            // k4: velocity at endpoint using k3
            glm::vec2 pos4 = glm::vec2(x, y) + k3;
            glm::vec2 k4 = getVelocityAt(u, v, pos4.x, pos4.y) * (-dt0);
            
            // RK4 weighted average
            glm::vec2 displacement = (k1 + 2.0f * k2 + 2.0f * k3 + k4) / 6.0f;
            glm::vec2 sourcePos = glm::vec2(x, y) + displacement;
            
            // Interpolate the value at the source position
            dest[i][j] = bilinearInterpolate(source, sourcePos.x, sourcePos.y);
        }
    }
    setBoundary(b, dest);
}

// Project velocity field to be mass-conserving (divergence-free)
void FluidSim::project(float u[GRID_SIZE_X][GRID_SIZE_Y], float v[GRID_SIZE_X][GRID_SIZE_Y],
                       float p[GRID_SIZE_X][GRID_SIZE_Y], float div[GRID_SIZE_X][GRID_SIZE_Y])
{
    float h = 1.0f / width;

    // Calculate divergence
    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            div[i][j] = -0.5f * h * (u[i + 1][j] - u[i - 1][j] + v[i][j + 1] - v[i][j - 1]);
            p[i][j] = 0;
        }
    }
    setBoundary(0, div);
    setBoundary(0, p);

    // Solve Poisson equation
    for (int k = 0; k < 20; k++)
    {
        for (int i = 1; i < width - 1; i++)
        {
            for (int j = 1; j < height - 1; j++)
            {
                p[i][j] = (div[i][j] + p[i + 1][j] + p[i - 1][j] +
                           p[i][j + 1] + p[i][j - 1]) /
                          4;
            }
        }
        setBoundary(0, p);
    }

    // Apply pressure gradient to velocity
    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            u[i][j] -= 0.5f * (p[i + 1][j] - p[i - 1][j]) / h;
            v[i][j] -= 0.5f * (p[i][j + 1] - p[i][j - 1]) / h;
        }
    }
    setBoundary(1, u);
    setBoundary(2, v);
}

// Set boundary conditions
void FluidSim::setBoundary(int b, float x[GRID_SIZE_X][GRID_SIZE_Y])
{
    // Walls
    for (int i = 1; i < width - 1; i++)
    {
        x[i][0] = b == 2 ? -x[i][1] : x[i][1];
        x[i][height - 1] = b == 2 ? -x[i][height - 2] : x[i][height - 2];
    }

    for (int j = 1; j < height - 1; j++)
    {
        x[0][j] = b == 1 ? -x[1][j] : x[1][j];
        x[width - 1][j] = b == 1 ? -x[width - 2][j] : x[width - 2][j];
    }

    // Corners
    x[0][0] = 0.5f * (x[1][0] + x[0][1]);
    x[0][height - 1] = 0.5f * (x[1][height - 1] + x[0][height - 2]);
    x[width - 1][0] = 0.5f * (x[width - 2][0] + x[width - 1][1]);
    x[width - 1][height - 1] = 0.5f * (x[width - 2][height - 1] + x[width - 1][height - 2]);
}

// Update velocity field
void FluidSim::velocityStep(float dt)
{

    // Add minute random noise to velocity field
    for (int i = 0; i < width; ++i)
    {
        for (int j = 0; j < height; ++j)
        {
            float noiseX = ((float)rand() / RAND_MAX - 0.5f) * 1e-4f;
            float noiseY = ((float)rand() / RAND_MAX - 0.5f) * 1e-4f;
            velocityX[i][j] += noiseX;
            velocityY[i][j] += noiseY;
        }
    }

    // Save previous state
    std::copy(&velocityX[0][0], &velocityX[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevVelocityX[0][0]);
    std::copy(&velocityY[0][0], &velocityY[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevVelocityY[0][0]);

    // Diffuse velocity
    diffuse(1, velocityX, prevVelocityX, VISCOSITY, dt);
    diffuse(2, velocityY, prevVelocityY, VISCOSITY, dt);

    // Project to ensure mass conservation
    project(velocityX, velocityY, prevVelocityX, prevVelocityY);

    // Save state before advection
    std::copy(&velocityX[0][0], &velocityX[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevVelocityX[0][0]);
    std::copy(&velocityY[0][0], &velocityY[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevVelocityY[0][0]);

    // Advect velocity field
    advect(1, velocityX, prevVelocityX, prevVelocityX, prevVelocityY, dt);
    advect(2, velocityY, prevVelocityY, prevVelocityX, prevVelocityY, dt);

    // Project again
    project(velocityX, velocityY, prevVelocityX, prevVelocityY);
}

// Update density field
void FluidSim::densityStep(float dt)
{
    // Save previous state
    std::copy(&density[0][0], &density[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevDensity[0][0]);

    // Diffuse density
    diffuse(0, density, prevDensity, DIFFUSION, dt);

    // Save state before advection
    std::copy(&density[0][0], &density[0][0] + GRID_SIZE_X * GRID_SIZE_Y, &prevDensity[0][0]);

    // Advect density field
    advect(0, density, prevDensity, velocityX, velocityY, dt);
}

void FluidSim::addDensity(int x, int y, float amount)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        density[x][y] += amount;
    }
}

void FluidSim::addVelocity(int x, int y, float amountX, float amountY)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        velocityX[x][y] += amountX;
        velocityY[x][y] += amountY;
    }
}

float FluidSim::getDensity(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return density[x][y];
    }
    return 0.0f;
}

glm::vec2 FluidSim::getVelocity(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return glm::vec2(velocityX[x][y], velocityY[x][y]);
    }
    return glm::vec2(0.0f);
}

glm::vec2 FluidSim::getNormalizedVelocity(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        glm::vec2 vel = glm::vec2(velocityX[x][y], velocityY[x][y]);
        float magnitude = glm::length(vel);
        if (magnitude > 0.001f) // Avoid division by zero
        {
            return vel / magnitude;
        }
    }
    return glm::vec2(0.0f);
}

float FluidSim::getVelocityMagnitude(int x, int y) const
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        return glm::length(glm::vec2(velocityX[x][y], velocityY[x][y]));
    }
    return 0.0f;
}

glm::vec3 FluidSim::getVelocityColor(int x, int y) const
{
    float speed = getVelocityMagnitude(x, y);

    // Normalize speed for color mapping (adjust max_speed as needed)
    const float max_speed = 10.0f;
    float normalized_speed = std::min(speed / max_speed, 1.0f);

    // Create a color gradient from blue (slow) to red (fast)
    // Blue -> Cyan -> Green -> Yellow -> Red
    glm::vec3 color;
    if (normalized_speed < 0.25f)
    {
        // Blue to Cyan
        float t = normalized_speed / 0.25f;
        color = glm::vec3(0.0f, t, 1.0f);
    }
    else if (normalized_speed < 0.5f)
    {
        // Cyan to Green
        float t = (normalized_speed - 0.25f) / 0.25f;
        color = glm::vec3(0.0f, 1.0f, 1.0f - t);
    }
    else if (normalized_speed < 0.75f)
    {
        // Green to Yellow
        float t = (normalized_speed - 0.5f) / 0.25f;
        color = glm::vec3(t, 1.0f, 0.0f);
    }
    else
    {
        // Yellow to Red
        float t = (normalized_speed - 0.75f) / 0.25f;
        color = glm::vec3(1.0f, 1.0f - t, 0.0f);
    }

    return color;
}