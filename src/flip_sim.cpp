#include "flip_sim.h"

#include <algorithm>
#include <cmath>

using namespace std;

FlipSim::FlipSim(float density, int width, int height, int spacing,
                 float particleRadius, int maxParticles) {
    // Grid
    density_ = density;
    width_ = width;
    height_ = height;
    spacing_ = spacing;

    // Grid spec
    fNumX_ = int(floor(width / spacing) + 1);
    fNumY_ = int(floor(height / spacing) + 1);
    h_ = fmax(width / fNumX_, height / fNumY_);
    fInvSpacing_ = 1.0 / h_;
    fNumCells_ = fNumX_ * fNumY_;

    // Velocity fields
    u_ = new float[fNumCells_];
    v_ = new float[fNumCells_];
    du_ = new float[fNumCells_];
    dv_ = new float[fNumCells_];
    prevU_ = new float[fNumCells_];
    prevV_ = new float[fNumCells_];

    // Pressure and scalar fields
    p_ = new float[fNumCells_];
    s_ = new float[fNumCells_];
    cellType_ = new int[fNumCells_];         // 0: fluid, 1: solid, 2: air
    cellColor_ = new float[3 * fNumCells_];  // RGB color for each cell

    // Particles
    maxParticles_ = maxParticles;
    particlePos_ =
        new float[2 * maxParticles];  // x, y position for each particle
    particleColor_ =
        new float[3 * maxParticles];  // RGB color for each particle

    for (int i = 0; i < maxParticles; i++) {
        particleColor_[3 * i + 2] = 1.0;  // Make all particles blue
    }

    particleVel_ =
        new float[2 * maxParticles];  // x, y velocity for each particle
    particleDensity_ = new float[maxParticles];  // density for each particle
    particleRestDensity_ = 0.0;

    particleRadius_ = particleRadius;
    pInvSpacing_ = 1.0 / (2.2 * particleRadius_);
    pNumX_ = int(floor(width * pInvSpacing_) + 1);
    pNumY_ = int(floor(height * pInvSpacing_) + 1);
    pNumCells_ = pNumX_ * pNumY_;

    numCellParticles_ = new int[pNumCells_];
    firstCellParticle_ = new int[pNumCells_ + 1];
    cellParticleIds_ = new int[maxParticles];

    numParticles_ = 0;
}

// Just to be safe, free all allocated memory
FlipSim::~FlipSim() {
    delete[] u_;
    delete[] v_;
    delete[] du_;
    delete[] dv_;
    delete[] prevU_;
    delete[] prevV_;
    delete[] p_;
    delete[] s_;
    delete[] cellType_;
    delete[] cellColor_;
    delete[] particlePos_;
    delete[] particleColor_;
    delete[] particleVel_;
    delete[] particleDensity_;
    delete[] numCellParticles_;
    delete[] firstCellParticle_;
    delete[] cellParticleIds_;
}

void FlipSim::integrateParticles(float dt, float gravity) {
    for (int i = 0; i < numParticles_; i++) {
        particleVel_[2 * i + 1] += gravity * dt;
        particlePos_[2 * i] += particleVel_[2 * i] * dt;
        particlePos_[2 * i + 1] += particleVel_[2 * i + 1] * dt;
    }
}

