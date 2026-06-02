#include "Snake.h"
#include "CubeRenderer.h"
#include "MyShaders.h"
#include "GameConfig.h"
#include <windows.h>
#include <commdlg.h>
#include <GL/gl.h>
#include <cmath>

ObjModel Snake::headModel;
ObjModel Snake::bodyModel;
ObjModel Snake::tailModel;

bool Snake::headModelLoaded = false;
bool Snake::bodyModelLoaded = false;
bool Snake::tailModelLoaded = false;

static const double PI = 3.14159265358979323846;

static bool chooseImageFile(char outPath[260])
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

static bool chooseObjFile(char outPath[260])
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

static Vector3 safeNormalize(const Vector3& v)
{
    double l = v.length();
    if (l < 1e-9) return Vector3(0, 0, 0);
    return Vector3(v.x() / l, v.y() / l, v.z() / l);
}

static Vector3 clampAxisDir(const Vector3& v)
{
    double ax = fabs(v.x());
    double ay = fabs(v.y());
    double az = fabs(v.z());

    if (ax >= ay && ax >= az) return Vector3((v.x() >= 0) ? 1.0 : -1.0, 0.0, 0.0);
    if (ay >= ax && ay >= az) return Vector3(0.0, (v.y() >= 0) ? 1.0 : -1.0, 0.0);
    return Vector3(0.0, 0.0, (v.z() >= 0) ? 1.0 : -1.0);
}

static void applyForwardOrientation(const Vector3& fwdRaw)
{
    Vector3 f = safeNormalize(fwdRaw);
    if (f.length() < 1e-9) return;

    double yawDeg = atan2(f.y(), f.x()) * 180.0 / PI;

    double horiz = sqrt(f.x() * f.x() + f.y() * f.y());
    double pitchDeg = -atan2(f.z(), horiz) * 180.0 / PI;

    glRotated(yawDeg, 0.0, 0.0, 1.0);
    glRotated(pitchDeg, 0.0, 1.0, 0.0);
}

Snake::Snake()
    : speed(1.75f),
    turnSpeed(8.0f),
    segmentSize(0.3f),
    followDistance(0.6f),
    headLoaded(false),
    bodyLoaded(false),
    tailLoaded(false),
    growCounter(0),
    dead(false)
{
    direction = Vector3(-1, 0, 0);
    targetDirection = direction;
    upDirection = Vector3(0, 0, 1);
    targetUp = upDirection;

    segments.clear();
    Vector3 head(0, 0, 0);
    segments.push_back(head);
    segments.push_back(head - direction * followDistance);
    segments.push_back(head - direction * (followDistance * 2.0f));
}

void Snake::initDefaults()
{
    static bool inProgress = false;
    if (headModelLoaded && bodyModelLoaded && tailModelLoaded &&
        headLoaded && bodyLoaded && tailLoaded)
        return;

    if (inProgress) return;
    inProgress = true;

    if (!headModelLoaded) { headModelLoaded = (headModel.LoadModel("models/snake_head.obj") == 1); }
    if (!bodyModelLoaded) { bodyModelLoaded = (bodyModel.LoadModel("models/snake_body.obj") == 1); }
    if (!tailModelLoaded) { tailModelLoaded = (tailModel.LoadModel("models/snake_tail.obj") == 1); }

    if (!headLoaded) { headTexture.LoadTexture("textures/snake_head.jpg"); headLoaded = true; }
    if (!bodyLoaded) { bodyTexture.LoadTexture("textures/snake_body.jpg"); bodyLoaded = true; }
    if (!tailLoaded) { tailTexture.LoadTexture("textures/snake_tail.jpg"); tailLoaded = true; }

    inProgress = false;
}

void Snake::setDirection(const Vector3& newDir)
{
    Vector3 nd = safeNormalize(newDir);
    if (nd.length() < 1e-9) return;

    nd = clampAxisDir(nd);

    Vector3 cur = safeNormalize(direction);
    if (cur.length() > 0.9) {
        if ((cur & nd) < -0.9) return;
    }

    targetDirection = nd;

    Vector3 u = clampAxisDir(safeNormalize(targetUp));
    if (u.length() < 0.5 || fabs(u & nd) > 0.5) {
        if (fabs(nd.z()) < 0.5) u = Vector3(0, 0, 1);
        else                    u = Vector3(1, 0, 0);
    }
    targetUp = clampAxisDir(u);
}

