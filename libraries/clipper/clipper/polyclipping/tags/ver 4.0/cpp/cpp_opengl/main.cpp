#include <windows.h>
#include <commctrl.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <ctime>
#include <cmath>
#include <cstring>
#include <sstream>
#include "clipper.hpp"

using namespace std;
using namespace clipper;

enum poly_color_type { pctSubject, pctClip, pctSolution };

//global vars ...
HWND		 hWnd;
HWND         hStatus; 
HDC			 hDC;
HGLRC		 hRC;
ClipType     ct = ctIntersection;
PolyFillType pft = pftEvenOdd;
Polygons sub, clp, sol;

//------------------------------------------------------------------------------

wstring str2wstr(const std::string &s) {
	int slength = (int)s.length() + 1;
	int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
	wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}
//------------------------------------------------------------------------------

struct cleanupItem {   
    GLdouble *pvert;   
    cleanupItem* next;
};   
//------------------------------------------------------------------------------

// Set up pixel format for graphics initialization
void SetupPixelFormat()
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;
    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 16;
    ppfd->cDepthBits = 16;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    pixelformat = ChoosePixelFormat(hDC, ppfd);
    SetPixelFormat(hDC, pixelformat, ppfd);
}
//------------------------------------------------------------------------------

// Initialize OpenGL graphics
void InitGraphics()
{
    hDC = GetDC(hWnd);
    SetupPixelFormat();
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
//------------------------------------------------------------------------------

Polygons MakeRandomPoly(int width, int height, int edgeCount)
{
	Polygons result(1);
	result[0].resize(edgeCount);
	for (int i = 0; i < edgeCount; i++)
	{
		result[0][i].X = rand()%(width-20) +10;
		result[0][i].Y = rand()%(height-20) +10;
	}
	return result;
}
//------------------------------------------------------------------------------

void ResizeGraphics(int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//------------------------------------------------------------------------------
// heap memory management for GLUtesselator ...
//------------------------------------------------------------------------------
   
cleanupItem* cleanupStore = NULL;   
   
void AddToCleanup(GLdouble *ptr)   
{   
    cleanupItem* temp=new cleanupItem;   
    temp->pvert=ptr;   
    temp->next = cleanupStore;   
    cleanupStore = temp;   
}   
   
void DoCleanup()   
{   
    if(cleanupStore == NULL) return;   
	cleanupItem* ci= cleanupStore;
    while(ci != NULL)   
    {   
        cleanupItem* temp = ci;   
        delete[] ci->pvert;   
        ci = ci->next;   
        delete temp;   
    };   
    cleanupStore = NULL;   
}   

//------------------------------------------------------------------------------
// GLUtesselator callback functions ...
//------------------------------------------------------------------------------

void CALLBACK BeginCallback(GLenum type)   
{   
    glBegin(type);   
} 

void CALLBACK EndCallback()   
{   
    glEnd();   
}

void CALLBACK VertexCallback(GLvoid *vertex)   
{   
	glVertex3dv( (const double *)vertex );   
} 

void CALLBACK CombineCallback(GLdouble coords[3], GLdouble *data[4], GLfloat weight[4], GLdouble **dataOut )   
{   
	GLdouble *vert = new GLdouble[3];
	vert[0] = coords[0];   
    vert[1] = coords[1];   
    vert[2] = coords[2];  
	*dataOut = vert;
	AddToCleanup(vert);
}   

void CALLBACK ErrorCallback(GLenum errorCode)   
{   
	std::wstring s = str2wstr( (char *)gluErrorString(errorCode) );
	SetWindowText(hWnd, s.c_str());
}   
//------------------------------------------------------------------------------

void DrawPolygon(clipper::Polygons &pgs, poly_color_type pct)
{
	switch (pct)
	{
		case pctSubject: glColor4f(0.0f, 0.0f, 1.0f, 0.05f); break;
		case pctClip: glColor4f(1.0f, 0.0f, 0.0f, 0.05f); break;
		default: glColor4f(0.0f, 1.0f, 0.0f, 0.1f);
	}

	GLUtesselator* tess = gluNewTess();
    gluTessCallback(tess, GLU_TESS_BEGIN, (void (CALLBACK*)())&BeginCallback);    
    gluTessCallback(tess, GLU_TESS_VERTEX, (void (CALLBACK*)())&VertexCallback);    
    gluTessCallback(tess, GLU_TESS_END, (void (CALLBACK*)())&EndCallback);   
    gluTessCallback(tess, GLU_TESS_COMBINE, (void (CALLBACK*)())&CombineCallback);   
    gluTessCallback(tess, GLU_TESS_ERROR, (void (CALLBACK*)())&ErrorCallback);   
	
	if (pft == pftEvenOdd)
		gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD); else
		gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);

	gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE); //GL_FALSE
	gluTessBeginPolygon(tess, NULL); 
	for (clipper::Polygons::size_type i = 0; i < pgs.size(); ++i)
	{
		gluTessBeginContour(tess);
		for (clipper::Polygon::size_type j = 0; j < pgs[i].size(); ++j)
		{
			GLdouble *vert = new GLdouble[3];
			vert[0] = pgs[i][j].X;
			vert[1] = pgs[i][j].Y;
			vert[2] = 0;
			AddToCleanup(vert);
			gluTessVertex(tess, vert, vert); 
		}
		gluTessEndContour(tess); 
	}
	gluTessEndPolygon(tess);
	DoCleanup();

	switch (pct)
	{
		case pctSubject: glColor4f(0.0f, 0.0f, 0.3f, 0.3f); break;
		case pctClip: glColor4f(0.3f, 0.0f, 0.0f, 0.3f); break;
		default: glColor4f(0.0f, 0.2f, 0.0f, 0.5f);
	}
	if (pct == pctSolution) glLineWidth(1.2f); else glLineWidth(1);

	gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE); //GL_FALSE
	gluTessBeginPolygon(tess, NULL); 
	for (clipper::Polygons::size_type i = 0; i < pgs.size(); ++i)
	{
		gluTessBeginContour(tess);
		for (clipper::Polygon::size_type j = 0; j < pgs[i].size(); ++j)
		{
			GLdouble *vert = new GLdouble[3];
			vert[0] = pgs[i][j].X;
			vert[1] = pgs[i][j].Y;
			vert[2] = 0;
			AddToCleanup(vert);
			gluTessVertex(tess, vert, vert); 
		}
		gluTessEndContour(tess);
	}
	gluTessEndPolygon(tess);

	//final cleanup ...
	gluDeleteTess(tess);
	DoCleanup();
}
//------------------------------------------------------------------------------

