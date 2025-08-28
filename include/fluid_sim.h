#ifndef FLUID_SIM_H
#define FLUID_SIM_H

#include <vector>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>
#include <memory>

// Grid-based Eulerian fluid simulation parameters
const int GRID_SIZE_X = 200;
const int GRID_SIZE_Y = 200;
const float VISCOSITY = 0.0001f;
const float DIFFUSION = 0.0f;
const float PRESSURE = 0.5f;

class FluidSim
{
public:
    FluidSim();
    void step(float dt);

    // Methods for interacting with the fluid
    void addDensity(int x, int y, float amount);
    void addVelocity(int x, int y, float amountX, float amountY);

    // Methods to access grid data for rendering
    float getDensity(int x, int y) const;
    glm::vec2 getVelocity(int x, int y) const;

    // Velocity visualization methods
    glm::vec2 getNormalizedVelocity(int x, int y) const;
    float getVelocityMagnitude(int x, int y) const;
    glm::vec3 getVelocityColor(int x, int y) const; // Returns RGB color based on speed

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    // Grid properties
    int width, height;
    float density[GRID_SIZE_X][GRID_SIZE_Y];
    float velocityX[GRID_SIZE_X][GRID_SIZE_Y];
    float velocityY[GRID_SIZE_X][GRID_SIZE_Y];

    // Temporary arrays for simulation steps
    float prevDensity[GRID_SIZE_X][GRID_SIZE_Y];
    float prevVelocityX[GRID_SIZE_X][GRID_SIZE_Y];
    float prevVelocityY[GRID_SIZE_X][GRID_SIZE_Y];
    
    // Additional temporary arrays for MacCormack advection
    float tempField1[GRID_SIZE_X][GRID_SIZE_Y];
    float tempField2[GRID_SIZE_X][GRID_SIZE_Y];

    // Simulation methods
    void addSource(float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], float dt);
    void diffuse(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], float diff, float dt);
    
    // Main advection method - delegates to specific implementation
    void advect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt);
    
    // Specific advection implementations:
    void macCormackAdvect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt);    // Good balance of accuracy and performance
    void rk4Advect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt);           // Highest accuracy, slower
    void semiLagrangianAdvect(int b, float dest[GRID_SIZE_X][GRID_SIZE_Y], const float source[GRID_SIZE_X][GRID_SIZE_Y], const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float dt); // Fastest, most diffusive
    
    void project(float u[GRID_SIZE_X][GRID_SIZE_Y], float v[GRID_SIZE_X][GRID_SIZE_Y], float p[GRID_SIZE_X][GRID_SIZE_Y], float div[GRID_SIZE_X][GRID_SIZE_Y]);
    void setBoundary(int b, float x[GRID_SIZE_X][GRID_SIZE_Y]);
    
    // Helper methods
    float bilinearInterpolate(const float field[GRID_SIZE_X][GRID_SIZE_Y], float x, float y) const;
    glm::vec2 getVelocityAt(const float u[GRID_SIZE_X][GRID_SIZE_Y], const float v[GRID_SIZE_X][GRID_SIZE_Y], float x, float y) const;

    // Helper methods
    void velocityStep(float dt);
    void densityStep(float dt);
};

#endif // FLUID_SIM_H