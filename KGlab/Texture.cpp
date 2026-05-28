#include "Texture.h"

#include <windows.h>
#include <GL/gl.h>
#include <cstring>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::~Texture()
{
    if (texId != 0)
        glDeleteTextures(1, &texId);
}

void Texture::LoadTexture(const std::string& texture_file_name)
{
    if (texId != 0)
        glDeleteTextures(1, &texId);

    int x = 0, y = 0, n = 0;
    unsigned char* data = stbi_load(texture_file_name.c_str(), &x, &y, &n, 4);

    if (!data || x <= 0 || y <= 0)
    {
        if (data)
            stbi_image_free(data);

        texId = 0;
        OutputDebugStringA((std::string("Texture load failed: ") + texture_file_name + "\n").c_str());
        return;
    }

    glGenTextures(1, &texId);

    int curent_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &curent_texture);

    glBindTexture(GL_TEXTURE_2D, texId);

    unsigned char* _tmp = new unsigned char[x * 4];
    for (int i = 0; i < y / 2; ++i)
    {
        std::memcpy(_tmp, data + i * x * 4, x * 4);
        std::memcpy(data + i * x * 4, data + (y - 1 - i) * x * 4, x * 4);
        std::memcpy(data + (y - 1 - i) * x * 4, _tmp, x * 4);
    }
    delete[] _tmp;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, curent_texture);
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, texId);
}