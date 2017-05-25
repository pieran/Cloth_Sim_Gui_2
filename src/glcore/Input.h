#pragma once

//Wrapper for all keyboard/mouse crap
#include "external\glad.h"
#include "external\glfw3.h"
#include "Vector2.h"
#include "Matrix4.h"
#include "Ray.h"
#include <vector>
#include <functional>

#define MOUSE_LEFT 0
#define MOUSE_RIGHT 1
#define MOUSE_MIDDLE 2


enum KeyModifier {
	KeyModifier_Shift = 0,
	KeyModifier_Control,
};

class Input_MouseListener
{
public:
	//returns true if focus is requested, else false to pass mouse input down the chain
	virtual bool OnUpdateMouseInput(bool hasFocus) = 0;

};

class Input
{
	friend class Window;
	friend class ImGui_Wrapper;
public:
	inline static const Vector2& GetMousePos() { return m_MousePos; }
	inline static const Vector2& GetMouseMovement() { return m_MouseMovement; }
	inline static const Vector2& GetMouseMovementClipSpace() { return m_MouseMovementOGL; }
	inline static const Ray& GetMouseWSRay() { return m_MouseRay; }
	inline static const Vector2& GetMouseScroll() { return m_MouseScroll; }

	inline static bool IsKeyDown(int keycode) { return !m_GUIHasFocus && m_KeyStates[keycode]; }
	inline static bool IsKeyUp(int keycode) { return m_GUIHasFocus || !m_KeyStates[keycode]; }
	inline static bool IsKeyHeld(int keycode) { return !m_GUIHasFocus && m_KeyStates[keycode] && m_OldKeyStates[keycode]; }
	inline static bool IsKeyToggled(int keycode) { return !m_GUIHasFocus && m_KeyStates[keycode] && !m_OldKeyStates[keycode]; }

	inline static bool IsKeyModifierActive(KeyModifier key) 
	{ 
		bool mod_active;
		switch (key)
		{
		case KeyModifier_Shift:
			mod_active = m_KeyStates[GLFW_KEY_LEFT_SHIFT] || m_KeyStates[GLFW_KEY_RIGHT_SHIFT];
			break;
		case KeyModifier_Control:
			mod_active = m_KeyStates[GLFW_KEY_LEFT_CONTROL] || m_KeyStates[GLFW_KEY_RIGHT_CONTROL];
			break;
		default:
			mod_active = false;
		}
		return m_NoInputFocus && mod_active;
	}

	inline static bool IsMouseDown(int mouseidx) { return m_NoInputFocus && m_MouseStates[mouseidx]; }
	inline static bool IsMouseUp(int mouseidx) { return !m_NoInputFocus || !m_MouseStates[mouseidx]; }
	inline static bool IsMouseHeld(int mouseidx) { return m_NoInputFocus && m_MouseStates[mouseidx] && m_OldMouseStates[mouseidx]; }
	inline static bool IsMouseToggled(int mouseidx) { return m_NoInputFocus && m_MouseStates[mouseidx] && !m_OldMouseStates[mouseidx]; }

	inline static const bool* GetKeyboardStatesArray() { return m_KeyStates; }

	inline static void AddMouseListener(Input_MouseListener* listener)
	{
		m_MouseListeners.push_back(listener);
	}
	inline static void RemoveMouseListener(Input_MouseListener* listener)
	{
		for (auto itr = m_MouseListeners.begin(), end = m_MouseListeners.end(); itr != end; ++itr)
		{
			if (*itr == listener)
			{
				m_MouseListeners.erase(itr);
				return;
			}
		}
	}

private:
	static void BindToWindow(GLFWwindow* window);
	static void SwapBuffers();
	static void UpdateMouseVars();

	static void Callback_MouseButton(GLFWwindow* window, int button, int action, int mods);
	static void Callback_MouseScroll(GLFWwindow* window, double xoffset, double yoffset);
	static void Callback_Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Callback_MouseMoved(GLFWwindow* window, double xpos, double ypos);
	static void Callback_MouseEnter(GLFWwindow* window, int entered);

	static bool m_NoInputFocus;
	static bool m_GUIHasFocus;

	

	static bool m_MouseInWindow;
	static Vector2 m_NewMouseScroll;
	static Vector2 m_MouseScroll;

	static Vector2 m_NewMousePos;
	static Vector2 m_MousePos;
	static Vector2 m_MouseMovement;
	static Ray m_MouseRay;

	//Clip-space mouse movement
	static Vector2 m_MouseMovementOGL;
	
	static bool m_NewMouseStates[3];
	static bool m_MouseStates[3];
	static bool m_OldMouseStates[3];

	static bool m_NewKeyStates[512];
	static bool m_KeyStates[512];
	static bool m_OldKeyStates[512];

	static std::vector<Input_MouseListener*> m_MouseListeners;
	static Input_MouseListener* m_MouseFocus;
	
private:
	Input() {}
	~Input() {}
	Input(Input& cpy) {}

	
};