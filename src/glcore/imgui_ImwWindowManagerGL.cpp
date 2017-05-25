//#include "imgui_ImwWindowManagerGL.h"
//#include "ImwPlatformWindowDX11.h"
//
//#include <imgui_impl_dx11.h>
//
//using namespace ImWindow;
//
//ImwWindowManagerGL::ImwWindowManagerGL()
//{
//	ImwPlatformWindowDX11::InitDX11();
//}
//
//ImwWindowManagerGL::~ImwWindowManagerGL()
//{
//	ImwPlatformWindowDX11::ShutdownDX11();
//	//ImGui_ImplDX11_Shutdown();
//}
//
//ImwPlatformWindow* ImwWindowManagerGL::CreatePlatformWindow(bool bMain, ImwPlatformWindow* pParent, bool bDragWindow)
//{
//	IM_ASSERT(m_pCurrentPlatformWindow == NULL);
//	ImwPlatformWindowDX11* pWindow = new ImwPlatformWindowDX11(bMain, bDragWindow, CanCreateMultipleWindow());
//	if (pWindow->Init(pParent))
//	{
//		return (ImwPlatformWindow*)pWindow;
//	}
//	else
//	{
//		delete pWindow;
//		return NULL;
//	}
//}
//
//ImVec2 ImwWindowManagerGL::GetCursorPos()
//{
//	POINT oPoint;
//	::GetCursorPos(&oPoint);
//	return ImVec2(oPoint.x, oPoint.y);
//}
//
//bool ImwWindowManagerGL::IsLeftClickDown()
//{
//	return GetAsyncKeyState(VK_LBUTTON);
//}
