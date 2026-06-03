#include "Render.h"
#include "GUItextRectangle.h"
#include "Texture.h"
#include "Snake.h"
#include "Apple.h"
#include "Camera.h"
#include "Light.h"
#include "MyShaders.h"
#include "GameConfig.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

extern OpenGL gl;
Light light;
Camera camera;
Snake snake;
std::vector<Apple> apples;

bool texturing = true;
bool lightning = true;
bool alpha = false;
bool paused = false;
bool snakeHeadCamera = false;
int eatenApples = 0;

int bestScore = 0;
static const char* BEST_SCORE_FILE = "best_score.txt";

bool gameOverDialogPending = false;

Shader snakeShader;
bool   shadersReady = false;
int    locUTexture = -1;
int    locUUseTex = -1;
int    locUTexGenMode = -1;

static void loadBestScore()
{
    std::ifstream f(BEST_SCORE_FILE);
    if (f.is_open()) {
        int v = 0;
        if (f >> v) {
            if (v < 0) v = 0;
            bestScore = v;
        }
        f.close();
    }
}

static void saveBestScore()
{
    std::ofstream f(BEST_SCORE_FILE, std::ios::trunc);
    if (f.is_open()) {
        f << bestScore;
        f.close();
    }
}

static void updateBestScore()
{
    if (eatenApples > bestScore) {
        bestScore = eatenApples;
        saveBestScore();
    }
}

static bool pickObjFile(char outPath[260])
{
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(outPath, 260);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = outPath;
    ofn.nMaxFile = 260;
    ofn.lpstrFilter = "OBJ Models (*.obj;*.obj_m)\0*.obj;*.obj_m\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "models";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameA(&ofn) == TRUE;
}

static bool pickImageFile(char outPath[260])
{
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(outPath, 260);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = outPath;
    ofn.nMaxFile = 260;
    ofn.lpstrFilter = "Image Files (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "textures";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    return GetOpenFileNameA(&ofn) == TRUE;
}

void switchModes(OpenGL* sender, KeyEventArg arg)
{
    auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
    switch (key)
    {
    case 'L': lightning = !lightning; break;
    case 'T': texturing = !texturing; break;
    case 'A': alpha = !alpha; break;
    case 'C': snakeHeadCamera = !snakeHeadCamera; break;
    }
}

GuiTextRectangle text;

bool isPositionOccupied(const Vector3& pos, float minDist) {
    for (const auto& seg : snake.getSegments()) {
        if ((pos - seg).length() < minDist) return true;
    }
    return false;
}

Vector3 generateRandomPosition() {
    Vector3 newPos;
    do {
        float x = ((float)rand() / RAND_MAX) * 2 * GameConfig::WORLD_SIZE - GameConfig::WORLD_SIZE;
        float y = ((float)rand() / RAND_MAX) * 2 * GameConfig::WORLD_SIZE - GameConfig::WORLD_SIZE;
        float z = ((float)rand() / RAND_MAX) * 2 * GameConfig::WORLD_SIZE - GameConfig::WORLD_SIZE;
        newPos = Vector3(x, y, z);
    } while (isPositionOccupied(newPos, snake.getSegmentSize() * 1.2f));
    return newPos;
}

static void restartGame()
{
    snake.reset();
    eatenApples = 0;
    paused = false;
    gameOverDialogPending = false;

    apples.clear();
    for (int i = 0; i < GameConfig::MAX_APPLES; ++i) {
        Apple a;
        a.setPosition(generateRandomPosition());
        apples.push_back(a);
    }
}

