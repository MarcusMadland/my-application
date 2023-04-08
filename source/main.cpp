#include <SDL.h>
#include <SDL_syswm.h>
#include <iostream>

#include "mapp/app.hpp"
#include "application_layer.hpp"
#include "mapp/window.hpp"
#include "mapp/event.hpp"
#include "mrender/testing.hpp"

class WindowSDL : public mapp::Window
{
public:
    WindowSDL(const char* name, uint32_t width, uint32_t height)
        : mapp::Window(name, width, height)
        , window(nullptr)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) 
        {
            //printf("SDL could not initialize. SDL_Error: %s\n", SDL_GetError()); @todo logger
            return;
        }

        window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
            width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        if (window == nullptr) 
        {
            printf("Window could not be created. SDL_Error: %s\n", SDL_GetError()); // @todo logger
            return;
        }
    }

    ~WindowSDL() 
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    virtual void onUpdate(const float& dt) override 
    { 
        // Handle Events
        SDL_Event e;
        while (SDL_PollEvent(&e)) 
        {
            if (e.type == SDL_WINDOWEVENT) 
            {
                switch (e.window.event) 
                {
                    case SDL_WINDOWEVENT_RESIZED: 
                    {
                        mapp::WindowResizeEvent event = mapp::WindowResizeEvent(
                            e.window.data1, e.window.data2);
                        eventCallback(event);
                        break;
                    }
                        
                    case SDL_WINDOWEVENT_CLOSE: 
                    {
                        mapp::WindowCloseEvent event;
                        eventCallback(event);
                        break;
                    }  
                }
            }
        }
    }

    virtual void* getNativeWindow() override 
    {
#if !SDL_VIDEO_DRIVER_EMSCRIPTEN
        SDL_SysWMinfo wmi;
        SDL_VERSION(&wmi.version);
        if (!SDL_GetWindowWMInfo(window, &wmi)) {
            printf(
                "SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
                SDL_GetError());
            return nullptr;
        }
#endif

#if SDL_VIDEO_DRIVER_WINDOWS
        return wmi.info.win.window;
#elif SDL_VIDEO_DRIVER_COCOA
        return wmi.info.cocoa.window;
#elif SDL_VIDEO_DRIVER_X11
        return(void*)(uintptr_t) wmi.info.x11.window;
#elif SDL_VIDEO_DRIVER_EMSCRIPTEN
        return (void*)"#canvas";
#endif 
    }

    virtual void* getNativeDisplay() override 
    {
    #if SDL_VIDEO_DRIVER_X11
        return wmi.info.x11.display;
    #endif
        return nullptr;
    }

private:
    SDL_Window* window;
};

class Application : public mapp::App
{
public:
    Application(mapp::Window* window) : mapp::App(window)
    {
        pushLayer(new ApplicationLayer());
    };
};

int main(int argc, char** argv)
{
    WindowSDL* window = new WindowSDL("My Application", 1280, 720);
    Application* app = new Application(window);
    app->run();

    return 0;
}