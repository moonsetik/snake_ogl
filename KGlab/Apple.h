#pragma once
#include "Vector3.h"
#include "Texture.h"
#include "ObjLoader.h"
#include <string>

class Apple {
    Vector3 position;
    float size;

    static ObjModel model;
    static bool modelLoaded;
    static unsigned int modelList; // display list

    static Texture modelTexture;
    static bool modelTextureLoaded;

    static void ensureDefaults();

public:
    Apple();

    void setPosition(const Vector3& pos);
    Vector3 getPosition() const;

    static void loadModel(const std::string& filename);
    static void loadModelTexture(const std::string& filename);
    static void loadModelAndTexture(const std::string& modelFile, const std::string& textureFile);

    void draw();

    float getSize() const { return size; }
};