void FlipSim::pushParticlesApart(int numIters) {
    float colorDiffusionCoeff = 0.001;

    // Count particles per cell
    for (int i = 0; i < pNumCells_; i++) {
        numCellParticles_[i] = 0;  // Reset count
    }

    for (int i = 0; i < numParticles_; i++) {
        float x = particlePos_[2 * i];
        float y = particlePos_[2 * i + 1];

        int xi = clamp(int(floor(x * pInvSpacing_)), 0, pNumX_ - 1);
        int yi = clamp(int(floor(y * pInvSpacing_)), 0, pNumY_ - 1);
        int cellNr = xi * pNumY_ + yi;
        numCellParticles_[cellNr]++;
    }

    // Partial Sums
    int first = 0;

    for (int i = 0; i < pNumCells_; i++) {
        first += numCellParticles_[i];
        firstCellParticle_[i] = first;
    }
    firstCellParticle_[pNumCells_] = first;  // guard

    // Fill particles into cells
    for (int i = 0; i < numParticles_; i++) {
        float x = particlePos_[2 * i];
        float y = particlePos_[2 * i + 1];

        int xi = clamp(int(floor(x * pInvSpacing_)), 0, pNumX_ - 1);
        int yi = clamp(int(floor(y * pInvSpacing_)), 0, pNumY_ - 1);
        int cellNr = xi * pNumY_ + yi;
        firstCellParticle_[cellNr]--;
        cellParticleIds_[firstCellParticle_[cellNr]] = i;
    }

    // Push particles apart
    float minDist = 2.0 * particleRadius_;
    float minDist2 = minDist * minDist;

    for (int iter = 0; iter < numIters; iter++) {
        for (int i = 0; i < numParticles_; i++) {
            float px = particlePos_[2 * i];
            float py = particlePos_[2 * i + 1];

            int pxi = int(floor(px * pInvSpacing_));
            int pyi = int(floor(py * pInvSpacing_));
            int x0 = max(pxi - 1, 0);
            int y0 = max(pyi - 1, 0);
            int x1 = min(pxi + 1, pNumX_ - 1);
            int y1 = min(pyi + 1, pNumY_ - 1);

            for (int xi = x0; xi <= x1; xi++) {
                for (int yi = y0; yi <= y1; yi++) {
                    int cellNr = xi * pNumY_ + yi;
                    int first = firstCellParticle_[cellNr];
                    int last = firstCellParticle_[cellNr + 1];
                    for (int j = first; j < last; j++) {
                        int id = cellParticleIds_[j];
                        if (id == i) continue;  // Don't compare to yourself

                        float qx = particlePos_[2 * id];
                        float qy = particlePos_[2 * id + 1];

                        float dx = qx - px;
                        float dy = qy - py;
                        float dist2 = dx * dx + dy * dy;
                        if (dist2 > minDist2 || dist2 == 0.0) {
                            continue;  // Not too close or identical
                        }
                        float dist = sqrt(dist2);
                        float scale = 0.5 * (minDist - dist) / dist;
                        dx *= scale;
                        dy *= scale;
                        particlePos_[2 * i] -= dx;
                        particlePos_[2 * i + 1] -= dy;
                        particlePos_[2 * id] += dx;
                        particlePos_[2 * id + 1] += dy;

                        // Diffuse colors
                        for (int k = 0; k < 3; k++) {
                            float color0 = particleColor_[3 * i + k];
                            float color1 = particleColor_[3 * id + k];
                            float color = (color0 + color1) * 0.5;
                            particleColor_[3 * i + k] =
                                color0 + (color - color0) * colorDiffusionCoeff;
                            particleColor_[3 * id + k] =
                                color1 + (color - color1) * colorDiffusionCoeff;
                        }
                    }
                }
            }
        }
    }
}

void FlipSim::handleParticleCollisions(float obstacleX, float obstacleY,
                                       float obstacleRadius) {
    float minDist = obstacleRadius + particleRadius_;
    float mindDist2 = minDist * minDist;

    float minX = h_ + particleRadius_;
    float maxX = (fNumX_ - 1) * h_ - particleRadius_;
    float minY = h_ + particleRadius_;
    float maxY = (fNumY_ - 1) * h_ - particleRadius_;

    for (int i = 0; i < numParticles_; i++) {
        float x = particlePos_[2 * i];
        float y = particlePos_[2 * i + 1];

        float dx = x - obstacleX;
        float dy = y - obstacleY;
        float dist2 = dx * dx + dy * dy;

        // Obstacle collision
        if (dist2 < mindDist2) {
            particleVel_[2 * i] = obstacleVelX;
            particleVel_[2 * i + 1] = obstacleVelY;
        }

        // Wall collisions
        if (x < minX) {
            x = minX;
            particleVel_[2 * i] = 0.0;
        }

        if (x > maxX) {
            x = maxX;
            particleVel_[2 * i] = 0.0;
        }

        if (y < minY) {
            y = minY;
            particleVel_[2 * i + 1] = 0.0;
        }

        if (y > maxY) {
            y = maxY;
            particleVel_[2 * i + 1] = 0.0;
        }
    }
}

