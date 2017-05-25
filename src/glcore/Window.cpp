#include "Window.h"
#include <functional>
#include "imgui_wrapper.h"
#include "Input.h"

GLFWwindow* Window::glfwWindow = NULL;
int Window::m_FrameWidth = 0;
int Window::m_FrameHeight = 0;
float Window::m_AspectRatio = 0.0f;
VideoEncoder* Window::m_Encoder = NULL;


void Window::glfw_frameresized_callback(GLFWwindow* window, int width, int height)
{
	Window::m_FrameHeight = height;
	Window::m_FrameWidth = width;
	Window::m_AspectRatio = width / (float)height;
}

bool Window::Initialize(int width, int height, std::string title)
{
	glfwWindow = NULL;

	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

	glfwWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (!glfwWindow)
	{
		return false;
	}

	glfwMakeContextCurrent(glfwWindow);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	ImGui_Wrapper::Init(glfwWindow, true);

	using namespace std::placeholders;
	glfwSetWindowSizeCallback(glfwWindow, &Window::glfw_frameresized_callback);
	Input::BindToWindow(glfwWindow);

	glfwGetWindowSize(glfwWindow, &m_FrameWidth, &m_FrameHeight);
	m_AspectRatio = width / (float)height;

	m_Encoder = new VideoEncoder();
	
	return true;
}

bool Window::Destroy()
{
	if (glfwWindow)
	{
		glfwDestroyWindow(glfwWindow);
		glfwWindow = NULL;

		m_Encoder->EndEncoding();
		delete m_Encoder;
		m_Encoder = NULL;
	}
	ImGui_Wrapper::Shutdown();
	glfwTerminate();

	return true;
}

void Window::BeginFrame()
{
	glfwPollEvents();
	ImGui_Wrapper::NewFrame();
	Input::SwapBuffers();
}


void Window::EndFrame()
{
	static bool imgui_metrics_window = false;
	static bool imgui_demo_window = false;
	static bool imgui_style_window = false;
	static bool imgui_guide_window = false;
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("ImGUI"))
		{
			ImGui::MenuItem("Metrics", "", &imgui_metrics_window);
			ImGui::MenuItem("Style Editor", "", &imgui_style_window);
			ImGui::MenuItem("User Guide", "", &imgui_guide_window);					
			ImGui::MenuItem("Demo Window", "", &imgui_demo_window);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (imgui_metrics_window)
	{
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiSetCond_FirstUseEver);
		ImGui::ShowMetricsWindow(&imgui_metrics_window);
	}

	if (imgui_demo_window)
		ImGui::ShowTestWindow(&imgui_demo_window);

	if (imgui_style_window)
	{
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Style Reference", &imgui_style_window);
		ImGui::ShowStyleEditor();
		ImGui::End();
	}

	if (imgui_guide_window)
	{
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("ImGui Guide", &imgui_guide_window);
		ImGui::ShowUserGuide();
		ImGui::End();
	}

	ImGui::Render();
	glfwSwapBuffers(glfwWindow);

	m_Encoder->EncodeFrame();
}

void Window::SetWindowTitle(std::string title, ...)
{
	va_list args;
	va_start(args, title);

	char buf[1024];
	memset(buf, 0, 1024);

	vsnprintf_s(buf, 1023, _TRUNCATE, title.c_str(), args);
	va_end(args);

	glfwSetWindowTitle(glfwWindow, buf);
}

void Window::SetRenderDimensions(int width, int height)
{
	m_FrameWidth = width;
	m_FrameHeight = height;
	m_AspectRatio = m_FrameWidth / (float)m_FrameHeight;

	glfwSetWindowSize(glfwWindow, m_FrameWidth, m_FrameHeight);
}