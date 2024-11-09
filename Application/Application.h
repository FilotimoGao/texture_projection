#pragma once
#include"Texture.h"
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>

#define app Application::getInstance()

class Application {
public:
	~Application();
	static Application* getInstance();

	bool init(const int& width, const int& height);
	void update();
	void destory();

private:
	Application();
	static Application* instance;
	uint32_t appWidth;
	uint32_t appHeight;
	GLFWwindow* appWindow;
};