void FlipSim::updateParticleDensity() {
    float h2 = 0.5 * h_;

    // Copy density field
    float density[maxParticles_];
    copy(particleDensity_, particleDensity_ + maxParticles_, density);
    fill(density, density + maxParticles_, 0.0);

    for (int i = 0; i < numParticles_; i++) {
        float x = particlePos_[2 * i];
        float y = particlePos_[2 * i + 1];

        x = clamp(x, h_, (fNumX_ - 1) * h_);
        y = clamp(y, h_, (fNumY_ - 1) * h_);

        int x0 = int(floor((x - h2) * fInvSpacing_));
        float tx = ((x - h2) - x0 * h_) * fInvSpacing_;
        int x1 = min(x0 + 1, fNumX_ - 1);

        int y0 = int(floor((y - h2) * fInvSpacing_));
        float ty = ((y - h2) - y0 * h_) * fInvSpacing_;
        int y1 = min(y0 + 1, fNumY_ - 1);

        float sx = 1.0 - tx;
        float sy = 1.0 - ty;

        if (x0 < fNumX_ && y0 < fNumY_) density[x0 * fNumY_ + y0] += sx * sy;
        if (x1 < fNumX_ && y0 < fNumY_) density[x1 * fNumY_ + y0] += tx * sy;
        if (x1 < fNumX_ && y1 < fNumY_) density[x1 * fNumY_ + y1] += tx * ty;
        if (x0 < fNumX_ && y1 < fNumY_) density[x0 * fNumY_ + y1] += sx * ty;
    }

    if (particleRestDensity_ == 0.0) {
        float sum = 0.0f;
        int numFluidCells = 0;

        for (int i = 0; i < fNumCells_; i++) {
            if (cellType_[i] == 0) {  // Fluid cell
                sum += density[i];
                numFluidCells++;
            }
        }

        if (numFluidCells > 0) {
            particleRestDensity_ = sum / numFluidCells;
        }
    }
}

