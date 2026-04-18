
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

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

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

const char* SFX_FOLDER_PRIMARY = "assets/sfx/";
const char* SFX_FOLDER_FALLBACK = "../assets/sfx/";
const char* SFX_FOLDER_ALT1 = "sfx/";
const char* SFX_FOLDER_ALT2 = "../sfx/";
const char* SFX_FILE_CLAIM = "claim.wav";
const char* SFX_FILE_UNLOCK = "unlock.wav";
const char* SFX_FILE_LEVEL_UP = "level_up.wav";
const char* SFX_FILE_COMPLETE_GAME = "complete_game.wav";
const char* SFX_FILE_GAME_OVER = "game_over.wav";
const float SFX_MASTER_VOLUME = 0.38f; // 0.0 - 1.0, reduce if too loud

const int TOTAL_LEVELS = 4;
const int MAX_KEYS = 3;
const int MAX_OBSTACLES = 6;

const char* levelNames[TOTAL_LEVELS] = {"Easy", "Medium", "Hard", "Ground"};
const LevelType levelTypes[TOTAL_LEVELS] = {LEVEL_ROOM, LEVEL_ROOM, LEVEL_ROOM, LEVEL_GROUND};
const float levelHalfSizes[TOTAL_LEVELS] = {ROOM_HALF_SIZE, ROOM_HALF_SIZE, ROOM_HALF_SIZE, GROUND_HALF_SIZE};
const int keysRequiredByLevel[TOTAL_LEVELS] = {1, 2, 3, 0};
const int obstacleCountByLevel[TOTAL_LEVELS] = {3, 4, 4, 3};
const float moveSpeedScale[TOTAL_LEVELS] = {1.00f, 0.92f, 0.84f, 1.00f};
const float pickupRadiusByLevel[TOTAL_LEVELS] = {0.95f, 0.82f, 0.72f, 0.90f};
const float levelTimeLimitByLevel[TOTAL_LEVELS] = {10.0f, 20.0f, 30.0f, 20.0f};
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
    unsigned char* data = stbi_load("assets/images/image.png", &width, &height, &channels, 4);
    if (!data) data = stbi_load("../assets/images/image.png", &width, &height, &channels, 4);
    if (data) {
        glGenTextures(1, &startScreenTexture);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
        cout << "Loaded assets/images/image.png successfully.\n";
    } else {
        cout << "Failed to load assets/images/image.png. Reason: " << stbi_failure_reason() << "\n";
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
bool victorySoundPlayed = false;
bool gameOverSoundPlayed = false;
bool isFullscreen = false;
int windowedX = 60;
int windowedY = 60;
int windowedWidth = 900;
int windowedHeight = 650;
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
const float GHOST_WEAPON_RANGE = 15.0f; // Gun range - much longer than sword
const float GHOST_BODY_RADIUS = 0.85f;
const float INTERACT_PICKUP_RANGE = 1.60f;
const float MEDKIT_HOLD_REQUIRED = 1.0f;

const int NUM_WARRIORS = 6;
bool ghostAlive[NUM_WARRIORS] = {false, false, false, false, false, false};
int  ghostHealth[NUM_WARRIORS];
float ghostX[NUM_WARRIORS];
float ghostZ[NUM_WARRIORS];
float ghostHitFlashTimer[NUM_WARRIORS] = {0};
// Spread-out starting positions
const float WARRIOR_START_X[NUM_WARRIORS] = {  0.0f, -7.0f,  7.0f, -5.0f,  5.0f,  0.0f };
const float WARRIOR_START_Z[NUM_WARRIORS] = { -6.0f, -2.0f, -2.0f,  6.0f,  6.0f, 10.0f };
Vec2 weaponPickupPos = {-7.0f, 10.5f};
Vec2 healthPickupPos = {7.0f, 9.8f};
bool weaponClaimed = false;
bool healthPickupClaimed = false;
bool groundDoorUnlockedByBoss = false;
bool medkitUsed = false;
float medkitHoldTimer = 0.0f;
bool groundIntroActive = false;
float swordSwingTimer = 0.0f;
bool wave2Spawned = false;

// ---- Gun / shooting variables ----
float gunRecoilTimer = 0.0f;
float muzzleFlashTimer = 0.0f;

// ---- Torch flicker variables ----
struct TorchFlicker {
    float rate;
    float phase;
    float intensity;
};
TorchFlicker torchFlickers[4];

// ---- Lava crack pulse ----
float lavaPulsePhase = 0.0f;

// ---- Bonus Clock Pickup (Level 3+) ----
bool bonusClockActive = false;
float bonusClockX = 0.0f;
float bonusClockZ = 0.0f;
float bonusClockSpawnTimer = 0.0f; // countdown to next spawn
float bonusClockNextInterval = 7.0f; // random 5-11
float bonusClockBannerTimer = 0.0f; // "+8 SEC!" display timer
const float BONUS_CLOCK_RADIUS = 0.9f;
const float BONUS_CLOCK_TIME_BONUS = 8.0f;

// ---- Obstacle hit counter (5 hits = instant game over) ----
int obstacleHitCount = 0;
const int MAX_OBSTACLE_HITS = 5;

bool keyStates[256] = {false};
bool specialStates[256] = {false};
bool mouseInitialized = false;
int lastMouseX = 0;
int lastMouseY = 0;

void startLevel(int levelIndex);
void damagePlayer(int amount, bool bypassCooldown = false);
void handleObstacleHit();
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

float randFloat(float lo, float hi) {
    return lo + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (hi - lo);
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

void drawPanelBox2D(float x, float y, float w, float h,
                    float bodyR, float bodyG, float bodyB,
                    float borderR, float borderG, float borderB) {
    glColor3f(borderR * 0.45f, borderG * 0.45f, borderB * 0.45f);
    drawFilledRect2D(x - 3.0f, y - 3.0f, w + 6.0f, h + 6.0f);

    glColor3f(borderR, borderG, borderB);
    drawFilledRect2D(x, y, w, h);

    glColor3f(bodyR, bodyG, bodyB);
    drawFilledRect2D(x + 2.0f, y + 2.0f, w - 4.0f, h - 4.0f);

    glColor3f(borderR * 1.10f, borderG * 1.10f, borderB * 1.10f);
    drawFilledRect2D(x + 6.0f, y + h - 10.0f, w - 12.0f, 3.0f);
}

void drawText2D(float x, float y, const char* text) {
    GLfloat originalColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glGetFloatv(GL_CURRENT_COLOR, originalColor);

    glColor4f(0.02f, 0.03f, 0.05f, 0.85f);
    glRasterPos2f(x + 1.3f, y - 1.3f);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }

    glColor4fv(originalColor);
    glRasterPos2f(x, y);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
}

int bitmapTextWidth(const char* text) {
    return glutBitmapLength(GLUT_BITMAP_9_BY_15, reinterpret_cast<const unsigned char*>(text));
}

void drawText2DRightAligned(float rightX, float y, const char* text) {
    drawText2D(rightX - static_cast<float>(bitmapTextWidth(text)), y, text);
}

void drawText2DCentered(float centerX, float y, const char* text) {
    drawText2D(centerX - static_cast<float>(bitmapTextWidth(text)) * 0.5f, y, text);
}

int timesRomanTextWidth(const char* text) {
    return glutBitmapLength(GLUT_BITMAP_TIMES_ROMAN_24, reinterpret_cast<const unsigned char*>(text));
}

void drawTimesRomanText2D(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *p);
    }
}

void drawTimesRomanText2DCentered(float centerX, float y, const char* text) {
    drawTimesRomanText2D(centerX - static_cast<float>(timesRomanTextWidth(text)) * 0.5f, y, text);
}

void drawStrokeText(float x, float y, float scale, float lineWidth, const char* text) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    glLineWidth(lineWidth);
    for (const char* p = text; *p; ++p) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    }
    glLineWidth(1.0f);
    glPopMatrix();
}

void drawStrokeTextCentered(float centerX, float y, float scale, float lineWidth, const char* text) {
    float width = 0.0f;
    for (const char* p = text; *p; ++p) {
        width += glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
    }
    width *= scale;
    drawStrokeText(centerX - width * 0.5f, y, scale, lineWidth, text);
}

void applySfxMasterVolume() {
#ifdef _WIN32
    float v = SFX_MASTER_VOLUME;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    DWORD channelVolume = static_cast<DWORD>(v * 65535.0f);
    DWORD packedVolume = (channelVolume & 0xFFFF) | ((channelVolume & 0xFFFF) << 16);
    waveOutSetVolume(reinterpret_cast<HWAVEOUT>(WAVE_MAPPER), packedVolume);
#endif
}

void playSoundEffect(const char* fileName) {
#ifdef _WIN32
    applySfxMasterVolume();

    auto tryPlay = [&](const string& path) -> bool {
        return PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT) != 0;
    };

    auto tryPlayMci = [&](const string& path) -> bool {
        static int slot = 0;
        char alias[32];
        snprintf(alias, sizeof(alias), "sfx%d", slot);
        slot = (slot + 1) % 8;

        char cmd[768];
        snprintf(cmd, sizeof(cmd), "close %s", alias);
        mciSendStringA(cmd, NULL, 0, NULL);

        snprintf(cmd, sizeof(cmd), "open \"%s\" type waveaudio alias %s", path.c_str(), alias);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
            return false;
        }

        snprintf(cmd, sizeof(cmd), "setaudio %s volume to %d", alias, static_cast<int>(SFX_MASTER_VOLUME * 1000.0f));
        mciSendStringA(cmd, NULL, 0, NULL);

        snprintf(cmd, sizeof(cmd), "play %s", alias);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
            snprintf(cmd, sizeof(cmd), "close %s", alias);
            mciSendStringA(cmd, NULL, 0, NULL);
            return false;
        }

        return true;
    };

    // 1) Try common working-directory relative paths
    const char* folders[] = {SFX_FOLDER_PRIMARY, SFX_FOLDER_FALLBACK, SFX_FOLDER_ALT1, SFX_FOLDER_ALT2};
    for (int i = 0; i < 4; ++i) {
        string path = string(folders[i]) + fileName;
        if (tryPlay(path) || tryPlayMci(path)) {
            cout << "SFX played: " << path << "\n";
            return;
        }
    }

    // 2) Try paths relative to executable location (works even when cwd changes)
    char exePath[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        string exeDir(exePath);
        size_t sep = exeDir.find_last_of("\\/");
        if (sep != string::npos) {
            exeDir = exeDir.substr(0, sep);

            const char* exeRelativeFolders[] = {
                "\\assets\\sfx\\",
                "\\..\\assets\\sfx\\",
                "\\sfx\\",
                "\\..\\sfx\\",
                "\\"
            };

            for (int i = 0; i < 5; ++i) {
                string path = exeDir + exeRelativeFolders[i] + fileName;
                if (tryPlay(path) || tryPlayMci(path)) {
                    cout << "SFX played: " << path << "\n";
                    return;
                }
            }
        }
    }

    cout << "SFX missing/unplayable: " << fileName << "\n";