void Snake::pitch(int sign)
{
    Vector3 f = clampAxisDir(safeNormalize(targetDirection));
    Vector3 u = clampAxisDir(safeNormalize(targetUp));
    if (f.length() < 0.5 || u.length() < 0.5) return;

    Vector3 newF, newU;
    if (sign >= 0) {       
        newF = u;
        newU = f * -1.0;
    }
    else {                 
        newF = u * -1.0;
        newU = f;
    }

    targetDirection = clampAxisDir(newF);
    targetUp = clampAxisDir(newU);
}

void Snake::yaw(int sign)
{
    Vector3 f = clampAxisDir(safeNormalize(targetDirection));
    Vector3 u = clampAxisDir(safeNormalize(targetUp));
    if (f.length() < 0.5 || u.length() < 0.5) return;

    Vector3 right = clampAxisDir(f ^ u);

    if (sign >= 0)          
        targetDirection = clampAxisDir(right * -1.0);
    else                   
        targetDirection = clampAxisDir(right);

    targetUp = u;           
}

void Snake::roll(int sign)
{
    Vector3 f = clampAxisDir(safeNormalize(targetDirection));
    Vector3 u = clampAxisDir(safeNormalize(targetUp));
    if (f.length() < 0.5 || u.length() < 0.5) return;

    Vector3 right = clampAxisDir(f ^ u);

    if (sign >= 0)
        targetUp = clampAxisDir(right);
    else
        targetUp = clampAxisDir(right * -1.0);

    targetDirection = f;
}

void Snake::update(double deltaTime)
{
    if (dead) return;

    float dt = (float)deltaTime;
    if (dt <= 0.0f) return;

    float k = turnSpeed * dt;
    if (k > 1.0f) k = 1.0f;

    direction = safeNormalize(direction + (targetDirection - direction) * k);
    if (direction.length() < 1e-9) direction = targetDirection;

    upDirection = safeNormalize(upDirection + (targetUp - upDirection) * k);
    if (upDirection.length() < 1e-9) upDirection = targetUp;

    double dot = direction & upDirection;
    upDirection = safeNormalize(upDirection - direction * dot);
    if (upDirection.length() < 0.5) {
        Vector3 fb = (fabs(direction.z()) < 0.9) ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
        upDirection = safeNormalize(fb - direction * (direction & fb));
        if (upDirection.length() < 0.5) upDirection = Vector3(0, 0, 1);
    }

    Vector3 newHead = segments[0] + direction * (speed * dt);

    float limit = GameConfig::WORLD_SIZE;
    if (newHead.x() > limit) newHead.setCoords(limit, newHead.y(), newHead.z());
    if (newHead.x() < -limit) newHead.setCoords(-limit, newHead.y(), newHead.z());
    if (newHead.y() > limit) newHead.setCoords(newHead.x(), limit, newHead.z());
    if (newHead.y() < -limit) newHead.setCoords(newHead.x(), -limit, newHead.z());
    if (newHead.z() > limit) newHead.setCoords(newHead.x(), newHead.y(), limit);
    if (newHead.z() < -limit) newHead.setCoords(newHead.x(), newHead.y(), -limit);

    segments[0] = newHead;

    for (size_t i = 1; i < segments.size(); ++i)
    {
        Vector3 prev = segments[i - 1];
        Vector3 cur2 = segments[i];
        Vector3 delta = prev - cur2;
        double dist = delta.length();

        if (dist > followDistance)
        {
            Vector3 dir2 = safeNormalize(delta);
            segments[i] = prev - dir2 * followDistance;
        }
    }

    if (growCounter > 0)
    {
        Vector3 tail = segments.back();
        Vector3 tailDir(0, 0, 0);
        if (segments.size() >= 2) tailDir = safeNormalize(segments[segments.size() - 2] - tail);
        else tailDir = safeNormalize(direction);

        if (tailDir.length() < 1e-9) tailDir = Vector3(1, 0, 0);

        segments.push_back(tail - tailDir * followDistance);
        growCounter--;
    }
}

