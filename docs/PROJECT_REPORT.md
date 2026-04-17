# 3D Escape Room Game - Project Report Details

---

## Abstract

The **3D Escape Room Game** is an interactive game where players experience a first-person escape room adventure. The game is built using C++ programming language and OpenGL graphics technology. Players must navigate through four challenging levels, collect keys, avoid obstacles, and manage their health while racing against the clock. Each level gets progressively harder with more obstacles and less time. The game demonstrates important computer graphics concepts like 3D environment creation, object movement, collision detection, and real-time performance. This document explains how the game works, what components it needs, and how it was built.

---

## Problem Statement

Most escape room games are either flat 2D experiences or don't give players much freedom to move around. This project was created to solve several challenges:

1. **Make it Feel Real** - Create a 3D experience where you can look around freely from a first-person view, just like you're actually in the room.

2. **Make it Progressively Harder** - Design four levels that get harder as you progress, with more obstacles to avoid and less time to complete them.

3. **Keep Gameplay Fun** - Create controls and game mechanics that feel smooth and responsive, including collision detection that doesn't feel unfair.

4. **Make it Look Professional** - Use a nice color scheme and clean graphics so the game looks polished and not amateur.

5. **Save Your Progress** - Keep track of high scores so players can compete with themselves and others to get the best score.

---

## Scope

### Features We Built:

- **Four Game Levels** - Easy, Medium, Hard, and Ground Survival
- **3D Graphics** - You see the world from a first-person view using OpenGL
- **Player Controls** - Move with WASD, look around with arrow keys
- **Obstacle and Wall Collision** - The game detects when you touch things and prevents you from walking through walls
- **Health System** - Start with 3 health; touching obstacles costs 1 health each
- **Obstacle Movement** - On harder levels (2-3), obstacles move around randomly
- **Timer System** - Each level has a countdown (10-20 seconds); running out of time costs 1 health
- **Key Collection** - Find keys to complete and advance levels
- **Real-Time Display** - Screen shows your health, obstacles, and time remaining
- **High-Score System** - Automatically saves and ranks your best scores
- **GitHub Setup** - Project uploaded with team member access

### Features We Did NOT Include:

- Online multiplayer or networking
- Advanced physics or realistic gravity
- Enemy AI that hunts you down
- Sound effects or music
- Complex shadow effects or fancy lighting
- Mobile or console versions
- Level editor or custom level creation
- Procedural random level generation
- Modding support

---

## Algorithm

### Step 1: Start the Game

Player launches the game and is taken to the first level.

### Step 2: Display the 3D Environment

The game shows the player a 3D escape room with obstacles and a goal to reach. The player sees a health counter, a timer, and obstacle information on the screen.

### Step 3: Player Moves Through the Room

The player uses WASD keys to move and arrow keys to look around. As the player moves:

- They can walk freely through open space
- They cannot pass through walls (walls block movement)
- They can get close to obstacles but must avoid touching them

### Step 4: Check for Collisions

As the player moves, the game constantly checks if the player has hit anything:

**A) Touching a Wall:**

- The player stops and cannot move through the wall
- No health damage (walls are safe)

**B) Touching an Obstacle:**

- The player loses 1 health point
- The game prevents rapid damage by waiting 1.5 seconds before the next damage
- Player can continue moving

**C) Running Out of Time:**

- The timer counts down as the player plays
- When time reaches zero, the player loses 1 health point
- The level fails and the game moves to the next attempt

### Step 5: Obstacles Move (Levels 2 and 3 Only)

In the Medium and Hard levels, obstacles move randomly around the room. The player must navigate around these moving obstacles to avoid them.

### Step 6: Find the Key to Complete the Level

The player must search the room and find a key object. Collecting the key completes the level successfully.

### Step 7: Progress to Next Level

After completing a level, the player advances to the next, more difficult level. There are 4 levels total:

- Level 1 (Easy): 3 static obstacles
- Level 2 (Medium): 4 moving obstacles
- Level 3 (Hard): 6 moving obstacles
- Level 4 (Survival): Ground-based challenges with mixed obstacles

### Step 8: Health Management

The player starts with 3 health points. They must be careful:

- Lose 1 health from touching obstacles
- Lose 1 health when time runs out
- When health reaches zero, the game ends

### Step 9: Save Score

When the game ends (win or lose), the game saves the player's score, remaining health, and bonus time to a high-score file.

### Step 10: Display Results

The player sees their final score, ranking on the high-score list, and the option to play again or quit.