#else
    (void)fileName;
#endif
}

void playVictorySound() {
    if (victorySoundPlayed) {
        return;
    }
    victorySoundPlayed = true;
    playSoundEffect(SFX_FILE_COMPLETE_GAME);
}

void playGameOverSound() {
    if (gameOverSoundPlayed) {
        return;
    }
    gameOverSoundPlayed = true;
    playSoundEffect(SFX_FILE_GAME_OVER);
}

void playClaimSound() {
    playSoundEffect(SFX_FILE_CLAIM);
}

void playUnlockSound() {
    playSoundEffect(SFX_FILE_UNLOCK);
}

void playLevelUpSound() {
    playSoundEffect(SFX_FILE_LEVEL_UP);
}

void toggleFullscreen() {
    if (!isFullscreen) {
        windowedX = glutGet(GLUT_WINDOW_X);
        windowedY = glutGet(GLUT_WINDOW_Y);
        windowedWidth = glutGet(GLUT_WINDOW_WIDTH);
        windowedHeight = glutGet(GLUT_WINDOW_HEIGHT);

        glutFullScreen();
        isFullscreen = true;
    } else {
        glutReshapeWindow(windowedWidth, windowedHeight);
        glutPositionWindow(windowedX, windowedY);
        isFullscreen = false;
    }

    glutPostRedisplay();
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
    victorySoundPlayed = false;
    gameOverSoundPlayed = false;
    resetInputStates();
    resetNameEntry();
}

void startGame() {
    bonusTimeScore = 0;
    pendingScore = 0;
    playerLives = MAX_LIVES;
    gameCompleted = false;
    gameOver = false;
    victorySoundPlayed = false;
    gameOverSoundPlayed = false;
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

void initTorchFlickers() {
    for (int i = 0; i < 4; ++i) {
        torchFlickers[i].rate = randFloat(3.5f, 7.0f);
        torchFlickers[i].phase = randFloat(0.0f, 6.28f);
        torchFlickers[i].intensity = randFloat(0.7f, 1.0f);
    }
}

void spawnBonusClock() {
    float hs = worldHalfSize();
    float margin = 1.5f;
    bonusClockX = randFloat(-hs + margin, hs - margin);
    bonusClockZ = randFloat(-hs + margin, hs - margin);
    bonusClockActive = true;
    cout << "Bonus clock spawned!\n";
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

    // Init 6 warriors, but only first 2 are active in wave 1
    bool groundActive = isGroundLevel();
    for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
        ghostAlive[gi]          = groundActive && (gi < 2);
        ghostHealth[gi]         = GHOST_MAX_HEALTH;
        ghostX[gi]              = WARRIOR_START_X[gi];
        ghostZ[gi]              = WARRIOR_START_Z[gi];
        ghostHitFlashTimer[gi]  = 0.0f;
    }
    wave2Spawned = false;
    weaponClaimed = false;
    healthPickupClaimed = false;
    groundDoorUnlockedByBoss = false;
    medkitUsed = false;
    medkitHoldTimer = 0.0f;
    groundIntroActive = isGroundLevel();
    swordSwingTimer = 0.0f;
    gunRecoilTimer = 0.0f;
    muzzleFlashTimer = 0.0f;

    // Bonus clock reset
    bonusClockActive = false;
    bonusClockBannerTimer = 0.0f;
    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);

    // Reset obstacle hit counter each level
    obstacleHitCount = 0;

    mouseInitialized = false;
    resetInputStates();
    initTorchFlickers();
}

void startLevel(int levelIndex) {
    currentLevel = levelIndex;
    gameState = STATE_PLAYING;
    gameOver = false;
    gameOverSoundPlayed = false;
    for (int i = 0; i < obstacleCount(); ++i) {
        activeObstacles[i] = levelObstacles[currentLevel][i];
        activeObstacles[i].dx = 0.0f;
        activeObstacles[i].dz = 0.0f;

        if (currentLevel == 1 || currentLevel == 2) {
            // Level 3 (Hard) now has much slower obstacle movement
            float speedMult = (currentLevel == 2) ? 0.35f : 0.95f;
            float vx = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * speedMult;
            float vz = (static_cast<float>(rand() % 200) / 100.0f - 1.0f) * speedMult;
            if (fabs(vx) < 0.15f) vx = (vx < 0.0f) ? -0.15f : 0.15f;
            if (fabs(vz) < 0.15f) vz = (vz < 0.0f) ? -0.15f : 0.15f;
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
    float minBound = -halfSize + WALL_THICKNESS + PLAYER_RADIUS + 0.15f;
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
        handleObstacleHit();
        return false;
    }

    if (isGroundLevel()) {
        for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
            if (!ghostAlive[gi]) continue;
            float ghostDistance = distance2D(newX, newZ, ghostX[gi], ghostZ[gi]);
            if (ghostDistance < GHOST_BODY_RADIUS) {
                damagePlayer(1, false);
                return false;
            }
        }
    }

    return true;
}

void handleObstacleHit() {
    if (gameState != STATE_PLAYING) return;
    if (damageCooldown > 0.0f) return;  // cooldown prevents rapid-fire hits

    damageCooldown = 1.5f;
    obstacleHitCount++;
    cout << "Obstacle hit! " << obstacleHitCount << "/" << MAX_OBSTACLE_HITS << "\n";

    if (obstacleHitCount >= MAX_OBSTACLE_HITS) {
        obstacleHitCount = MAX_OBSTACLE_HITS;
        gameOver = true;
        gameState = STATE_GAME_OVER;
        resetInputStates();
        playGameOverSound();
        cout << "Game over! Touched obstacle " << MAX_OBSTACLE_HITS << " times.\n";
    }
}

void damagePlayer(int amount, bool bypassCooldown) {
    if (gameState != STATE_PLAYING) {
        return;
    }

    if (!bypassCooldown && damageCooldown > 0.0f) {
        return;
    }

    damageCooldown = 1.5f;
    playerLives -= amount;
    if (playerLives <= 0) {
        playerLives = 0;
        gameOver = true;
        gameState = STATE_GAME_OVER;
        resetInputStates();
        playGameOverSound();
        cout << "Game over. Health depleted or hit obstacle.\n";
        return;
    }

    cout << "Health lost. Health left: " << playerLives << "\n";
}

// =============================================
// ENVIRONMENT DECORATION RENDERING
// =============================================

void renderTorch(float x, float y, float z, int torchIndex) {
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    TorchFlicker& tf = torchFlickers[torchIndex % 4];
    float flicker = 0.6f + 0.4f * tf.intensity * (0.5f + 0.5f * sin(time * tf.rate + tf.phase));
    float flicker2 = 0.5f + 0.5f * sin(time * tf.rate * 1.7f + tf.phase + 1.0f);

    // Torch bracket (metal)
    glColor3f(0.25f, 0.22f, 0.20f);
    drawBox(x, y - 0.3f, z, 0.08f, 0.6f, 0.08f);

    // Torch top holder
    glColor3f(0.30f, 0.25f, 0.18f);
    drawBox(x, y, z, 0.14f, 0.12f, 0.14f);

    // Fire glow (animated)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Inner flame (bright yellow-orange)
    glColor4f(1.0f, 0.85f * flicker, 0.15f, 0.9f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.18f + flicker2 * 0.05f, z);
    glutSolidSphere(0.08f, 8, 8);
    glPopMatrix();

    // Outer flame (orange-red)
    glColor4f(1.0f, 0.45f * flicker, 0.05f, 0.6f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.22f + flicker2 * 0.08f, z);
    glutSolidSphere(0.14f, 8, 8);
    glPopMatrix();

    // Flame halo
    glColor4f(1.0f, 0.6f, 0.1f, 0.15f * flicker);
    glPushMatrix();
    glTranslatef(x, y + 0.15f, z);
    glutSolidSphere(0.35f, 10, 10);
    glPopMatrix();

    glDisable(GL_BLEND);
}

