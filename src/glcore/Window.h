#pragma once

#include "external\glad.h"
#include "external\glfw3.h"

#include "TSingleton.h"
#include "TCallbackList.h"
#include "Vector2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "VideoEncoder.h"

//Singleton wrapper around glfw window object
class Window
{
public:
	static bool Initialize(int width, int height, std::string title);
	static bool Destroy();
	
	static VideoEncoder* GetVideoEncoder() { return m_Encoder; }

	static void GetRenderDimensions(int* o_width, int* o_height) { *o_width = m_FrameWidth; *o_height = m_FrameHeight; }
	static void SetRenderDimensions(int width, int height);
	static float GetAspectRatio() { return m_AspectRatio; }

	static void SetWindowTitle(std::string title, ...);
	

	static inline bool IsInitialized() { return (glfwWindow != NULL); }
	static inline int WindowShouldClose() { return glfwWindowShouldClose(glfwWindow); }

	static void BeginFrame();
	static void EndFrame();
public:
	static GLFWwindow* glfwWindow;

protected:
	static int m_FrameWidth, m_FrameHeight;
	static float m_AspectRatio;

	static void glfw_frameresized_callback(GLFWwindow* window, int width, int height);

	static VideoEncoder* m_Encoder;
private:
	Window() {}
	~Window() {}
	Window(Window& w) {}
};