void DrawGraphics()
{
	//this can take a few moments ...
	HCURSOR hWaitCursor = LoadCursor(NULL, IDC_WAIT);
	SetCursor(hWaitCursor);
	SetClassLong(hWnd, GCL_HCURSOR, (DWORD)hWaitCursor);

	//fill background with a light off-gray color ...
	glClearColor(GLclampf(0.9), GLclampf(0.9), GLclampf(0.85), 1);
	glClear(GL_COLOR_BUFFER_BIT);

	DrawPolygon(sub, pctSubject);
	DrawPolygon(clp, pctClip);
	DrawPolygon(sol, pctSolution);
	
	std::wstring s;
	switch (ct)
	{
		case ctUnion: s = L"Clipper Demo - Union"; break;
		case ctDifference: s = L"Clipper Demo - Difference"; break;
		case ctXor: s = L"Clipper Demo - XOR"; break;
		default: s = L"Clipper Demo - Intersection"; break;
	}
	if (pft == pftEvenOdd) s += L" (EvenOdd fill)"; 
	else s += L" (NonZero fill)"; 
	SetWindowText(hWnd, s.c_str());

	HCURSOR hArrowCursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(hArrowCursor);
	SetClassLong(hWnd, GCL_HCURSOR, (DWORD)hArrowCursor);
}
//------------------------------------------------------------------------------

