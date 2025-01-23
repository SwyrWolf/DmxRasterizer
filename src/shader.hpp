#pragma once

#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/glad.h>

class Shader {
public:
    unsigned int ID;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;
    void setFloat(const std::string& name, float value) const;
    void setFloatArray(const std::string& name, const float* values, int size) const;

private:
    unsigned int compileShader(const std::string& path, GLenum type);
};

#endif
