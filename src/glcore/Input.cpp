#include "Input.h"
#include "Window.h"
#include "external\imgui.h"
#include "SceneManager.h"

bool Input::m_GUIHasFocus = false;
bool Input::m_NoInputFocus = true;
bool Input::m_MouseInWindow = true;
Ray Input::m_MouseRay;

std::vector<Input_MouseListener*> Input::m_MouseListeners;
Input_MouseListener* Input::m_MouseFocus;

Vector2 Input::m_NewMouseScroll;
Vector2 Input::m_MouseScroll;

Vector2 Input::m_NewMousePos;
Vector2 Input::m_MousePos;
Vector2 Input::m_MouseMovement;
Vector2 Input::m_MouseMovementOGL;

bool Input::m_NewMouseStates[3];
bool Input::m_MouseStates[3];
bool Input::m_OldMouseStates[3];

bool Input::m_NewKeyStates[512];
bool Input::m_KeyStates[512];
bool Input::m_OldKeyStates[512];




void Input::BindToWindow(GLFWwindow* window)
{
	m_GUIHasFocus = false;
	m_NoInputFocus = true;
	m_MouseInWindow = true;

	m_MouseListeners.clear();
	m_MouseFocus = NULL;

	m_NewMouseScroll = Vector2(0.f, 0.f);
	m_MouseScroll = Vector2(0.f, 0.f);
	m_MouseMovement = Vector2(0.f, 0.f);
	m_MouseMovementOGL = Vector2(0.f, 0.f);

	double mouse_x, mouse_y;
	glfwGetCursorPos(window, &mouse_x, &mouse_y);
	m_NewMousePos = Vector2((float)mouse_x, (float)mouse_y);
	m_MousePos = Vector2((float)mouse_x, (float)mouse_y);
	

	memset(m_NewMouseStates, 0, 3 * sizeof(bool));
	memset(m_MouseStates, 0, 3 * sizeof(bool));
	memset(m_OldMouseStates, 0, 3 * sizeof(bool));

	memset(m_NewKeyStates, 0, 512 * sizeof(bool));
	memset(m_KeyStates, 0, 512 * sizeof(bool));
	memset(m_OldKeyStates, 0, 512 * sizeof(bool));

	glfwSetMouseButtonCallback(window, &Input::Callback_MouseButton);
	glfwSetScrollCallback(window, &Input::Callback_MouseScroll);
	glfwSetKeyCallback(window, &Input::Callback_Keyboard);
	glfwSetCursorPosCallback(window, &Input::Callback_MouseMoved);
	glfwSetCursorEnterCallback(window, &Input::Callback_MouseEnter);
}

void Input::SwapBuffers()
{
	//Mouse Position
	m_MouseMovement = m_NewMousePos - m_MousePos;
	m_MousePos = m_NewMousePos;

	int width, height;
	Window::GetRenderDimensions(&width, &height);

	m_MouseMovementOGL = Vector2(m_MouseMovement.x / (float)width * 2.f, m_MouseMovement.y / (float)height * 2.f);

	//Mouse Scroll
	m_MouseScroll = m_NewMouseScroll;
	m_NewMouseScroll = Vector2(0.f, 0.f);

	//Mouse Buttons
	memcpy(m_OldMouseStates, m_MouseStates, 3 * sizeof(bool));
	memcpy(m_MouseStates, m_NewMouseStates, 3 * sizeof(bool));

	//Keys
	memcpy(m_OldKeyStates, m_KeyStates, 512 * sizeof(bool));
	memcpy(m_KeyStates, m_NewKeyStates, 512 * sizeof(bool));


	Input::m_GUIHasFocus = ImGui::GetIO().WantCaptureKeyboard;	
	UpdateMouseVars();	

	Input::m_NoInputFocus = (!m_GUIHasFocus && m_MouseFocus == NULL);
}

void Input::UpdateMouseVars()
{
	if (!m_MouseInWindow)
	{
		m_MouseFocus = NULL;
	}
	else
	{
		int width, height;
		Window::GetRenderDimensions(&width, &height);
		Matrix4 projview = SceneManager::Instance()->GetProjViewMatrix();

		Vector2 mouse_clip = Vector2(m_MousePos.x / (float)width, 1.f - (m_MousePos.y / (float)height));
		mouse_clip = mouse_clip * 2.f - 1.f;

		Matrix4 invProjview = Matrix4::Inverse(projview);
		m_MouseRay.pos = invProjview * Vector3(mouse_clip.x, mouse_clip.y, -1.f);

		Vector3 mouse_ray_far = invProjview * Vector3(mouse_clip.x, mouse_clip.y, 1.f);
		m_MouseRay.dir = mouse_ray_far - m_MouseRay.pos;
		m_MouseRay.dir.Normalise();


		//Handle Mouse RayCasts
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			ImGui::SetMouseCursor(0);

			m_NoInputFocus = true;
			const auto oldMouseFocus = m_MouseFocus;
			if (m_MouseFocus)
			{
				if (!m_MouseFocus->OnUpdateMouseInput(true))
					m_MouseFocus = NULL;
			}
			if (!m_MouseFocus)
			{
				for (int i = (int)m_MouseListeners.size() - 1; i >= 0 && m_MouseListeners[i] != oldMouseFocus; --i)
				{
					if (m_MouseListeners[i]->OnUpdateMouseInput(false))
					{
						m_MouseFocus = m_MouseListeners[i];
						break;
					}
				}
			}
		}
	}
}

void Input::Callback_MouseEnter(GLFWwindow* window, int entered)
{
	m_MouseInWindow = (bool)entered;
}

void Input::Callback_MouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (button >= 0 && button < 3)
	{
		m_NewMouseStates[button] = (action == GLFW_PRESS);
	}
}

void Input::Callback_MouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	m_NewMouseScroll.x += (float)xoffset;
	m_NewMouseScroll.y += (float)yoffset;
}

void Input::Callback_Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE)
	{
		if (key >= 0 && key < 512)
		{
			m_NewKeyStates[key] = (action == GLFW_PRESS);
		}
	}
}

void Input::Callback_MouseMoved(GLFWwindow* window, double xpos, double ypos)
{
	m_NewMousePos = Vector2((float)xpos, (float)ypos);
}