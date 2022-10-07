#pragma once
#include "core.h"
#include "win_window.h"
#include "Event.h"
#include "renderer.h"

namespace Engine {
	class application
	{
	public:
		application();
		application(int width, int height);
		~application();

		void set_cursor(std::string c);
		
		virtual void on_update() = 0;
		glm::vec2 mouse_pos();

	protected:
		std::unique_ptr<win_window> m_window;
		std::shared_ptr<renderer> m_renderer;
		lua_State* m_Lua;
	};
}

