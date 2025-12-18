#pragma once

#include <string_view>
#ifndef SHADER_H
#define SHADER_H

#include <string>
#include "./external/vendor/glad.h"

class Shader {
public:
    unsigned int ID;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    Shader(std::string_view vertexSrc, std::string_view fragmentSrc);
    void use() const;
    void setFloat(const std::string& name, float value) const;
    void setFloatArray(const std::string& name, const float* values, int size) const;

private:
    unsigned int compileFromSrc(const std::string_view source, GLenum type);
    unsigned int compileFromFile(const std::string& path, GLenum type);
};

#endif