void FlipSim::transferVelocities(bool toGrid, float flipRatio) {
    float h2 = 0.5 * h_;

    if (toGrid) {
        copy(u_, u_ + fNumCells_, prevU_);
        copy(v_, v_ + fNumCells_, prevV_);

        fill(du_, du_ + fNumCells_, 0.0);
        fill(dv_, dv_ + fNumCells_, 0.0);
        fill(u_, u_ + fNumCells_, 0.0);
        fill(v_, v_ + fNumCells_, 0.0);

        for (int i = 0; i < fNumCells_; i++)
            cellType_[i] = s_[i] == 0.0 ? SOLID_CELL : AIR_CELL;

        for (int i = 0; i < numParticles_; i++) {
            float x = particlePos_[2 * i];
            float y = particlePos_[2 * i + 1];
            int xi = clamp(int(floor(x * fInvSpacing_)), 0, fNumX_ - 1);
            int yi = clamp(int(floor(y * fInvSpacing_)), 0, fNumY_ - 1);
            int cellNr = xi * fNumY_ + yi;
            if (cellType_[cellNr] == AIR_CELL) cellType_[cellNr] = FLUID_CELL;
        }
    }

    for (int component = 0; component < 2; component++) {
        float dx = component == 0 ? 0.0 : h2;
        float dy = component == 0 ? h2 : 0.0;

        float* f = component == 0 ? u_ : v_;
        float* prevF = component == 0 ? prevU_ : prevV_;
        float* d = component == 0 ? du_ : dv_;

        for (int i = 0; i < numParticles_; i++) {
            float y = particlePos_[2 * i + 1];
            float x = particlePos_[2 * i];

            x = clamp(x, h_, (fNumX_ - 1) * h_);
            y = clamp(y, h_, (fNumY_ - 1) * h_);

            int x0 = min(int(floor((x - dx)) * fInvSpacing_), fNumX_ - 2);
            float tx = ((x - dx) - x0 * h_) * fInvSpacing_;
            int x1 = min(x0 + 1, fNumX_ - 2);

            int y0 = min(int(floor((y - dy)) * fInvSpacing_), fNumY_ - 2);
            float ty = ((y - dy) - y0 * h_) * fInvSpacing_;
            int y1 = min(y0 + 1, fNumY_ - 2);

            float sx = 1.0 - tx;
            float sy = 1.0 - ty;

            float d0 = sx * sy;
            float d1 = tx * sy;
            float d2 = tx * ty;
            float d3 = sx * ty;

            int nr0 = x0 * fNumY_ + y0;
            int nr1 = x1 * fNumY_ + y0;
            int nr2 = x1 * fNumY_ + y1;
            int nr3 = x0 * fNumY_ + y1;

            if (toGrid) {
                float pv = particleVel_[2 * i + component];
                f[nr0] += pv * d0;
                d[nr0] += d0;
                f[nr1] += pv * d1;
                d[nr1] += d1;
                f[nr2] += pv * d2;
                d[nr2] += d2;
                f[nr3] += pv * d3;
                d[nr3] += d3;
            } else {
                int offset = component == 0 ? fNumY_ : 1;
                float valid0 = cellType_[nr0] != AIR_CELL ||
                                       cellType_[nr0 - offset] != AIR_CELL
                                   ? 1.0
                                   : 0.0;
                float valid1 = cellType_[nr1] != AIR_CELL ||
                                       cellType_[nr1 - offset] != AIR_CELL
                                   ? 1.0
                                   : 0.0;
                float valid2 = cellType_[nr2] != AIR_CELL ||
                                       cellType_[nr2 - offset] != AIR_CELL
                                   ? 1.0
                                   : 0.0;
                float valid3 = cellType_[nr3] != AIR_CELL ||
                                       cellType_[nr3 - offset] != AIR_CELL
                                   ? 1.0
                                   : 0.0;

                float v = particleVel_[2 * i + component];
                float d = valid0 * d0 + valid1 * d1 + valid2 * d2 + valid3 * d3;

                if (d > 0.0) {
                    float picV = (valid0 * d0 * f[nr0] + valid1 * d1 * f[nr1] +
                                  valid2 * d2 * f[nr2] + valid3 * d3 * f[nr3]) /
                                 d;
                    float corr = (valid0 * d0 * (f[nr0] - prevF[nr0]) +
                                  valid1 * d1 * (f[nr1] - prevF[nr1]) +
                                  valid2 * d2 * (f[nr2] - prevF[nr2]) +
                                  valid3 * d3 * (f[nr3] - prevF[nr3])) /
                                 d;
                    float flipV = v + corr;

                    particleVel_[2 * i + component] =
                        (1.0 - flipRatio) * picV + flipRatio * flipV;
                }
            }
        }

        if (toGrid) {
            int length = sizeof(f) / sizeof(float);
            for (int i = 0; i < length; i++) {
                if (d[i] > 0.0) f[i] /= d[i];
            }

            // restore solid cells

            for (int i = 0; i < fNumX_; i++) {
                for (int j = 0; j < fNumY_; j++) {
                    bool solid = cellType_[i * fNumY_ + j] == SOLID_CELL;
                    if (solid || (i > 0 && cellType_[(i - 1) * fNumY_ + j] ==
                                               SOLID_CELL))
                        u_[i * fNumY_ + j] = prevU_[i * fNumY_ + j];
                    if (solid ||
                        (j > 0 && cellType_[i * fNumY_ + j - 1] == SOLID_CELL))
                        v_[i * fNumY_ + j] = prevV_[i * fNumY_ + j];
                }
            }
        }
    }
}

void FlipSim::solveIncrompressibility(int numIters, float dt,
                                      float overRelaxation,
                                      bool compensateDrift) {
    fill(p_, p_ + fNumCells_, 0.0);
    copy(prevU_, prevU_ + fNumCells_, u_);
    copy(prevV_, prevV_ + fNumCells_, v_);

    int n = fNumY_;
    float cp = density_ * h_ / dt;

    for (int iter = 0; iter < numIters; iter++) {
        for (int i = 1; i < fNumX_ - 1; i++) {
            for (int j = 1; j < fNumY_ - 1; j++) {
                if (cellType_[i * n + j] != FLUID_CELL) continue;

                int center = i * n + j;
                int left = (i - 1) * n + j;
                int right = (i + 1) * n + j;
                int bottom = i * n + (j - 1);
                int top = i * n + (j + 1);

                float s = s_[center];
                float sx0 = s_[left];
                float sx1 = s_[right];
                float sy0 = s_[bottom];
                float sy1 = s_[top];
                s = sx0 + sx1 + sy0 + sy1;
                if (s == 0.0) continue;

                float div = (u_[right] - u_[center]) + (v_[top] - v_[center]);

                if (particleRestDensity_ > 0.0 && compensateDrift) {
                    float k = 1.0;
                    float compression =
                        particleDensity_[i * n + j] - particleRestDensity_;
                    if (compression > 0.0) {
                        div = div - k * compression;
                    }
                }

                float p = -div / s;
                p *= overRelaxation;

                p_[center] += cp * p;

                u_[center] -= p * sx0;
                u_[right] += p * sx0;
                v_[center] -= p * sy0;
                v_[top] += p * sy1;
            }
        }
    }
}

