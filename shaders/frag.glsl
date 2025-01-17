#version 330 core

out vec4 FragColor;

uniform float dmxData[512];

void main() {
    int blockX = int(gl_FragCoord.x / 8.0);
    int blockY = int(gl_FragCoord.y / 8.0);

    // 512 * 8 = 4096 -- 4096 / 4 = 1024
    int blockIndex = blockX + blockY * (1024/8);

    if (blockIndex < 512) {
        float brightness = dmxData[blockIndex];
        FragColor = vec4(brightness, brightness, brightness, 1.0); // Render grayscale block
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); 
    }
}