static void drawWorldBounds()
{
    const float s = GameConfig::WORLD_SIZE;
    const float step = GameConfig::GRID_STEP;

    GLboolean wasLighting = glIsEnabled(GL_LIGHTING);
    GLboolean wasTexture = glIsEnabled(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(1.0f);
    glColor3f(0.45f, 0.12f, 0.12f);
    glBegin(GL_LINES);
    for (float v = -s; v <= s + 0.001f; v += step) {
        glVertex3f(-s, v, -s); glVertex3f(-s, v, s);
        glVertex3f(-s, -s, v); glVertex3f(-s, s, v);

        glVertex3f(s, v, -s); glVertex3f(s, v, s);
        glVertex3f(s, -s, v); glVertex3f(s, s, v);

        glVertex3f(v, -s, -s); glVertex3f(v, -s, s);
        glVertex3f(-s, -s, v); glVertex3f(s, -s, v);

        glVertex3f(v, s, -s); glVertex3f(v, s, s);
        glVertex3f(-s, s, v); glVertex3f(s, s, v);

        glVertex3f(v, -s, -s); glVertex3f(v, s, -s);
        glVertex3f(-s, v, -s); glVertex3f(s, v, -s);

        glVertex3f(v, -s, s); glVertex3f(v, s, s);
        glVertex3f(-s, v, s); glVertex3f(s, v, s);
    }
    glEnd();

    glLineWidth(2.5f);
    glColor3f(1.0f, 0.25f, 0.25f);
    glBegin(GL_LINES);

    glVertex3f(-s, -s, -s); glVertex3f(s, -s, -s);
    glVertex3f(s, -s, -s); glVertex3f(s, s, -s);
    glVertex3f(s, s, -s); glVertex3f(-s, s, -s);
    glVertex3f(-s, s, -s); glVertex3f(-s, -s, -s);

    glVertex3f(-s, -s, s); glVertex3f(s, -s, s);
    glVertex3f(s, -s, s); glVertex3f(s, s, s);
    glVertex3f(s, s, s); glVertex3f(-s, s, s);
    glVertex3f(-s, s, s); glVertex3f(-s, -s, s);

    glVertex3f(-s, -s, -s); glVertex3f(-s, -s, s);
    glVertex3f(s, -s, -s); glVertex3f(s, -s, s);
    glVertex3f(s, s, -s); glVertex3f(s, s, s);
    glVertex3f(-s, s, -s); glVertex3f(-s, s, s);

    glEnd();

    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);

    if (wasLighting) glEnable(GL_LIGHTING);
    if (wasTexture)  glEnable(GL_TEXTURE_2D);
}

void initRender()
{
    srand((unsigned)time(NULL));

    loadBestScore();

    camera.setPosition(5, 5, 6);
    gl.WheelEvent.reaction(&camera, &Camera::Zoom);
    gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
    gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
    gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
    gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);

    gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
    gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
    gl.KeyUpEvent.reaction(&light, &Light::StopDrug);

    gl.KeyDownEvent.reaction([](OpenGL* sender, KeyEventArg arg) {
        int key = arg.key;

        if (snakeHeadCamera)
        {
            if (key == VK_UP)     snake.pitch(+1);
            if (key == VK_DOWN)   snake.pitch(-1);
            if (key == VK_LEFT)   snake.yaw(+1);
            if (key == VK_RIGHT)  snake.yaw(-1);
            if (key == VK_RETURN) snake.roll(+1);
            if (key == VK_SHIFT)  snake.roll(-1);
        }
        else
        {
            if (key == VK_LEFT)   snake.setDirection(Vector3(0, -1, 0));
            if (key == VK_RIGHT)  snake.setDirection(Vector3(0, 1, 0));
            if (key == VK_UP)     snake.setDirection(Vector3(-1, 0, 0));
            if (key == VK_DOWN)   snake.setDirection(Vector3(1, 0, 0));
            if (key == VK_RETURN) snake.setDirection(Vector3(0, 0, 1));
            if (key == VK_SHIFT)  snake.setDirection(Vector3(0, 0, -1));
        }

        if (key == VK_SPACE)  paused = !paused;

        auto ch = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));
        if (ch == 'H') snake.openHeadTextureDialog();
        if (ch == 'B') snake.openBodyTextureDialog();
        if (ch == 'N') snake.openTailTextureDialog();
        if (ch == 'O') snake.openTextureDialog();

        if (ch == 'R') restartGame();

        if (ch == 'M') {
            char objFile[260];
            if (pickObjFile(objFile))
            {
                char texFile[260];
                if (pickImageFile(texFile)) {
                    Apple::loadModelAndTexture(objFile, texFile);
                }
                else {
                    Apple::loadModel(objFile);
                }
            }
        }
        });

    gl.KeyDownEvent.reaction(switchModes);

    text.setSize(512, 520);
    snake.initDefaults();

    apples.clear();
    for (int i = 0; i < GameConfig::MAX_APPLES; ++i) {
        Apple a;
        a.setPosition(generateRandomPosition());
        apples.push_back(a);
    }

    snakeShader.VshaderFileName = "shaders/snake.vert";
    snakeShader.FshaderFileName = "shaders/snake.frag";
    snakeShader.LoadShaderFromFile();
    snakeShader.Compile();
    shadersReady = true;

    locUTexture = glGetUniformLocationARB(snakeShader.program, "uTexture");
    locUUseTex = glGetUniformLocationARB(snakeShader.program, "uUseTexture");
    locUTexGenMode = glGetUniformLocationARB(snakeShader.program, "uTexGenMode");
}

