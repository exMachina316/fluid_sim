#ifndef FLIP_SIM_H
#define FLIP_SIM_H

enum CellType { AIR_CELL = 0, FLUID_CELL = 1, SOLID_CELL = 2 };

class FlipSim {
   public:
    FlipSim(float density, int width, int height, int spacing,
            float particleRadius, int maxParticles);
    ~FlipSim();

    void integrateParticles(float dt, float gravity);
    void pushParticlesApart(int numIters);
    void handleParticleCollisions(float obstacleX, float obstacleY,
                                  float obstacleRadius);
    void updateParticleDensity();
    void transferVelocities(bool toGrid, float flipRatio);
    void solveIncrompressibility(int numIters, float dt, float overRelaxation,
                                 bool compensateDrift = true);
    void updateParticleColors();
    void setSciColor(int cellNr, float val, float minVal, float maxVal);
    void updateCellColors();
    void simulate(float dt, float gravity, float flipRatio,
                  int numPressureIters, int numParticleIters,
                  float overRelaxation, bool compensateDrift,
                  bool seperateParticles, float obstacleX, float obstacleY,
                  float obstacleRadius);

    float obstacleVelX;
    float obstacleVelY;

   private:
    float density_;
    float *u_;
    float *v_;
    float *du_;
    float *dv_;
    float *prevU_;
    float *prevV_;
    float *p_;
    float *s_;
    int *cellType_;
    float *cellColor_;

    int width_;
    int height_;
    int spacing_;
    int fNumX_;
    int fNumY_;
    int fNumCells_;

    float h_;
    float fInvSpacing_;

    float particleRadius_;
    int maxParticles_;
    float *particlePos_;
    float *particleColor_;
    float *particleVel_;
    float *particleDensity_;
    float particleRestDensity_;
    float pInvSpacing_;
    int pNumX_;
    int pNumY_;
    int pNumCells_;

    int *numCellParticles_;
    int *firstCellParticle_;
    int *cellParticleIds_;

    int numParticles_;
};

#endif  // FLIP_SIM_H