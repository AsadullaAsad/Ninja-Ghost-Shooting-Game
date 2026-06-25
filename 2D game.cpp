#include <windows.h>
#include <GL/glut.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/gl.h>
#define GL_CLAMP_TO_EDGE 0x812F
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include<algorithm>
#include <string>

GLuint enemyTexture;
GLuint textureID;
GLuint playerTexture;
GLuint ghostTexture;
GLuint bulletTexture;
int missedEnemies = 0;
int score = 0, highScore = 0;
GLuint menuBackgroundTexture;
GLuint ghostBackgroundTexture;
GLuint ninjaBackgroundTexture;
GLuint instructionTexture;

enum GameState { MENU, PLAYING, GAMEOVER, INSTRUCTION };
GameState gameState = MENU;

enum PlayerType { NONE, NINJA, GHOST };
PlayerType currentPlayerType = NONE;

struct Vec2
{
    float x, y;
};

class Player
{
public:
    Vec2 pos = {0.0f, -0.9f};
    float speed = 0.05f;
    int health = 3; // Added health field to fix the bug

    void moveLeft()
    {
        if (pos.x > -0.95f) pos.x -= speed;
    }
    void moveRight()
    {
        if (pos.x <  0.95f) pos.x += speed;
    }
    Vec2 bulletPos()
    {
        return {pos.x, pos.y + 0.08f};
    }
};

class Bullet
{
public:
    Vec2 pos;
    float speed = 0.03f;
    bool destroyed = false;

    Bullet(Vec2 start) : pos(start) {}
    void update()
    {
        pos.y += speed;
        if (pos.y > 1.0f) destroyed = true;
    }
};

class Enemy
{
public:
    Vec2 pos;
    float speed = 0.01f;
    bool destroyed = false;

    Enemy(Vec2 start) : pos(start) {}
    void update()
    {
        pos.y -= speed;
        if (pos.y < -1.0f) destroyed = true;
    }
};

Player player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
float enemyTimer = 0.0f;
bool keyStates[256] = { false };

GLuint backgroundTexture;

GLuint loadTexture(const char* filename)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    channels = 4;

    if (!data)
    {
        std::cerr << "[ERROR] Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return texID;
}

void drawTexturedRect(Vec2 center, float w, float h)
{
    float x = center.x, y = center.y;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(x - w, y - h);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(x + w, y - h);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(x + w, y + h);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(x - w, y + h);
    glEnd();
}

bool checkCollision(const Bullet& b, const Enemy& e)
{
    return std::abs(b.pos.x - e.pos.x) < 0.05f && std::abs(b.pos.y - e.pos.y) < 0.05f;
}

void update()
{
    if (gameState != PLAYING) return;

    if (keyStates['a']) player.moveLeft();
    if (keyStates['d']) player.moveRight();
    if (keyStates[' ']) bullets.push_back(Bullet(player.bulletPos()));

    enemyTimer += 0.02f;
    if (enemyTimer > 1.0f)
    {
        float x = (std::rand() % 200) / 100.0f - 1.0f;
        enemies.push_back(Enemy({x, 1.0f}));
        enemyTimer = 0;
    }

    for (auto& b : bullets) b.update();
    for (auto& e : enemies)
    {
        e.update();
        if (!e.destroyed && std::fabs(e.pos.x - player.pos.x) < 0.05f && std::fabs(e.pos.y - player.pos.y) < 0.08f)
        {
            e.destroyed = true;
            player.health--;
            if (player.health <= 0) gameState = GAMEOVER;
        }
        if (e.pos.y < -1.0f)
        {
            e.destroyed = true;
            missedEnemies++;
            if (missedEnemies >= 5) gameState = GAMEOVER;
        }
    }

    for (auto& b : bullets)
    {
        for (auto& e : enemies)
        {
            if (!b.destroyed && !e.destroyed && checkCollision(b, e))
            {
                b.destroyed = true;
                e.destroyed = true;
                score++;
                if (score > highScore) highScore = score;
            }
        }
    }

    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet& b)
    {
        return b.destroyed;
    }), bullets.end());
    enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](Enemy& e)
    {
        return e.destroyed;
    }), enemies.end());

    glutPostRedisplay();
}