---

## User Requirements

### What Players Can Do:

1. **Move Around** - Use WASD keys to walk forward, backward, left, and right through the game world
2. **Look Around** - Use arrow keys to rotate the view and look in different directions
3. **Avoid Obstacles** - Navigate around moving obstacles (on harder levels) without touching them
4. **Find Keys** - Search each level to locate a key that unlocks the next level
5. **Complete Levels** - Finish each level by finding the key before time runs out
6. **Stay Alive** - Manage your health (3 lives) carefully by avoiding obstacles and not running out of time
7. **See Your Progress** - The screen shows your health, remaining time, and how many obstacles are in the room
8. **Compete** - See your score on the high-score list and try to beat your own records

### How the Game Works:

1. Players start with 3 health points
2. Touching an obstacle costs 1 health
3. Running out of time costs 1 health
4. When health reaches 0, the game is over
5. Walls are safe to touch (no health loss)
6. The game runs smoothly at 60 frames per second
7. High scores are saved automatically so they're still there when you play again
8. The game doesn't crash or have bugs that ruin the experience

---

## Hardware Requirements

### Minimum Requirements (Game Will Run):

- **Processor**: Intel Core i5 or AMD equivalent, 2.0 GHz or faster
- **RAM**: 2 GB of memory
- **Graphics Card**: NVIDIA GeForce GTX 460 or AMD Radeon HD 5850 (or newer equivalent)
- **Storage**: 50 MB of free disk space
- **Display**: 1024x768 screen resolution or higher
- **Input**: Mouse and keyboard

### Recommended Requirements (Game Runs Best):

- **Processor**: Intel Core i7 or AMD equivalent, 3.0 GHz or faster
- **RAM**: 8 GB of memory
- **Graphics Card**: NVIDIA GeForce GTX 1050 Ti or AMD Radeon RX 580 (or newer)
- **Storage**: 100 MB of free SSD space
- **Display**: 1920x1080 HD screen
- **Input**: Mouse and keyboard with comfortable keys

### What We Used to Build It:

- **Operating System**: Windows 10 or Windows 11
- **Processor**: Intel Core i7
- **RAM**: 8 GB
- **Graphics Card**: NVIDIA or AMD with good driver support

---

## Software Requirements

### What You Need to Run the Game:

- **Operating System**: Windows 7, 8, 10, or 11 (any version, 32-bit or 64-bit)
- **Updated Graphics Drivers**: For NVIDIA or AMD graphics cards (to support OpenGL)
- **Microsoft Visual C++ Runtime**: Usually already installed on Windows

### What Programmers Need to Build the Game from Source Code:

- **Programming Language**: C++ (using C++17 standard)
- **Code Editor**: Visual Studio Code
- **Compiler**: GCC (g++ version 10.0 or higher)
- **Version Control**: Git and GitHub account

### Libraries the Game Uses:

1. **FreeGLUT 3.x** - Handles windows and user input (keyboard, mouse)
2. **OpenGL 2.1** - Draws all the 3D graphics on screen
3. **GLU** - Helper functions for OpenGL
4. **stb_image.h** - Loads background images (PNG/JPG files)
5. **Standard C++ Libraries** - Math, file reading/writing, text handling

### How to Build the Game:

Open a command prompt and type:

```bash
g++ -Wall -Wextra -g3 -std=c++17 main.cpp -o output/main.exe -lfreeglut -lopengl32 -lglu32
```

This creates the game file `main.exe` that you can run.

---

## Screenshots

**[Add your game screenshots here]**

You can include:

- A screenshot from Level 1 (easy mode with 3 obstacles)
- A screenshot from Level 2 (medium mode with 4 moving obstacles)
- A screenshot from Level 3 (hard mode with 6 moving obstacles)
- A screenshot showing the Health display and Timer
- A screenshot of the High-Score screen
- A screenshot showing the Game-Over screen

---

## Additional Notes

**Project Details:**

- **Team Size**: 4 students (university project)
- **Code Repository**: GitHub - 3D-Escape-Room-Game organization
- **Status**: Complete and working
- **Total Code Lines**: Approximately 1700 lines of C++ code

**What This Project Shows:**

- How to create a 3D game using C++ and OpenGL
- Real-time collision detection and physics
- Managing game state and saving data
- 3D graphics rendering and camera controls
- File input/output for high-score persistence

---

**Report Date**: April 17, 2026  
**Project Version**: 1.0  
**Created By**: 3D Escape Room Game Development Team