void Render(double delta_time)
{
    if (gameOverDialogPending && snake.isDead()) {
        gameOverDialogPending = false;
        MessageBoxA(NULL, "Game Over!", "Game Over", MB_OK | MB_ICONINFORMATION);
    }

    if (!paused && !snake.isDead()) {
        snake.update(delta_time);

        if (snake.checkWallCollision(GameConfig::WORLD_SIZE) || snake.checkSelfCollision()) {
            snake.kill();
            gameOverDialogPending = true;
            updateBestScore();
        }
    }

    if (!snake.isDead()) {
        float headRadius = 0.5f;
        for (auto& apple : apples) {
            if ((snake.getHeadPosition() - apple.getPosition()).length() < headRadius) {
                snake.grow();
                apple.setPosition(generateRandomPosition());
                eatenApples++;
            }
        }
    }

    if (gl.isKeyPressed('F'))
        light.SetPosition(camera.x(), camera.y(), camera.z());

    if (snakeHeadCamera)
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        Vector3 head = snake.getHeadPosition();
        Vector3 dir = snake.getDirection().normalize();
        Vector3 up = snake.getUp().normalize();

        float camBack = 4.0f;
        float camUp = 1.2f;

        Vector3 camPos = head - dir * camBack + up * camUp;
        Vector3 target = head + dir * 2.0;

        gluLookAt(
            camPos.x(), camPos.y(), camPos.z(),
            target.x(), target.y(), target.z(),
            up.x(), up.y(), up.z()
        );
    }
    else
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        Vector3 head = snake.getHeadPosition();

        double targetX = head.x();
        double targetY = head.y();
        double targetZ = head.z();

        double eyeX = targetX + camera.x();
        double eyeY = targetY + camera.y();
        double eyeZ = targetZ + camera.z();

        gluLookAt(
            eyeX, eyeY, eyeZ,
            targetX, targetY, targetZ,
            0.0, 0.0, 1.0
        );
    }

    light.SetUpLight();

    if (snakeHeadCamera)
    {
        Vector3 head = snake.getHeadPosition();
        Vector3 dir = snake.getDirection().normalize();
        Vector3 up = snake.getUp().normalize();

        Vector3 lp = head + dir * 0.3 + up * 0.75;

        light.SetPosition(lp.x(), lp.y(), lp.z());

        float lpos[] = { (float)lp.x(), (float)lp.y(), (float)lp.z(), 1.0f };
        float ldiff[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float lamb[] = { 0.35f, 0.35f, 0.35f, 1.0f };
        float lspec[] = { 1.0f, 1.0f, 1.0f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, lpos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, ldiff);
        glLightfv(GL_LIGHT0, GL_AMBIENT, lamb);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lspec);

        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);

        glEnable(GL_LIGHT0);
    }

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);

    gl.DrawAxes();

    if (lightning) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    if (texturing) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    if (alpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else glDisable(GL_BLEND);

    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);

    float amb[] = { 0.4f, 0.4f, 0.4f, 1.f };
    float dif[] = { 0.9f, 0.9f, 0.9f, 1.f };
    float spec[] = { 0.3f, 0.3f, 0.3f, 1.f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16);
    glShadeModel(GL_SMOOTH);

    drawWorldBounds();

    if (shadersReady && lightning) {
        snakeShader.UseShader();
        glUniform1iARB(locUTexture, 0);
        glUniform1iARB(locUUseTex, texturing ? 1 : 0);

        glUniform1iARB(locUTexGenMode, 1);
        snake.draw();

        glUniform1iARB(locUTexGenMode, 0);
        for (auto& apple : apples) apple.draw();

        Shader::DontUseShaders();
    }
    else {
        snake.draw();
        for (auto& apple : apples) apple.draw();
    }

    light.DrawLightGizmo();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::wstringstream ss;
    ss << std::fixed << std::setprecision(3);

    if (snake.isDead()) ss << L"############ GAME OVER ############\n"
        << L"Нажмите R для новой игры\n\n";
    else if (paused)    ss << L"************ ПАУЗА ************\n\n";

    if (snakeHeadCamera)
        ss << L"=== Управление (режим C, относительно направления) ===\n"
        << L"Стрелка вверх   - петля вверх\n"
        << L"Стрелка вниз    - петля вниз\n"
        << L"Стрелка влево   - поворот влево\n"
        << L"Стрелка вправо  - поворот вправо\n"
        << L"Enter           - крен (+)\n"
        << L"Shift           - крен (-)\n";
    else
        ss << L"=== Управление (дальняя камера, оси мира) ===\n"
        << L"Стрелка вверх   - -X\n"
        << L"Стрелка вниз    - +X\n"
        << L"Стрелка влево   - -Y\n"
        << L"Стрелка вправо  - +Y\n"
        << L"Enter           - +Z\n"
        << L"Shift           - -Z\n";

    ss << L"Пробел          - пауза\n"
        << L"R               - новая игра\n"
        << L"H               - текстура головы\n"
        << L"B               - текстура тела\n"
        << L"N               - текстура хвоста\n"
        << L"O               - текстура на всё\n"
        << L"M               - модель+текстура яблока\n"
        << L"C               - камера на голове/обычная\n\n"
        << L"=== Настройки ===\n"
        << L"T - " << (texturing ? L"[вкл]выкл" : L"вкл[выкл]") << L" текстур\n"
        << L"L - " << (lightning ? L"[вкл]выкл" : L"вкл[выкл]") << L" освещение\n"
        << L"A - " << (alpha ? L"[вкл]выкл" : L"вкл[выкл]") << L" альфа-наложение\n\n"
        << L"=== Управление светом ===\n"
        << L"F - переместить свет в камеру\n"
        << L"G - двигать свет по горизонтали\n"
        << L"G+ЛКМ - двигать свет по вертикали\n\n"
        << L"Координаты света: (" << std::setw(6) << light.x() << L"," << std::setw(6) << light.y() << L"," << std::setw(6) << light.z() << L")\n"
        << L"Позиция головы:   (" << std::setw(6) << snake.getHeadPosition().x() << L"," << std::setw(6) << snake.getHeadPosition().y() << L"," << std::setw(6) << snake.getHeadPosition().z() << L")\n"
        << L"Длина змеи:       " << snake.getSegments().size() << L"\n"
        << L"Яблок собрано:    " << eatenApples << L"\n"
        << L"Лучший счёт:      " << bestScore << L"\n"
        << L"Delta time:       " << delta_time << L" sec\n";

    text.setPosition(10, gl.getHeight() - 10 - 480);
    text.setText(ss.str().c_str());
    text.Draw();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    if (lightning) glEnable(GL_LIGHTING);
    if (texturing) glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
}