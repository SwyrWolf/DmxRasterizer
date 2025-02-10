#version 460 core

out vec4 FragColor;

uniform sampler1D dmxDataTexture; // DMX data texture
uniform vec2 resolution;          // Screen resolution

void main() {
	vec2 blockSize = vec2(16.0, 16.0);
	ivec2 blockCoord = ivec2(gl_FragCoord.x / blockSize.x, (resolution.y - gl_FragCoord.y) / blockSize.y);
	
	int blockIndex = blockCoord.y + blockCoord.x * int(resolution.y / blockSize.y);
	blockIndex = clamp(blockIndex, 0, 1559);

	float brightness = texelFetch(dmxDataTexture, blockIndex, 0).r;
	FragColor = vec4(brightness, brightness, brightness, 1.0);
}