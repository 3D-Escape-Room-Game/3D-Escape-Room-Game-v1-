#include <GL/glut.h>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

struct Vec2 {
    float x;
    float z;
};

struct Obstacle {
    float x;
    float z;
    float sx;
    float sz;
    float dx = 0.0f; // Velocity X
    float dz = 0.0f; // Velocity Z
};

enum LevelType {
    LEVEL_ROOM,
    LEVEL_GROUND
};

enum GameState {
    STATE_MENU,
    STATE_RULES,
    STATE_PLAYING,
    STATE_HIGHSCORES,
    STATE_NAME_ENTRY,
    STATE_GAME_OVER
};

struct HighScoreEntry {
    string name;
    int score;
    int livesLeft;
    int bonusTime;
};

const float ROOM_HALF_SIZE = 5.0f;
const float GROUND_HALF_SIZE = 16.0f;
const float WALL_THICKNESS = 0.3f;
const float PLAYER_RADIUS = 0.25f;
const float BASE_MOVE_SPEED = 0.07f;
const float ROT_SPEED = 0.03f;
const float PITCH_SPEED = 0.02f;
const float MOUSE_SENSITIVITY = 0.0035f;
const float PITCH_LIMIT = 1.45f;
const float PLAYER_EYE_HEIGHT = 1.0f;
const float JUMP_VELOCITY = 4.2f;
const float GRAVITY_ACCEL = 9.2f;
const float FALL_DAMAGE_THRESHOLD = 1.15f;
const int MAX_LIVES = 3;
const char* HIGHSCORE_FILE = "highscores.txt";

const int TOTAL_LEVELS = 4;
const int MAX_KEYS = 3;
const int MAX_OBSTACLES = 6;

const char* levelNames[TOTAL_LEVELS] = {"Easy", "Medium", "Hard", "Ground"};
const LevelType levelTypes[TOTAL_LEVELS] = {LEVEL_ROOM, LEVEL_ROOM, LEVEL_ROOM, LEVEL_GROUND};
const float levelHalfSizes[TOTAL_LEVELS] = {ROOM_HALF_SIZE, ROOM_HALF_SIZE, ROOM_HALF_SIZE, GROUND_HALF_SIZE};
const int keysRequiredByLevel[TOTAL_LEVELS] = {1, 2, 3, 0};
const int obstacleCountByLevel[TOTAL_LEVELS] = {3, 4, 6, 3};
const float moveSpeedScale[TOTAL_LEVELS] = {1.00f, 0.92f, 0.84f, 1.00f};
const float pickupRadiusByLevel[TOTAL_LEVELS] = {0.95f, 0.82f, 0.72f, 0.90f};
const float levelTimeLimitByLevel[TOTAL_LEVELS] = {10.0f, 15.0f, 20.0f, 20.0f};
const float doorWidthByLevel[TOTAL_LEVELS] = {1.6f, 1.6f, 1.6f, 2.6f};
const float doorHeightByLevel[TOTAL_LEVELS] = {2.6f, 2.6f, 2.6f, 3.6f};
const float doorThicknessByLevel[TOTAL_LEVELS] = {0.16f, 0.16f, 0.16f, 0.20f};