void renderLavaCracks() {
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    float pulse = 0.5f + 0.5f * sin(time * 1.8f + lavaPulsePhase);
    float pulse2 = 0.5f + 0.5f * sin(time * 2.5f + lavaPulsePhase + 2.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Multiple lava cracks across the floor
    float crackY = 0.02f;

    // Crack 1 - diagonal
    glColor4f(0.95f, 0.3f * pulse, 0.02f, 0.7f * pulse);
    drawBox(-1.5f, crackY, -1.0f, 2.2f, 0.02f, 0.06f);
    drawBox(-0.5f, crackY, -0.5f, 0.06f, 0.02f, 1.2f);

    // Crack 2
    glColor4f(1.0f, 0.35f * pulse2, 0.05f, 0.65f * pulse2);
    drawBox(1.8f, crackY, 2.0f, 1.8f, 0.02f, 0.05f);
    drawBox(2.5f, crackY, 1.5f, 0.05f, 0.02f, 1.5f);

    // Crack 3
    glColor4f(0.9f, 0.25f * pulse, 0.0f, 0.6f * pulse);
    drawBox(-2.2f, crackY, 2.5f, 1.5f, 0.02f, 0.07f);
    drawBox(-3.0f, crackY, 2.0f, 0.07f, 0.02f, 1.8f);

    // Crack 4
    glColor4f(1.0f, 0.4f * pulse2, 0.08f, 0.5f * pulse2);
    drawBox(0.5f, crackY, -3.0f, 2.0f, 0.02f, 0.05f);
    drawBox(1.3f, crackY, -2.5f, 0.05f, 0.02f, 1.3f);

    // Glow spots at crack intersections
    glColor4f(1.0f, 0.5f, 0.0f, 0.2f * pulse);
    drawBox(-0.5f, crackY + 0.01f, -0.5f, 0.3f, 0.01f, 0.3f);
    drawBox(2.5f, crackY + 0.01f, 1.5f, 0.25f, 0.01f, 0.25f);
    drawBox(-3.0f, crackY + 0.01f, 2.0f, 0.2f, 0.01f, 0.2f);

    glDisable(GL_BLEND);
}

void renderWallChains() {
    float halfSize = worldHalfSize();
    float wallZ = halfSize - WALL_THICKNESS * 0.5f - 0.05f;
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    // Two chain sets on the far (back) wall
    float chainPositions[2] = {-2.0f, 2.0f};

    for (int c = 0; c < 2; ++c) {
        float cx = chainPositions[c];
        float sway = sin(time * 0.8f + c * 1.5f) * 0.03f;

        // Mounting ring
        glColor3f(0.35f, 0.33f, 0.30f);
        glPushMatrix();
        glTranslatef(cx, 2.8f, wallZ);
        glPopMatrix();

        // Chain links (hanging down)
        for (int i = 0; i < 6; ++i) {
            float linkY = 2.5f - i * 0.22f;
            float linkSway = sway * (i + 1) * 0.5f;
            glColor3f(0.32f - i * 0.02f, 0.30f - i * 0.02f, 0.28f - i * 0.01f);

            glPushMatrix();
            glTranslatef(cx + linkSway, linkY, wallZ);
            if (i % 2 == 0) {
                glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            }
            glutSolidTorus(0.015f, 0.04f, 6, 12);
            glPopMatrix();
        }

        // Hanging ring at bottom
        glColor3f(0.28f, 0.26f, 0.24f);
        glPushMatrix();
        glTranslatef(cx + sway * 3.5f, 1.1f, wallZ);
        glutSolidTorus(0.025f, 0.08f, 8, 16);
        glPopMatrix();
    }
}

void renderCornerPillars() {
    float halfSize = worldHalfSize();
    float pillarSize = 0.35f;
    float pillarHeight = 3.8f;

    // Four corner positions near the entrance wall (back wall Z+)
    float positions[4][2] = {
        {-halfSize + 0.5f, halfSize - 0.5f},
        { halfSize - 0.5f, halfSize - 0.5f},
        {-halfSize + 0.5f, -halfSize + 0.8f},
        { halfSize - 0.5f, -halfSize + 0.8f}
    };

    for (int i = 0; i < 4; ++i) {
        float px = positions[i][0];
        float pz = positions[i][1];

        // Stone pillar body
        glColor3f(0.38f, 0.36f, 0.33f);
        drawBox(px, pillarHeight * 0.5f, pz, pillarSize, pillarHeight, pillarSize);

        // Base
        glColor3f(0.32f, 0.30f, 0.28f);
        drawBox(px, 0.15f, pz, pillarSize + 0.12f, 0.30f, pillarSize + 0.12f);

        // Carved cap
        glColor3f(0.42f, 0.40f, 0.36f);
        drawBox(px, pillarHeight + 0.08f, pz, pillarSize + 0.10f, 0.16f, pillarSize + 0.10f);

        // Cap detail
        glColor3f(0.35f, 0.32f, 0.28f);
        drawBox(px, pillarHeight + 0.20f, pz, pillarSize - 0.05f, 0.08f, pillarSize - 0.05f);
    }
}

void renderSkullOrnaments() {
    float halfSize = worldHalfSize();
    float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    // Skulls on left and right walls
    struct SkullPos { float x; float y; float z; float rotY; };
    SkullPos skulls[4] = {
        {-halfSize + WALL_THICKNESS * 0.5f + 0.12f, 2.2f, -1.5f, 90.0f},
        {-halfSize + WALL_THICKNESS * 0.5f + 0.12f, 2.2f,  1.5f, 90.0f},
        { halfSize - WALL_THICKNESS * 0.5f - 0.12f, 2.2f, -1.5f, -90.0f},
        { halfSize - WALL_THICKNESS * 0.5f - 0.12f, 2.2f,  1.5f, -90.0f}
    };

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(skulls[i].x, skulls[i].y, skulls[i].z);
        glRotatef(skulls[i].rotY, 0.0f, 1.0f, 0.0f);

        // Skull cranium
        glColor3f(0.85f, 0.82f, 0.72f);
        glPushMatrix();
        glScalef(1.0f, 1.15f, 0.9f);
        glutSolidSphere(0.14f, 12, 12);
        glPopMatrix();

        // Eye sockets
        float eyeGlow = 0.3f + 0.7f * (0.5f + 0.5f * sin(time * 2.0f + i * 1.5f));
        glColor3f(0.8f * eyeGlow, 0.15f * eyeGlow, 0.0f);
        drawBox(-0.05f, 0.02f, 0.11f, 0.04f, 0.04f, 0.03f);
        drawBox( 0.05f, 0.02f, 0.11f, 0.04f, 0.04f, 0.03f);

        // Nose
        glColor3f(0.25f, 0.22f, 0.18f);
        drawBox(0.0f, -0.03f, 0.12f, 0.025f, 0.03f, 0.02f);

        // Jaw
        glColor3f(0.80f, 0.77f, 0.68f);
        drawBox(0.0f, -0.10f, 0.06f, 0.10f, 0.04f, 0.08f);

        // Mounting plate
        glColor3f(0.30f, 0.28f, 0.25f);
        drawBox(0.0f, 0.0f, -0.06f, 0.18f, 0.22f, 0.03f);

        glPopMatrix();
    }
}

void renderStalactites() {
    float halfSize = worldHalfSize();
    float ceilingY = 4.0f;

    struct StalactiteInfo { float x; float z; float length; float width; };
    StalactiteInfo stalactites[] = {
        {-3.0f,  2.5f, 0.8f, 0.10f},
        {-1.5f, -3.0f, 1.2f, 0.12f},
        { 0.5f,  1.8f, 0.6f, 0.08f},
        { 2.5f, -1.0f, 1.0f, 0.11f},
        {-2.0f, -0.5f, 0.5f, 0.07f},
        { 3.2f,  3.0f, 0.7f, 0.09f},
        { 1.0f, -2.5f, 0.9f, 0.10f},
        {-3.5f,  0.5f, 0.55f, 0.08f},
        { 0.0f,  3.5f, 0.65f, 0.09f},
        { 2.0f,  2.0f, 0.45f, 0.07f}
    };
    int numStalactites = 10;

    for (int i = 0; i < numStalactites; ++i) {
        float sx = stalactites[i].x;
        float sz = stalactites[i].z;
        float len = stalactites[i].length;
        float w = stalactites[i].width;

        if (fabs(sx) > halfSize - 0.5f || fabs(sz) > halfSize - 0.5f) continue;

        // Main body (cone shape approximated with tapered boxes)
        glColor3f(0.35f, 0.33f, 0.30f);
        drawBox(sx, ceilingY - len * 0.25f, sz, w * 1.5f, len * 0.5f, w * 1.5f);

        glColor3f(0.30f, 0.28f, 0.26f);
        drawBox(sx, ceilingY - len * 0.55f, sz, w * 1.0f, len * 0.3f, w * 1.0f);

        // Tip
        glColor3f(0.25f, 0.24f, 0.22f);
        drawBox(sx, ceilingY - len * 0.8f, sz, w * 0.5f, len * 0.2f, w * 0.5f);

        // Drip effect (tiny sphere at tip, subtle)
        float time = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
        float dripPhase = fmod(time * 0.3f + i * 1.1f, 3.0f);
        if (dripPhase < 0.5f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.4f, 0.5f, 0.6f, 0.5f * (1.0f - dripPhase * 2.0f));
            glPushMatrix();
            glTranslatef(sx, ceilingY - len - dripPhase * 0.3f, sz);
            glutSolidSphere(0.02f, 6, 6);
            glPopMatrix();
            glDisable(GL_BLEND);
        }
    }
}

// =============================================
// BONUS CLOCK RENDERING
// =============================================

