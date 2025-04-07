'''
1. Profile with: 
`python -m cProfile -o program.prof eulerian_sim_2.py`
2. Visualize with:
`snakeviz program.prof`
'''

import pygame
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 7

# Window dimensions
GRID_WIDTH, GRID_HEIGHT = 100, 100
WINDOW_WIDTH, WINDOW_HEIGHT = GRID_WIDTH * SCALE, GRID_HEIGHT * SCALE

# Add space for the dial
DIAL_RADIUS = 50
WINDOW_HEIGHT += DIAL_RADIUS * 2 + 20  # Extra space for dial

# Set up the display
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Fluid Simulation with Gravity Control")

# Colors
BLACK = (0, 0, 0)
GREY = (128, 128, 128)
BLUE = (0, 0, 255)
WHITE = (255, 255, 255)
RED = (255, 0, 0)

# Simulation settings
FRAME_RATE = 120
GRAVITY = 10.0
OVERELAX_FACT = 1.9
NUM_PRESSURE_ITERATIONS = 20
deltaTime = 1/3

obstacle_center_r = GRID_HEIGHT // 2
obstacle_center_c = GRID_WIDTH // 2
OBSTACLE_RADIUS = 10

# Initialize grid
grid = [[{"type": 1, 'u': 0, 'v': 0, "density": 0} for _ in range(GRID_WIDTH)] for _ in range(GRID_HEIGHT)]   # 0: solid, 1: fluid

# Gravity direction (in radians, 0 means upwards)
gravity_angle = 0

def draw_dial():
    """Draw the gravity direction dial"""
    # Dial center position
    center_x = WINDOW_WIDTH // 2
    center_y = WINDOW_HEIGHT - DIAL_RADIUS - 10

    # Draw outer circle
    pygame.draw.circle(screen, WHITE, (center_x, center_y), DIAL_RADIUS, 2)

    # Draw direction line
    end_x = center_x + DIAL_RADIUS * math.cos(gravity_angle - math.pi/2)
    end_y = center_y + DIAL_RADIUS * math.sin(gravity_angle - math.pi/2)
    pygame.draw.line(screen, RED, (center_x, center_y), (end_x, end_y), 3)

    # Draw small center circle
    pygame.draw.circle(screen, RED, (center_x, center_y), 5)

def get_dial_angle(mouse_pos, dial_center):
    """Calculate angle from dial center to mouse position"""
    dx = mouse_pos[0] - dial_center[0]
    dy = mouse_pos[1] - dial_center[1]
    return math.atan2(dy, dx) + math.pi/2