const Vec2 levelKeyPositions[TOTAL_LEVELS][MAX_KEYS] = {
    {{2.7f, -3.2f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
    {{3.1f, -3.3f}, {-2.5f, 3.5f}, {0.0f, 0.0f}},
    {{3.2f, -3.3f}, {-3.5f, -2.5f}, {2.5f, 2.5f}},
    {{-5.4f, 4.8f}, {0.0f, 0.0f}, {0.0f, 0.0f}}
};

const float levelKeyHeights[TOTAL_LEVELS][MAX_KEYS] = {
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f},
    {0.35f, 0.35f, 0.35f}
};

const float obstacleHeightByLevel[TOTAL_LEVELS][MAX_OBSTACLES] = {
    {1.05f, 1.20f, 1.05f, 0.0f, 0.0f, 0.0f},
    {1.2f, 1.6f, 1.15f, 1.15f, 0.0f, 0.0f},
    {1.9f, 1.9f, 2.1f, 1.4f, 1.7f, 1.7f},
    {1.2f, 1.2f, 1.2f, 0.0f, 0.0f, 0.0f}
};

const Obstacle levelObstacles[TOTAL_LEVELS][MAX_OBSTACLES] = {
    {
        {-2.0f, 1.4f, 0.75f, 0.75f},
        {0.0f, -1.6f, 0.90f, 0.90f},
        {2.0f, 1.2f, 0.75f, 0.75f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    },
    {
        {-1.2f, 0.2f, 2.1f, 0.55f},
        {1.5f, -1.5f, 2.3f, 0.55f},
        {-2.7f, 2.4f, 1.2f, 0.60f},
        {2.4f, -2.6f, 1.1f, 0.60f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    },
    {
        {-2.8f, 1.3f, 0.55f, 3.8f},
        {2.6f, -0.5f, 0.55f, 3.5f},
        {0.0f, 0.9f, 3.1f, 0.55f},
        {0.0f, -2.1f, 3.0f, 0.55f},
        {-3.7f, -0.9f, 0.70f, 2.8f},
        {3.5f, 1.8f, 0.70f, 2.8f}
    },
    {
        {-4.4f, 2.8f, 0.9f, 0.9f},
        {3.8f, 1.8f, 1.1f, 1.1f},
        {0.2f, -1.8f, 2.2f, 2.2f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f}
    }
};

GLuint startScreenTexture = 0;

void loadBackgroundTexture() {
    int width, height, channels;
    unsigned char* data = stbi_load("3d-escape.png", &width, &height, &channels, 4);
    if (!data) data = stbi_load("../3d-escape.png", &width, &height, &channels, 4);
    if (data) {
        glGenTextures(1, &startScreenTexture);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        cout << "Loaded 3d-escape.png successfully.\n";
    } else {
        cout << "Failed to load 3d-escape.png. Reason: " << stbi_failure_reason() << "\n";
    }
}

float camX = 0.0f;
float camY = 1.0f;
float camZ = 3.5f;
float yaw = 0.0f;
float pitch = 0.0f;

GameState gameState = STATE_MENU;
int menuSelection = 0;
const int MENU_ITEMS = 3;
const char* menuLabels[MENU_ITEMS] = {"Start", "Highscore List", "Quit"};

vector<HighScoreEntry> highScores;
char playerName[16] = {0};
int playerNameLength = 0;
int pendingScore = 0;
int bonusTimeScore = 0;

int currentLevel = 0;
int keysCollected = 0;
Obstacle activeObstacles[MAX_OBSTACLES];
int playerLives = MAX_LIVES;
bool keyTaken[MAX_KEYS] = {false, false, false};
bool doorOpening = false;
float doorAngle = 0.0f;
bool gameCompleted = false;
bool gameOver = false;
int levelBannerFrames = 0;
float levelTimeRemaining = 0.0f;
float damageCooldown = 0.0f;
float weaponAttackCooldown = 0.0f;
int lastTickMs = 0;
float verticalVelocity = 0.0f;
bool airborne = false;
float fallStartCamY = PLAYER_EYE_HEIGHT;

const int GHOST_MAX_HEALTH = 6;
const float GHOST_MOVE_SPEED = 1.25f;
const float GHOST_TOUCH_DAMAGE_RANGE = 1.15f;
const float GHOST_WEAPON_RANGE = 2.55f;
const float GHOST_BODY_RADIUS = 0.85f;
const float INTERACT_PICKUP_RANGE = 1.60f;
const float MEDKIT_HOLD_REQUIRED = 1.0f;

bool ghostAlive = false;
int ghostHealth = GHOST_MAX_HEALTH;
float ghostX = 0.0f;
float ghostZ = -5.8f;
Vec2 weaponPickupPos = {-7.0f, 10.5f};
Vec2 healthPickupPos = {7.0f, 9.8f};
bool weaponClaimed = false;
bool healthPickupClaimed = false;
bool groundDoorUnlockedByBoss = false;
bool medkitUsed = false;
float medkitHoldTimer = 0.0f;
bool groundIntroActive = false;
float swordSwingTimer = 0.0f;
float ghostHitFlashTimer = 0.0f;

bool keyStates[256] = {false};
bool specialStates[256] = {false};
bool mouseInitialized = false;
int lastMouseX = 0;
int lastMouseY = 0;

void startLevel(int levelIndex);
void damagePlayer(int amount, bool bypassCooldown = false);
void tryGhostAttack();

float clampf(float value, float low, float high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

float distance2D(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrt(dx * dx + dz * dz);
}

float distance3D(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float dz = z1 - z2;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

bool isGroundLevel() {
    return levelTypes[currentLevel] == LEVEL_GROUND;
}

float worldHalfSize() {
    return levelHalfSizes[currentLevel];
}

float currentMoveSpeed() {
    return BASE_MOVE_SPEED * moveSpeedScale[currentLevel];
}

int keysRequired() {
    return keysRequiredByLevel[currentLevel];
}

int obstacleCount() {
    return obstacleCountByLevel[currentLevel];
}

float pickupRadius() {
    return pickupRadiusByLevel[currentLevel];
}

float currentDoorWidth() {
    return doorWidthByLevel[currentLevel];
}

float currentDoorHeight() {
    return doorHeightByLevel[currentLevel];
}

float currentDoorThickness() {
    return doorThicknessByLevel[currentLevel];
}

float currentDoorZ() {
    return -worldHalfSize() + WALL_THICKNESS * 0.5f;
}

void drawBox(float x, float y, float z, float sx, float sy, float sz) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawFilledRect2D(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawText2D(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
}

int bitmapTextWidth(const char* text) {
    return glutBitmapLength(GLUT_BITMAP_HELVETICA_18, reinterpret_cast<const unsigned char*>(text));
}

void drawText2DRightAligned(float rightX, float y, const char* text) {
    drawText2D(rightX - static_cast<float>(bitmapTextWidth(text)), y, text);
}

void drawText2DCentered(float centerX, float y, const char* text) {
    drawText2D(centerX - static_cast<float>(bitmapTextWidth(text)) * 0.5f, y, text);
}

void resetInputStates() {
    for (int i = 0; i < 256; ++i) {
        keyStates[i] = false;
        specialStates[i] = false;
    }
}

void resetNameEntry() {
    playerName[0] = '\0';
    playerNameLength = 0;
}

void sortHighScores() {
    sort(highScores.begin(), highScores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.livesLeft != b.livesLeft) return a.livesLeft > b.livesLeft;
        return a.bonusTime > b.bonusTime;
    });

    if (highScores.size() > 10) {
        highScores.resize(10);
    }
}

void loadHighScores() {
    highScores.clear();

    ifstream file(HIGHSCORE_FILE);
    if (!file.is_open()) {
        return;
    }

    HighScoreEntry entry;
    while (file >> entry.name >> entry.score >> entry.livesLeft >> entry.bonusTime) {
        highScores.push_back(entry);
    }

    sortHighScores();
}

void saveHighScores() {
    sortHighScores();

    ofstream file(HIGHSCORE_FILE, ios::trunc);
    if (!file.is_open()) {
        cout << "Could not save high scores.\n";
        return;
    }

    for (const auto& entry : highScores) {
        file << entry.name << ' ' << entry.score << ' ' << entry.livesLeft << ' ' << entry.bonusTime << '\n';
    }
}

void addHighScore(const string& name, int score, int livesLeft, int bonusTime) {
    HighScoreEntry entry;
    entry.name = name.empty() ? "PLAYER" : name;
    entry.score = score;
    entry.livesLeft = livesLeft;
    entry.bonusTime = bonusTime;
    highScores.push_back(entry);
    sortHighScores();
    saveHighScores();
}

void startMenu() {
    gameState = STATE_MENU;
    menuSelection = 0;
    gameCompleted = false;
    gameOver = false;
    resetInputStates();
    resetNameEntry();
}

void startGame() {
    bonusTimeScore = 0;
    pendingScore = 0;
    playerLives = MAX_LIVES;
    gameCompleted = false;
    gameOver = false;
    gameState = STATE_PLAYING;
    startLevel(0);
}

void openHighScoreScreen() {
    gameState = STATE_HIGHSCORES;
    resetInputStates();
}

void beginNameEntry() {
    gameState = STATE_NAME_ENTRY;
    resetInputStates();
    resetNameEntry();
}

void finishGameToHighScore() {
    pendingScore = playerLives * 1000 + bonusTimeScore;
    beginNameEntry();
}

void returnToMenuAfterFailure() {
    gameState = STATE_GAME_OVER;
    resetInputStates();
}

void resetLevelCore() {
    keysCollected = 0;
    for (int i = 0; i < MAX_KEYS; ++i) {
        keyTaken[i] = false;
    }

    doorOpening = false;
    doorAngle = 0.0f;
    camX = 0.0f;
    camY = PLAYER_EYE_HEIGHT;
    camZ = isGroundLevel() ? (worldHalfSize() - 3.0f) : 3.6f;
    yaw = 0.0f;
    pitch = 0.0f;
    levelBannerFrames = 220;
    levelTimeRemaining = levelTimeLimitByLevel[currentLevel];
    damageCooldown = 0.0f;
    weaponAttackCooldown = 0.0f;
    verticalVelocity = 0.0f;
    airborne = false;
    fallStartCamY = camY;

    ghostAlive = isGroundLevel();
    ghostHealth = GHOST_MAX_HEALTH;
    ghostX = 0.0f;
    ghostZ = -5.8f;
    weaponClaimed = false;
    healthPickupClaimed = false;
    groundDoorUnlockedByBoss = false;
    medkitUsed = false;
    medkitHoldTimer = 0.0f;
    groundIntroActive = isGroundLevel();
    swordSwingTimer = 0.0f;
    ghostHitFlashTimer = 0.0f;

    mouseInitialized = false;
    resetInputStates();
}

void startLevel(int levelIndex) {
    currentLevel = levelIndex;
    gameState = STATE_PLAYING;
    gameOver = false;
    for (int i = 0; i < obstacleCount(); ++i) {
        activeObstacles[i] = levelObstacles[currentLevel][i];
        activeObstacles[i].dx = 0.0f;
        activeObstacles[i].dz = 0.0f;

        if (currentLevel == 1 || currentLevel == 2) {
            float vx = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * 0.95f;
            float vz = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * 0.95f;
            if (fabs(vx) < 0.25f) vx = (vx < 0.0f) ? -0.25f : 0.25f;
            if (fabs(vz) < 0.25f) vz = (vz < 0.0f) ? -0.25f : 0.25f;
            activeObstacles[i].dx = vx;
            activeObstacles[i].dz = vz;
        }
    }
    resetLevelCore();
    lastTickMs = glutGet(GLUT_ELAPSED_TIME);
    cout << "Entered Level " << (currentLevel + 1) << " (" << levelNames[currentLevel] << ")\n";
}

bool doorBlocksPassage() {
    return doorAngle < 80.0f;
}

float obstacleTopAtIndex(int obstacleIndex) {
    return obstacleHeightByLevel[currentLevel][obstacleIndex];
}

bool isInsideObstacleFootprint(const Obstacle& obstacle, float x, float z, float padding = PLAYER_RADIUS) {
    float minX = obstacle.x - obstacle.sx * 0.5f - padding;
    float maxX = obstacle.x + obstacle.sx * 0.5f + padding;
    float minZ = obstacle.z - obstacle.sz * 0.5f - padding;
    float maxZ = obstacle.z + obstacle.sz * 0.5f + padding;
    return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
}

float supportSurfaceHeightAt(float x, float z) {
    float supportHeight = 0.0f;

    if (isGroundLevel()) {
        return supportHeight;
    }

    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];
        float top = obstacleTopAtIndex(i);
        if (top <= 0.0f) {
            continue;
        }

        if (isInsideObstacleFootprint(obstacle, x, z, PLAYER_RADIUS * 0.6f) && top > supportHeight) {
            supportHeight = top;
        }
    }

    return supportHeight;
}

bool collidesWithObstacle(float x, float z, float feetHeight) {
    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];
        float top = obstacleTopAtIndex(i);
        if (top <= 0.0f) {
            continue;
        }

        if (isInsideObstacleFootprint(obstacle, x, z) && feetHeight < (top - 0.05f)) {
            return true;
        }
    }
    return false;
}

bool canMoveTo(float newX, float newZ) {
    float halfSize = worldHalfSize();
    float minBound = -halfSize + WALL_THICKNESS + PLAYER_RADIUS + 0.15f; // Add tiny padding to prevent instant spawn deaths
    float maxBound = halfSize - WALL_THICKNESS - PLAYER_RADIUS - 0.15f;

    if (newX < minBound || newX > maxBound || newZ > maxBound) {
        return false;
    }

    if (newZ < minBound) {
        bool withinDoorOpening = fabs(newX) < (currentDoorWidth() * 0.5f - PLAYER_RADIUS * 0.5f);
        if (!(withinDoorOpening && !doorBlocksPassage())) {
            return false;
        }
    }

    float feetHeight = camY - PLAYER_EYE_HEIGHT;
    if (collidesWithObstacle(newX, newZ, feetHeight)) {
        damagePlayer(1, false);
        return false;
    }

    if (isGroundLevel() && ghostAlive) {
        float ghostDistance = distance2D(newX, newZ, ghostX, ghostZ);
        if (ghostDistance < GHOST_BODY_RADIUS) {
            damagePlayer(1, false);
            return false;
        }
    }

    return true;
}