void UpdatePolygons(bool updateSolutionOnly)
{
	if (!updateSolutionOnly)
	{
		RECT r;
		GetWindowRect(hStatus, &r);
		int statusHeight = r.bottom - r.top;
		GetClientRect(hWnd, &r);
		sub = MakeRandomPoly(r.right, r.bottom - statusHeight, 50);
		clp = MakeRandomPoly(r.right, r.bottom - statusHeight, 50);
	}
	Clipper c4;
	c4.AddPolygons(sub, ptSubject);
	c4.AddPolygons(clp, ptClip);
	c4.Execute(ct, sol, pft, pft);

	InvalidateRect(hWnd, NULL, false); 
}

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM  wParam, LPARAM  lParam)
{
	int clientwidth, clientheight;
    switch (uMsg)
    {

	case WM_SIZE:
		clientwidth = LOWORD(lParam);
		clientheight = HIWORD(lParam);
		ResizeGraphics(clientwidth, clientheight);
		SetWindowPos(hStatus, NULL, 0, clientheight, clientwidth, 0, SWP_NOACTIVATE | SWP_NOZORDER);
        return 0;

	case WM_PAINT:
		HDC hdc;
		PAINTSTRUCT ps;
		hdc = BeginPaint(hWnd, &ps);
		//do the drawing ...
		DrawGraphics();
		SwapBuffers(hdc);
		EndPaint(hWnd, &ps);		
		return 0;

    case WM_CLOSE: 
        DestroyWindow(hWnd);
        return 0;
 
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

	case WM_HELP:
        MessageBox(hWnd, 
			L"Clipper Demo tips...\n\n"
			L"I - for Intersection operations.\n"
			L"U - for Union operations.\n" 
			L"D - for Difference operations.\n"
			L"X - for XOR operations.\n" 
			L"------------------------------\n" 
			L"E - for EvenOdd fills.\n" 
			L"N - for NonZero fills.\n" 
			L"------------------------------\n" 
			L"SPACE, ENTER or click to refresh.\n" 
			L"F1 - to see this help dialog again.\n"
			L"Esc - to quit.\n",
			L"Clipper Demo - Help", 0);
        return 0;

    case WM_CHAR:
		switch (wParam)
		{
			case VK_ESCAPE: PostQuitMessage(0); break; 
			case 'e': 
			case 'E': pft = pftEvenOdd; UpdatePolygons(true); break;
			case 'n': 
			case 'N': pft = pftNonZero; UpdatePolygons(true); break;
			case 'i': 
			case 'I': ct = ctIntersection; UpdatePolygons(true); break;
			case 'd': 
			case 'D': ct = ctDifference; UpdatePolygons(true); break;
			case 'u': 
			case 'U': ct = ctUnion; UpdatePolygons(true); break;
			case 'x': 
			case 'X': ct = ctXor; UpdatePolygons(true); break;
			case VK_SPACE:
			case VK_RETURN: UpdatePolygons(false);
		}
        return 0;

	case WM_LBUTTONUP:
		UpdatePolygons(false);
		return 0;

	// Default event handler
    default: return DefWindowProc (hWnd, uMsg, wParam, lParam); 
    }  
}
//------------------------------------------------------------------------------

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    const LPCWSTR appname = TEXT("Clipper Demo");

    WNDCLASS wndclass;
    MSG      msg;
 
    // Define the window class
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = (WNDPROC)MainWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(hInstance, appname);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wndclass.lpszMenuName  = appname;
    wndclass.lpszClassName = appname;
 
    // Register the window class
    if (!RegisterClass(&wndclass)) return FALSE;
 
    // Create the window
    hWnd = CreateWindow(
            appname,
            appname,
            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            640,
            480,
            NULL,
            NULL,
            hInstance,
            NULL);
 
    if (!hWnd) return FALSE;

	InitCommonControls();
	hStatus = CreateWindowEx(0, L"msctls_statusbar32", NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, hWnd, (HMENU)0, hInstance, NULL);
	SetWindowText(hStatus, L"  F1 for help");

    // Initialize OpenGL
    InitGraphics();

	srand((unsigned)time(0)); 
	UpdatePolygons(false);

    // Display the window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Event loop
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (!GetMessage(&msg, NULL, 0, 0)) return TRUE;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
}
//------------------------------------------------------------------------------
