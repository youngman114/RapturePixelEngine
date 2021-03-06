#ifndef _RAPTURE_PIXEL_ENGINE_H_INCLUDED
#define _RAPTURE_PIXEL_ENGINE_H_INCLUDED

#include <cstring>
#include <stdlib.h>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <functional>
#include <chrono>

#ifdef __linux__
#include <X11/Xlib.h>
#endif

// ---------------------------
// --- CLASSES AND STRUCTS ---
// ---------------------------
#pragma region CLASSES AND STRUCTS
namespace rpe {
    /// Forward definition for RapturePixelEngine
    typedef class RapturePixelEngine;

    /// POD (Plain old data) event object
    struct Event {
        /// Type of the occured event
        enum class EventType : uint8_t {
            NONE = 0,
            KEY = 1,
        } type;
        
        /// Specific for EventType::Key, defines key state
        enum class KeyEventType { PRESS = 0, RELEASE = 1, };

        struct KeyEvent {
            KeyEventType type;
        };

        union {
            KeyEvent keyEvent;
        };

        /// Create new empty event object
        Event() : type(EventType::NONE) {}
        /// Create a new event of a specific type
        Event(EventType type) : type(type) {}

        #ifdef __linux__
        /// Forge a new event based on XEVENT
        Event(const XEvent* xevent);
        #endif
    };


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
        void CreateWindow(
            int x = 16, 
            int y = 16, 
            unsigned int width = 256, 
            unsigned int height = 256,
            const char* title = "RapturePixelEngine Window");
        /// Make graphics and initialize GL context
        void CreateGraphics();
        /// Show the window 
        void ShowWindow();
        /// Process all the incoming events in the library requires so
        void PollEvents(rpe::RapturePixelEngine*);
        /// Set the window title
        void SetWindowTitle(const char*);

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

        int x, y;
        unsigned int width, height;
        const char* title;

        double deltaTime;

        struct {
            /// Fires just when any window event happens
            std::function<void(const Event&)> OnEventCallback = [](const Event&) {}; 
            /// Fires when the application has just started, but initialized
            std::function<void()> OnBegin = []() {};
            /// Fires when the application is done
            std::function<void()> OnEnd = []() {};
            /// Fires when a key is pressed, guaranteed to be Event::KeyEvent 
            std::function<void(const Event&)> OnKey = OnEventCallback;
        } callbacks;

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

        void SetWindowTitle(const char* title) {
            platform->SetWindowTitle(title);       
        }

        void Construct(
            int x = 16, 
            int y = 16, 
            unsigned int width = 256, 
            unsigned int height = 256,
            const char* title = "RapturePixelEngine Window") {
            
            this->x = x, this->y = y, this->width = width, this->height = height,
            this->title = title;

            theThread = std::thread([]() { TheThread(); });
        }

        void Start(bool join) {
            isRunning = true;
            if (join) {
                theThread.join();
            }
        }

        /// Mutex lock for thread safety
        std::mutex mtx;
        /// Lock to prevent changes
        std::condition_variable lock;
        /// Defines if the thread is active or not
        std::atomic_bool isRunning{false};
        /// Main Engine thread
        std::thread theThread;

        static void TheThread() {
            // Instance reference
            auto instance = RapturePixelEngine::instance();
            auto platform = instance->platform;

            std::chrono::steady_clock::time_point lastFrameTime, currentFrameTime;
            
            // Creation has to be called here, so the thread recieves control
            platform->CreateWindow(
                instance->x, 
                instance->y, 
                instance->width, 
                instance->height, 
                instance->title);
            platform->CreateGraphics();

            platform->ShowWindow();

            // Multithreading lock
            std::unique_lock<std::mutex> lock(instance->mtx);

            // Wait for isRunning to be true
            instance->lock.wait(lock, []() {
                return RapturePixelEngine::instance()->isRunning.load();
            });

            // Unlock all the locks!
            lock.unlock();
            instance->lock.notify_all();
                
            instance->callbacks.OnBegin();

            // Main loop, everything happens here
            // All roads lead to ~~Rome~~ for(;;)

            for(;;) {
                using namespace std::chrono; 
                // Time delta calculation
                currentFrameTime = steady_clock::now();
                instance->deltaTime = duration_cast<duration<double>>(
                        currentFrameTime - lastFrameTime).count();
                lastFrameTime = currentFrameTime;
                
                platform->PollEvents(instance);
            }

            instance->callbacks.OnEnd();
        }
    };
    #pragma endregion // CLASSES AND STRUCTS 

    // Pointer type for RapturePixelEngine
    using RapturePtr = RapturePixelEngine*;
}
// ------------------------------
// --- METHOD IMPLEMENTATIONS ---
// ------------------------------ 
#pragma region METHOD IMPLEMENTATIONS
#ifdef __linux__
void rpe::Platform::CreateWindow(
    int x, 
    int y, 
    unsigned int width, 
    unsigned int height,
    const char* title) {

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
        x, y, width, height,
        // border with, border, background
        1, XBlackPixel(d, screen), XWhitePixel(d, screen));

    XSelectInput(d, w, 
    ExposureMask | KeyPressMask | KeyReleaseMask);
    XStoreName(d, w, title);

    XAutoRepeatOff(d);
}

void rpe::Platform::CreateGraphics() {
    // TODO
}

void rpe::Platform::ShowWindow() {
    XMapWindow(d, w);
    XFlush(d);
}

void rpe::Platform::SetWindowTitle(const char* title) {
    XStoreName(d, w, title);
}

void rpe::Platform::PollEvents(rpe::RapturePixelEngine* engine) {
    XEvent tmp;
    Event out;
    
    bool newEvent = XCheckWindowEvent(d, w, 
    ExposureMask | KeyPressMask | KeyReleaseMask, 
    &tmp);

    switch (tmp.type)
    {
    case KeyPress:
        out = Event(Event::EventType::KEY);
        out.keyEvent.type = Event::KeyEventType::PRESS;
        if(newEvent) engine->callbacks.OnKey(out);
        break;
    case KeyRelease:
        out = Event(Event::EventType::KEY);
        out.keyEvent.type = Event::KeyEventType::RELEASE;
        
        if(newEvent) engine->callbacks.OnKey(out);
        break;

    default:
        out = Event(Event::EventType::NONE);
        break;
    }
    
    if(newEvent) engine->callbacks.OnEventCallback(out);

    return;
}
#endif // __linux__

#pragma endregion // METHOD IMPLEMENTATIONS

#endif // _RAPTURE_PIXEL_ENGINE_H_INCLUDED