void renderText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_TIMES_ROMAN_24)
{
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

void drawRect(Vec2 pos, float width, float height, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(pos.x - width, pos.y - height);
    glVertex2f(pos.x + width, pos.y - height);
    glVertex2f(pos.x + width, pos.y + height);
    glVertex2f(pos.x - width, pos.y + height);
    glEnd();
}

void drawPlayer(Vec2 pos)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, (currentPlayerType == GHOST) ? ghostTexture : playerTexture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    drawTexturedRect(pos, 0.07f, 0.08f);
    glDisable(GL_TEXTURE_2D);
}

void drawEnemy(Vec2 pos)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, enemyTexture);
    drawTexturedRect(pos, 0.05f, 0.05f);
    glDisable(GL_TEXTURE_2D);
}

void renderScoreAndHealth()
{
    glColor3f(1, 1, 1);
    renderText(-0.95f, 0.9f, "Score: " + std::to_string(score));
    renderText(0.6f, 0.9f, "High Score: " + std::to_string(highScore));
    float barX = -0.95f, barY = 0.8f;
    float barWidth = 0.05f;
    for (int i = 0; i < player.health; ++i)
        drawRect({barX + i * (barWidth + 0.01f), barY}, barWidth / 2, 0.025f, 0.0f, 1.0f, 0.0f);
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    if (gameState == MENU)
        glBindTexture(GL_TEXTURE_2D, menuBackgroundTexture);
    else if (gameState == PLAYING)
    {
        if (currentPlayerType == GHOST)
            glBindTexture(GL_TEXTURE_2D, ghostBackgroundTexture);
        else if (currentPlayerType == NINJA)
            glBindTexture(GL_TEXTURE_2D, ninjaBackgroundTexture);
        else
            glBindTexture(GL_TEXTURE_2D, menuBackgroundTexture);
    }
    drawTexturedRect({0.0f, 0.0f}, 1.0f, 1.0f);
    glDisable(GL_TEXTURE_2D);
    if (gameState == MENU)
    {
        glBindTexture(GL_TEXTURE_2D, menuBackgroundTexture);
    }

    else if (gameState == PLAYING)
    {
        if (currentPlayerType == GHOST)
            glBindTexture(GL_TEXTURE_2D, ghostBackgroundTexture);
        else if (currentPlayerType == NINJA)
            glBindTexture(GL_TEXTURE_2D, ninjaBackgroundTexture);
        else
            glBindTexture(GL_TEXTURE_2D, menuBackgroundTexture);
        renderScoreAndHealth();
    }


    if (gameState == MENU)
    {
        glColor3f(1, 1, 1);
        renderText(-0.1f, -0.1f, "SHOOTING GAME", GLUT_BITMAP_TIMES_ROMAN_24);
        renderText(-0.1f, -0.2f, "Press N to Start Little NINJA");
        renderText(-0.1f, -0.3f, "Press G to Start Little GHOST");
        renderText(-0.0f, -0.4f, "Press I for INSTRUCTION");
    }
    else if (gameState == PLAYING)
    {
        drawPlayer(player.pos);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, bulletTexture);
        for (auto& b : bullets)
            drawTexturedRect(b.pos, 0.01f, 0.015f); // bullet size
        glDisable(GL_TEXTURE_2D);

        for (auto& e : enemies)
            drawEnemy(e.pos);
        glColor3f(1, 1, 1);
        char buffer[64];
        sprintf(buffer, "Missed: %d / 5", missedEnemies);
        renderText( 0.0f, 0.9f, buffer);
    }
