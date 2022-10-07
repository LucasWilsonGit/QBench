#include "core.h"

namespace Engine {
	struct window_desc
	{
		bool		m_fullscreen;
		uint32_t	m_width;
		uint32_t	m_height;
		uint32_t	m_pX;
		uint32_t	m_pY;
		bool		m_vsync;
		std::string m_title;

		window_desc(
			std::string const& title = "Quantbench",
			uint32_t width = 800,
			uint32_t height = 600,
			uint32_t pos_x = 10,
			uint32_t pos_y = 50
		) : m_width(width), m_height(height), m_pX(pos_x), m_pY(pos_y), m_fullscreen(false), m_vsync(false), m_title(title)
		{

		}
	};

	class window {
	public:
		static std::unique_ptr<window> create(const window_desc& params = window_desc());

		virtual ~window() = default;

		virtual void on_update() = NULL;
		virtual void toggle_vsync(bool enabled) = NULL;
		virtual void toggle_mouse(bool enabled) = NULL;
		virtual bool mouse_visible() const = NULL;
		virtual bool vsync_enabled() const = NULL;

		virtual uint32_t width() const = NULL; //immutable return value
		virtual uint32_t height() const = NULL;
	};
}
