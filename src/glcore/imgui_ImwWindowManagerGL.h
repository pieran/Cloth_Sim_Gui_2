//#pragma once
//#include "external/imgui.h"
//#include "external/ImWindow/ImwConfig.h"
//#include "external/ImWindow/ImwWindowManager.h"
//
//namespace ImWindow
//{
//	class ImwWindowManagerGL : public ImwWindowManager
//	{
//	public:
//		ImwWindowManagerGL();
//		virtual							~ImwWindowManagerGL();
//	protected:
//		virtual bool					CanCreateMultipleWindow() { return true; }
//		virtual ImwPlatformWindow*		CreatePlatformWindow(bool bMain, ImwPlatformWindow* pParent, bool bDragWindow);
//
//		virtual ImVec2					GetCursorPos();
//		virtual bool					IsLeftClickDown();
//	};
//}