#include <Windows.h>
#include <iostream>
#include <cmath>

using namespace std;


#define Title TEXT("DynamicHeart")
#define WindowWidth 800
#define WindowHeight 800
constexpr auto PI = 3.14159f;
#define FPS 60
#define MAX_INSERT 1000
double MAX_LOG = 1 / log(MAX_INSERT);

HWND hWnd = NULL;
HDC hDC = NULL;
HDC hMemDC = NULL;
HBITMAP hBitmapBuffer = NULL;
HBITMAP hBitmapDevice = NULL;
BYTE* buffer = NULL;
bool isExist = false;


//base 不检查
inline void _DrawPoint(int nPos, BYTE r, BYTE g, BYTE b, BYTE a = 255) {
	buffer[nPos++] = ((buffer[nPos] * (255 - a)) >> 8) + ((b * a) >> 8);
	buffer[nPos++] = ((buffer[nPos] * (255 - a)) >> 8) + ((g * a) >> 8);
	buffer[nPos] = ((buffer[nPos] * (255 - a)) >> 8) + ((r * a) >> 8);
}
//(x,y)入口
inline void DrawPoint(int x, int y, BYTE r, BYTE g, BYTE b, BYTE a = 255) {
	if (x > WindowWidth || x<0 || y>WindowHeight || y < 0) {
		return;
	}
	int nPos = (y * WindowWidth + x) * 4;
	_DrawPoint(nPos, r, g, b, a);
}

template<typename T>
T random(T a, T b) {
	T length = b-a;
	double r = static_cast<double>(rand()) / RAND_MAX;
	return static_cast<T>(r * length + a);
}

struct Point {
	double x;
	double y;
};

Point insert(Point a, Point b,double tx,double ty) {
	//a -> b
	a.x += (b.x - a.x) * tx;
	a.y += (b.y - a.y) * ty;
	return a;
}
int frame = 0;

void func() {
	// x = 16sin(t)^3
	// y = 13cos(t)-5cos(2t)-2cos(3t)-cos(4t)
	// t = 0..2pi
	double x, y;
	double _x, _y;
	double dx, dy;
	Point p;

	//[-1,1]
	double frameRate = 0.5*sin(2 * PI / FPS * frame);

	for (float t = 0; t < 2 * PI; t += 0.001f) {
		// Base heat
		_x = 16 * pow(sin(t), 3);
		_y = 13 * cos(t) - 5 * cos(2 * t) - 2 * cos(3 * t) - cos(4 * t);
		
		// Outside heat
		//scale up
		x = _x * 10;
		y = _y * 10;
		dx = log(random(1, MAX_INSERT)) * MAX_LOG;
		dy = log(random(1, MAX_INSERT)) * MAX_LOG;
		p = insert({ x,y }, { 0,0 }, frameRate * dx, frameRate * dy);
		//|--------->x
		//|    ↑y
		//|    |
		//|----|---->x
		//|    |
		//↓y  |
		//map to window
		x = p.x + WindowWidth / 2;
		y = -p.y + WindowHeight / 2;
		DrawPoint(x, y, 255, 184 + 56 * frameRate, 190 + 20 * frameRate);

		// Inside 
		x = _x * 10;
		y = _y * 10;
		//[0,10)
		//[0,1
		dx = 1 - log(random(1, MAX_INSERT)) * MAX_LOG;
		dy = 1 - log(random(1, MAX_INSERT)) * MAX_LOG;
		p = insert({ x,y }, { 0,0 }, dx, dy);

		// Zoom
		dx = log(random(1, MAX_INSERT)) * MAX_LOG;
		dy = log(random(1, MAX_INSERT)) * MAX_LOG;
		p = insert(p, { 0,0 }, frameRate * dx, frameRate * dy);


		x = p.x + WindowWidth / 2;
		y = -p.y + WindowHeight / 2;
		//DrawPoint(x, y, 255 - 255 + 255 * frameRate, 0, 0);
		DrawPoint(x, y, 255, 184 + 56 * frameRate, 190 + 20 * frameRate);
	}

	++frame;
	frame %= 60;
}


static LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_KEYDOWN: {
		break;
	}
	case WM_KEYUP: {
		if (wParam == VK_ESCAPE) {
			isExist = true;
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	default: {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	}
	return 0;
}


void update()
{
	MSG msg;
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	memset(buffer, 0, WindowWidth* WindowHeight * 4 * sizeof(BYTE));
	func();
	BitBlt(hDC, 0, 0, WindowWidth, WindowHeight, hMemDC, 0, 0, SRCCOPY);
}


int InitEngine() {
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_BYTEALIGNCLIENT;
	wc.lpfnWndProc = (WNDPROC)WinProc;
	wc.hInstance = hInstance;//GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = Title;
	wc.lpszMenuName = Title;

	RegisterClass(&wc);
	int w = GetSystemMetrics(SM_CXSCREEN);
	int h = GetSystemMetrics(SM_CYSCREEN);
	//Create a new window
	hWnd = CreateWindowEx(
		WS_EX_LAYERED,
		Title,
		Title,
		WS_POPUP,
		(w - WindowWidth) / 2, (h - WindowHeight) / 2,//position
		WindowWidth, WindowHeight,//size
		NULL,
		NULL,
		hInstance,
		NULL
	);
	if (hWnd == 0) {
		return GetLastError();
	}
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, ULW_COLORKEY);
	RECT rectWindow;
	RECT rectClient;
	GetWindowRect(hWnd, &rectWindow);
	GetClientRect(hWnd, &rectClient);
	rectWindow.right += rectWindow.right - rectWindow.left - rectClient.right;
	rectWindow.bottom += rectWindow.bottom - rectWindow.top - rectClient.bottom;

	MoveWindow(hWnd, rectWindow.left, rectWindow.top, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, false);

	//创建内存缓冲画板
	hDC = GetDC(hWnd);
	hMemDC = CreateCompatibleDC(hDC);//创建与窗口兼容的内存设备上下文环境(创建内存缓冲画板)
	BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), WindowWidth, -WindowHeight, 1, 32, BI_RGB,
		WindowWidth* WindowHeight * 4, 0, 0, 0, 0 } };
	LPVOID ptr;
	hBitmapBuffer = CreateDIBSection(hMemDC, &bi, DIB_RGB_COLORS, &ptr, 0, 0);//创建缓冲区画布
	hBitmapDevice = (HBITMAP)SelectObject(hMemDC, hBitmapBuffer);//选择缓冲区画布到缓冲区画板，并保留原始画布
	buffer = (BYTE*)ptr;
	memset(buffer, 0, WindowWidth * WindowHeight * 4 * sizeof(BYTE));

	ShowWindow(hWnd, SW_NORMAL);
	UpdateWindow(hWnd);
	return 0;
}


void FreeEngine() {
	if (hMemDC) {
		if (hBitmapDevice) {
			SelectObject(hMemDC, hBitmapDevice);
			hBitmapDevice = NULL;
		}
		DeleteDC(hMemDC);
		hMemDC = NULL;
	}
	if (hBitmapBuffer) {
		DeleteObject(hBitmapBuffer);
		hBitmapBuffer = NULL;
	}
	if (hDC) {
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
	if (hWnd) {
		CloseWindow(hWnd);
		hWnd = NULL;
	}
}


int main() {

	InitEngine();
	

	while (!isExist) {
		update();
	}
	printf("End\n");
	FreeEngine();
	return 0;
}
