# OpenGL docs

// 
## Main.cpp
```cpp
glBindTexture(GL_TEXTURE_1D, dmxDataTexture);
glTexSubImage1D(GL_TEXTURE_1D, 0, 0, ArtNet::TOTAL_DMX_CHANNELS, GL_RED, GL_FLOAT, dmxDataNormalized);
```

## frag.glsl
```cpp
#version 330 core

out vec4 FragColor;

uniform sampler1D dmxDataTexture; // DMX data texture
uniform vec2 resolution;          // Screen resolution

void main() {
	vec2 blockSize = vec2(16.0, 16.0);
	ivec2 blockCoord = ivec2(gl_FragCoord.x / blockSize.x, (resolution.y - gl_FragCoord.y) / blockSize.y);
	
	int blockIndex = blockCoord.y + blockCoord.x * int(resolution.y / blockSize.y);
	blockIndex = clamp(blockIndex, 0, 1559);
	float brightness = texture(dmxDataTexture, float(blockIndex) / 1560.0).r;
	FragColor = vec4(brightness, brightness, brightness, 1.0);
}
```

## glad.h
```c
#define GL_TEXTURE_1D 0x0DE0 // 36227
```

https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage1D.xhtml