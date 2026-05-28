#include "Apple.h"
#include "CubeRenderer.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

ObjModel Apple::model;
bool Apple::modelLoaded = false;
unsigned int Apple::modelList = 0;

Texture Apple::modelTexture;
bool Apple::modelTextureLoaded = false;

static void appleRebuildList(unsigned int& list, ObjModel& m)
{
    if (list != 0) { glDeleteLists(list, 1); list = 0; }
    unsigned int id = glGenLists(1);
    if (id == 0) return;
    glNewList(id, GL_COMPILE);
    m.Draw();
    glEndList();
    list = id;
}

void Apple::ensureDefaults()
{
    if (!modelLoaded) {
        if (model.LoadModel("models/apple.obj") == 1) {
            modelLoaded = true;
            appleRebuildList(modelList, model);
        }
    }
    if (!modelTextureLoaded) {
        modelTexture.LoadTexture("textures/apple.jpg");
        modelTextureLoaded = true;
    }
}

Apple::Apple() : position(0, 0, 0), size(0.25f) { ensureDefaults(); }

void   Apple::setPosition(const Vector3& pos) { position = pos; }
Vector3 Apple::getPosition() const { return position; }

void Apple::loadModel(const std::string& filename)
{
    modelLoaded = (model.LoadModel(filename.c_str()) == 1);
    if (modelLoaded) appleRebuildList(modelList, model);
}

void Apple::loadModelTexture(const std::string& filename)
{
    modelTexture.LoadTexture(filename);
    modelTextureLoaded = true;
}

void Apple::loadModelAndTexture(const std::string& modelFile, const std::string& textureFile)
{
    loadModel(modelFile);
    loadModelTexture(textureFile);
}

void Apple::draw()
{
    if (modelLoaded && modelList != 0)
    {
        if (modelTextureLoaded) { glEnable(GL_TEXTURE_2D); modelTexture.Bind(); }
        else { glDisable(GL_TEXTURE_2D); }

        glPushMatrix();
        glTranslated(position.x(), position.y(), position.z());
        glScaled(size, size, size);
        glCallList(modelList);   // <-- одно обращение вместо обхода всей модели
        glPopMatrix();
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        CubeRenderer::draw(position, size);
    }
}