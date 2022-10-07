#include "win_window.h"
#include "GLFW/glfw3.h"

namespace Engine {

	static bool glfw_initialized = false;
	static void glfw_error_callback(int error, const char* desc)
	{
		std::cout << "GLFW error " << error << ": " << desc << std::endl;
	}

	win_window::win_window(const window_desc& desc) : m_eventhandler(EventHandler()) {
		std::cout << "pre init" << std::endl;

		init(desc);
	}

	win_window::~win_window() {
		close();
	}

	void win_window::on_update() {

		glfwPollEvents();
		glfwSwapBuffers(m_window);
	}

	void win_window::toggle_vsync(bool enabled) {
		glfwSwapInterval(enabled ? 1 : 0);
		m_state.vsync = enabled;
	}

	void win_window::toggle_mouse(bool enabled) {
		glfwSetInputMode(m_window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);//DISBALED locks the cursor so it cannot leave the window client rect
		m_state.mouse_visible = enabled;
	}

	bool win_window::mouse_visible() const {
		return m_state.mouse_visible;
	}

	bool win_window::vsync_enabled() const {
		return m_state.vsync;
	}

	uint32_t win_window::width() const {
		return m_state.width;
	}

	uint32_t win_window::height() const {
		return m_state.height;
	}

	void win_window::init(const window_desc& desc)
	{
		m_state.title = desc.m_title;
		m_state.width = desc.m_width;
		m_state.height = desc.m_height;
		m_state.mouse_x = desc.m_width * .5f;
		m_state.mouse_y = desc.m_height * .5f;
		m_state.event_handler = &m_eventhandler;

		//todo log window creation
		if (!glfw_initialized) {
			int32_t s = glfwInit();
			if (s == GLFW_TRUE) {
				std::cout << "GLFW initialized" << std::endl;
				glfw_initialized = true;
			}
			else
			{
				std::cout << "failed to initialize glfw" << std::endl;
				const char* err = new char[256];
				glfwGetError(&err);
				std::cout << "error: " << err << std::endl;
			}
			glfwSetErrorCallback((GLFWerrorfun)glfw_error_callback);
		}

		if (desc.m_fullscreen) {
			int* monitor_array = new int;
			glfwGetMonitors(monitor_array);
			m_window = glfwCreateWindow(
				desc.m_width,
				desc.m_height,
				desc.m_title.c_str(),
				(GLFWmonitor*)monitor_array[0],
				nullptr
			);
			delete monitor_array;
		}
		else
		{
			m_window = glfwCreateWindow(
				desc.m_width,
				desc.m_height,
				desc.m_title.c_str(),
				nullptr,
				nullptr
			);
		}

		glfwMakeContextCurrent(m_window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cout << "Failed to initialize OpenGL context" << std::endl;
		}

		const unsigned char* vendor = glGetString(GL_VENDOR);
		const unsigned char* renderer = glGetString(GL_RENDERER);
		const unsigned char* ver = glGetString(GL_VERSION);
		std::cout << "VENDOR: " << vendor << std::endl;
		std::cout << "RENDERER: " << renderer << std::endl;
		std::cout << "GLVersion: " << ver << std::endl;

		glfwSetWindowPos(m_window, desc.m_pX, desc.m_pY);

		glfwSetWindowUserPointer(m_window, &m_state);
		toggle_vsync(desc.m_vsync);

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* handle, int32_t width, int32_t height) {
			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			state.width = width;
			state.height = height;

			EventData e;
			WindowResizeEventInfo f;
			f.width = width;
			f.height = height;
			e.eventflags = EVENT_WINDOWRESIZE;
			e.eventinfo.wri = f;

			state.event_handler->Fire(e);
		});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* handle) {
			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			EventData e;
			e.eventflags = EVENT_WINDOWCLOSE;
			state.event_handler->Fire(e);
		});

		glfwSetKeyCallback(m_window, [](GLFWwindow* handle, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			EventData e;
			KeyEventInfo k;
			k.action = action;
			k.key = key;
			k.mods = mods;
			k.scancode = scancode;
			switch (action)
			{
				case GLFW_PRESS:
				{
					e.eventflags = EVENT_KEYDOWN;
				}
				break;
				case GLFW_RELEASE:
				{
					e.eventflags = EVENT_KEYUP;
				}
				break;
				case GLFW_REPEAT:
				{
					e.eventflags = EVENT_KEYDOWN;
				}
				break;
				default:
					e.eventflags = EVENT_KEYDOWN;
				break;
			}
			e.eventinfo.ki = k;
			state.event_handler->Fire(e);
		});

		glfwSetCharCallback(m_window, [](GLFWwindow* handle, uint32_t codepoint) {
			std::wstring s;
			s += (wchar_t)codepoint;

			int len2 = (int)s.length() + 1; //add 1 for null terminator
			char* buffer = new char[2];
			buffer[1] = '\0';
			WideCharToMultiByte(CP_UTF8, 0, s.c_str(), len2, buffer, 1, 0, 0);
			std::string str(buffer);

			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			EventData e;
			e.eventflags = EVENT_CHAR;

			e.eventinfo.ci.character = ((uint16_t*)str.c_str())[0];
			state.event_handler->Fire(e);

			delete[] buffer; //->Fire immediately calls the listeners in the same thread, I don't expect a race condition with this deletion vs Listeners using the event info. 
			//Shouldn't happen anyway because I copied out the first char from the string
		});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* handle, int32_t button, int32_t action, int32_t mods) {
			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			EventData e;
			MouseButtonEventInfo m;
			m.button = button;
			m.action = action;
			m.mods = mods;
			e.eventinfo.mbi = m;
			e.eventflags = EVENT_MOUSEBUTTON;
			state.event_handler->Fire(e);
		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* handle, double dX, double dY)
		{
			window_state& state = *((window_state*)glfwGetWindowUserPointer(handle));
			EventData e;
			MouseScrollEventInfo m;
			m.dX = dX;
			m.dY = dY;
			e.eventinfo.msi = m;
			e.eventflags = EVENT_MOUSESCROLL;
			state.event_handler->Fire(e);
			//WARN("scroll0");
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* handle, double x, double y)
		{
				window_state* state = ((window_state*)glfwGetWindowUserPointer(handle));
				EventData e;
				MouseMoveEventInfo m;
				double dX, dY; 
				dX = x - state->mouse_x;
				dY = y - state->mouse_y;
				m.xpos = dX;
				m.ypos = dY;
				state->mouse_x = x;
				state->mouse_y = y;

				e.eventinfo.mmi = m;
				e.eventflags = EVENT_MOUSEMOVE;
				state->event_handler->Fire(e);
		});
	}

	void win_window::close() {
		glfwDestroyWindow(m_window);
	}

	void win_window::set_cursor(LPCSTR cursortype) {
		SetCursor(LoadCursorA(NULL, cursortype));
	}

	EventHandler& win_window::get_event_handler() {
		return m_eventhandler;
	}

	glm::vec2 win_window::mouse_position() {
		window_state& state = *((window_state*)glfwGetWindowUserPointer(m_window));
		return glm::vec2{ state.mouse_x, state.mouse_y };
	}
}