void Snake::drawSegmentObj(const Vector3& pos, const Vector3& fwd, ObjModel& model, bool modelLoaded,
    Texture& tex, bool texLoaded)
{
    if (texLoaded) {
        glEnable(GL_TEXTURE_2D);
        if (glActiveTexture) glActiveTexture(GL_TEXTURE0);
        tex.Bind();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    if (modelLoaded) {
        glPushMatrix();
        glTranslated(pos.x(), pos.y(), pos.z());
        applyForwardOrientation(fwd);
        glScaled(segmentSize, segmentSize, segmentSize);
        model.Draw();
        glPopMatrix();
    }
    else {
        glDisable(GL_TEXTURE_2D);
        CubeRenderer::draw(pos, segmentSize);
    }

    glDisable(GL_TEXTURE_2D);
}

void Snake::draw()
{
    for (size_t i = 0; i < segments.size(); ++i)
    {
        Vector3 fwd(0, 0, 0);
        if (i == 0) {
            fwd = direction;
        }
        else {
            fwd = segments[i - 1] - segments[i];
            if (fwd.length() < 1e-9) fwd = direction;
        }

        if (i == 0)
            drawSegmentObj(segments[i], fwd, headModel, headModelLoaded, headTexture, headLoaded);
        else if (i + 1 == segments.size())
            drawSegmentObj(segments[i], fwd, tailModel, tailModelLoaded, tailTexture, tailLoaded);
        else
            drawSegmentObj(segments[i], fwd, bodyModel, bodyModelLoaded, bodyTexture, bodyLoaded);
    }
}

Vector3 Snake::getHeadPosition() const
{
    return segments[0];
}

static double pointToSegmentDist(const Vector3& p, const Vector3& a, const Vector3& b)
{
    Vector3 ab = b - a;
    double abLen2 = ab & ab;
    if (abLen2 < 1e-12) return (p - a).length();

    Vector3 ap = p - a;
    double t = (ap & ab) / abLen2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;

    Vector3 closest = a + ab * t;
    return (p - closest).length();
}

bool Snake::checkSelfCollision() const
{
    if (segments.size() < 2) return false;

    const Vector3& head = segments[0];

    const double threshold = followDistance * 0.7;

    for (size_t i = 1; i < segments.size(); ++i)
    {
        if ((head - segments[i]).length() < threshold)
            return true;

        if (i + 1 < segments.size())
        {
            if (pointToSegmentDist(head, segments[i], segments[i + 1]) < threshold)
                return true;
        }
    }
    return false;
}

bool Snake::checkWallCollision(float worldSize) const
{
    const Vector3& h = segments[0];
    return fabs(h.x()) >= worldSize ||
        fabs(h.y()) >= worldSize ||
        fabs(h.z()) >= worldSize;
}

void Snake::reset()
{
    direction = Vector3(-1, 0, 0);
    targetDirection = direction;
    upDirection = Vector3(0, 0, 1);
    targetUp = upDirection;

    segments.clear();
    Vector3 head(0, 0, 0);
    segments.push_back(head);
    segments.push_back(head - direction * followDistance);
    segments.push_back(head - direction * (followDistance * 2.0f));

    growCounter = 0;
    dead = false;
}

void Snake::changeTexture(const std::string& filename)
{
    bodyTexture.LoadTexture(filename);
    bodyLoaded = true;

    headTexture.LoadTexture(filename);
    headLoaded = true;

    tailTexture.LoadTexture(filename);
    tailLoaded = true;
}

void Snake::openTextureDialog()
{
    char path[260];
    if (!chooseImageFile(path)) return;
    changeTexture(path);
}

void Snake::openHeadTextureDialog()
{
    char path[260];
    if (!chooseImageFile(path)) return;
    headTexture.LoadTexture(path);
    headLoaded = true;
}

void Snake::openBodyTextureDialog()
{
    char path[260];
    if (!chooseImageFile(path)) return;
    bodyTexture.LoadTexture(path);
    bodyLoaded = true;
}

void Snake::openTailTextureDialog()
{
    char path[260];
    if (!chooseImageFile(path)) return;
    tailTexture.LoadTexture(path);
    tailLoaded = true;
}

void Snake::openHeadModelDialog()
{
    char objPath[260];
    if (!chooseObjFile(objPath)) return;
    headModelLoaded = (headModel.LoadModel(objPath) == 1);

    char texPath[260];
    if (chooseImageFile(texPath)) {
        headTexture.LoadTexture(texPath);
        headLoaded = true;
    }
}

void Snake::openBodyModelDialog()
{
    char objPath[260];
    if (!chooseObjFile(objPath)) return;
    bodyModelLoaded = (bodyModel.LoadModel(objPath) == 1);

    char texPath[260];
    if (chooseImageFile(texPath)) {
        bodyTexture.LoadTexture(texPath);
        bodyLoaded = true;
    }
}

void Snake::openTailModelDialog()
{
    char objPath[260];
    if (!chooseObjFile(objPath)) return;
    tailModelLoaded = (tailModel.LoadModel(objPath) == 1);

    char texPath[260];
    if (chooseImageFile(texPath)) {
        tailTexture.LoadTexture(texPath);
        tailLoaded = true;
    }
}