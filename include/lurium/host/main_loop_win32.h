// TODO: define LURIUM_NO_GL for canvas-only
// eller inkludere en annen fil, main_loop_win32gl, kan extend fra main_loop_win32; gl_main_loop
// main_loop_gl ->

#include <Windows.h>
#include <gl/glew.h>

struct main_loop_state {
};

struct main_loop : main_loop_base {
	LARGE_INTEGER hires_start;
	LARGE_INTEGER hires_ticks_per_second;
	HDC hDC;				/* device context */
	HGLRC hRC;				/* opengl context */
	HWND  hWnd;				/* window */
	HINSTANCE hInstance;
	HWND hWelcomeTextbox;

	main_loop(main_loop_state&) {
		hInstance = GetModuleHandle(NULL);
		if (!register_class()) {
			assert(false);
		}

		hWnd = create_gl_window(L"minimal", 0, 0, 640, 480, PFD_TYPE_RGBA, 0);
		assert(hWnd != NULL);

		create_welcome_textbox();

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)this);
		hDC = GetDC(hWnd);
		hRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hRC);

		glewInit();
	}

	int run() {

		MSG   msg;				/* message */

		ShowWindow(hWnd, SW_SHOW);

		QueryPerformanceFrequency(&hires_ticks_per_second);
		QueryPerformanceCounter(&hires_start);

		for (;;) {

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {

					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} else {

				const int frames_per_second = 60;
				DWORD ticks = get_timer();

				render();

				ticks = get_timer() - ticks;

				if (ticks < 1000 / frames_per_second) {
					Sleep((1000 / frames_per_second) - ticks);
				}
			}
		}

		wglMakeCurrent(NULL, NULL);
		ReleaseDC(hWnd, hDC);
		wglDeleteContext(hRC);
		DestroyWindow(hWnd);

		return msg.wParam;
	}

	~main_loop() {
		UnregisterClass(L"OpenGL", hInstance);
	}

	DWORD get_timer() {
		LARGE_INTEGER hires_now;

		QueryPerformanceCounter(&hires_now);

		hires_now.QuadPart -= hires_start.QuadPart;
		hires_now.QuadPart *= 1000;
		hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

		return (DWORD)hires_now.QuadPart;
	}

	void get_mouse_event(LPARAM lParam, mouse_event_data& e) {
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		e.id = 0;
		e.x = LOWORD(lParam) / (float)rcClient.right * 2 - 1;
		e.y = HIWORD(lParam) / (float)rcClient.bottom * 2 - 1;
		e.left_button = false;
		e.right_button = false;
	}

	static LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		static PAINTSTRUCT ps;
		main_loop* self;
		mouse_event_data me;

		switch (uMsg) {
			case WM_MOUSEMOVE: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				self->get_mouse_event(lParam, me);
				self->mousemove(me);
				return 0;
			}
			case WM_LBUTTONDOWN: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				self->get_mouse_event(lParam, me);
				me.left_button = true;
				self->lbuttondown(me);
				return 0;
			}
			case WM_LBUTTONUP: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				self->get_mouse_event(lParam, me);
				me.left_button = true;
				self->lbuttonup(me);
				return 0;
			}
			case WM_SIZE: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				window_event_data e;
				e.width = LOWORD(lParam);
				e.height = HIWORD(lParam);
				self->resize(e);
				return 0;
			}

			case WM_CHAR:
				switch (wParam) {
				case 27:			/* ESC key */
					PostQuitMessage(0);
					break;
				}
				return 0;

			case WM_KEYDOWN: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				keyboard_event_data e;
				e.char_code = wParam;
				self->keydown(e);
				break;
			}

			case WM_KEYUP: {
				self = (main_loop*)GetWindowLongPtr(hWnd, GWL_USERDATA);
				keyboard_event_data e;
				e.char_code = wParam;
				self->keyup(e);
				break;
			}

			case WM_CLOSE:
				PostQuitMessage(0);
				return 0;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	bool register_class() {
		WNDCLASS wc;
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC)WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"OpenGL";

		if (!RegisterClass(&wc)) {
			MessageBox(NULL, L"RegisterClass() failed:  "
				L"Cannot register window class.", L"Error", MB_OK);
			return false;
		}
		return true;
	}

	static HWND create_gl_window(TCHAR* title, int x, int y, int width, int height, BYTE type, DWORD flags) {
		int         pf;
		HDC         hDC;
		HWND        hWnd;
		PIXELFORMATDESCRIPTOR pfd;
		static HINSTANCE hInstance = 0;

		RECT rcWindow;
		SetRect(&rcWindow, x, y, width, height);
		DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		AdjustWindowRect(&rcWindow, dwStyle, FALSE);

		hWnd = CreateWindow(L"OpenGL", title, dwStyle, x, y, width, height, NULL, NULL, hInstance, NULL);

		if (hWnd == NULL) {
			MessageBox(NULL, L"CreateWindow() failed:  Cannot create a window.", L"Error", MB_OK);
			return NULL;
		}

		hDC = GetDC(hWnd);

		/* there is no guarantee that the contents of the stack that become
		the pfd are zeroed, therefore _make sure_ to clear these bits. */
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
		pfd.iPixelType = type;
		pfd.cColorBits = 32;

		pf = ChoosePixelFormat(hDC, &pfd);
		if (pf == 0) {
			MessageBox(NULL, L"ChoosePixelFormat() failed:  "
				L"Cannot find a suitable pixel format.", L"Error", MB_OK);
			return 0;
		}

		if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
			MessageBox(NULL, L"SetPixelFormat() failed:  "
				L"Cannot set format specified.", L"Error", MB_OK);
			return 0;
		}

		DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		ReleaseDC(hWnd, hDC);

		return hWnd;
	}

	void create_welcome_textbox() {

		hWelcomeTextbox = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN, 0, 0, 100, 50, hWnd, (HMENU)NULL, hInstance, 0);

	}

	virtual void set_welcome_textbox_visibility(bool visible) {
		LONG styles = GetWindowLongPtr(hWelcomeTextbox, GWL_STYLE);
		if (visible) {
			//styles |= WS_VISIBLE;
			ShowWindow(hWelcomeTextbox, SW_SHOW);
		} else {
			ShowWindow(hWelcomeTextbox, SW_HIDE);
			//styles &= ~WS_VISIBLE;
		}
		//SetWindowLongPtr(hWelcomeTextbox, GWL_STYLE, styles);
	}

	virtual void set_welcome_textbox_position(int x, int y, int width, int height) {
		SetWindowPos(hWelcomeTextbox, HWND_TOP, x, y, width, height, 0);
	}

	virtual void get_welcome_text(std::string& text) {
		WCHAR pc[64];
		int bytes = GetWindowText(hWelcomeTextbox, pc, 64);
		text = std::string(pc, pc + bytes);
	}

};