void FlipSim::updateParticleColors() {
    for (int i = 0; i < numParticles_; i++) {
        float s = 0.01f;

        particleColor_[3 * i] = clamp(particleColor_[3 * i] - s, 0.0f, 1.0f);
        particleColor_[3 * i + 1] =
            clamp(particleColor_[3 * i + 1] - s, 0.0f, 1.0f);
        particleColor_[3 * i + 2] =
            clamp(particleColor_[3 * i + 2] + s, 0.0f, 1.0f);

        float x = particlePos_[2 * i];
        float y = particlePos_[2 * i + 1];
        int xi = clamp(int(floor(x * fInvSpacing_)), 1, fNumX_ - 1);
        int yi = clamp(int(floor(y * fInvSpacing_)), 1, fNumY_ - 1);
        int cellNr = xi * fNumY_ + yi;

        if (particleRestDensity_ > 0.0) {
            float relDensity = particleDensity_[cellNr] / particleRestDensity_;
            if (relDensity < 0.7) {
                float s = 0.8;
                particleColor_[3 * i] = s;
                particleColor_[3 * i + 1] = s;
                particleColor_[3 * i + 2] = 1.0;
            }
        }
    }
}

void FlipSim::setSciColor(int cellNr, float val, float minVal, float maxVal) {
    val = min(max(val, minVal), maxVal - 0.0001f);
    float d = maxVal - minVal;
    val = d == 0.0 ? 0.5 : (val - minVal) / d;
    float m = 0.25;
    int num = int(floor(val / m));
    float s = (val - num * m) / m;
    float r, g, b;

    switch (num) {
        case 0:
            r = 0.0;
            g = s;
            b = 1.0;
            break;
        case 1:
            r = 0.0;
            g = 1.0;
            b = 1.0 - s;
            break;
        case 2:
            r = s;
            g = 1.0;
            b = 0.0;
            break;
        case 3:
            r = 1.0;
            g = 1.0 - s;
            b = 0.0;
            break;
    }
    cellColor_[3 * cellNr] = r;
    cellColor_[3 * cellNr + 1] = g;
    cellColor_[3 * cellNr + 2] = b;
}

void FlipSim::updateCellColors() {
    fill(cellColor_, cellColor_ + 3 * fNumCells_, 0.0);

    for (int i = 0; i < fNumCells_; i++) {
        if (cellType_[i] == SOLID_CELL) {
            cellColor_[3 * i] = 0.5;
            cellColor_[3 * i + 1] = 0.5;
            cellColor_[3 * i + 2] = 0.5;
        } else if (cellType_[i] == FLUID_CELL) {
            float d = particleDensity_[i];
            if (particleRestDensity_ > 0.0) d /= particleRestDensity_;
            setSciColor(i, d, 0.0, 2.0);
        }
    }
}

void FlipSim::simulate(float dt, float gravity, float flipRatio,
                       int numPressureIters, int numParticleIters,
                       float overRelaxation, bool compensateDrift,
                       bool seperateParticles, float obstacleX, float obstacleY,
                       float obstacleRadius) {
    int numSubSteps = 1;
    float sdt = dt / numSubSteps;

    for (int step = 0; step < numSubSteps; step++) {
        integrateParticles(sdt, gravity);
        if (seperateParticles) {
            pushParticlesApart(numParticleIters);
        }
        handleParticleCollisions(obstacleX, obstacleY, obstacleRadius);
        transferVelocities(true, flipRatio);
        updateParticleDensity();
        solveIncrompressibility(numPressureIters, sdt, overRelaxation,
                                compensateDrift);
        transferVelocities(false, flipRatio);
    }

    updateParticleColors();
    updateCellColors();
}