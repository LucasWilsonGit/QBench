#pragma once
#include "Window.h"
#include "EventHandler.h"
#include "core.h"

#define CURSOR_HOURGLASS IDC_WAIT
#define CURSOR_DEFAULT IDC_ARROW
#define CURSOR_HAND IDC_HAND
#define CURSOR_IBEAM IDC_IBEAM
#define CURSOR_SIZEVERTICAL IDC_SIZENS
#define CURSOR_SIZEHORIZONTAL IDC_SIZEWE

//Based on Engine::win_window class from IN3026 Advanced Games Technology (Chris Child)
namespace Engine {

    class win_window : public Engine::window
    {
    public:
        win_window(const window_desc &desc);
        ~win_window();

        void on_update() override;
        void toggle_vsync(bool enabled) override;
        void toggle_mouse(bool enabled) override;
        bool mouse_visible() const override;
        bool vsync_enabled() const override;

        uint32_t width() const override;
        uint32_t height() const override;

        void set_cursor(LPCSTR cursortype);
        EventHandler& get_event_handler();

        glm::vec2 mouse_position();

    private:
        virtual void init(const window_desc& desc);
        virtual void close();

        GLFWwindow* m_window = 0;
        EventHandler m_eventhandler;
        
        struct window_state
        {
            std::string title;
            float mouse_x;
            float mouse_y;
            uint32_t width;
            uint32_t height;
            bool vsync;
            bool fullscreen;
            bool mouse_visible;
            bool mouse_locked; 
            EventHandler* event_handler;
        };

        window_state m_state;
    };

}