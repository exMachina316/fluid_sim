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

// Advect the field using semi-Lagrangian method
void FluidSim::advect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y],
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