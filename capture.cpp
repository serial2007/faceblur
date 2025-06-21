#include <opencv2/opencv.hpp>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif
#include <unistd.h>

#ifdef _WIN32
cv::Mat captureScreen() {
    static HWND hwnd = GetDesktopWindow();
    static HDC hwindowDC = GetDC(hwnd);
    static HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    static RECT windowsize;
    static int width = 0, height = 0;
    static bool initialized = false;

    if (!initialized) {
        GetClientRect(hwnd, &windowsize);
        width = windowsize.right;
        height = windowsize.bottom;
        initialized = true;
    }

    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    SelectObject(hwindowCompatibleDC, hbwindow);
    BitBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, SRCCOPY);

    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), width, -height, 1, 24, BI_RGB };
    cv::Mat mat(height, width, CV_8UC3);
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hbwindow);
    return mat;
}
#endif

#ifdef __linux__
extern bool captureWindow_window;
cv::Mat captureWindow_Fullscreen();
cv::Mat captureWindow_Window();
cv::Mat captureScreen()
{
	if(captureWindow_window)
		return captureWindow_Window();
	else return captureWindow_Fullscreen();
}

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