void damagePlayer(int amount, bool bypassCooldown) {
    if (gameState != STATE_PLAYING) {
        return;
    }

    if (!bypassCooldown && damageCooldown > 0.0f) {
        return;
    }

    damageCooldown = 1.5f; // Increased cooldown to give a distinct flashing red warning
    playerLives -= amount;
    if (playerLives <= 0) {
        playerLives = 0;
        gameOver = true;
        gameState = STATE_GAME_OVER;
        resetInputStates();
        cout << "Game over. Health depleted or hit obstacle.\n";
        return;
    }

    cout << "Health lost. Health left: " << playerLives << "\n";
    // We no longer restart the level! You keep your progress but lose a life.
}

void renderKeyModel(float scale) {
    glPushMatrix();
    glScalef(scale, scale, scale);

    glColor3f(1.0f, 0.88f, 0.08f);

    glPushMatrix();
    glTranslatef(-0.42f, 0.08f, 0.0f);
    glScalef(0.70f, 0.70f, 0.70f);
    glutSolidTorus(0.10f, 0.24f, 14, 24);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.23f, 0.0f, 0.0f);
    glScalef(1.20f, 0.12f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.66f, -0.12f, 0.0f);
    glScalef(0.11f, 0.26f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.82f, 0.10f, 0.0f);
    glScalef(0.11f, 0.22f, 0.16f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

void drawRoomLights() {
    glColor3f(0.95f, 0.92f, 0.72f);
    drawBox(-2.4f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
    drawBox(0.0f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
    drawBox(2.4f, 3.85f, 0.0f, 0.45f, 0.10f, 0.80f);
}

void renderDoorOrGate() {
    float doorWidth = currentDoorWidth();
    float doorHeight = currentDoorHeight();
    float doorThickness = currentDoorThickness();
    float doorZ = currentDoorZ();
    float hingeX = -doorWidth * 0.5f;

    glPushMatrix();
    glTranslatef(hingeX, 0.0f, doorZ);
    glRotatef(-doorAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(doorWidth * 0.5f, doorHeight * 0.5f, 0.0f);

    if (isGroundLevel()) {
        glColor3f(0.45f, 0.48f, 0.52f);
        drawBox(0.0f, 0.0f, 0.0f, doorWidth, doorHeight, doorThickness);

        glColor3f(0.25f, 0.28f, 0.32f);
        for (int i = -2; i <= 2; ++i) {
            drawBox(i * 0.42f, 0.0f, 0.03f, 0.08f, doorHeight * 0.88f, doorThickness * 0.7f);
        }

        glColor3f(0.18f, 0.18f, 0.20f);
        drawBox(0.0f, doorHeight * 0.38f, 0.05f, doorWidth * 0.95f, 0.06f, doorThickness * 0.8f);
        drawBox(0.18f, -0.25f, 0.05f, 0.12f, 0.12f, doorThickness * 0.9f);
    } else {
        glColor3f(0.63f, 0.34f, 0.12f);
        drawBox(0.0f, 0.0f, 0.0f, doorWidth, doorHeight, doorThickness);

        glColor3f(0.45f, 0.22f, 0.08f);
        drawBox(0.0f, 0.6f, 0.06f, doorWidth * 0.84f, 0.08f, doorThickness * 0.8f);
        drawBox(0.0f, -0.2f, 0.06f, doorWidth * 0.84f, 0.08f, doorThickness * 0.8f);
        drawBox(-0.22f, -0.1f, 0.06f, 0.08f, doorHeight * 0.78f, doorThickness * 0.8f);

        glColor3f(0.78f, 0.68f, 0.15f);
        drawBox(0.35f, -0.05f, 0.08f, 0.10f, 0.18f, doorThickness * 0.8f);
    }

    glPopMatrix();
}

void renderObstacles() {
    for (int i = 0; i < obstacleCount(); ++i) {
        const Obstacle& obstacle = activeObstacles[i];

        if (isGroundLevel()) {
            if (i == 0 || i == 2) {
                glPushMatrix();
                glTranslatef(obstacle.x, 0.0f, obstacle.z);
                glColor3f(0.42f, 0.28f, 0.14f);
                drawBox(0.0f, 0.45f, 0.0f, obstacle.sx * 0.45f, 0.9f, obstacle.sz * 0.45f);
                glColor3f(0.16f, 0.50f, 0.24f);
                glTranslatef(0.0f, 1.20f, 0.0f);
                glutSolidSphere(obstacle.sx * 0.85f, 18, 18);
                glPopMatrix();
            } else {
                glPushMatrix();
                glTranslatef(obstacle.x, 0.24f, obstacle.z);
                glColor3f(0.56f, 0.56f, 0.58f);
                glScalef(obstacle.sx * 0.8f, 0.45f, obstacle.sz * 0.8f);
                glutSolidSphere(0.5f, 12, 12);
                glPopMatrix();
            }
        } else {
            glColor3f(0.36f, 0.20f, 0.22f);
            float height = obstacleTopAtIndex(i);
            drawBox(obstacle.x, height * 0.5f, obstacle.z, obstacle.sx, height, obstacle.sz);
        }
    }
}

void renderGroundPickups() {
    if (!isGroundLevel()) {
        return;
    }

    float spin = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.12f;

    if (!weaponClaimed) {
        glPushMatrix();
        glTranslatef(weaponPickupPos.x, 0.48f, weaponPickupPos.z);
        glRotatef(spin, 0.0f, 1.0f, 0.0f);

        glColor3f(0.26f, 0.18f, 0.12f);
        drawBox(0.0f, -0.02f, 0.0f, 0.13f, 0.52f, 0.13f);
        glColor3f(0.72f, 0.72f, 0.74f);
        drawBox(0.0f, 0.28f, 0.0f, 0.36f, 0.08f, 0.10f);
        glColor3f(0.84f, 0.86f, 0.92f);
        drawBox(0.0f, 0.62f, 0.0f, 0.08f, 0.78f, 0.10f);
        glColor3f(0.95f, 0.96f, 0.99f);
        drawBox(0.0f, 1.00f, 0.0f, 0.05f, 0.22f, 0.08f);

        glPopMatrix();
    }

    if (!healthPickupClaimed) {
        glPushMatrix();
        glTranslatef(healthPickupPos.x, 0.42f, healthPickupPos.z);
        glRotatef(-spin * 0.8f, 0.0f, 1.0f, 0.0f);
        glColor3f(0.90f, 0.20f, 0.22f);
        drawBox(0.0f, 0.0f, 0.0f, 0.62f, 0.34f, 0.42f);
        glColor3f(0.96f, 0.96f, 0.96f);
        drawBox(0.0f, 0.10f, 0.0f, 0.16f, 0.26f, 0.32f);
        drawBox(0.0f, 0.10f, 0.0f, 0.46f, 0.26f, 0.12f);
        glPopMatrix();
    }
}

void renderGhostBoss() {
    if (!isGroundLevel() || !ghostAlive) {
        return;
    }

    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));
    float hover = 1.08f + sin(time * 0.0045f) * 0.20f;
    float pulse = 0.75f + 0.25f * (sin(time * 0.0075f) * 0.5f + 0.5f);
    float hurtFlash = ghostHitFlashTimer > 0.0f ? (ghostHitFlashTimer / 0.25f) : 0.0f;
    float shake = hurtFlash * 0.10f * sin(time * 0.09f);

    glPushMatrix();
    glTranslatef(ghostX + shake, hover, ghostZ - shake * 0.4f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.70f + 0.30f * hurtFlash, 0.84f - 0.35f * hurtFlash, 0.98f - 0.45f * hurtFlash, 0.85f);
    glutSolidSphere(0.55f, 20, 20);

    glPushMatrix();
    glTranslatef(0.0f, -0.46f, 0.0f);
    glScalef(0.95f, 1.35f, 0.95f);
    glColor4f(0.64f + 0.22f * hurtFlash, 0.78f - 0.20f * hurtFlash, 0.95f - 0.25f * hurtFlash, 0.72f);
    glutSolidSphere(0.52f, 20, 20);
    glPopMatrix();

    glPushMatrix();
    glScalef(1.35f, 1.20f, 1.35f);
    glColor4f(0.56f, 0.70f, 0.95f, 0.18f + pulse * 0.12f);
    glutSolidSphere(0.58f, 20, 20);
    glPopMatrix();

    glColor3f(0.20f, 0.10f, 0.30f);
    drawBox(-0.16f, 0.08f, 0.46f, 0.10f, 0.10f, 0.04f);
    drawBox(0.16f, 0.08f, 0.46f, 0.10f, 0.10f, 0.04f);

    glColor3f(0.90f, 0.12f, 0.20f);
    drawBox(0.0f, -0.12f, 0.47f, 0.18f, 0.07f, 0.04f);

    glDisable(GL_BLEND);

    glPopMatrix();
}

void renderHeldSword() {
    if (!(gameState == STATE_PLAYING && isGroundLevel() && weaponClaimed)) {
        return;
    }

    float forwardX = cos(pitch) * sin(yaw);
    float forwardY = sin(pitch);
    float forwardZ = -cos(pitch) * cos(yaw);

    float rightX = cos(yaw);
    float rightZ = sin(yaw);

    float upX = -sin(pitch) * sin(yaw);
    float upY = cos(pitch);
    float upZ = sin(pitch) * cos(yaw);

    float swing = swordSwingTimer > 0.0f ? (swordSwingTimer / 0.22f) : 0.0f;
    float swingAngle = sin((1.0f - swing) * 3.1415926f) * 36.0f;

    float baseX = camX + forwardX * 0.72f + rightX * 0.30f + upX * -0.20f;
    float baseY = camY + forwardY * 0.72f + upY * -0.20f;
    float baseZ = camZ + forwardZ * 0.72f + rightZ * 0.30f + upZ * -0.20f;

    glPushMatrix();
    glTranslatef(baseX, baseY, baseZ);
    glRotatef((-yaw * 57.29578f) + 28.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch * 57.29578f - 15.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-swingAngle, 0.0f, 0.0f, 1.0f);

    glColor3f(0.30f, 0.20f, 0.12f);
    drawBox(0.0f, -0.18f, 0.0f, 0.08f, 0.42f, 0.08f);
    glColor3f(0.75f, 0.75f, 0.78f);
    drawBox(0.0f, 0.03f, 0.0f, 0.26f, 0.06f, 0.10f);
    glColor3f(0.86f, 0.88f, 0.94f);
    drawBox(0.0f, 0.52f, 0.0f, 0.06f, 0.95f, 0.11f);
    glColor3f(0.96f, 0.97f, 0.99f);
    drawBox(0.0f, 0.95f, 0.0f, 0.04f, 0.24f, 0.08f);

    if (swordSwingTimer > 0.0f) {
        float swingRatio = swordSwingTimer / 0.22f;
        if (swingRatio > 1.0f) swingRatio = 1.0f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.80f, 0.95f, 1.0f, 0.22f * swingRatio);
        drawBox(0.12f, 0.72f, 0.0f, 0.52f, 0.62f, 0.08f);
        glDisable(GL_BLEND);
    }

    glPopMatrix();
}

void renderKeys() {
    float spin = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.08f;
    for (int i = 0; i < keysRequired(); ++i) {
        if (keyTaken[i]) continue;

        const Vec2& keyPos = levelKeyPositions[currentLevel][i];
        float keyY = levelKeyHeights[currentLevel][i];
        glPushMatrix();
        glTranslatef(keyPos.x, keyY, keyPos.z);
        glRotatef(spin + i * 25.0f, 0.0f, 1.0f, 0.0f);
        renderKeyModel(isGroundLevel() ? 0.60f : 0.50f);
        glPopMatrix();
    }
}

void renderRoomLevel() {
    float halfSize = worldHalfSize();

    glColor3f(0.42f, 0.45f, 0.50f);
    drawBox(0.0f, -0.10f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    glColor3f(0.34f, 0.38f, 0.44f);
    drawBox(0.0f, -0.06f, 0.0f, halfSize * 1.95f, 0.03f, halfSize * 1.95f);

    glColor3f(0.10f, 0.12f, 0.17f);
    drawBox(0.0f, 4.0f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    float wallR = 0.30f - 0.03f * static_cast<float>(currentLevel);
    float wallG = 0.35f - 0.03f * static_cast<float>(currentLevel);
    float wallB = 0.50f - 0.02f * static_cast<float>(currentLevel);
    if (wallR < 0.20f) wallR = 0.20f;
    if (wallG < 0.24f) wallG = 0.24f;
    if (wallB < 0.40f) wallB = 0.40f;
    glColor3f(wallR, wallG, wallB);

    drawBox(-halfSize + WALL_THICKNESS * 0.5f, 2.0f, 0.0f, WALL_THICKNESS, 4.0f, halfSize * 2.0f);
    drawBox(halfSize - WALL_THICKNESS * 0.5f, 2.0f, 0.0f, WALL_THICKNESS, 4.0f, halfSize * 2.0f);
    drawBox(0.0f, 2.0f, halfSize - WALL_THICKNESS * 0.5f, halfSize * 2.0f, 4.0f, WALL_THICKNESS);

    float doorWidth = currentDoorWidth();
    float doorZ = currentDoorZ();
    float sideWidth = (halfSize * 2.0f - doorWidth) * 0.5f;
    float leftCenterX = -halfSize + sideWidth * 0.5f;
    float rightCenterX = halfSize - sideWidth * 0.5f;

    drawBox(leftCenterX, 2.0f, doorZ, sideWidth, 4.0f, WALL_THICKNESS);
    drawBox(rightCenterX, 2.0f, doorZ, sideWidth, 4.0f, WALL_THICKNESS);
    drawBox(0.0f, 3.5f, doorZ, doorWidth, 1.0f, WALL_THICKNESS);

    drawRoomLights();
}

void renderGroundLevel() {
    float halfSize = worldHalfSize();

    glColor3f(0.18f, 0.50f, 0.18f);
    drawBox(0.0f, -0.12f, 0.0f, halfSize * 2.0f, 0.24f, halfSize * 2.0f);

    glColor3f(0.14f, 0.40f, 0.15f);
    drawBox(0.0f, -0.07f, 0.0f, halfSize * 1.70f, 0.05f, halfSize * 1.70f);

    glColor3f(0.52f, 0.41f, 0.24f);
    drawBox(0.0f, -0.03f, -1.0f, 2.35f, 0.04f, halfSize * 1.78f);
    drawBox(0.0f, -0.03f, 8.9f, 1.55f, 0.04f, 5.2f);
    drawBox(0.0f, -0.03f, -11.9f, 1.75f, 0.04f, 5.8f);

    glColor3f(0.88f, 0.86f, 0.55f);
    drawBox(6.7f, 5.9f, -6.8f, 0.9f, 0.9f, 0.9f);

    glColor3f(0.33f, 0.26f, 0.15f);
    drawBox(-halfSize + 0.15f, 1.0f, 0.0f, 0.25f, 2.0f, halfSize * 2.0f);
    drawBox(halfSize - 0.15f, 1.0f, 0.0f, 0.25f, 2.0f, halfSize * 2.0f);
    drawBox(0.0f, 1.0f, halfSize - 0.15f, halfSize * 2.0f, 2.0f, 0.25f);

    float doorWidth = currentDoorWidth();
    float doorZ = currentDoorZ();
    float sideWidth = (halfSize * 2.0f - doorWidth) * 0.5f;

    glColor3f(0.40f, 0.32f, 0.22f);
    drawBox(-halfSize + sideWidth * 0.5f, 1.0f, doorZ, sideWidth, 2.0f, 0.25f);
    drawBox(halfSize - sideWidth * 0.5f, 1.0f, doorZ, sideWidth, 2.0f, 0.25f);
    drawBox(0.0f, 2.0f, doorZ, doorWidth, 0.65f, 0.25f);

    glColor3f(0.48f, 0.36f, 0.20f);
    drawBox(0.0f, -0.02f, doorZ + 0.85f, 2.2f, 0.04f, 1.4f);
    drawBox(0.0f, -0.02f, doorZ + 3.20f, 2.5f, 0.04f, 1.5f);

    glColor3f(0.85f, 0.86f, 0.90f);
    drawBox(-5.8f, 1.0f, 2.2f, 0.9f, 2.0f, 0.9f);
    drawBox(5.4f, 1.0f, -2.0f, 1.1f, 2.0f, 1.1f);
    drawBox(0.5f, 1.0f, -4.8f, 1.5f, 2.0f, 1.5f);
}

void drawHUDBar(float x, float y, float w, float h, float ratio, float r, float g, float b) {
    ratio = clampf(ratio, 0.0f, 1.0f);
    glColor3f(0.15f, 0.15f, 0.15f);
    drawFilledRect2D(x, y, w, h);
    glColor3f(r, g, b);
    drawFilledRect2D(x + 2.0f, y + 2.0f, (w - 4.0f) * ratio, h - 4.0f);
}

void drawHUD() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    char line1[180];
    char line2[180];
    char timerLine[64];
    int timeSeconds = static_cast<int>(levelTimeRemaining);
    if (timeSeconds < 0) timeSeconds = 0;
    int minutes = timeSeconds / 60;
    int seconds = timeSeconds % 60;
    snprintf(timerLine, sizeof(timerLine), "Time: %02d:%02d", minutes, seconds);

    snprintf(line1, sizeof(line1), "Story: Escape the rooms, then survive the ground. Level %d (%s)", currentLevel + 1, levelNames[currentLevel]);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(15.0f, static_cast<float>(h - 25), line1);

    if (gameState == STATE_PLAYING) {
        if (isGroundLevel()) {
            snprintf(line2, sizeof(line2), "Ghost HP: %d/%d   Sword: %s   Medkit: %s   Health: %d/%d", ghostHealth, GHOST_MAX_HEALTH, weaponClaimed ? "Claimed" : "Not Claimed", (!healthPickupClaimed ? "Not Claimed" : (medkitUsed ? "Used" : "Ready")), playerLives, MAX_LIVES);
        } else {
            snprintf(line2, sizeof(line2), "Keys: %d / %d   Obstacles: %d   Health: %d/%d   Difficulty: %s", keysCollected, keysRequired(), obstacleCount(), playerLives, MAX_LIVES, levelNames[currentLevel]);
        }
        glColor3f(0.95f, 0.95f, 0.7f);
        drawText2D(15.0f, static_cast<float>(h - 52), line2);

        if (isGroundLevel()) {
            if (groundIntroActive) {
                glColor3f(0.95f, 0.95f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Read all rules, then press ENTER to start this ground fight.");
            } else if (!weaponClaimed) {
                glColor3f(1.0f, 0.95f, 0.2f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Claim the sword first (Press E near sword). Then left-click to attack ghost.");
            } else if (ghostAlive) {
                glColor3f(1.0f, 0.78f, 0.35f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Fight the ghost with LEFT CLICK. Claim medkit and hold 4 to use it.");
            } else if (!doorOpening) {
                glColor3f(0.7f, 1.0f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Ghost defeated. Escape door is unlocking.");
            } else {
                glColor3f(0.7f, 1.0f, 1.0f);
                drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Run through the opened escape door to complete the game.");
            }
        } else if (keysCollected < keysRequired()) {
            glColor3f(1.0f, 0.95f, 0.2f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Find keys (some are above obstacles). Press E near key. Space = jump.");
        } else if (!doorOpening) {
            glColor3f(0.7f, 1.0f, 0.7f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Go to the door and press E to unlock this level.");
        } else {
            glColor3f(0.7f, 1.0f, 1.0f);
            drawText2D(15.0f, static_cast<float>(h - 80), "Objective: Walk through the opened exit.");
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DRightAligned(static_cast<float>(w - 15), static_cast<float>(h - 25), timerLine);
        drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 55), 220.0f, 18.0f, static_cast<float>(playerLives) / static_cast<float>(MAX_LIVES), 0.25f, 0.95f, 0.35f);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 75), "Health");

        if (isGroundLevel() && healthPickupClaimed && !medkitUsed && !groundIntroActive) {
            float medkitRatio = medkitHoldTimer / MEDKIT_HOLD_REQUIRED;
            if (medkitRatio < 0.0f) medkitRatio = 0.0f;
            if (medkitRatio > 1.0f) medkitRatio = 1.0f;
            drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 102), 220.0f, 14.0f, medkitRatio, 0.95f, 0.25f, 0.25f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 118), "Hold 4 to use medkit");
        }

        if (isGroundLevel() && !groundIntroActive && ghostAlive) {
            float ghostRatio = static_cast<float>(ghostHealth) / static_cast<float>(GHOST_MAX_HEALTH);
            if (ghostRatio < 0.0f) ghostRatio = 0.0f;
            if (ghostRatio > 1.0f) ghostRatio = 1.0f;
            float barW = 300.0f;
            float barX = static_cast<float>(w) * 0.5f - barW * 0.5f;
            drawHUDBar(barX, static_cast<float>(h - 55), barW, 16.0f, ghostRatio, 0.88f, 0.20f, 0.22f);
            glColor3f(1.0f, 0.92f, 0.92f);
            drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h - 74), "GHOST HEALTH");
        }
    }

    if (gameState == STATE_PLAYING) {
        float doorDist = distance2D(camX, camZ, 0.0f, currentDoorZ());
        if (doorDist < 3.0f) {
            if (!doorOpening) {
                if (isGroundLevel()) {
                    glColor3f(1.0f, 0.3f, 0.3f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "ESCAPE DOOR SEALED");
                    if (ghostAlive) {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Defeat the ghost to unlock the door.");
                    } else {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Ghost down. Door mechanism is unlocking...");
                    }
                } else if (keysCollected < keysRequired()) {
                    glColor3f(1.0f, 0.3f, 0.3f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "DOOR LOCKED");
                    char msg[64]; 
                    snprintf(msg, sizeof(msg), "Need %d more key(s)!", keysRequired() - keysCollected);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, msg);
                } else {
                    glColor3f(1.0f, 0.8f, 0.2f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "DOOR LOCKED (Keys Acquired)");
                    
                    if (doorDist < 1.65f) {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Press E to Insert Keys & Unlock");
                    } else {
                        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Walk closer to unlock");
                    }
                }
            } else if (doorAngle < 80.0f) {
                glColor3f(0.3f, 1.0f, 0.3f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, isGroundLevel() ? "ESCAPE DOOR UNLOCKED!" : "DOOR UNLOCKED!");
                drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, "Opening...");
            }
        }
    }

    if (levelBannerFrames > 0 && gameState == STATE_PLAYING) {
        char levelBanner[128];
        snprintf(levelBanner, sizeof(levelBanner), "LEVEL %d - %s", currentLevel + 1, levelNames[currentLevel]);
        glColor3f(0.4f, 1.0f, 0.9f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.54f, levelBanner);
    }

    if (damageCooldown > 0.0f && gameState == STATE_PLAYING) {
        // Red flashing effect for getting hurt
        float alpha = (damageCooldown / 1.5f) * 0.5f; // Max 50% opacity
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.0f, 0.0f, alpha);
        
        // Semi-transparent red border/fill around screen 
        drawFilledRect2D(0, 0, static_cast<float>(w), static_cast<float>(h)); 
        
        glColor3f(1.0f, 1.0f, 1.0f);
        // Add bold alert text
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.40f, "HEALTH COMPROMISED!");
        glDisable(GL_BLEND);
    }

    if (gameCompleted && gameState == STATE_NAME_ENTRY) {
        glColor3f(0.4f, 1.0f, 0.4f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.52f, "YOU COMPLETED EASY + MEDIUM + HARD + GROUND.");
        drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.47f, "A NEW CHAPTER IS FINISHED. PRESS ESC TO QUIT.");
    }

    if (isGroundLevel() && gameState == STATE_PLAYING && groundIntroActive) {
        float panelW = static_cast<float>(w) * 0.72f;
        float panelH = static_cast<float>(h) * 0.72f;
        float panelX = (static_cast<float>(w) - panelW) * 0.5f;
        float panelY = (static_cast<float>(h) - panelH) * 0.5f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.02f, 0.04f, 0.06f, 0.80f);
        drawFilledRect2D(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));

        glColor4f(0.08f, 0.12f, 0.18f, 0.95f);
        drawFilledRect2D(panelX, panelY, panelW, panelH);
        glColor4f(0.18f, 0.44f, 0.58f, 1.0f);
        drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

        glDisable(GL_BLEND);

        float y = panelY + panelH - 48.0f;
        glColor3f(0.64f, 0.94f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "GROUND LEVEL RULES - FINAL FIGHT");

        y -= 55.0f;
        glColor3f(0.95f, 0.95f, 0.90f);
        drawText2D(panelX + 28.0f, y, "1) Claim the sword first (Press E near the sword pickup)."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "2) Attack ghost only with LEFT MOUSE CLICK."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "3) Claim medkit (Press E near medkit), then HOLD 4 to use it."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "4) Ghost contact damages you, so keep distance and strike smartly."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "5) After ghost dies, door opens automatically."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "6) Run down the long road and cross the door to complete game."); y -= 46.0f;

        glColor3f(0.45f, 1.0f, 0.65f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "PRESS ENTER TO START GROUND BATTLE");
    }

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawRulesScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float panelX = static_cast<float>(w) * 0.18f;
    float panelY = static_cast<float>(h) * 0.15f;
    float panelW = static_cast<float>(w) * 0.64f;
    float panelH = static_cast<float>(h) * 0.70f;

    glColor3f(0.12f, 0.05f, 0.05f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.35f, 0.15f, 0.15f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(1.0f, 0.45f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 48.0f, "GAME RULES - SURVIVAL PROTOCOL");

    float y = panelY + panelH - 100.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 40.0f, y, "1) Each level has a STRICT 20-second timer!"); y -= 35.0f;
    drawText2D(panelX + 40.0f, y, "2) Find and collect all keys before time runs out."); y -= 35.0f;
    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2D(panelX + 40.0f, y, "3) DO NOT TOUCH OBSTACLES."); y -= 25.0f;
    drawText2D(panelX + 60.0f, y, "Touching obstacles = Instant Health Lost!"); y -= 35.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 40.0f, y, "4) Press E to interact (Pick up keys, unlock doors)."); y -= 35.0f;
    drawText2D(panelX + 40.0f, y, "5) Press SPACE to jump over gaps and obstacles."); y -= 45.0f;

    glColor3f(0.45f, 1.0f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, y, "PRESS ENTER TO START YOUR ESCAPE");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawMenuScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float t = static_cast<float>(glutGet(GLUT_ELAPSED_TIME));

    if (startScreenTexture == 0) {
        glBegin(GL_QUADS);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(0.0f, 0.0f);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(static_cast<float>(w), 0.0f);
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(static_cast<float>(w), static_cast<float>(h));
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(0.0f, static_cast<float>(h));
        glEnd();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float glowX = static_cast<float>(w) * 0.5f + sin(t * 0.0011f) * static_cast<float>(w) * 0.25f;
        float glowY = static_cast<float>(h) * 0.72f + cos(t * 0.0013f) * 36.0f;
        glColor4f(0.10f, 0.60f, 0.85f, 0.12f);
        drawFilledRect2D(glowX - 220.0f, glowY - 100.0f, 440.0f, 200.0f);
        glColor4f(0.05f, 0.35f, 0.60f, 0.10f);
        drawFilledRect2D(glowX - 330.0f, glowY - 170.0f, 660.0f, 340.0f);
        glDisable(GL_BLEND);
    }

    if (startScreenTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w, h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
        
        // Add a dark semi-transparent overlay over the image so it looks slightly blurry/dim
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.05f, 0.05f, 0.1f, 0.70f);
        drawFilledRect2D(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
        glDisable(GL_BLEND);
    }

    float panelX = static_cast<float>(w) * 0.22f;
    float panelY = static_cast<float>(h) * 0.15f;
    float panelW = static_cast<float>(w) * 0.56f;
    float panelH = static_cast<float>(h) * 0.72f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.09f, 0.13f, 0.88f);
    drawFilledRect2D(panelX - 8.0f, panelY - 8.0f, panelW + 16.0f, panelH + 16.0f);
    glDisable(GL_BLEND);

    glColor3f(0.08f, 0.14f, 0.18f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.15f, 0.28f, 0.34f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(0.52f, 1.0f, 0.80f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 72.0f, "3D ESCAPE ROOM");
    glColor3f(0.85f, 0.93f, 0.98f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 106.0f, "Rooms, climbing, timer pressure, and final survival.");

    float buttonW = panelW * 0.60f;
    float buttonH = 46.0f;
    float buttonX = panelX + (panelW - buttonW) * 0.5f;
    float firstButtonY = panelY + panelH * 0.52f;

    for (int i = 0; i < MENU_ITEMS; ++i) {
        float y = firstButtonY - i * 72.0f;
        bool selected = (i == menuSelection);

        glColor3f(selected ? 0.11f : 0.08f, selected ? 0.42f : 0.18f, selected ? 0.36f : 0.23f);
        drawFilledRect2D(buttonX, y, buttonW, buttonH);

        glColor3f(selected ? 0.55f : 0.24f, selected ? 0.98f : 0.45f, selected ? 0.85f : 0.52f);
        drawFilledRect2D(buttonX + 2.0f, y + buttonH - 5.0f, buttonW - 4.0f, 2.0f);

        if (selected) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.20f, 0.92f, 0.72f, 0.18f);
            drawFilledRect2D(buttonX - 3.0f, y - 3.0f, buttonW + 6.0f, buttonH + 6.0f);
            glDisable(GL_BLEND);
        }

        if (selected) {
            glColor3f(0.95f, 1.0f, 0.96f);
            drawText2D(buttonX + 18.0f, y + 29.0f, "> ");
        }

        glColor3f(selected ? 0.95f : 0.82f, selected ? 1.0f : 0.90f, selected ? 0.95f : 0.95f);
        drawText2DCentered(buttonX + buttonW * 0.5f + 12.0f, y + 29.0f, menuLabels[i]);
    }

    glColor3f(0.70f, 0.78f, 0.82f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + 38.0f, "Use W/S or Up/Down to navigate. Press Enter to select.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawHighScoreScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float panelX = static_cast<float>(w) * 0.14f;
    float panelY = static_cast<float>(h) * 0.12f;
    float panelW = static_cast<float>(w) * 0.72f;
    float panelH = static_cast<float>(h) * 0.76f;

    glColor3f(0.07f, 0.12f, 0.19f);
    drawFilledRect2D(panelX, panelY, panelW, panelH);
    glColor3f(0.14f, 0.34f, 0.45f);
    drawFilledRect2D(panelX + 4.0f, panelY + panelH - 6.0f, panelW - 8.0f, 2.5f);

    glColor3f(0.4f, 1.0f, 0.9f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 48.0f, "HIGH SCORES");

    float colRank = panelX + 44.0f;
    float colName = panelX + 110.0f;
    float colScore = panelX + panelW * 0.52f;
    float colLives = panelX + panelW * 0.70f;
    float colBonus = panelX + panelW * 0.84f;

    glColor3f(0.78f, 0.9f, 0.98f);
    drawText2D(colRank, panelY + panelH - 92.0f, "#");
    drawText2D(colName, panelY + panelH - 92.0f, "NAME");
    drawText2D(colScore, panelY + panelH - 92.0f, "SCORE");
    drawText2D(colLives, panelY + panelH - 92.0f, "HEALTH");
    drawText2D(colBonus, panelY + panelH - 92.0f, "BONUS");

    glColor3f(0.22f, 0.42f, 0.55f);
    drawFilledRect2D(panelX + 24.0f, panelY + panelH - 107.0f, panelW - 48.0f, 2.0f);

    float y = panelY + panelH - 140.0f;
    if (highScores.empty()) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "No scores yet.");
    } else {
        int count = static_cast<int>(highScores.size());
        if (count > 10) count = 10;
        for (int i = 0; i < count; ++i) {
            if (i % 2 == 0) {
                glColor3f(0.10f, 0.17f, 0.26f);
                drawFilledRect2D(panelX + 18.0f, y - 19.0f, panelW - 36.0f, 28.0f);
            }

            char rankText[8];
            char scoreText[20];
            char livesText[12];
            char bonusText[20];

            snprintf(rankText, sizeof(rankText), "%d", i + 1);
            snprintf(scoreText, sizeof(scoreText), "%d", highScores[i].score);
            snprintf(livesText, sizeof(livesText), "%d", highScores[i].livesLeft);
            snprintf(bonusText, sizeof(bonusText), "%d", highScores[i].bonusTime);

            glColor3f(0.95f, 0.95f, 0.88f);
            drawText2D(colRank, y, rankText);
            drawText2D(colName, y, highScores[i].name.c_str());
            drawText2D(colScore, y, scoreText);
            drawText2D(colLives, y, livesText);
            drawText2D(colBonus, y, bonusText);

            y -= 32.0f;
        }
    }

    glColor3f(0.75f, 0.8f, 0.85f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + 24.0f, "Press Esc to return.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawNameEntryScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(0.45f, 1.0f, 0.55f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.66f, "YOU ESCAPED!");

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.59f, "Enter your name for the high score list:");

    char nameLine[48];
    snprintf(nameLine, sizeof(nameLine), "%s_", playerNameLength == 0 ? "" : playerName);
    glColor3f(0.9f, 0.95f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.50f, nameLine);

    char scoreLine[120];
    snprintf(scoreLine, sizeof(scoreLine), "Current Score: %d", pendingScore);
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.41f, scoreLine);

    glColor3f(0.75f, 0.75f, 0.75f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.28f, "Type letters, Backspace deletes, Enter saves.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawGameOverScreen() {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.60f, "GAME OVER");
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.52f, "All health is gone.");
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "Press M for menu or Esc to quit.");

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void renderWorld() {
    if (isGroundLevel()) {
        renderGroundLevel();
    } else {
        renderRoomLevel();
    }

    renderObstacles();
    renderGroundPickups();
    renderGhostBoss();
    renderKeys();
    renderDoorOrGate();
    renderHeldSword();
}

