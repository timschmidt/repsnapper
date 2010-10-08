#include "stdafx.h"

// Program Entry (WinMain)

//#include <windows.h>													// Header File For The Windows Library

#include "gcode.h"
#include "ui.h"

CubeViewUI *cvui;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	cvui=new CubeViewUI;

	Fl::visual(FL_DOUBLE|FL_INDEX);

	char WindowTitle[100] = "GCodeView";
	char* W = &WindowTitle[0];
	cvui->show(1,&W);
//	cvui->code->Read("c:/ViewMe.gcode");

	return Fl::run();
}