void renderBonusClock() {
    if (!bonusClockActive) return;

    float time   = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    // Bob up/down gently – no Y-axis spin so face stays visible
    float hover  = 0.48f + 0.10f * sin(time * 2.2f);
    // Slight tilt toward player (face-forward) – tiny rock side to side
    float rock   = 4.0f * sin(time * 1.6f);

    glPushMatrix();
    glTranslatef(bonusClockX, hover, bonusClockZ);
    // Make clock face the player (rotate around Y so face points +Z)
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);   // face toward +X world = toward cam roughly
    glRotatef(rock,  1.0f, 0.0f, 0.0f);   // gentle rocking tilt

    // ── SIZES (match reference: thick green ring → gold body → white face) ──
    const float R_GREEN  = 0.310f;  // green outer ring radius
    const float R_GOLD   = 0.265f;  // gold body radius
    const float R_WHITE  = 0.210f;  // white face radius
    const float DEPTH    = 0.22f;   // disc thickness scale along Z

    // ── PULSING GREEN OUTER GLOW ─────────────────────────────────────────────
    float glow = 0.5f + 0.5f * sin(time * 4.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.05f, 0.85f, 0.15f, 0.10f + 0.08f * glow);
    glPushMatrix(); glScalef(1.0f, 1.0f, DEPTH * 0.6f);
    glutSolidSphere(R_GREEN + 0.06f, 20, 20);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── GREEN OUTER RING ─────────────────────────────────────────────────────
    // Solid dark-green ring (the thick border seen in the image)
    glColor3f(0.10f, 0.55f, 0.12f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH);
    glutSolidSphere(R_GREEN, 28, 28);
    glPopMatrix();

    // Slightly lighter green highlight on top-left (lighting feel)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.25f, 0.80f, 0.28f, 0.35f);
    glPushMatrix();
    glTranslatef(-0.06f, 0.08f, R_GREEN * DEPTH * 0.55f);
    glScalef(0.55f, 0.45f, 0.08f);
    glutSolidSphere(R_GREEN, 16, 16);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── GOLD / YELLOW BODY DISC ───────────────────────────────────────────────
    glColor3f(0.92f, 0.78f, 0.08f);   // bright gold – matches image
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH);
    glutSolidSphere(R_GOLD, 26, 26);
    glPopMatrix();

    // Shadow rim between green and gold (darker gold band at edge)
    glColor3f(0.62f, 0.50f, 0.04f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.85f);
    glutSolidSphere(R_GOLD + 0.010f, 26, 26);
    glPopMatrix();

    // ── WHITE / CREAM CLOCK FACE ─────────────────────────────────────────────
    glColor3f(0.97f, 0.96f, 0.90f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.60f);
    glutSolidSphere(R_WHITE, 26, 26);
    glPopMatrix();

    // Inner shadow ring on face edge (gives depth)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.55f, 0.50f, 0.30f, 0.22f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, DEPTH * 0.38f);
    glutSolidSphere(R_WHITE + 0.005f, 24, 24);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ── HOUR TICK MARKS (4 thick marks at 12/3/6/9, 8 thin for others) ───────
    float faceZ = R_WHITE * DEPTH * 0.62f + 0.004f;
    const float PI2 = 6.28318f;
    for (int i = 0; i < 12; ++i) {
        float ang  = (float)i / 12.0f * PI2;
        float r    = R_WHITE * 0.84f;
        float mx   = sin(ang) * r;
        float my   = cos(ang) * r;
        bool major = (i % 3 == 0);
        glColor3f(0.12f, 0.10f, 0.10f);
        glPushMatrix();
        glTranslatef(mx, my, faceZ);
        glRotatef(-(float)i / 12.0f * 360.0f, 0.0f, 0.0f, 1.0f);
        float tw = major ? 0.022f : 0.011f;
        float th = major ? 0.042f : 0.025f;
        drawBox(0.0f, 0.0f, 0.0f, tw, th, 0.006f);
        glPopMatrix();
    }

    // ── MINUTE HAND — shows whole minutes remaining (short & chunky) ──────────
    // Each full minute = 360 deg; starts at 12, sweeps clockwise as time elapses
    float totalMins  = levelTimeRemaining / 60.0f;          // e.g. 1.5 min remaining
    float minuteAng  = fmod(totalMins, 1.0f) * 360.0f;     // position within current minute
    glColor3f(0.08f, 0.07f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.006f);
    glRotatef(-minuteAng, 0.0f, 0.0f, 1.0f);
    drawBox(0.0f,  0.028f, 0.0f, 0.024f, 0.010f, 0.007f);
    drawBox(0.0f,  0.068f, 0.0f, 0.020f, 0.062f, 0.007f);
    drawBox(0.0f,  0.108f, 0.0f, 0.013f, 0.018f, 0.006f);
    glPopMatrix();

    // ── SECOND HAND (long, slender, red) — sweeps once per 60 s ──────────────
    // Maps seconds within the current minute: 0s→12 o'clock, 15s→3, 30s→6, 45s→9
    float curSecs   = fmod(levelTimeRemaining, 60.0f);      // 0..60
    float secondAng = (curSecs / 60.0f) * 360.0f;          // clockwise from 12
    glColor3f(0.90f, 0.10f, 0.10f);   // red second hand
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.011f);
    glRotatef(-secondAng, 0.0f, 0.0f, 1.0f);
    // Tail (short bit behind centre)
    drawBox(0.0f, -0.030f, 0.0f, 0.007f, 0.028f, 0.004f);
    // Main arm
    drawBox(0.0f,  0.068f, 0.0f, 0.007f, 0.110f, 0.004f);
    // Tip
    drawBox(0.0f,  0.150f, 0.0f, 0.005f, 0.020f, 0.004f);
    glPopMatrix();

    // ── CENTER BOSS (gold dot over both hands) ────────────────────────────────
    glColor3f(0.85f, 0.70f, 0.10f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, faceZ + 0.016f);
    glutSolidSphere(0.020f, 10, 10);
    glPopMatrix();

    // ── CROWN / STEM on top of clock ─────────────────────────────────────────
    glColor3f(0.82f, 0.70f, 0.10f);
    drawBox(0.0f, R_GREEN * 1.05f, 0.0f, 0.038f, 0.055f, 0.038f);
    glColor3f(0.65f, 0.52f, 0.06f);
    drawBox(0.0f, R_GREEN * 1.05f + 0.030f, 0.0f, 0.050f, 0.018f, 0.050f);

    glPopMatrix();
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

    // Gun pickup instead of sword
    if (!weaponClaimed) {
        glPushMatrix();
        glTranslatef(weaponPickupPos.x, 0.48f, weaponPickupPos.z);
        glRotatef(spin, 0.0f, 1.0f, 0.0f);

        // Gun body
        glColor3f(0.18f, 0.18f, 0.20f);
        drawBox(0.0f, 0.0f, 0.0f, 0.12f, 0.14f, 0.50f);
        // Gun barrel
        glColor3f(0.25f, 0.25f, 0.28f);
        drawBox(0.0f, 0.04f, 0.35f, 0.06f, 0.06f, 0.30f);
        // Gun grip
        glColor3f(0.30f, 0.20f, 0.10f);
        drawBox(0.0f, -0.14f, -0.05f, 0.10f, 0.20f, 0.12f);
        // Gun trigger guard
        glColor3f(0.22f, 0.22f, 0.24f);
        drawBox(0.0f, -0.06f, 0.08f, 0.04f, 0.06f, 0.10f);

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

void renderOneWarrior(int gi) {
    float timeSec    = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;
    float hurtFlash  = ghostHitFlashTimer[gi] > 0.0f ? (ghostHitFlashTimer[gi] / 0.25f) : 0.0f;
    float healthRatio = (float)ghostHealth[gi] / (float)GHOST_MAX_HEALTH;

    // Walking animation
    float walkCycle = timeSec * 3.8f + gi * 0.8f;  // offset so they don't sync
    float legSwing  = 22.0f * sin(walkCycle);
    float armSwing  = 18.0f * sin(walkCycle);
    float bodyBob   = 0.025f * fabs(sin(walkCycle));
    float headSway  = 2.5f  * sin(walkCycle * 0.5f);

    // Armor colour: dark steel, flashes red when hit
    float armorR = hurtFlash > 0.3f ? 0.85f : 0.18f;
    float armorG = hurtFlash > 0.3f ? 0.08f : 0.18f;
    float armorB = hurtFlash > 0.3f ? 0.06f : 0.20f;

    // Turn to face the player
    float dxP = camX - ghostX[gi];
    float dzP = camZ - ghostZ[gi];
    float faceYaw = atan2(dxP, dzP) * 57.29578f;

    // Ground shadow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.32f);
    glPushMatrix();
    glTranslatef(ghostX[gi], 0.01f, ghostZ[gi]);
    glScalef(1.0f, 0.02f, 0.75f);
    glutSolidSphere(0.40f, 16, 6);
    glPopMatrix();
    glDisable(GL_BLEND);

    glPushMatrix();
    glTranslatef(ghostX[gi], bodyBob, ghostZ[gi]);
    glRotatef(faceYaw, 0.0f, 1.0f, 0.0f);

    // LEGS
    for (int side = -1; side <= 1; side += 2) {
        float sx = side * 0.17f;
        float swing = side * legSwing;
        glPushMatrix();
        glTranslatef(sx, 0.52f, 0.0f);
        glRotatef(swing, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -0.52f, 0.0f);
        glColor3f(0.15f, 0.14f, 0.15f);
        drawBox(0.0f, 0.58f, 0.0f, 0.16f, 0.42f, 0.15f);
        glColor3f(armorR*0.6f+0.10f, armorG*0.6f+0.10f, armorB*0.6f+0.12f);
        drawBox(0.0f, 0.35f, 0.06f, 0.14f, 0.10f, 0.10f);
        glColor3f(0.15f, 0.14f, 0.15f);
        drawBox(0.0f, 0.18f, 0.0f, 0.13f, 0.34f, 0.13f);
        glColor3f(0.08f, 0.07f, 0.07f);
        drawBox(0.0f, 0.04f, 0.04f, 0.15f, 0.10f, 0.20f);
        glPopMatrix();
    }

    // TORSO
    glColor3f(armorR, armorG, armorB);
    drawBox(0.0f, 1.12f, 0.0f, 0.46f, 0.50f, 0.24f);
    glColor3f(armorR*0.6f+0.10f, armorG*0.6f+0.10f, armorB*0.6f+0.12f);
    drawBox(0.0f, 1.12f, 0.12f, 0.06f, 0.44f, 0.04f);
    glColor3f(0.20f, 0.18f, 0.18f);
    drawBox(0.0f, 0.84f, 0.0f, 0.36f, 0.18f, 0.20f);
    glColor3f(0.70f, 0.55f, 0.08f);
    drawBox(0.0f, 0.83f, 0.10f, 0.08f, 0.06f, 0.04f);

    // SHOULDER PAULDRONS
    for (int side = -1; side <= 1; side += 2) {
        glColor3f(armorR+0.04f, armorG+0.02f, armorB+0.02f);
        drawBox(side*0.32f, 1.36f, 0.0f, 0.12f, 0.12f, 0.26f);
        glColor3f(armorR*0.55f+0.08f, armorG*0.55f+0.08f, armorB*0.55f+0.10f);
        drawBox(side*0.36f, 1.36f, 0.0f, 0.05f, 0.08f, 0.22f);
    }

    // ARMS
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side*0.30f, 1.32f, 0.0f);
        glRotatef(-side*armSwing, 1.0f, 0.0f, 0.0f);
        glColor3f(armorR, armorG, armorB);
        drawBox(0.0f, -0.20f, 0.0f, 0.13f, 0.30f, 0.13f);
        glColor3f(0.12f, 0.11f, 0.12f);
        drawBox(0.0f, -0.37f, 0.0f, 0.11f, 0.08f, 0.11f);
        glColor3f(0.18f, 0.17f, 0.18f);
        drawBox(0.0f, -0.56f, 0.0f, 0.10f, 0.24f, 0.10f);
        glColor3f(0.10f, 0.09f, 0.10f);
        drawBox(0.0f, -0.74f, 0.02f, 0.12f, 0.12f, 0.14f);
        if (side == 1) {
            glColor3f(0.32f, 0.16f, 0.06f);
            drawBox(0.0f, -0.92f, 0.0f, 0.06f, 0.22f, 0.06f);
            glColor3f(0.70f, 0.55f, 0.08f);
            drawBox(0.0f, -0.80f, 0.0f, 0.24f, 0.04f, 0.04f);
            glColor3f(0.75f, 0.78f, 0.84f);
            drawBox(0.0f, -1.28f, 0.0f, 0.05f, 0.80f, 0.04f);
        }
        glPopMatrix();
    }

    // NECK
    glColor3f(0.58f, 0.42f, 0.34f);
    drawBox(0.0f, 1.52f, 0.0f, 0.12f, 0.12f, 0.12f);

    // HEAD + HELMET
    glPushMatrix();
    glTranslatef(0.0f, 1.66f, 0.0f);
    glRotatef(headSway, 0.0f, 1.0f, 0.0f);
    glColor3f(0.70f, 0.50f, 0.38f);
    drawBox(0.0f, -0.02f, 0.08f, 0.22f, 0.18f, 0.10f);
    float eyeR = (healthRatio < 0.4f || hurtFlash > 0.3f) ? 1.0f : 0.85f;
    float eyeG = (healthRatio < 0.4f || hurtFlash > 0.3f) ? 0.05f : 0.65f;
    float eyeB = (healthRatio < 0.4f || hurtFlash > 0.3f) ? 0.02f : 0.10f;
    for (int side = -1; side <= 1; side += 2) {
        float ex = side * 0.07f;
        glColor3f(0.05f, 0.04f, 0.04f);
        drawBox(ex, 0.02f, 0.145f, 0.06f, 0.04f, 0.03f);
        glColor3f(eyeR, eyeG, eyeB);
        drawBox(ex, 0.02f, 0.165f, 0.035f, 0.025f, 0.018f);
    }
    glColor3f(armorR+0.02f, armorG+0.02f, armorB+0.02f);
    glPushMatrix(); glScalef(1.0f, 1.05f, 0.95f); glutSolidSphere(0.198f, 20, 16); glPopMatrix();
    glColor3f(armorR*0.5f+0.14f, armorG*0.5f+0.12f, armorB*0.5f+0.14f);
    drawBox(0.0f, 0.08f, 0.165f, 0.30f, 0.05f, 0.05f);
    for (int side = -1; side <= 1; side += 2) {
        glColor3f(armorR, armorG, armorB);
        drawBox(side*0.168f, -0.04f, 0.08f, 0.06f, 0.16f, 0.14f);
    }
    glColor3f(0.80f, 0.06f, 0.06f);
    drawBox(0.0f, 0.25f, -0.02f, 0.038f, 0.10f, 0.12f);
    glPopMatrix();  // end head

    // HEALTH BAR
    {
        float barW = 0.60f, barH = 0.055f, barY = 2.08f;
        glColor3f(0.28f, 0.04f, 0.04f);
        drawBox(0.0f, barY, 0.0f, barW, barH, 0.02f);
        float fillW = barW * healthRatio;
        glColor3f(0.12f + (1.0f-healthRatio)*0.88f, healthRatio*0.82f, 0.04f);
        drawBox(-barW*0.5f + fillW*0.5f, barY, 0.01f, fillW, barH-0.01f, 0.02f);
    }

    glPopMatrix();  // end root
}