else if (gameState == GAMEOVER)
{
        glDisable(GL_TEXTURE_2D);
glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1, 0, 0);
    char buffer[64];
    renderText(-0.55f, -0.55f, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
    renderText(-0.55f, -0.85f, "Press R to restart");

    sprintf(buffer, "Missed: %d / 5", missedEnemies);
    renderText(0.1f, 0.9f, buffer);
    renderScoreAndHealth();
}


    else if (gameState == INSTRUCTION)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, instructionTexture);
        drawTexturedRect({0.0f, 0.0f}, 1.0f, 1.0f);  // Background image
        glDisable(GL_TEXTURE_2D);

        glColor3f(0, 0, 0);  // Black
        renderText(-0.1f, 0.7f, "INSTRUCTIONS", GLUT_BITMAP_TIMES_ROMAN_24);
        renderText(-0.2f, 0.5f, "1. Press 'N' to play as Little Ninja.");
        renderText(-0.2f, 0.4f, "2. Press 'G' to play as Little Ghost.");
        renderText(-0.2f, 0.3f, "3. Move using 'A' and 'D' keys.");
        renderText(-0.2f, 0.2f, "4. Press Space Bar to shoot.");
        renderText(-0.2f, 0.1f, "5. Don't let 3 enemies pass!");
        renderText(-0.2f, 0.0f, "6. Press 'R' to restart after game over.");
        glColor3f(1, 1, 1);  // Black
        renderText(-0.2f, -0.4f, "Press 'B' to return to MENU.");
    }
    glutSwapBuffers();
}


void timer(int value)
{
    update();
    glutTimerFunc(16, timer, 0);
}

void keyDown(unsigned char key, int x, int y)
{
    if (gameState == MENU && key == 'g')
    {
        currentPlayerType = GHOST;
        gameState = PLAYING;
        player.health = 3;
        score = 0;
        bullets.clear();
        enemies.clear();
        enemyTimer = 0;
        missedEnemies = 0;
    }
    else if (gameState == MENU && key == 'n')
    {
        currentPlayerType = NINJA;
        gameState = PLAYING;
        player.health = 3;
        score = 0;
        bullets.clear();
        enemies.clear();
        enemyTimer = 0;
        missedEnemies = 0;
    }
    else if (gameState == GAMEOVER && (key == 'r' || key == 'R'))
    {
        gameState = MENU;
        currentPlayerType = NONE;
        bullets.clear();
        enemies.clear();
        enemyTimer = 0;
        missedEnemies = 0;
    }
    else if (gameState == MENU && key == 'i')
    {
        gameState = INSTRUCTION;
    }
    else if (gameState == INSTRUCTION && (key == 'b' || key == 'B'))
    {
        gameState = MENU;
    }
    keyStates[key] = true;
    glutPostRedisplay();
}

void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = false;
}

int main(int argc, char** argv)
{
    std::srand((unsigned)time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Shooting Game");

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);

    ghostBackgroundTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/Picture6.jpg");
    if (!ghostBackgroundTexture)
    {
        std::cerr << "Ghost background texture loading failed. Exiting." << std::endl;
        return -1;
    }

    ninjaBackgroundTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/back1.jpg");
    if (!ninjaBackgroundTexture)
    {
        std::cerr << "Ninja background texture loading failed. Exiting." << std::endl;
        return -1;
    }

    enemyTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/enemy.png"); // egg
    if (!enemyTexture)
    {
        std::cerr << "Enemy texture loading failed. Exiting." << std::endl;
        return -1;
    }

    playerTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/man.png"); //ninja
    if (!playerTexture)
    {
        std::cerr << "Player texture loading failed. Exiting." << std::endl;
        return -1;
    }

    ghostTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/Picture4.png"); //ghost
    if (!ghostTexture)
    {
        std::cerr << "Ghost texture loading failed. Exiting." << std::endl;
        return -1;
    }

    bulletTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/knife.png");
    if (!bulletTexture)
    {
        std::cerr << "Bullet texture loading failed. Exiting." << std::endl;
        return -1;
    }


    menuBackgroundTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/back2.jpg");
    if (!menuBackgroundTexture)
    {
        std::cerr << "Menu background texture loading failed. Exiting." << std::endl;
        return -1;
    }
    instructionTexture = loadTexture("E:/xcelll/ewu materials/CSE420/MiniPro/back.jpg");
    glutDisplayFunc(render);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutTimerFunc(0, timer, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    PlaySound(TEXT("E:\\xcelll\\ewu materials\\CSE420\\MiniPro\\game.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
    glutMainLoop();
    return 0;
}