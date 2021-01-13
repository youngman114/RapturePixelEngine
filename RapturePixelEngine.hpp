#ifndef _RAPTURE_PIXEL_ENGINE_H_INCLUDED
#define _RAPTURE_PIXEL_ENGINE_H_INCLUDED

#include <cstring>
#include <stdlib.h>

#ifdef __linux__
#include <X11/Xlib.h>
#endif

// ---------------------------
// --- CLASSES AND STRUCTS ---
// ---------------------------
#pragma region CLASSES AND STRUCTS
class Platform {
protected:
    Platform() = default;

public:
    /// Get the only Platform instance
    static inline Platform* instance() {
        static Platform platform;
        return &platform;
    }

    // Singleton singleton singleton singleton

    Platform(const Platform&) = delete;
    Platform(Platform&&) = delete;
    Platform& operator=(const Platform&) = delete;
    Platform& operator=(Platform&&) = delete;

    /// Create the window and prepare the world!
    void CreateWindow();
    /// Make graphics and initialize GL context
    void CreateGraphics();
    /// Show the window 
    void ShowWindow();

private:
#ifdef __linux__
    Display* d;
    Window w;
#endif
};

class RapturePixelEngine {
protected:
    RapturePixelEngine() {
        platform = Platform::instance();
    }

    ~RapturePixelEngine() = default;

public:
    Platform* platform = nullptr;

    // Get the only RapturePixelEngine instance
    static inline RapturePixelEngine* instance() {
        static RapturePixelEngine engine;
        return &engine;
    }

    // No copying or assigning, it's a singleton all in all

    RapturePixelEngine(const RapturePixelEngine&) = delete;
    RapturePixelEngine(RapturePixelEngine&&) = delete;
    RapturePixelEngine& operator=(const RapturePixelEngine&) = delete;
    RapturePixelEngine& operator=(RapturePixelEngine&&) = delete;
};
#pragma endregion // CLASSES AND STRUCTS 

// Pointer type for RapturePixelEngine
using RapturePtr = RapturePixelEngine*;

// ------------------------------
// --- METHOD IMPLEMENTATIONS ---
// ------------------------------ 
#pragma region METHOD IMPLEMENTATIONS
#ifdef __linux__
void Platform::CreateWindow() {
    // Try open monitor
    d = XOpenDisplay(NULL);

    // Halt if couldn't
    if(d == nullptr) {
        printf("Can't connect X server.");
        std::exit(1);
    }

    int screen = XDefaultScreen(d);

    w = XCreateSimpleWindow(
        // display, parent window
        d, RootWindow(d, screen),
        // x, y, width, height 
        16, 16, 256, 256, 
        // border with, border, background
        1, XBlackPixel(d, screen), XWhitePixel(d, screen));

    XSelectInput(d, w, ExposureMask | KeyPressMask);
}

void Platform::CreateGraphics() {
    // TODO
}

void Platform::ShowWindow() {
    XMapWindow(d, w);
    XFlush(d);
}
#endif // __linux__

#pragma endregion // METHOD IMPLEMENTATIONS

#endif // _RAPTURE_PIXEL_ENGINE_H_INCLUDED