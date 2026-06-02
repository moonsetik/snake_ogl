#pragma once
#include "Vector3.h"
#include "Texture.h"
#include "ObjLoader.h"
#include <vector>
#include <string>

class Snake {
    std::vector<Vector3> segments;

    Vector3 direction;
    Vector3 targetDirection;

    float speed;
    float turnSpeed;
    float segmentSize;
    float followDistance;

    static ObjModel headModel;
    static ObjModel bodyModel;
    static ObjModel tailModel;

    static bool headModelLoaded;
    static bool bodyModelLoaded;
    static bool tailModelLoaded;

    Texture headTexture;
    Texture bodyTexture;
    Texture tailTexture;

    bool headLoaded;
    bool bodyLoaded;
    bool tailLoaded;

    int growCounter;
    bool dead;

    void drawSegmentObj(const Vector3& pos, const Vector3& fwd, ObjModel& model, bool modelLoaded, Texture& tex, bool texLoaded);

public:
    Snake();

    void initDefaults();

    void update(double deltaTime);
    void draw();

    void setDirection(const Vector3& newDir);

    void changeTexture(const std::string& filename);
    void openTextureDialog();

    void openHeadTextureDialog();
    void openBodyTextureDialog();
    void openTailTextureDialog();

    void openHeadModelDialog();
    void openBodyModelDialog();
    void openTailModelDialog();

    void grow(int amount = 1) { growCounter += amount; }

    Vector3 getHeadPosition() const;
    Vector3 getDirection() const { return direction; }

    bool checkSelfCollision() const;
    bool checkWallCollision(float worldSize) const;

    bool isDead() const { return dead; }
    void kill() { dead = true; }
    void reset();

    const std::vector<Vector3>& getSegments() const { return segments; }
    float getSegmentSize() const { return segmentSize; }
};