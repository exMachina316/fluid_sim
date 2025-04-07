# Fluid Simulation Algorithm Expermentations

This branch has all the algorithm experimentation for fluid simulations. They all use pygame for rendering and user interactions which admitedly is not very performant and hence the need for an OpenGL and CPP based implementation on the main branch.

Some use grid-based (Eulerian methods) and other use a lagrangian scheme, usually not very effecitvely.

The lastest experiment was with eulerian_sim_2.py which draws inspiration from the work of Minute Physics' Eulerian Sim and is the best and actually working simulation so far. 

## NOTE OF WISDOM
I have also used a fair bit of GenAI for this branch to expedite the process of development, but it came up short and thus the best one is implemented by reading papers and watching youtube videos to understand the famous Navier Stokes Equations. Once this was done, the debugging and handling of boundry conditions was handled by AI assistants.