void renderGhostBoss() {
    if (!isGroundLevel()) return;
    for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
        if (ghostAlive[gi]) renderOneWarrior(gi);
    }
}


// =============================================
// HELD GUN RENDERING (replaces sword for Level 4)
// =============================================

void renderHeldGun() {
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

    float recoil = gunRecoilTimer > 0.0f ? (gunRecoilTimer / 0.15f) : 0.0f;
    float recoilKick = sin(recoil * 3.14159f) * 0.06f;

    float baseX = camX + forwardX * 0.55f + rightX * 0.28f + upX * -0.22f;
    float baseY = camY + forwardY * 0.55f + upY * -0.22f;
    float baseZ = camZ + forwardZ * 0.55f + rightZ * 0.28f + upZ * -0.22f;

    // Pull back on recoil
    baseX -= forwardX * recoilKick;
    baseY -= forwardY * recoilKick;
    baseZ -= forwardZ * recoilKick;

    glPushMatrix();
    glTranslatef(baseX, baseY, baseZ);
    glRotatef((-yaw * 57.29578f) + 10.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch * 57.29578f - 8.0f, 1.0f, 0.0f, 0.0f);

    // Gun body (main receiver)
    glColor3f(0.15f, 0.15f, 0.17f);
    drawBox(0.0f, 0.0f, 0.0f, 0.06f, 0.06f, 0.28f);

    // Gun barrel
    glColor3f(0.20f, 0.20f, 0.22f);
    drawBox(0.0f, 0.015f, 0.22f, 0.035f, 0.035f, 0.20f);

    // Gun slide top
    glColor3f(0.18f, 0.18f, 0.20f);
    drawBox(0.0f, 0.04f, 0.02f, 0.055f, 0.025f, 0.22f);

    // Grip
    glColor3f(0.28f, 0.18f, 0.10f);
    drawBox(0.0f, -0.08f, -0.06f, 0.05f, 0.12f, 0.06f);

    // Trigger
    glColor3f(0.12f, 0.12f, 0.14f);
    drawBox(0.0f, -0.02f, 0.04f, 0.015f, 0.04f, 0.02f);

    // Muzzle flash
    if (muzzleFlashTimer > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float flashIntensity = muzzleFlashTimer / 0.08f;
        glColor4f(1.0f, 0.9f, 0.3f, 0.8f * flashIntensity);
        glPushMatrix();
        glTranslatef(0.0f, 0.015f, 0.35f);
        glutSolidSphere(0.04f * flashIntensity, 8, 8);
        glPopMatrix();
        glColor4f(1.0f, 0.5f, 0.1f, 0.4f * flashIntensity);
        glPushMatrix();
        glTranslatef(0.0f, 0.015f, 0.38f);
        glutSolidSphere(0.07f * flashIntensity, 8, 8);
        glPopMatrix();
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

    // Dark stone floor
    glColor3f(0.25f, 0.23f, 0.22f);
    drawBox(0.0f, -0.10f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    // Floor tiles pattern
    glColor3f(0.22f, 0.20f, 0.19f);
    drawBox(0.0f, -0.06f, 0.0f, halfSize * 1.95f, 0.03f, halfSize * 1.95f);

    // Dark ceiling
    glColor3f(0.08f, 0.08f, 0.10f);
    drawBox(0.0f, 4.0f, 0.0f, halfSize * 2.0f, 0.20f, halfSize * 2.0f);

    // Dungeon walls - darker, more stone-like
    float wallR = 0.22f - 0.02f * static_cast<float>(currentLevel);
    float wallG = 0.20f - 0.02f * static_cast<float>(currentLevel);
    float wallB = 0.25f - 0.01f * static_cast<float>(currentLevel);
    if (wallR < 0.15f) wallR = 0.15f;
    if (wallG < 0.14f) wallG = 0.14f;
    if (wallB < 0.18f) wallB = 0.18f;
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

    // ---- ENVIRONMENT DECORATIONS ----

    // 🔥 Torches at four corners
    renderTorch(-halfSize + 0.6f, 2.5f, -halfSize + 0.6f, 0);
    renderTorch( halfSize - 0.6f, 2.5f, -halfSize + 0.6f, 1);
    renderTorch(-halfSize + 0.6f, 2.5f,  halfSize - 0.6f, 2);
    renderTorch( halfSize - 0.6f, 2.5f,  halfSize - 0.6f, 3);

    // 🌋 Lava cracks in the floor
    renderLavaCracks();

    // ⛓️ Wall chains on far wall
    renderWallChains();

    // 🪨 Stone corner pillars
    renderCornerPillars();

    // 💀 Skull ornaments on side walls
    renderSkullOrnaments();

    // 🏔️ Ceiling stalactites
    renderStalactites();
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
    glColor3f(0.05f, 0.05f, 0.07f);
    drawFilledRect2D(x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f);
    glColor3f(0.18f, 0.22f, 0.28f);
    drawFilledRect2D(x, y, w, h);
    glColor3f(r, g, b);
    drawFilledRect2D(x + 2.0f, y + 2.0f, (w - 4.0f) * ratio, h - 4.0f);
    glColor3f(0.80f, 0.90f, 1.00f);
    drawFilledRect2D(x + 2.0f, y + h - 3.0f, (w - 4.0f) * ratio, 1.0f);
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

    snprintf(line1, sizeof(line1), "MISSION: Break out, survive the arena. STAGE %d (%s)", currentLevel + 1, levelNames[currentLevel]);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2D(15.0f, static_cast<float>(h - 25), line1);

    if (gameState == STATE_PLAYING) {
        if (isGroundLevel()) {
            // Count alive warriors
            int aliveCount = 0;
            for (int gi = 0; gi < NUM_WARRIORS; ++gi) if (ghostAlive[gi]) aliveCount++;
            snprintf(line2, sizeof(line2), "HOSTILES: %d   WEAPON: %s   MEDKIT: %s   HP: %d/%d",
                aliveCount,
                weaponClaimed ? "Claimed" : "Not Claimed",
                (!healthPickupClaimed ? "Not Claimed" : (medkitUsed ? "Used" : "Ready")),
                playerLives, MAX_LIVES);
        } else {
            snprintf(line2, sizeof(line2), "KEYS: %d/%d   COLLISIONS: %d/%d   HP: %d/%d   MODE: %s", keysCollected, keysRequired(), obstacleHitCount, MAX_OBSTACLE_HITS, playerLives, MAX_LIVES, levelNames[currentLevel]);
        }
        glColor3f(0.95f, 0.95f, 0.7f);
        drawText2D(15.0f, static_cast<float>(h - 52), line2);

        if (isGroundLevel()) {
            int aliveCount = 0;
            for (int gi = 0; gi < NUM_WARRIORS; ++gi) if (ghostAlive[gi]) aliveCount++;
            if (groundIntroActive) {
                glColor3f(0.95f, 0.95f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Read combat intel, then press ENTER to begin.");
            } else if (!weaponClaimed) {
                glColor3f(1.0f, 0.95f, 0.2f);
                drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Secure weapon (E near pickup), then fire with LEFT CLICK.");
            } else if (aliveCount > 0) {
                glColor3f(1.0f, 0.78f, 0.35f);
                drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Eliminate hostiles. Grab medkit and hold 4 to trigger.");
            } else if (!doorOpening) {
                glColor3f(0.7f, 1.0f, 0.7f);
                drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Arena cleared. Escape gate is unlocking.");
            } else {
                glColor3f(0.7f, 1.0f, 1.0f);
                drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Push through the opened gate to finish mission.");
            }
        } else if (keysCollected < keysRequired()) {
            glColor3f(1.0f, 0.95f, 0.2f);
            drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Collect all keys. E to grab, SPACE to clear obstacles.");
        } else if (!doorOpening) {
            glColor3f(0.7f, 1.0f, 0.7f);
            drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Move to the gate and press E to unlock.");
        } else {
            glColor3f(0.7f, 1.0f, 1.0f);
            drawText2D(15.0f, static_cast<float>(h - 80), "OBJECTIVE: Advance through the opened exit.");
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2DRightAligned(static_cast<float>(w - 15), static_cast<float>(h - 25), timerLine);
        drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 55), 220.0f, 18.0f, static_cast<float>(playerLives) / static_cast<float>(MAX_LIVES), 0.25f, 0.95f, 0.35f);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 75), "HP CORE");

        if (isGroundLevel() && healthPickupClaimed && !medkitUsed && !groundIntroActive) {
            float medkitRatio = medkitHoldTimer / MEDKIT_HOLD_REQUIRED;
            if (medkitRatio < 0.0f) medkitRatio = 0.0f;
            if (medkitRatio > 1.0f) medkitRatio = 1.0f;
            drawHUDBar(static_cast<float>(w - 250), static_cast<float>(h - 102), 220.0f, 14.0f, medkitRatio, 0.95f, 0.25f, 0.25f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText2D(static_cast<float>(w - 250), static_cast<float>(h - 118), "Hold 4 to inject medkit");
        }

        // Ghost health HUD: compact bars for each warrior
        if (isGroundLevel() && !groundIntroActive) {
            int aliveCount2 = 0;
            for (int gi = 0; gi < NUM_WARRIORS; ++gi) if (ghostAlive[gi]) aliveCount2++;
            if (aliveCount2 > 0) {
                float barW = 40.0f;
                float startX = static_cast<float>(w) * 0.5f - (NUM_WARRIORS * (barW + 4)) * 0.5f;
                for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
                    float ratio = (float)ghostHealth[gi] / (float)GHOST_MAX_HEALTH;
                    if (ratio < 0.0f) ratio = 0.0f;
                    drawHUDBar(startX + gi*(barW+4), static_cast<float>(h - 55), barW, 14.0f,
                               ratio * (ghostAlive[gi] ? 1.0f : 0.0f), 0.88f, 0.20f, 0.22f);
                }
                glColor3f(1.0f, 0.92f, 0.92f);
                drawText2DCentered(static_cast<float>(w)*0.5f, static_cast<float>(h - 73), "WARRIOR HEALTH");
            }
        }

        // ---- Bonus Clock HUD (all levels) ----
        {
            if (bonusClockActive) {
                float dist = distance2D(camX, camZ, bonusClockX, bonusClockZ);
                char clockMsg[80];
                snprintf(clockMsg, sizeof(clockMsg), ">> BONUS CLOCK nearby (%.1fm) - walk over it! <<", dist);
                glColor3f(0.2f, 1.0f, 0.4f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, 40.0f, clockMsg);
            } else {
                char clockMsg[80];
                snprintf(clockMsg, sizeof(clockMsg), "Next +8s clock in: %.1fs", bonusClockSpawnTimer);
                glColor3f(0.55f, 0.8f, 0.55f);
                drawText2DCentered(static_cast<float>(w) * 0.5f, 40.0f, clockMsg);
            }
        }

        // "+8 SEC!" floating banner
        if (bonusClockBannerTimer > 0.0f) {
            float alpha = bonusClockBannerTimer / 1.5f;
            float yOffset = (1.5f - bonusClockBannerTimer) * 40.0f;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.1f, 1.0f, 0.3f, alpha);
            drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.35f + yOffset, "+8 SEC!");
            glDisable(GL_BLEND);
        }

        // Crosshair for gun (ground level with weapon)
        if (isGroundLevel() && weaponClaimed) {
            glColor3f(1.0f, 0.3f, 0.3f);
            float cx = static_cast<float>(w) * 0.5f;
            float cy = static_cast<float>(h) * 0.5f;
            drawFilledRect2D(cx - 8.0f, cy - 1.0f, 16.0f, 2.0f);
            drawFilledRect2D(cx - 1.0f, cy - 8.0f, 2.0f, 16.0f);
        }
    }

    if (gameState == STATE_PLAYING) {
        float doorDist = distance2D(camX, camZ, 0.0f, currentDoorZ());
        if (doorDist < 3.0f) {
            if (!doorOpening) {
                if (isGroundLevel()) {
                    glColor3f(1.0f, 0.3f, 0.3f);
                    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.45f, "ESCAPE DOOR SEALED");
                    int aliveAtDoor = 0;
                    for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
                        if (ghostAlive[gi]) {
                            aliveAtDoor++;
                        }
                    }
                    if (aliveAtDoor > 0) {
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
        float alpha = (damageCooldown / 1.5f) * 0.5f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.0f, 0.0f, alpha);

        drawFilledRect2D(0, 0, static_cast<float>(w), static_cast<float>(h));

        glColor3f(1.0f, 1.0f, 1.0f);
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

        drawPanelBox2D(panelX, panelY, panelW, panelH, 0.08f, 0.12f, 0.18f, 0.18f, 0.44f, 0.58f);

        glDisable(GL_BLEND);

        float y = panelY + panelH - 48.0f;
        glColor3f(0.64f, 0.94f, 1.0f);
        drawText2DCentered(static_cast<float>(w) * 0.5f, y, "GROUND LEVEL RULES - FINAL FIGHT");

        y -= 55.0f;
        glColor3f(0.95f, 0.95f, 0.90f);
        drawText2D(panelX + 28.0f, y, "1) Claim the gun first (Press E near the gun pickup)."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "2) Shoot ghost with LEFT MOUSE CLICK (aim at ghost)."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "3) Claim medkit (Press E near medkit), then HOLD 4 to use it."); y -= 34.0f;
        drawText2D(panelX + 28.0f, y, "4) Ghost contact damages you, keep distance and shoot!"); y -= 34.0f;
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

    float panelX = static_cast<float>(w) * 0.12f;
    float panelY = static_cast<float>(h) * 0.08f;
    float panelW = static_cast<float>(w) * 0.76f;
    float panelH = static_cast<float>(h) * 0.84f;

    drawPanelBox2D(panelX, panelY, panelW, panelH, 0.12f, 0.05f, 0.05f, 0.35f, 0.15f, 0.15f);

    glColor3f(1.0f, 0.45f, 0.45f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH - 42.0f, "GAME RULES - SURVIVAL PROTOCOL");

    float y = panelY + panelH - 82.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "1) Each level has a timer. Collect all keys before time runs out!"); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "2) Find and collect all keys, then unlock the door."); y -= 30.0f;
    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2D(panelX + 30.0f, y, "3) DO NOT TOUCH OBSTACLES - they cost you health!"); y -= 22.0f;
    drawText2D(panelX + 45.0f, y, "Walls move in Medium and Hard, but at manageable speed."); y -= 30.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "4) Press E to interact (Pick up keys, unlock doors)."); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "5) Press SPACE to jump over gaps and obstacles."); y -= 30.0f;
    glColor3f(0.3f, 1.0f, 0.5f);
    drawText2D(panelX + 30.0f, y, "6) [Level 3+] Bonus clocks spawn! Walk over for +8 seconds."); y -= 30.0f;
    glColor3f(0.95f, 0.95f, 0.88f);
    drawText2D(panelX + 30.0f, y, "7) Level 4 (Ground): Pick up the GUN and SHOOT the ghost!"); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "8) Dungeon rooms have torches, lava cracks, and skulls."); y -= 30.0f;
    drawText2D(panelX + 30.0f, y, "9) Mouse to look around. WASD to move."); y -= 40.0f;

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

    if (startScreenTexture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, startScreenTexture);
        glColor3f(1.0f, 1.0f, 1.0f);

        // 1. Draw base sharp image
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(w, h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(w, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 0.0f);
        glEnd();

        // 2. Multi-pass pseudo blur
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glColor4f(1.0f, 1.0f, 1.0f, 0.15f);
        float blurRadius = 2.5f;
        float offsets[4][2] = {
            {-blurRadius, -blurRadius},
            {blurRadius, -blurRadius},
            {-blurRadius, blurRadius},
            {blurRadius, blurRadius}
        };

        for (int i = 0; i < 4; ++i) {
            float ox = offsets[i][0];
            float oy = offsets[i][1];
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(ox, h + oy);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(w + ox, h + oy);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(w + ox, oy);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(ox, oy);
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);

        // 3. Dark dimming overlay to make text pop
        glColor4f(0.05f, 0.05f, 0.08f, 0.35f);
        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(w, 0.0f);
        glVertex2f(w, h);
        glVertex2f(0.0f, h);
        glEnd();

        glDisable(GL_BLEND);
    } else {
        // Fallback background
        glBegin(GL_QUADS);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(0.0f, 0.0f);
        glColor3f(0.03f, 0.07f, 0.11f); glVertex2f(static_cast<float>(w), 0.0f);
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(static_cast<float>(w), static_cast<float>(h));
        glColor3f(0.02f, 0.03f, 0.08f); glVertex2f(0.0f, static_cast<float>(h));
        glEnd();
    }

    // Draw "3D ESCAPE ROOM" subtitle and descriptions
    float headerY = static_cast<float>(h) * 0.81f;
    float subtitleY = static_cast<float>(h) * 0.765f;
    float headerScale = 0.24f;

    glColor3f(0.12f, 0.08f, 0.03f);
    drawStrokeTextCentered(static_cast<float>(w) * 0.5f + 2.0f, headerY - 3.0f, headerScale, 3.6f, "3D ESCAPE ROOM");

    glColor3f(0.98f, 0.84f, 0.30f);
    drawStrokeTextCentered(static_cast<float>(w) * 0.5f, headerY, headerScale, 3.8f, "3D ESCAPE ROOM");

    glColor3f(0.85f, 0.93f, 0.98f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, subtitleY, "Rooms, climbing, timer pressure, and final survival.");

    // Center buttons over the door
    float firstButtonY = static_cast<float>(h) * 0.58f;
    float optionScale = 0.22f;

    for (int i = 0; i < MENU_ITEMS; ++i) {
        float y = firstButtonY - i * 65.0f;
        bool selected = (i == menuSelection);

        char optionText[64];
        int j = 0;
        for (; menuLabels[i][j] && j < static_cast<int>(sizeof(optionText)) - 1; ++j) {
            optionText[j] = static_cast<char>(toupper(static_cast<unsigned char>(menuLabels[i][j])));
        }
        optionText[j] = '\0';

        if (selected) {
            float pulse = 0.5f + 0.5f * sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.009f);

            glColor3f(0.12f, 0.08f, 0.02f);
            drawStrokeTextCentered(static_cast<float>(w) * 0.5f - 128.0f, y - 4.0f, optionScale * 0.95f, 2.6f, ">>");

            glColor3f(0.95f, 0.80f + 0.20f * pulse, 0.12f);
            drawStrokeTextCentered(static_cast<float>(w) * 0.5f - 130.0f, y - 2.0f, optionScale * 0.95f, 2.8f, ">>");
        }

        glColor3f(0.08f, 0.07f, 0.05f);
        drawStrokeTextCentered(static_cast<float>(w) * 0.5f + 2.5f, y - 4.0f, optionScale, selected ? 3.0f : 2.4f, optionText);

        if (selected) {
            float pulse = 0.5f + 0.5f * sin(static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.009f);
            glColor3f(1.0f, 0.80f + 0.20f * pulse, 0.20f);
            drawStrokeTextCentered(static_cast<float>(w) * 0.5f, y - 2.0f, optionScale, 3.2f, optionText);
        } else {
            glColor3f(0.92f, 0.86f, 0.76f);
            drawStrokeTextCentered(static_cast<float>(w) * 0.5f, y - 2.0f, optionScale, 2.5f, optionText);
        }
    }

    // Controls text at the bottom
    glColor3f(0.70f, 0.78f, 0.82f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, static_cast<float>(h) * 0.12f, "Use W/S or Up/Down to navigate. Press Enter to select.");


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

    drawPanelBox2D(panelX, panelY, panelW, panelH, 0.07f, 0.12f, 0.19f, 0.14f, 0.34f, 0.45f);

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
    float timeSec = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);
    float cx = fw * 0.5f;

    // ── DARK GRADIENT BACKGROUND ─────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Bottom: deep navy
    glBegin(GL_QUADS);
    glColor4f(0.02f, 0.04f, 0.12f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(fw, 0);
    // Top: dark purple
    glColor4f(0.08f, 0.02f, 0.20f, 1.0f);
    glVertex2f(fw, fh);
    glVertex2f(0, fh);
    glEnd();
    glDisable(GL_BLEND);

    // ── DECORATIVE STAR BURST (radiating lines) ───────────────────────────────
    float spinAngle = fmod(timeSec * 18.0f, 360.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < 24; ++i) {
        float ang  = (spinAngle + i * 15.0f) * 3.14159f / 180.0f;
        float alpha = 0.08f + 0.06f * sin(timeSec * 2.0f + i * 0.5f);
        glColor4f(1.0f, 0.85f, 0.20f, alpha);
        glVertex2f(cx, fh * 0.62f);
        glColor4f(1.0f, 0.85f, 0.20f, 0.0f);
        glVertex2f(cx + cos(ang) * fw * 0.55f, fh * 0.62f + sin(ang) * fh * 0.55f);
    }
    glEnd();
    glLineWidth(1.0f);

    // ── GLOW HALO behind title ────────────────────────────────────────────────
    float haloPulse = 0.15f + 0.10f * sin(timeSec * 2.5f);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.85f, 0.10f, haloPulse);
    glVertex2f(cx, fh * 0.63f);
    for (int i = 0; i <= 32; ++i) {
        float ang = i * 6.28318f / 32.0f;
        glColor4f(1.0f, 0.65f, 0.05f, 0.0f);
        glVertex2f(cx + cos(ang) * 240.0f, fh * 0.63f + sin(ang) * 60.0f);
    }
    glEnd();
    glDisable(GL_BLEND);

    // ── MAIN CONGRATS TITLE ───────────────────────────────────────────────────
    // Shadow
    glColor3f(0.4f, 0.25f, 0.0f);
    drawText2DCentered(cx + 3.0f, fh * 0.72f - 3.0f, "CONGRATULATIONS!");
    // Gold pulsing title
    float pulse = 0.5f + 0.5f * sin(timeSec * 3.0f);
    glColor3f(1.0f, 0.80f + 0.20f * pulse, 0.10f * pulse);
    drawText2DCentered(cx, fh * 0.72f, "CONGRATULATIONS!");

    // ── SUBTITLE ─────────────────────────────────────────────────────────────
    float subPulse = 0.6f + 0.4f * sin(timeSec * 2.0f + 1.0f);
    glColor3f(0.45f, 1.0f * subPulse, 0.55f);
    drawText2DCentered(cx, fh * 0.64f, "YOU DEFEATED ALL WARRIORS AND ESCAPED!");

    // ── DECORATIVE DIVIDER ────────────────────────────────────────────────────
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.75f, 0.10f, 0.0f);
    glVertex2f(cx - 280.0f, fh * 0.612f);
    glColor4f(1.0f, 0.75f, 0.10f, 0.8f);
    glVertex2f(cx - 80.0f, fh * 0.612f);
    glVertex2f(cx - 80.0f, fh * 0.606f);
    glColor4f(1.0f, 0.75f, 0.10f, 0.0f);
    glVertex2f(cx - 280.0f, fh * 0.606f);
    glColor4f(1.0f, 0.75f, 0.10f, 0.0f);
    glVertex2f(cx + 80.0f, fh * 0.612f);
    glColor4f(1.0f, 0.75f, 0.10f, 0.8f);
    glVertex2f(cx + 280.0f, fh * 0.612f);
    glVertex2f(cx + 280.0f, fh * 0.606f);
    glColor4f(1.0f, 0.75f, 0.10f, 0.0f);
    glVertex2f(cx + 80.0f, fh * 0.606f);
    glEnd();
    glDisable(GL_BLEND);

    // ── TROPHY ICON (OpenGL boxes) ────────────────────────────────────────────
    float tx = cx;
    float ty = fh * 0.54f;
    float trophyPulse = 0.92f + 0.08f * sin(timeSec * 4.0f);
    // Cup body
    glColor3f(1.0f * trophyPulse, 0.78f * trophyPulse, 0.05f);
    drawFilledRect2D(tx - 28.0f, ty,       56.0f, 40.0f);
    drawFilledRect2D(tx - 20.0f, ty + 40.0f, 40.0f, 14.0f);
    // Stem
    drawFilledRect2D(tx - 8.0f,  ty - 18.0f, 16.0f, 20.0f);
    // Base
    drawFilledRect2D(tx - 30.0f, ty - 26.0f, 60.0f, 10.0f);
    // Handles
    glColor3f(0.85f * trophyPulse, 0.62f, 0.02f);
    drawFilledRect2D(tx - 44.0f, ty + 10.0f, 18.0f, 20.0f);
    drawFilledRect2D(tx + 26.0f, ty + 10.0f, 18.0f, 20.0f);
    // Star on cup
    glColor3f(1.0f, 1.0f, 0.5f * trophyPulse);
    drawFilledRect2D(tx - 6.0f, ty + 14.0f, 12.0f, 12.0f);
    drawFilledRect2D(tx - 12.0f, ty + 18.0f, 24.0f, 4.0f);

    // ── STATS PANEL ───────────────────────────────────────────────────────────
    glColor3f(0.9f, 0.88f, 0.70f);
    drawText2DCentered(cx, fh * 0.44f, "- - - MISSION COMPLETE - - -");

    char scoreLine[80];
    snprintf(scoreLine, sizeof(scoreLine), "Final Score:  %d", pendingScore);
    glColor3f(0.20f, 1.0f, 0.45f);
    drawText2DCentered(cx, fh * 0.38f, scoreLine);

    // ── SHOOTING STARS (3 animated sparks) ───────────────────────────────────
    float starColors[3][3] = {{1.0f,0.85f,0.2f},{0.4f,0.9f,1.0f},{1.0f,0.5f,0.8f}};
    for (int i = 0; i < 3; ++i) {
        float phase  = fmod(timeSec * 0.6f + i * 0.45f, 1.0f);
        float starX  = fw * (0.12f + i * 0.38f) + cos(timeSec + i) * 40.0f;
        float starY  = fh * (0.20f + phase * 0.35f);
        float alpha  = sin(phase * 3.14159f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(starColors[i][0], starColors[i][1], starColors[i][2], alpha);
        drawFilledRect2D(starX - 6.0f, starY - 6.0f, 12.0f, 12.0f);
        drawFilledRect2D(starX - 2.0f, starY - 12.0f, 4.0f, 24.0f);
        glDisable(GL_BLEND);
    }

    // ── NAME ENTRY SECTION ────────────────────────────────────────────────────
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(cx, fh * 0.28f, "Enter your name for the Hall of Fame:");

    char nameLine[48];
    snprintf(nameLine, sizeof(nameLine), "%s_", playerNameLength == 0 ? "" : playerName);
    float namePulse = 0.7f + 0.3f * sin(timeSec * 4.0f);
    glColor3f(1.0f, namePulse, 0.15f);
    drawText2DCentered(cx, fh * 0.20f, nameLine);

    glColor3f(0.60f, 0.60f, 0.60f);
    drawText2DCentered(cx, fh * 0.12f, "Type your name, Backspace to delete, Enter to save.");

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

    float panelW = static_cast<float>(w) * 0.56f;
    float panelH = static_cast<float>(h) * 0.34f;
    float panelX = (static_cast<float>(w) - panelW) * 0.5f;
    float panelY = (static_cast<float>(h) - panelH) * 0.5f;
    drawPanelBox2D(panelX, panelY, panelW, panelH, 0.12f, 0.04f, 0.05f, 0.70f, 0.14f, 0.16f);

    glColor3f(1.0f, 0.35f, 0.35f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH * 0.70f, "GAME OVER");
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH * 0.50f, "All health is gone.");
    drawText2DCentered(static_cast<float>(w) * 0.5f, panelY + panelH * 0.33f, "Press M for menu or Esc to quit.");

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
    renderHeldGun();

    // Render bonus clock if active (all levels)
    renderBonusClock();
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
        // Darker dungeon atmosphere
        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
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

        // Set up atmospheric lighting for room levels
        if (!isGroundLevel()) {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);

            // Dim ambient for dungeon feel
            float ambient[] = {0.15f, 0.12f, 0.10f, 1.0f};
            float diffuse[] = {0.6f, 0.5f, 0.3f, 1.0f};
            float lightPos[] = {0.0f, 3.5f, 0.0f, 1.0f};
            glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        }

        renderWorld();

        if (!isGroundLevel()) {
            glDisable(GL_LIGHTING);
            glDisable(GL_LIGHT0);
            glDisable(GL_COLOR_MATERIAL);
        }

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
            playClaimSound();
            cout << "Gun claimed. You can now shoot with left mouse click.\n";
        }

        if (!healthPickupClaimed && distance2D(camX, camZ, healthPickupPos.x, healthPickupPos.z) < INTERACT_PICKUP_RANGE) {
            healthPickupClaimed = true;
            interacted = true;
            playClaimSound();
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

        float dist2D_ = distance2D(camX, camZ, keyPos.x, keyPos.z);
        float distY = fabs(camY - PLAYER_EYE_HEIGHT - keyY);

        if (dist2D_ < 1.5f && distY < 2.0f) {
            keyTaken[i] = true;
            keysCollected++;
            foundKey = true;
            playClaimSound();
            cout << "Key " << keysCollected << " collected in level " << (currentLevel + 1) << "\n";
        }
    }

    if (foundKey) return;

    float doorDistance = distance2D(camX, camZ, 0.0f, currentDoorZ());
    if (keysCollected >= keysRequired() && !doorOpening && doorDistance < 1.65f) {
        doorOpening = true;
        playUnlockSound();
        cout << "Exit unlocked for level " << (currentLevel + 1) << "\n";
    } else if (doorDistance < 1.65f && keysCollected < keysRequired()) {
        cout << "Exit locked. Need " << (keysRequired() - keysCollected) << " more key(s).\n";
    }
}

