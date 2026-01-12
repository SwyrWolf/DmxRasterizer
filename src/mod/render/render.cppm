module;

#include "../../external/vendor/glad.h"
#include <glfw3.h>
#include <SpoutGL/SpoutSender.h>

export module render;
import weretype;
import appState;

f32 vertices[] = {
	-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
	-1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
	1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
	
	-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
	1.0f, -1.0f,  1.0f, 0.0f, // Bottom-right
	1.0f,  1.0f,  1.0f, 1.0f  // Top-right
};

bool initGraphics() {
	if (!glfwInit()) {
		return false;
	}

	if(!app::debugMode) {
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}

	return true;
}