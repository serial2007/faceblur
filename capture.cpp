#include <opencv2/opencv.hpp>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
	#include <unistd.h>
#endif
	
extern bool captureWindow_window;
cv::Mat captureWindow_Fullscreen();
cv::Mat captureWindow_Window();
cv::Mat captureScreen()
{
	if(captureWindow_window)
		return captureWindow_Window();
	else return captureWindow_Fullscreen();
}

#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>

// Helper: 从HBITMAP创建cv::Mat
cv::Mat HBITMAPToMat(HBITMAP hBitmap) {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;  // 负值表示顶向下位图
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    cv::Mat mat(bmp.bmHeight, bmp.bmWidth, CV_8UC4);
    HDC hdc = GetDC(NULL);
    GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    return mat;
}

cv::Mat captureWindow_Fullscreen() {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY)) {
        std::cerr << "BitBlt failed\n";
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return {};
    }

    cv::Mat mat = HBITMAPToMat(hBitmap);

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    cv::Mat mat_bgr;
    cv::cvtColor(mat, mat_bgr, cv::COLOR_BGRA2BGR);
    return mat_bgr;
}

cv::Mat captureWindow_Window() {
    static HWND target_win = NULL;
    static bool initialized = false;

    if (!initialized) {
        std::cout << "请点击一个窗口用于截图（Timeout=5s）\n";
        std::cout << "请点击窗口并按 Enter 键确认...\n";

        // 简单实现：让用户输入窗口句柄或者点击桌面（可改进用更复杂的钩子）
        // 这里演示让用户手动输入窗口句柄（十六进制）
        std::cout << "请输入目标窗口句柄（十六进制），或 0 代表全屏：";
        std::string input;
        std::getline(std::cin, input);
        if (input == "0") {
            target_win = NULL;
        } else {
            try {
                target_win = (HWND)std::stoul(input, nullptr, 16);
            } catch (...) {
                std::cerr << "输入无效，使用全屏\n";
                target_win = NULL;
            }
        }

        initialized = true;
    }

    if (target_win == NULL) {
        return captureWindow_Fullscreen();
    }

    RECT rect;
    if (!GetWindowRect(target_win, &rect)) {
        std::cerr << "无法获取窗口位置，使用全屏\n";
        return captureWindow_Fullscreen();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0) {
        std::cerr << "窗口大小异常，使用全屏\n";
        return captureWindow_Fullscreen();
    }

    HDC hWindowDC = GetWindowDC(target_win);
    HDC hMemoryDC = CreateCompatibleDC(hWindowDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hWindowDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    if (!BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY)) {
        std::cerr << "BitBlt 失败\n";
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(target_win, hWindowDC);
        return {};
    }

    cv::Mat mat = HBITMAPToMat(hBitmap);

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(target_win, hWindowDC);

    cv::Mat mat_bgr;
    cv::cvtColor(mat, mat_bgr, cv::COLOR_BGRA2BGR);
    return mat_bgr;
}

#endif

#ifdef __linux__

cv::Mat captureWindow_Fullscreen() {
    static Display* display = XOpenDisplay(nullptr);
    static int screen = DefaultScreen(display);
    static Window root = RootWindow(display, screen);
    static int width = 0, height = 0;
    static bool initialized = false;

    if (!initialized) {
        XWindowAttributes gwa;
        XGetWindowAttributes(display, root, &gwa);
        width = gwa.width;
        height = gwa.height;
        initialized = true;
    }

    XImage* img = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!img) return cv::Mat();

    cv::Mat mat(height, width, CV_8UC4, img->data);
    cv::Mat mat_bgr;
    cv::cvtColor(mat, mat_bgr, cv::COLOR_BGRA2BGR);

    XDestroyImage(img);
    return mat_bgr.clone();  // clone 必须，否则数据在 XDestroyImage 后失效
}

cv::Mat captureWindow_Window() {
    static Display* display = XOpenDisplay(nullptr);
    static Window root = DefaultRootWindow(display);
    static Window target_win = None;
    static bool initialized = false;

    if (!initialized) {
        std::cout << "请点击一个窗口用于截图（Timeout=5s)\n";

        int grab_result = XGrabPointer(display, root, False, ButtonPressMask,
                                       GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        if (grab_result != GrabSuccess) {
            std::cerr << "无法抓取鼠标\n";
            return {};
        }

        // 设置超时：最多等待5秒
        XEvent event;
        bool clicked = false;
        auto start = std::chrono::steady_clock::now();

        while (true) {
            if (XPending(display) > 0) {
                XNextEvent(display, &event);
                if (event.type == ButtonPress) {
                    target_win = event.xbutton.subwindow;
                    if (target_win == None) target_win = root;
                    clicked = true;
                    break;
                }
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) {
                std::cerr << "等待点击超时，使用 root 窗口。\n";
                target_win = root;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        XUngrabPointer(display, CurrentTime);
        initialized = true;
    }

    XWindowAttributes attr;
    if (!XGetWindowAttributes(display, target_win, &attr)) {
        std::cerr << "无法获取窗口属性\n";
        return {};
    }

    int width = attr.width;
    int height = attr.height;

    XImage* img = XGetImage(display, target_win, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!img) {
        std::cerr << "截图失败\n";
        return {};
    }

    cv::Mat mat(height, width, CV_8UC4, img->data);
    cv::Mat mat_bgr;
    cv::cvtColor(mat, mat_bgr, cv::COLOR_BGRA2BGR);
    XDestroyImage(img);
    return mat_bgr.clone();
}



#endif