void tryGhostAttack() {
    bool anyGhostAlive = false;
    for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
        if (ghostAlive[gi]) {
            anyGhostAlive = true;
            break;
        }
    }

    if (!(gameState == STATE_PLAYING && isGroundLevel() && !groundIntroActive && anyGhostAlive)) {
        return;
    }

    if (!weaponClaimed) {
        cout << "You need the gun first. Claim it before attacking.\n";
        return;
    }

    if (weaponAttackCooldown > 0.0f) {
        cout << "Gun is reloading. Wait a moment.\n";
        return;
    }

    // Gun shooting - ray test against warrior body/head (head = double damage)
    float dirX = cos(pitch) * sin(yaw);
    float dirY = sin(pitch);
    float dirZ = -cos(pitch) * cos(yaw);

    // Always trigger fire effects
    gunRecoilTimer = 0.15f;
    muzzleFlashTimer = 0.08f;
    weaponAttackCooldown = 0.40f;

    int bestTarget = -1;
    bool bestHitHead = false;
    float bestT = GHOST_WEAPON_RANGE + 1.0f;

    auto rayHitsSphere = [&](float cx, float cy, float cz, float radius, float& tHit) -> bool {
        float ocX = cx - camX;
        float ocY = cy - camY;
        float ocZ = cz - camZ;
        float t = ocX * dirX + ocY * dirY + ocZ * dirZ;
        if (t < 0.0f || t > GHOST_WEAPON_RANGE) {
            return false;
        }
        float distSq = ocX * ocX + ocY * ocY + ocZ * ocZ - t * t;
        float rSq = radius * radius;
        if (distSq > rSq) {
            return false;
        }
        tHit = t;
        return true;
    };

    for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
        if (!ghostAlive[gi]) continue;

        float t = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) / 1000.0f + static_cast<float>(gi) * 0.77f;
        float bodyBob = sin(t * 2.8f) * 0.028f;

        float bodyCenterX = ghostX[gi];
        float bodyCenterY = 1.12f + bodyBob;
        float bodyCenterZ = ghostZ[gi];

        float headCenterX = ghostX[gi];
        float headCenterY = 1.66f + bodyBob;
        float headCenterZ = ghostZ[gi];

        float tHead = 0.0f;
        if (rayHitsSphere(headCenterX, headCenterY, headCenterZ, 0.24f, tHead)) {
            if (tHead < bestT) {
                bestT = tHead;
                bestTarget = gi;
                bestHitHead = true;
            }
        }

        float tBody = 0.0f;
        if (rayHitsSphere(bodyCenterX, bodyCenterY, bodyCenterZ, 0.46f, tBody)) {
            if (tBody < bestT) {
                bestT = tBody;
                bestTarget = gi;
                bestHitHead = false;
            }
        }
    }

    if (bestTarget < 0) {
        cout << "Shot missed!\n";
        return;
    }

    int damage = bestHitHead ? 2 : 1;
    ghostHealth[bestTarget] -= damage;
    ghostHitFlashTimer[bestTarget] = 0.25f;
    if (bestHitHead) {
        cout << "HEADSHOT on warrior " << bestTarget << "! -" << damage << " HP: " << ghostHealth[bestTarget] << "\n";
    } else {
        cout << "Hit warrior " << bestTarget << "! -" << damage << " HP: " << ghostHealth[bestTarget] << "\n";
    }

    if (ghostHealth[bestTarget] <= 0) {
        ghostHealth[bestTarget] = 0;
        ghostAlive[bestTarget]  = false;
        cout << "Warrior " << bestTarget << " defeated!\n";

        if (!wave2Spawned) {
            if (!ghostAlive[0] && !ghostAlive[1]) {
                wave2Spawned = true;
                cout << "Wave 1 defeated! 4 more warriors appearing!\n";
                for (int gi = 2; gi < NUM_WARRIORS; ++gi) {
                    ghostAlive[gi] = true;
                    // Reset their positions in case they need it
                    ghostX[gi] = WARRIOR_START_X[gi];
                    ghostZ[gi] = WARRIOR_START_Z[gi];
                    ghostHealth[gi] = GHOST_MAX_HEALTH;
                }
            }
        } else {
            // Check if ALL remaining warriors are dead
            bool allDead = true;
            for (int gi = 2; gi < NUM_WARRIORS; ++gi) {
                if (ghostAlive[gi]) { allDead = false; break; }
            }
            if (allDead) {
                groundDoorUnlockedByBoss = true;
                doorOpening = true;
                playUnlockSound();
                cout << "All warriors defeated! Escape door unlocking...\n";
            }
        }
    }
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(tolower(key));

    if (lower == 't') {
        playClaimSound();
        cout << "SFX test triggered (claim.wav).\n";
    }

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

    if (key == GLUT_KEY_F11) {
        toggleFullscreen();
        return;
    }

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
        playLevelUpSound();
        cout << "Level " << (currentLevel + 1) << " complete. Moving to next level.\n";
        startLevel(currentLevel + 1);
    } else {
        bonusTimeScore += static_cast<int>(levelTimeRemaining);
        gameCompleted = true;
        gameState = STATE_NAME_ENTRY;
        resetInputStates();
        playVictorySound();
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

        if (gunRecoilTimer > 0.0f) {
            gunRecoilTimer -= dt;
            if (gunRecoilTimer < 0.0f) gunRecoilTimer = 0.0f;
        }

        if (muzzleFlashTimer > 0.0f) {
            muzzleFlashTimer -= dt;
            if (muzzleFlashTimer < 0.0f) muzzleFlashTimer = 0.0f;
        }

        // ghostHitFlashTimer per-warrior is updated in the AI loop

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

        // Bonus clock banner timer
        if (bonusClockBannerTimer > 0.0f) {
            bonusClockBannerTimer -= dt;
            if (bonusClockBannerTimer < 0.0f) bonusClockBannerTimer = 0.0f;
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

        // ---- Bonus Clock Logic (all levels) ----
        {
            if (!bonusClockActive) {
                bonusClockSpawnTimer -= dt;
                if (bonusClockSpawnTimer <= 0.0f) {
                    spawnBonusClock();
                    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);
                }
            } else {
                // Check if player walks over clock
                float clockDist = distance2D(camX, camZ, bonusClockX, bonusClockZ);
                if (clockDist < BONUS_CLOCK_RADIUS) {
                    levelTimeRemaining += BONUS_CLOCK_TIME_BONUS;
                    bonusClockActive = false;
                    bonusClockBannerTimer = 1.5f;
                    bonusClockSpawnTimer = randFloat(5.0f, 11.0f);
                    cout << "+8 seconds bonus! Time remaining: " << levelTimeRemaining << "\n";
                }
            }
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

        if (isGroundLevel()) {
            for (int gi = 0; gi < NUM_WARRIORS; ++gi) {
                if (!ghostAlive[gi]) continue;

                float dxToPlayer = camX - ghostX[gi];
                float dzToPlayer = camZ - ghostZ[gi];
                float distToPlayer = sqrt(dxToPlayer*dxToPlayer + dzToPlayer*dzToPlayer);

                if (distToPlayer > 0.01f) {
                    float invDist  = 1.0f / distToPlayer;
                    float perpX    = -dzToPlayer * invDist;
                    float perpZ    =  dxToPlayer * invDist;
                    // Each warrior gets a slightly different strafe phase
                    float strafeFactor = sin(static_cast<float>(now) * 0.004f + gi * 1.1f) * 0.80f;
                    float desiredX = (dxToPlayer * invDist) * 0.82f + perpX * strafeFactor * 0.55f;
                    float desiredZ = (dzToPlayer * invDist) * 0.82f + perpZ * strafeFactor * 0.55f;
                    float len = sqrt(desiredX*desiredX + desiredZ*desiredZ);
                    if (len > 0.0001f) { desiredX /= len; desiredZ /= len; }

                    float speedBoost = weaponClaimed ? 1.20f : 1.0f;
                    float step = GHOST_MOVE_SPEED * speedBoost * dt;
                    if (step > distToPlayer) step = distToPlayer;

                    ghostX[gi] += desiredX * step;
                    ghostZ[gi] += desiredZ * step;

                    float minB = -worldHalfSize() + WALL_THICKNESS + GHOST_BODY_RADIUS + 0.2f;
                    float maxB =  worldHalfSize() - WALL_THICKNESS - GHOST_BODY_RADIUS - 0.2f;
                    ghostX[gi] = clampf(ghostX[gi], minB, maxB);
                    ghostZ[gi] = clampf(ghostZ[gi], minB, maxB);
                }

                if (distance2D(camX, camZ, ghostX[gi], ghostZ[gi]) < GHOST_TOUCH_DAMAGE_RANGE) {
                    damagePlayer(1, false);
                }

                if (ghostHitFlashTimer[gi] > 0.0f) {
                    ghostHitFlashTimer[gi] -= dt;
                    if (ghostHitFlashTimer[gi] < 0.0f) ghostHitFlashTimer[gi] = 0.0f;
                }
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
            float len = sqrt(inputX * inputX + inputZ * inputZ);
            inputX /= len;
            inputZ /= len;

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
    initTorchFlickers();
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
    cout << "Controls: WASD move, Mouse look, E interact, Left click shoot (ground), Hold 4 medkit, Space jump, Esc menu.\n";

    glutMainLoop();
    return 0;
}
