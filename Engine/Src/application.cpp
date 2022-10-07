#include "application.h"

namespace Engine {
	application::application(int width, int height) {
		Engine::window_desc desc;
		desc.m_width = width;
		desc.m_height = height;
		desc.m_fullscreen = false;

		m_window = std::make_unique<win_window>(desc);
		m_renderer = renderer::get_instance();


		EventHandler& wineventhandlr = m_window->get_event_handler();

		std::cout << "application" << std::endl;
	}

	application::application() : application(800, 600) {

	}

	application::~application() {

		WARN("application::~application()");
		renderer::exit();
		m_renderer.reset();
		
	}

	void application::set_cursor(std::string c) {
		if (c == "Default") {
			SetCursor(LoadCursorW(NULL, CURSOR_DEFAULT));
		}
		else if(c == "Edit") {
			SetCursor(LoadCursorW(NULL, CURSOR_IBEAM));
		}
		else if (c == "Hourglass") {
			SetCursor(LoadCursorW(NULL, CURSOR_HOURGLASS));
		}
		else if (c == "ResizeVertical") {
			SetCursor(LoadCursorW(NULL, CURSOR_SIZEVERTICAL));
		}
		else if (c == "Pointer") {
			SetCursor(LoadCursorW(NULL, CURSOR_HAND));
		}
		else if (c == "ResizeHorizontal") {
			SetCursor(LoadCursorW(NULL, CURSOR_SIZEHORIZONTAL));
		}
	}

	glm::vec2 application::mouse_pos() {
		return m_window->mouse_position();
	}
}