void display() {
    if (gameState == STATE_GAME_OVER) {
        glClearColor(0.07f, 0.02f, 0.02f, 1.0f);
    } else if (gameState == STATE_NAME_ENTRY) {
        glClearColor(0.04f, 0.10f, 0.06f, 1.0f);
    } else if (gameState == STATE_HIGHSCORES) {
        glClearColor(0.05f, 0.05f, 0.12f, 1.0f);
    } else if (gameState == STATE_MENU) {
        glClearColor(0.06f, 0.08f, 0.09f, 1.0f);
    } else if (gameCompleted) {
        glClearColor(0.04f, 0.10f, 0.06f, 1.0f);
    } else if (isGroundLevel()) {
        glClearColor(0.52f, 0.74f, 0.95f, 1.0f);
    } else {
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (gameState == STATE_PLAYING) {
        float lookX = cos(pitch) * sin(yaw);
        float lookY = sin(pitch);
        float lookZ = -cos(pitch) * cos(yaw);

        gluLookAt(camX, camY, camZ,
                  camX + lookX, camY + lookY, camZ + lookZ,
                  0.0f, 1.0f, 0.0f);

        renderWorld();
        drawHUD();
    } else {
        if (gameState == STATE_RULES) {
            drawRulesScreen();
        } else if (gameState == STATE_MENU) {
            drawMenuScreen();
        } else if (gameState == STATE_HIGHSCORES) {
            drawHighScoreScreen();
        } else if (gameState == STATE_NAME_ENTRY) {
            drawNameEntryScreen();
        } else if (gameState == STATE_GAME_OVER) {
            drawGameOverScreen();
        }
    }

    glutSwapBuffers();
}

void tryInteraction() {
    if (gameState != STATE_PLAYING) return;

    if (isGroundLevel()) {
        bool interacted = false;

        if (!weaponClaimed && distance2D(camX, camZ, weaponPickupPos.x, weaponPickupPos.z) < INTERACT_PICKUP_RANGE) {
            weaponClaimed = true;
            interacted = true;
            cout << "Sword claimed. You can now fight with left mouse click.\n";
        }

        if (!healthPickupClaimed && distance2D(camX, camZ, healthPickupPos.x, healthPickupPos.z) < INTERACT_PICKUP_RANGE) {
            healthPickupClaimed = true;
            interacted = true;
            cout << "Medkit claimed. Hold key 4 to use it when needed.\n";
        }

        if (!interacted) {
            float doorDistance = distance2D(camX, camZ, 0.0f, currentDoorZ());
            if (doorDistance < 1.8f && !doorOpening) {
                cout << "Escape door is sealed. Defeat the ghost first.\n";
            }
        }
        return;
    }

    bool foundKey = false;
    for (int i = 0; i < keysRequired(); ++i) {
        if (keyTaken[i]) continue;

        const Vec2& keyPos = levelKeyPositions[currentLevel][i];
        float keyY = levelKeyHeights[currentLevel][i];
        
        float dist2D = distance2D(camX, camZ, keyPos.x, keyPos.z);
        float distY = fabs(camY - PLAYER_EYE_HEIGHT - keyY);
        
        // Greatly increased pickup radius (cylindrical check vs spherical)
        if (dist2D < 1.5f && distY < 2.0f) {
            keyTaken[i] = true;
            keysCollected++;
            foundKey = true;
            cout << "Key " << keysCollected << " collected in level " << (currentLevel + 1) << "\n";
        }
    }

    if (foundKey) return;

    float doorDistance = distance2D(camX, camZ, 0.0f, currentDoorZ());
    if (keysCollected >= keysRequired() && !doorOpening && doorDistance < 1.65f) {
        doorOpening = true;
        cout << "Exit unlocked for level " << (currentLevel + 1) << "\n";
    } else if (doorDistance < 1.65f && keysCollected < keysRequired()) {
        cout << "Exit locked. Need " << (keysRequired() - keysCollected) << " more key(s).\n";
    }
}

void tryGhostAttack() {
    if (!(gameState == STATE_PLAYING && isGroundLevel() && !groundIntroActive && ghostAlive)) {
        return;
    }

    if (!weaponClaimed) {
        cout << "You need the sword first. Claim it before attacking.\n";
        return;
    }

    if (weaponAttackCooldown > 0.0f) {
        cout << "Sword swing is recovering. Click after a moment.\n";
        return;
    }

    float toGhostX = ghostX - camX;
    float toGhostZ = ghostZ - camZ;
    float dist = sqrt(toGhostX * toGhostX + toGhostZ * toGhostZ);
    if (dist > GHOST_WEAPON_RANGE) {
        cout << "Too far. Move closer to hit the ghost.\n";
        return;
    }

    float dirX = sin(yaw);
    float dirZ = -cos(yaw);
    float invDist = (dist > 0.0001f) ? (1.0f / dist) : 0.0f;
    float faceDot = dirX * (toGhostX * invDist) + dirZ * (toGhostZ * invDist);
    if (faceDot < 0.08f) {
        cout << "Face the ghost to land your sword hit.\n";
        return;
    }

    ghostHealth -= 1;
    swordSwingTimer = 0.22f;
    weaponAttackCooldown = 0.35f;
    ghostHitFlashTimer = 0.25f;
    cout << "Sword hit! Ghost health: " << ghostHealth << "\n";

    if (ghostHealth <= 0) {
        ghostHealth = 0;
        ghostAlive = false;
        groundDoorUnlockedByBoss = true;
        doorOpening = true;
        cout << "Ghost defeated! Escape door unlocking...\n";
    }
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(tolower(key));
    if (gameState == STATE_PLAYING) {
        if (isGroundLevel() && groundIntroActive) {
            if (lower == 13 || lower == 'e') {
                groundIntroActive = false;
                cout << "Ground battle started. Good luck.\n";
            } else if (lower == 27) {
                startMenu();
            }
            return;
        }

        keyStates[lower] = true;

        if (lower == 'e') {
            tryInteraction();
        }

        if (key == ' ' && !airborne) {
            airborne = true;
            verticalVelocity = JUMP_VELOCITY;
            fallStartCamY = camY;
        }

        if (lower == 27) {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_MENU) {
        if (lower == 'w' || lower == 'k') {
            menuSelection = (menuSelection + MENU_ITEMS - 1) % MENU_ITEMS;
        } else if (lower == 's' || lower == 'j') {
            menuSelection = (menuSelection + 1) % MENU_ITEMS;
        } else if (lower == 13 || lower == 'e') {
            if (menuSelection == 0) {
                gameState = STATE_RULES;
                resetInputStates();
            } else if (menuSelection == 1) {
                openHighScoreScreen();
            } else {
                exit(0);
            }
        } else if (lower == 27) {
            exit(0);
        }
        return;
    }

    if (gameState == STATE_RULES) {
        if (lower == 13 || lower == 'e') {
            startGame();
        } else if (lower == 27) {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_HIGHSCORES) {
        if (lower == 27 || lower == 'm') {
            startMenu();
        }
        return;
    }

    if (gameState == STATE_NAME_ENTRY) {
        if (lower == 13) {
            string name = playerNameLength == 0 ? string("PLAYER") : string(playerName);
            addHighScore(name, pendingScore, playerLives, bonusTimeScore);
            startMenu();
            return;
        }

        if (lower == 8) {
            if (playerNameLength > 0) {
                playerName[--playerNameLength] = '\0';
            }
            return;
        }

        if (isalpha(lower) && playerNameLength < 15) {
            playerName[playerNameLength++] = static_cast<char>(toupper(lower));
            playerName[playerNameLength] = '\0';
        }
        return;
    }

    if (gameState == STATE_GAME_OVER) {
        if (lower == 'm' || lower == 27) {
            startMenu();
        }
        return;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(tolower(key));
    if (gameState == STATE_PLAYING) {
        keyStates[lower] = false;
    }
}

void specialKeyDown(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key >= 0 && key < 256) {
        specialStates[key] = true;
    }
}

void specialKeyUp(int key, int x, int y) {
    (void)x;
    (void)y;

    if (key >= 0 && key < 256) {
        specialStates[key] = false;
    }
}

void mouseLook(int x, int y) {
    if (gameState != STATE_PLAYING) {
        lastMouseX = x;
        lastMouseY = y;
        return;
    }

    if (!mouseInitialized) {
        lastMouseX = x;
        lastMouseY = y;
        mouseInitialized = true;
        return;
    }

    int deltaX = x - lastMouseX;
    int deltaY = y - lastMouseY;

    lastMouseX = x;
    lastMouseY = y;

    yaw += static_cast<float>(deltaX) * MOUSE_SENSITIVITY;
    pitch -= static_cast<float>(deltaY) * MOUSE_SENSITIVITY;
    pitch = clampf(pitch, -PITCH_LIMIT, PITCH_LIMIT);

    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    (void)x;
    (void)y;

    if (gameState != STATE_PLAYING || groundIntroActive) {
        return;
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && isGroundLevel()) {
        tryGhostAttack();
    }
}

void applyMovement(float dx, float dz) {
    float targetX = camX + dx;
    float targetZ = camZ + dz;

    if (canMoveTo(targetX, camZ)) {
        camX = targetX;
    }

    if (canMoveTo(camX, targetZ)) {
        camZ = targetZ;
    }
}

void updateVerticalMovement(float dt) {
    if (!airborne && fabs(verticalVelocity) < 0.0001f) {
        float supportHeight = supportSurfaceHeightAt(camX, camZ);
        float feetHeight = camY - PLAYER_EYE_HEIGHT;
        if (feetHeight > supportHeight + 0.03f) {
            airborne = true;
            verticalVelocity = 0.0f;
            fallStartCamY = camY;
        }
    }

    if (airborne || fabs(verticalVelocity) > 0.0001f) {
        verticalVelocity -= GRAVITY_ACCEL * dt;
        camY += verticalVelocity * dt;

        float supportHeight = supportSurfaceHeightAt(camX, camZ);
        float feetHeight = camY - PLAYER_EYE_HEIGHT;
        if (verticalVelocity <= 0.0f && feetHeight <= supportHeight) {
            camY = supportHeight + PLAYER_EYE_HEIGHT;
            float fallDrop = fallStartCamY - camY;
            airborne = false;
            verticalVelocity = 0.0f;

            if (fallDrop > FALL_DAMAGE_THRESHOLD) {
                cout << "Bad fall detected (drop: " << fallDrop << ").\n";
                damagePlayer(1, true);
            }
        }
    }
}

void handleLevelExit() {
    if (currentLevel < TOTAL_LEVELS - 1) {
        bonusTimeScore += static_cast<int>(levelTimeRemaining);
        cout << "Level " << (currentLevel + 1) << " complete. Moving to next level.\n";
        startLevel(currentLevel + 1);
    } else {
        bonusTimeScore += static_cast<int>(levelTimeRemaining);
        gameCompleted = true;
        gameState = STATE_NAME_ENTRY;
        resetInputStates();
        finishGameToHighScore();
        cout << "All levels complete!\n";
    }
}

void update(int value) {
    (void)value;

    int now = glutGet(GLUT_ELAPSED_TIME);
    if (lastTickMs == 0) {
        lastTickMs = now;
    }
    float dt = static_cast<float>(now - lastTickMs) / 1000.0f;
    lastTickMs = now;

    if (gameState == STATE_PLAYING) {
        if (swordSwingTimer > 0.0f) {
            swordSwingTimer -= dt;
            if (swordSwingTimer < 0.0f) {
                swordSwingTimer = 0.0f;
            }
        }

        if (ghostHitFlashTimer > 0.0f) {
            ghostHitFlashTimer -= dt;
            if (ghostHitFlashTimer < 0.0f) {
                ghostHitFlashTimer = 0.0f;
            }
        }

        if (damageCooldown > 0.0f) {
            damageCooldown -= dt;
            if (damageCooldown < 0.0f) {
                damageCooldown = 0.0f;
            }
        }

        if (weaponAttackCooldown > 0.0f) {
            weaponAttackCooldown -= dt;
            if (weaponAttackCooldown < 0.0f) {
                weaponAttackCooldown = 0.0f;
            }
        }

        if (isGroundLevel() && groundIntroActive) {
            if (levelBannerFrames > 0) {
                levelBannerFrames--;
            }
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        levelTimeRemaining -= dt;
        if (levelTimeRemaining <= 0.0f) {
            damagePlayer(1, true);
            if (gameState == STATE_PLAYING) {
                cout << "Time expired. Resetting timer and reducing health.\n";
                levelTimeRemaining = levelTimeLimitByLevel[currentLevel];
            }
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        float halfSize = worldHalfSize();
        for (int i = 0; i < obstacleCount(); ++i) {
            Obstacle& obs = activeObstacles[i];
            obs.x += obs.dx * dt;
            obs.z += obs.dz * dt;
            
            // Bounce off walls
            if (obs.x + obs.sx * 0.5f > halfSize || obs.x - obs.sx * 0.5f < -halfSize) {
                obs.dx = -obs.dx;
                obs.x += obs.dx * dt; 
            }
            if (obs.z + obs.sz * 0.5f > halfSize || obs.z - obs.sz * 0.5f < -halfSize) {
                obs.dz = -obs.dz;
                obs.z += obs.dz * dt;
            }
        }

        if (isGroundLevel() && ghostAlive) {
            float dxToPlayer = camX - ghostX;
            float dzToPlayer = camZ - ghostZ;
            float distToPlayer = sqrt(dxToPlayer * dxToPlayer + dzToPlayer * dzToPlayer);

            if (distToPlayer > 0.01f) {
                float strafeX = 0.0f;
                float strafeZ = 0.0f;
                float invDist = 1.0f / distToPlayer;
                float perpX = -dzToPlayer * invDist;
                float perpZ = dxToPlayer * invDist;
                float strafeFactor = sin(static_cast<float>(now) * 0.004f) * 0.80f;
                strafeX = perpX * strafeFactor;
                strafeZ = perpZ * strafeFactor;

                float chaseX = dxToPlayer * invDist;
                float chaseZ = dzToPlayer * invDist;

                float desiredX = chaseX * 0.82f + strafeX * 0.55f;
                float desiredZ = chaseZ * 0.82f + strafeZ * 0.55f;
                float desiredLen = sqrt(desiredX * desiredX + desiredZ * desiredZ);
                if (desiredLen > 0.0001f) {
                    desiredX /= desiredLen;
                    desiredZ /= desiredLen;
                }

                float speedBoost = weaponClaimed ? 1.20f : 1.0f;
                float step = GHOST_MOVE_SPEED * speedBoost * dt;
                if (step > distToPlayer) {
                    step = distToPlayer;
                }

                ghostX += desiredX * step;
                ghostZ += desiredZ * step;

                float minBound = -worldHalfSize() + WALL_THICKNESS + GHOST_BODY_RADIUS + 0.2f;
                float maxBound = worldHalfSize() - WALL_THICKNESS - GHOST_BODY_RADIUS - 0.2f;
                ghostX = clampf(ghostX, minBound, maxBound);
                ghostZ = clampf(ghostZ, minBound, maxBound);
            }

            if (distance2D(camX, camZ, ghostX, ghostZ) < GHOST_TOUCH_DAMAGE_RANGE) {
                damagePlayer(1, false);
            }
        }

        if (isGroundLevel() && groundDoorUnlockedByBoss && !doorOpening) {
            doorOpening = true;
        }

        if (isGroundLevel() && healthPickupClaimed && !medkitUsed) {
            if (keyStates['4']) {
                medkitHoldTimer += dt;
                if (medkitHoldTimer >= MEDKIT_HOLD_REQUIRED) {
                    medkitUsed = true;
                    medkitHoldTimer = 0.0f;
                    int oldHealth = playerLives;
                    playerLives += 2;
                    if (playerLives > MAX_LIVES) {
                        playerLives = MAX_LIVES;
                    }
                    cout << "Medkit used. Health: " << oldHealth << " -> " << playerLives << "\n";
                }
            } else {
                medkitHoldTimer = 0.0f;
            }
        }

        float forwardX = sin(yaw);
        float forwardZ = -cos(yaw);
        float rightX = cos(yaw);
        float rightZ = sin(yaw);

        float inputX = 0.0f;
        float inputZ = 0.0f;

        if (keyStates['w']) inputZ += 1.0f;
        if (keyStates['s']) inputZ -= 1.0f;
        if (keyStates['a']) inputX -= 1.0f;
        if (keyStates['d']) inputX += 1.0f;

        if (inputX != 0.0f || inputZ != 0.0f) {
            // Normalize input so diagonal movement isn't faster
            float len = sqrt(inputX * inputX + inputZ * inputZ);
            inputX /= len;
            inputZ /= len;

            // Frame-rate independent speed scaling 
            // (assuming original 0.07 was tuned for ~16ms frame / 60 FPS)
            float moveSpeed = currentMoveSpeed() * (dt / 0.016666f);

            float dx = (inputZ * forwardX + inputX * rightX) * moveSpeed;
            float dz = (inputZ * forwardZ + inputX * rightZ) * moveSpeed;

            applyMovement(dx, dz);
        }

        updateVerticalMovement(dt);

        if (gameState != STATE_PLAYING) {
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        if (specialStates[GLUT_KEY_LEFT]) yaw -= ROT_SPEED;
        if (specialStates[GLUT_KEY_RIGHT]) yaw += ROT_SPEED;
        if (specialStates[GLUT_KEY_UP]) pitch += PITCH_SPEED;
        if (specialStates[GLUT_KEY_DOWN]) pitch -= PITCH_SPEED;

        pitch = clampf(pitch, -PITCH_LIMIT, PITCH_LIMIT);

        if (doorOpening && doorAngle < 92.0f) {
            float openSpeed = 1.0f + static_cast<float>(currentLevel) * 0.7f;
            doorAngle += openSpeed;
            if (doorAngle > 92.0f) {
                doorAngle = 92.0f;
            }
        }

        if (doorAngle > 80.0f && camZ < (currentDoorZ() - 0.4f)) {
            handleLevelExit();
        }

        if (levelBannerFrames > 0) {
            levelBannerFrames--;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void reshape(int w, int h) {
    if (h == 0) {
        h = 1;
    }

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70.0, static_cast<double>(w) / static_cast<double>(h), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    srand(static_cast<unsigned int>(time(nullptr)));
    loadHighScores();
    loadBackgroundTexture();
    playerLives = MAX_LIVES;
    gameOver = false;
    gameCompleted = false;
    startMenu();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 650);
    glutCreateWindow("3D Escape Room - Multi Level");

    init();
    reshape(900, 650);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouseButton);
    glutPassiveMotionFunc(mouseLook);
    glutMotionFunc(mouseLook);
    glutTimerFunc(16, update, 0);

    cout << "Escape room ready. Use the menu to start.\n";
    cout << "Controls in game: WASD move, Mouse/Arrow keys look, E interact, Left click attack (ground), Hold 4 medkit, Esc menu.\n";

    glutMainLoop();
    return 0;
}
