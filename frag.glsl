#version 330 core

out vec4 FragColor;

uniform float dmxData[15];

void main() {
    int blockX = int(gl_FragCoord.x / 64.0); 

    if (blockX < 15) {
        float brightness = dmxData[blockX]; 
        FragColor = vec4(brightness, brightness, brightness, 1.0); // Render grayscale block
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); 
    }
}
