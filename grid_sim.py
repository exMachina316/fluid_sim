import pygame
import random
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 40

# Window dimensions
GRID_WIDTH, GRID_HEIGHT = 16, 16
WINDOW_WIDTH, WINDOW_HEIGHT = GRID_WIDTH * SCALE, GRID_HEIGHT * SCALE

# Add space for the dial
DIAL_RADIUS = 50
WINDOW_HEIGHT += DIAL_RADIUS * 2 + 20  # Extra space for dial

# Set up the display
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Fluid Simulation with Gravity Control")

# Colors
BLACK = (0, 0, 0)
BLUE = (0, 0, 255)
WHITE = (255, 255, 255)
RED = (255, 0, 0)

# Simulation settings
FRAME_RATE = 30
NUM_PARTICLES = 10

# Initialize grid
grid = [[False for _ in range(GRID_WIDTH)] for _ in range(GRID_HEIGHT)]

# Gravity direction (in radians, 0 means downward)
gravity_angle = math.pi  # Start with downward gravity

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

def move_particle(r, c):
    """Move particle based on gravity direction with improved settling"""
    if not grid[r][c]:
        return

    # Calculate movement direction based on gravity angle
    dx = round(math.cos(gravity_angle - math.pi/2))
    dy = round(math.sin(gravity_angle - math.pi/2))
    
    # Check if particle should settle
    # A particle settles if it has support in the gravity direction
    new_r, new_c = r + dy, c + dx
    
    # If there's a particle or wall in the direction of gravity, try to settle
    if (new_r < 0 or new_r >= GRID_HEIGHT or 
        new_c < 0 or new_c >= GRID_WIDTH or 
        grid[new_r][new_c]):
        
        # Check neighbors perpendicular to gravity direction
        perp_dx = -dy  # Perpendicular direction
        perp_dy = dx
        
        # If particle has support and neighbors, it should settle
        has_support = False
        
        # Check both perpendicular directions
        for d in [-1, 1]:
            check_r = r + perp_dy * d
            check_c = c + perp_dx * d
            
            if (0 <= check_r < GRID_HEIGHT and 
                0 <= check_c < GRID_WIDTH and 
                grid[check_r][check_c]):
                has_support = True
                break
        
        if has_support:
            return  # Particle settles here
    
    # If particle shouldn't settle, proceed with movement
    if (0 <= new_r < GRID_HEIGHT and 
        0 <= new_c < GRID_WIDTH and 
        not grid[new_r][new_c]):
        grid[r][c] = False
        grid[new_r][new_c] = True
        return
    
    # Try diagonal movements if direct movement is blocked
    diagonals = []
    if dx != 0 and dy != 0:  # For diagonal gravity
        diagonals = [(dy, 0), (0, dx)]
    else:  # For cardinal gravity
        diagonals = [(dy+dx, dx+dy), (dy-dx, dx-dy)]
    
    random.shuffle(diagonals)
    
    for d_dy, d_dx in diagonals:
        new_r, new_c = r + d_dy, c + d_dx
        if (0 <= new_r < GRID_HEIGHT and 
            0 <= new_c < GRID_WIDTH and 
            not grid[new_r][new_c]):
            grid[r][c] = False
            grid[new_r][new_c] = True
            return

def update(screen):
    """Update and draw simulation"""
    # Update particles in direction of gravity
    if abs(math.cos(gravity_angle)) > abs(math.sin(gravity_angle)):
        # Primarily horizontal movement
        rows = range(GRID_HEIGHT)
        if math.cos(gravity_angle) > 0:  # Moving right
            cols = range(GRID_WIDTH - 1, -1, -1)
        else:  # Moving left
            cols = range(GRID_WIDTH)
    else:
        # Primarily vertical movement
        cols = range(GRID_WIDTH)
        if math.sin(gravity_angle) > 0:  # Moving down
            rows = range(GRID_HEIGHT - 1, -1, -1)
        else:  # Moving up
            rows = range(GRID_HEIGHT)
    
    for r in rows:
        for c in cols:
            move_particle(r, c)
    
    # Draw
    screen.fill(BLACK)
    
    # Draw grid particles
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]:
                pygame.draw.circle(screen, BLUE,
                                (c * SCALE + SCALE // 2, r * SCALE + SCALE // 2),
                                SCALE // 3)
    
    # Draw dial
    draw_dial()

def initialize_particles():
    """Initialize particles in the center"""
    used_cells = []
    
    for _ in range(NUM_PARTICLES):
        while True:
            r = random.randint(0, GRID_HEIGHT - 1)
            c = random.randint(0, GRID_WIDTH - 1)
            if (r, c) not in used_cells:
                grid[r][c] = True
                used_cells.append((r, c))
                break

def main():
    global gravity_angle
    running = True
    clock = pygame.time.Clock()
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
                elif mouse_pos[1] < WINDOW_HEIGHT - 2*DIAL_RADIUS:
                    # Add particle if clicked in grid area
                    grid_r = mouse_pos[1] // SCALE
                    grid_c = mouse_pos[0] // SCALE
                    if 0 <= grid_r < GRID_HEIGHT and 0 <= grid_c < GRID_WIDTH:
                        grid[grid_r][grid_c] = True
            elif event.type == pygame.MOUSEBUTTONUP:
                dragging_dial = False
            elif event.type == pygame.MOUSEMOTION and dragging_dial:
                dial_center = (WINDOW_WIDTH // 2, WINDOW_HEIGHT - DIAL_RADIUS - 10)
                gravity_angle = get_dial_angle(pygame.mouse.get_pos(), dial_center)

        update(screen)
        pygame.display.flip()
        clock.tick(FRAME_RATE)

    pygame.quit()

if __name__ == "__main__":
    main()