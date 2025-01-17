#version 330 core

out vec4 FragColor;

uniform sampler1D dmxDataTexture; // DMX data texture
uniform vec2 resolution;          // Screen resolution

void main() {
    vec2 blockSize = vec2(16.0, 16.0);
    ivec2 blockCoord = ivec2(gl_FragCoord.xy / blockSize);
    int blockIndex = blockCoord.x + blockCoord.y * int(resolution.x / blockSize.x);
    blockIndex = clamp(blockIndex, 0, 1559);
    float brightness = texture(dmxDataTexture, float(blockIndex) / 1560.0).r;
    FragColor = vec4(brightness, brightness, brightness, 1.0);
}
