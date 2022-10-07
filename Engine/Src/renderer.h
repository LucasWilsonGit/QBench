#pragma once
#include "core.h"
#include "gl_buffer.h"
#include "gl_shader.h"
#include "vertex_buffer.h"
#include <map>

namespace Engine {
	struct rect_vert {
		glm::vec3 pos;
		glm::vec2 UV;
		glm::vec4 rgba;
	};

	struct filled_rect_data {
		rect_vert t00;
		rect_vert t01;
		rect_vert t02;
		rect_vert t10;
		rect_vert t11;
		rect_vert t12;
	};

	struct rect_data {
		rect_vert t00;
		rect_vert t01;
		rect_vert t02;
		rect_vert t03;
	};

	struct character {
		glm::vec2 bearing;
		glm::vec2 size;
		glm::vec2 uv_topleft;
		glm::vec2 uv_topright;
		glm::vec2 uv_bottomleft;
		glm::vec2 uv_bottomright;
		float advance;
	};

	struct RectParams {
		int32_t x;
		int32_t y;
		int32_t width;
		int32_t height;
		int32_t z;
		glm::vec4 color;
		int32_t border;
	};

	struct FilledRectParams {
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t width;
		int32_t height;
		glm::vec4 color;
	};

	struct ColorParams {
		float_t r;
		float_t g;
		float_t b;
		float_t a;
	};

	struct TextParams {
		std::string s;
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t width;
		int32_t height;
		int32_t size;
		glm::vec4 color;
		bool wrap;
	};

	struct RenderCandleInfo {
		glm::vec4 ohlc;
		float volume;
	};

	class renderer
	{
	private:
		GLuint m_rect_VAO;
		GLuint m_filled_rect_VAO;
		GLuint m_text_VAO;
		GLuint m_chart_VAO;
		GLuint m_font_texture;
		GLuint m_candle_VAO;

		GLuint m_active_VAO;

		std::shared_ptr<gl_buffer> m_active_buffer;
		
		std::shared_ptr<vertex_buffer> m_rect_vbo;
		std::shared_ptr<vertex_buffer> m_filled_rect_vbo;
		std::shared_ptr<vertex_buffer> m_text_vbo;
		GLuint m_chart_vbo;
		GLuint m_candle_vbo;

		std::vector<TextParams> m_text_local_data; //local vector populated by push_rect stage

		static bool shutdown;

		void bind_buffer(std::shared_ptr<gl_buffer> buffer);
		void bind_shader(std::shared_ptr<gl_shader> shader);
		std::shared_ptr<gl_shader> active_shader();
		GLuint gen_vao();

		int32_t last_border_size;
		glm::vec2 m_viewport_size;

		FT_Library ft;
		std::map<char, character> m_characterdata;
		
		//std::shared_ptr<TextRenderer> m_text_manager;
		renderer();
	public:
		~renderer();
		renderer(const renderer&) = delete;
		renderer(const renderer&&) = delete;

		static std::shared_ptr<renderer>& get_instance();

		void bind_vao(GLuint vao);

		//void SetRenderTarget();
		void push_rect(RectParams);
		void draw_rects();
		void push_filled_rect(FilledRectParams);
		void draw_filled_rects();
		void push_text(const TextParams&);
		void draw_text();
		void set_viewport_size(int32_t width, int32_t height);
		void draw_chart_rect(const glm::vec2& pos, const glm::vec2& size);
		void write_candle_gpu_data(void* data, size_t size);
		void draw_candles(int count, const glm::mat4& projection);

		glm::vec2 get_viewport_size();
		glm::vec2 get_text_bounds(const std::string& s, int32_t width, int32_t size);

		static void exit(); 
	};

}