def is_within_dial(mouse_pos):
    """Check if mouse position is within dial area"""
    dial_center = (WINDOW_WIDTH // 2, WINDOW_HEIGHT - DIAL_RADIUS - 10)
    dx = mouse_pos[0] - dial_center[0]
    dy = mouse_pos[1] - dial_center[1]
    return (dx * dx + dy * dy) <= DIAL_RADIUS * DIAL_RADIUS

def draw_grid():
    """Draw the grid"""
    for row in range(GRID_HEIGHT+1):
        pygame.draw.line(screen, WHITE, (0, row * SCALE), (GRID_WIDTH * SCALE, row * SCALE))

    for col in range(GRID_HEIGHT+1):
        pygame.draw.line(screen, WHITE, (col * SCALE, 0), (col * SCALE, GRID_HEIGHT * SCALE))

def draw_velocity(r, c):
    """Draw the velocity vector at the grid cell"""
    u = grid[r][c]["u"]
    v = grid[r][c]["v"]
    pygame.draw.line(screen, BLUE, (c * SCALE + SCALE//2, r * SCALE + SCALE//2),
                    (c * SCALE + SCALE//2 + u, r * SCALE + SCALE//2 + v), 2)

def apply_advection():
    """Apply advection to all fluid particles"""
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:
                r0, c0 = trace_particle(r, c)
                d, u, v = lin_interp(r0, c0)
                grid[r][c]["u"] = u
                grid[r][c]["v"] = v
                grid[r][c]["density"] = d

def lin_interp(r0, c0):
    '''
    Performs bi-linear interpolation on velocity field
    Parameters:
    r0: The row coordinate (float) of the previous cell
    c0: The column coordinate (float) of the previous cell
    Returns:
    tuple: Interpolated (u, v) velocity components
    '''
    # Get the integer indices
    r = int(r0)
    c = int(c0)

    # Get the decimal parts for interpolation weights
    r_frac = r0 - r
    c_frac = c0 - c

    # Clamp indices to grid boundaries
    r = max(0, min(r, GRID_HEIGHT - 2))
    c = max(0, min(c, GRID_WIDTH - 2))

    # Get velocities at the four neighboring cells
    u00 = grid[r][c]["u"]
    u10 = grid[r+1][c]["u"]
    u01 = grid[r][c+1]["u"]
    u11 = grid[r+1][c+1]["u"]

    v00 = grid[r][c]["v"]
    v10 = grid[r+1][c]["v"]
    v01 = grid[r][c+1]["v"]
    v11 = grid[r+1][c+1]["v"]

    d00 = grid[r][c]["density"]
    d10 = grid[r+1][c]["density"]
    d01 = grid[r][c+1]["density"]
    d11 = grid[r+1][c+1]["density"]

    # Bilinear interpolation
    u = (1 - r_frac) * (1 - c_frac) * u00 + \
        r_frac * (1 - c_frac) * u10 + \
        (1 - r_frac) * c_frac * u01 + \
        r_frac * c_frac * u11

    v = (1 - r_frac) * (1 - c_frac) * v00 + \
        r_frac * (1 - c_frac) * v10 + \
        (1 - r_frac) * c_frac * v01 + \
        r_frac * c_frac * v11

    d = (1 - r_frac) * (1 - c_frac) * d00 + \
        r_frac * (1 - c_frac) * d10 + \
        (1 - r_frac) * c_frac * d01 + \
        r_frac * c_frac * d11

    return d, u, v

def trace_particle(r, c):
    """Trace particle back in time using current velocity"""
    u = grid[r][c]["u"]  # u is horizontal velocity
    v = grid[r][c]["v"]  # v is vertical velocity

    r0 = r - v*deltaTime
    c0 = c - u*deltaTime
    # pygame.draw.line(screen, RED, (c * SCALE + SCALE//2, r * SCALE + SCALE//2), (c0 * SCALE + SCALE//2, r0 * SCALE + SCALE//2), 2)
    return r0, c0

def get_color(mag):
    """Get color based on magnitude"""
    r = int(255*mag/(1+mag))
    r = r if r>0 else 0
    return r, r, r

# Physics functions
def apply_forces():
    """Apply gravity to all fluid particles"""
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:
                grid[r][c]["u"] += GRAVITY * math.sin(gravity_angle) * deltaTime
                grid[r][c]["v"] -= GRAVITY * math.cos(gravity_angle) * deltaTime

    # Enforce no-slip boundary conditions
    for r in range(1, GRID_HEIGHT-1):
        for c in range(1, GRID_WIDTH-1):
            if not grid[r][c]["type"]:  # If this is a solid cell
                # Handle vertical boundaries
                if grid[r][c-1]["type"]:  # Fluid cell on the left
                    grid[r][c-1]["u"] = 0  # Zero horizontal velocity at boundary
                if grid[r][c+1]["type"]:  # Fluid cell on the right
                    grid[r][c]["u"] = 0    # Zero horizontal velocity at boundary

                # Handle horizontal boundaries
                if grid[r-1][c]["type"]:  # Fluid cell above
                    grid[r-1][c]["v"] = 0  # Zero vertical velocity at boundary
                if grid[r+1][c]["type"]:  # Fluid cell below
                    grid[r][c]["v"] = 0    # Zero vertical velocity at boundary

                # Handle corner cases - average diagonal velocities
                if grid[r-1][c-1]["type"]:  # Top-left
                    grid[r-1][c-1]["u"] *= 0.5
                    grid[r-1][c-1]["v"] *= 0.5
                if grid[r-1][c+1]["type"]:  # Top-right
                    grid[r-1][c]["u"] *= 0.5
                    grid[r-1][c+1]["v"] *= 0.5
                if grid[r+1][c-1]["type"]:  # Bottom-left
                    grid[r][c-1]["u"] *= 0.5
                    grid[r+1][c-1]["v"] *= 0.5
                if grid[r+1][c+1]["type"]:  # Bottom-right
                    grid[r][c]["u"] *= 0.5
                    grid[r+1][c]["v"] *= 0.5

def apply_projection():
    """Apply pressure projection for incompressibility"""
    for _ in range(NUM_PRESSURE_ITERATIONS):
        for r in range(1, GRID_HEIGHT-1):
            for c in range(1, GRID_WIDTH-1):
                if grid[r][c]["type"]:
                    # Calculate divergence
                    div = (grid[r][c]["u"] + grid[r][c]["v"])\
                        - (grid[r-1][c]["u"] + grid[r][c-1]["v"])*grid[r][c]["type"]

                    # Calculate pressure using divergence
                    p = -OVERELAX_FACT * div / (2+2*grid[r][c]["type"])

                    # Apply pressure to velocities
                    grid[r][c]["u"] += p
                    grid[r][c]["v"] += p
                    grid[r-1][c]["u"] -= p*grid[r][c]["type"]
                    grid[r][c-1]["v"] -= p*grid[r][c]["type"]

def update(screen):
    """Update and draw simulation"""

    # Draw background
    screen.fill(BLACK)

    # Draw grid
    # draw_grid()

    # Apply forces
    apply_forces()

    # Advect the velocity field
    apply_advection()

    # Apply pressure projection
    apply_projection()

    # Create the source
    for c in range(GRID_WIDTH//2 - 5, GRID_WIDTH//2 + 5):
        grid[GRID_HEIGHT-1][c]["density"] = 1

    # Draw grid particles only if they are fluid
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:

                # Draw particle
                color = get_color(grid[r][c]["density"])
                pygame.draw.rect(screen, color, (c * SCALE, r * SCALE, SCALE, SCALE))

                # Draw velocity
                # draw_velocity(r, c)

    draw_obstacle()

    # Draw dial
    draw_dial()

def draw_obstacle():
    pygame.draw.circle(screen, GREY, (obstacle_center_c*SCALE, obstacle_center_r*SCALE), OBSTACLE_RADIUS*SCALE)

def initialize_particles():
    """Initialize particles in the center"""

    # Make left and right walls solid
    for r in range(GRID_HEIGHT):
        grid[r][0]["type"] = 0
        grid[r][GRID_WIDTH-1]["type"] = 0

    # Make top and bottom walls solid
    for c in range(GRID_WIDTH):
        grid[0][c]["type"] = 0
        grid[GRID_HEIGHT-1][c]["type"] = 0

    update_obstacle_position()

def update_obstacle_position():
    """Update the obstacle's position and mark cells as solid"""
    for r in range(1, GRID_HEIGHT-1):
        for c in range(1, GRID_WIDTH-1):
            dr = r - obstacle_center_r
            dc = c - obstacle_center_c
            distance = round(math.sqrt(dr*dr + dc*dc), 2)
            if distance < OBSTACLE_RADIUS:
                grid[r][c]["type"] = 0

def is_within_obstacle(mouse_pos):
    """Check if mouse position is within obstacle area"""
    grid_r = mouse_pos[1] // SCALE
    grid_c = mouse_pos[0] // SCALE
    dr = grid_r - obstacle_center_r
    dc = grid_c - obstacle_center_c
    return (dr*dr + dc*dc) <= OBSTACLE_RADIUS * OBSTACLE_RADIUS

def main():
    global gravity_angle, obstacle_center_c, obstacle_center_r
    running = True
    clock = pygame.time.Clock()
    dragging_obstacle = False
    dragging_dial = False

    initialize_particles()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.MOUSEBUTTONDOWN:
                mouse_pos = pygame.mouse.get_pos()
                if is_within_dial(mouse_pos):
                    dragging_dial = True
                    dial_center = (WINDOW_WIDTH // 2, WINDOW_HEIGHT - DIAL_RADIUS - 10)
                    gravity_angle = get_dial_angle(mouse_pos, dial_center)
                elif is_within_obstacle(mouse_pos):
                    dragging_obstacle = True
            elif event.type == pygame.MOUSEBUTTONUP:
                dragging_dial = False
                dragging_obstacle = False
            elif event.type == pygame.MOUSEMOTION:
                if dragging_dial:
                    dial_center = (WINDOW_WIDTH // 2, WINDOW_HEIGHT - DIAL_RADIUS - 10)
                    gravity_angle = get_dial_angle(pygame.mouse.get_pos(), dial_center)
                elif dragging_obstacle:
                    mouse_pos = pygame.mouse.get_pos()
                    new_r = mouse_pos[1] // SCALE
                    new_c = mouse_pos[0] // SCALE
                    if (OBSTACLE_RADIUS < new_r < GRID_HEIGHT-OBSTACLE_RADIUS and
                        OBSTACLE_RADIUS < new_c < GRID_WIDTH-OBSTACLE_RADIUS):
                        # Reset previous obstacle cells to fluid
                        for r in range(1, GRID_HEIGHT-1):
                            for c in range(1, GRID_WIDTH-1):
                                grid[r][c]["type"] = 1
                        # Update obstacle position
                        obstacle_center_r = new_r
                        obstacle_center_c = new_c
                        update_obstacle_position()

        update(screen)
        pygame.display.flip()
        clock.tick(FRAME_RATE)

    pygame.quit()

if __name__ == "__main__":
    main()