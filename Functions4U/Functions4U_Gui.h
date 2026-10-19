// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#ifndef _Functions4U_Functions4U_Gui_h_
#define _Functions4U_Functions4U_Gui_h_

#include "Functions4U.h"

namespace Upp {

Image NativePathIconX(const char *path, bool folder, int flags = 0);

class CParserPlus : public CParser {
public:
	explicit CParserPlus(const char *ptr) : CParser(ptr) {}
	String ReadIdPlus();
};

bool SaveImage(const Image &img, int qualityBpp, const String &fileName, String ext = "");
bool PrintImage(const Image &img, int x = 0, int y = 0, int width = Null, int height = Null);
void DrawRectLine(Draw& w, int x, int y, int width, int height, int lineWidth, const Color &color);
void DrawRectLine(Draw& w, Point &pos, Size &s, int lineWidth, const Color &color);
void DrawRectLine(Draw& w, Rect &r, int lineWidth, const Color &color);

int GetEditWidth(const String &str, const Font font);

class ConsoleOutput {
public:
	ConsoleOutput() 						 {Init();}
	explicit ConsoleOutput(bool forceWindow) {Init(forceWindow);}
	virtual ~ConsoleOutput(); 
	
	bool Init(bool forceWindow = false);

private:
#ifdef PLATFORM_WIN32
	bool ownConsole;
#endif
};

void ArrayCtrlWhenBar(Bar &menu, ArrayCtrl &array, bool header = true, bool edit = false, const Function<void ()>& whenAction = Null);
void ArrayCtrlRowCopy(const ArrayCtrl &array, bool header);
void ArrayCtrlRowPaste(ArrayCtrl &array);
void ArrayCtrlRowSelect(ArrayCtrl &array);
Vector<int> ArrayCtrlSelectedGet(const ArrayCtrl &array);
int ArrayCtrlSelectedGetCount(const ArrayCtrl &array);
Vector<Vector<Value>> ArrayCtrlGet(const ArrayCtrl &arr);
void ArrayCtrlSet(ArrayCtrl &array, const Vector<Vector<Value>> &vals, int fromRow = 0, int fromCol = 0);

void ArrayCtrlFill(ArrayCtrl &array, const Grid &g, bool removeEmpty);
void ArrayCtrlVirtual(ArrayCtrl &array, const Grid &g);
void ArrayCtrlVirtual_SetHeader(ArrayCtrl &array, Grid &g, int id, String text);
void ArrayCtrlVirtual_UpdateHeaders(ArrayCtrl &array, const Grid &g);
	
template <class T>
T *GetParentCtrlP(Ctrl *ths) {
	while (ths->GetParent() != nullptr) {
		ths = ths->GetParent();
		T *main;
		if ((main = dynamic_cast<T*>(ths)) != nullptr)
			return main;
	}
	return nullptr;
}

template <class T>
T &GetParentCtrl(Ctrl *ths) {
	T *p = GetParentCtrlP<T>(ths);
	if (p)
		return *p;
	NEVER_(t_("Parent does not found in GetParentCtrl"));
	throw Exc(t_("Parent does not found in GetParentCtrl"));
}

template <class T>
class WithEvents : public T {
public:
	Function<void(Point, dword)> OnMouseEnter;
	Function<void(Point, dword)> OnMouseMove;
	Function<void(Point, dword)> OnLeftDown;
	Function<void(Point, dword)> OnLeftDouble;
	Function<void(Point, dword)> OnRightDown;
	Function<void(Point, dword)> OnRightDouble;
	Function<void(Point, dword)> OnMiddleDown;
	Function<void(Point, dword)> OnMiddleDouble;
	Function<void(Point, int, dword)> OnMouseWheel;
	Function<void()> OnMouseLeave;
	Function<void()> OnFocus;
	Function<void()> OnLostFocus;
	
private:
	virtual void MouseEnter(Point p, dword keyflags)	{OnMouseEnter(p, keyflags);}
	virtual void MouseMove(Point p, dword keyflags)		{OnMouseMove(p, keyflags);}
	virtual void LeftDown(Point p, dword keyflags)		{OnLeftDown(p, keyflags);}
	virtual void LeftDouble(Point p, dword keyflags)	{OnLeftDouble(p, keyflags);}
	virtual void RightDown(Point p, dword keyflags)		{OnRightDown(p, keyflags);}
	virtual void RightDouble(Point p, dword keyflags)	{OnRightDouble(p, keyflags);}
	virtual void MiddleDown(Point p, dword keyflags)	{OnMiddleDown(p, keyflags);}
	virtual void MiddleDouble(Point p, dword keyflags)	{OnMiddleDouble(p, keyflags);}
	virtual void MouseWheel(Point p, int zdelta, dword keyflags){OnMouseWheel(p, zdelta, keyflags);}
	virtual void MouseLeave()							{OnMouseLeave();}
	virtual void GotFocus() 						  	{OnFocus();}
	virtual void LostFocus() 						  	{OnLostFocus();}
};

int GetDropWidth(const DropChoice &drop);
int GetDropWidth(const DropList &drop);
	
}

#endif
