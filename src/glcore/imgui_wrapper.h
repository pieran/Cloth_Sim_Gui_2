// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "external\glad.h"
#include "external\imgui.h"
#include "external\glfw3.h"

enum Cursor
{
	Cursor_Default = 0,
	Cursor_IBeam,
	Cursor_Move,
	Cursor_VResize,
	Cursor_HResize,
	Cursor_NESWResize,
	Cursor_NWSEResize
};

class ImGui_Wrapper
{
public:
	
	static bool        Init(GLFWwindow* window, bool install_callbacks);
	static void        Shutdown();
	static void        NewFrame();

	// Use if you want to reset your rendering device without losing ImGui state.
	static void        InvalidateDeviceObjects();
	static bool        CreateDeviceObjects();

	static void			SetCursor(Cursor cursor) { ImGui::SetMouseCursor(cursor); }

private:
	ImGui_Wrapper() {}
	~ImGui_Wrapper() {}
	ImGui_Wrapper(ImGui_Wrapper& a) {}

	static void RenderDrawLists(ImDrawData* draw_data);
	static const char* GetClipboardText(void* user_data);
	static void SetClipboardText(void* user_data, const char* text);
	static void CharCallback(GLFWwindow*, unsigned int c);
	static bool CreateFontsTexture();

	// Data
	static GLFWwindow*  g_Window;
	static double       g_Time;
	static GLFWcursor*  g_Cursors[ImGuiMouseCursor_Count_];

	static GLuint       g_FontTexture;
	static int          g_ShaderHandle, g_VertHandle, g_FragHandle;
	static int          g_AttribLocationTex, g_AttribLocationProjMtx;
	static int          g_AttribLocationPosition, g_AttribLocationUV, g_AttribLocationColor;
	static unsigned int g_VboHandle, g_VaoHandle, g_ElementsHandle;
};