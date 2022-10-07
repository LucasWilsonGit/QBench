#include "renderer.h"
#include "gl_shader.h"
#include "core.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

namespace Engine {
	void CheckGLError(const char* desc) {
		int error = glGetError();
		if (error) {
			ERROR(std::dec << desc << " glError: 0x" << std::hex << error);
		}
	}

	bool BigEndian() {
		uint32_t _endiantest = 0xFF000000;
		return (uint8_t)((uint8_t*)(&_endiantest)[0]) == 0xFF;
	}

	renderer::renderer() :
		m_rect_VAO(gen_vao()),
		m_filled_rect_VAO(gen_vao()),
		m_text_VAO(gen_vao()),
		m_chart_VAO(gen_vao()),
		m_rect_vbo(std::make_shared<vertex_buffer>()),
		m_filled_rect_vbo(std::make_shared<vertex_buffer>()),
		m_text_vbo(std::make_shared<vertex_buffer>(1 << 24)),
		m_active_buffer(m_rect_vbo),
		last_border_size(0),
		m_viewport_size(glm::vec2(800, 600)),
		m_characterdata(std::map<char, character>{}),
		m_text_local_data(std::vector<TextParams>{})
	{
		m_text_local_data.reserve(2048 * sizeof(TextParams)); //Fairly generous allocation of space for 2048 characters of text in the frame
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) {
			ERROR("renderer::renderer() failed to initialize FreeType")
		}

		FT_Face face;
		if (FT_New_Face(ft, "fonts/arial.ttf", 0, &face)) {
			ERROR("Failed to load font 'arial.ttf' with error:");
		}
		else
		{
			FT_Set_Pixel_Sizes(face, 0, 64);
			
			GLint v;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &v);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glGenTextures(1, &m_font_texture);

			uint32_t max_dim = (1 + (face->size->metrics.height >> 5)) * ceilf(sqrtf(128));
			int tex_width = 512;
			while (tex_width < max_dim) tex_width <<= 1; //multiply the width by 2
			int tex_height = tex_width;

			char* pixels = new char[(tex_width * tex_height)];
			memset(pixels, 0, tex_width * tex_height);
			glm::ivec2 pen_pos{ 0,0 };

			FT_Select_Charmap(face, FT_ENCODING_UNICODE);

			FT_UInt index;
			FT_ULong c = FT_Get_First_Char(face, &index);
			FT_Error err;
			while (index) {
				err = FT_Load_Char(face, c, FT_LOAD_RENDER);
				if (err != 0) {
					ERROR("Failed to load glyph, error: " << err);
					err = 0;
				}

				if (pen_pos.x + face->glyph->bitmap.width + 1 >= tex_width) {
					pen_pos.x = 0;
					pen_pos.y += ((face->size->metrics.height >> 6) + 1);
				}

				if (face->glyph->bitmap.width > 0) {

					for (int row = 0; row < face->glyph->bitmap.rows; row++) {
						for (int col = 0; col < face->glyph->bitmap.width; col++) {
							glm::ivec2 xy = pen_pos + glm::ivec2(col, row);
							pixels[(xy.y+1) * tex_width + xy.x] = face->glyph->bitmap.buffer[row * face->glyph->bitmap.pitch + col];
						}
					}
				}
				else
				{
					WARN("renderer::renderer() font arial.ttf missing glyph bitmap for char " << c);
				}
				character k;
				k.size = glm::vec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
				k.uv_topleft = ((glm::vec2)pen_pos + glm::vec2(0.f, 0.f))/ glm::vec2(tex_width, tex_height);
				k.uv_topright = ((glm::vec2)pen_pos + glm::vec2(k.size.x, 0.f)) / glm::vec2(tex_width, tex_height);
				k.uv_bottomleft = ((glm::vec2)pen_pos + glm::vec2(0.f, k.size.y + 1.f)) / glm::vec2(tex_width, tex_height);
				k.uv_bottomright = ((glm::vec2)pen_pos + k.size + glm::vec2(0.f, 1.f)) / glm::vec2(tex_width, tex_height);
				/*k.uv_topleft = glm::vec2(0, 0);
				k.uv_topright = glm::vec2(1, 0);
				k.uv_bottomleft = glm::vec2(0, 1);
				k.uv_bottomright = glm::vec2(1, 1);*/
				k.bearing = glm::vec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
				k.advance = face->glyph->advance.x >> 6;
				m_characterdata.emplace(std::pair<char, character>{c, k});

				pen_pos.x += face->glyph->bitmap.width + 1;
				
				c = FT_Get_Next_Char(face, c, &index);
			}
			
			glBindTexture(GL_TEXTURE_2D, m_font_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width, tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
			//glGenerateMipmap(GL_TEXTURE_2D);

			//stbi_write_bmp("TextAtlas.bmp", tex_width, tex_height, 1, pixels);

			glBindTexture(GL_TEXTURE_2D, 0);
			

			FT_Done_FreeType(ft);
			delete[] pixels;

			glPixelStorei(GL_UNPACK_ALIGNMENT, v);
		}
		
		CheckGLError("wtf");

		bind_vao(m_rect_VAO);
		bind_buffer(m_rect_vbo);
		//set up the vertex attrib ptrs for the rect vbo 

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rect_vert), 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(GLfloat) * 3) );

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(float) * 5));

		//as per OpenGL 4.3 Khronos spec, glGetVertexAttribPointerv queries the Vertex Array Object (VAO state carries VertexAttribPointers, i.e reuse indices for separate VAOs)
		bind_vao(m_filled_rect_VAO);
		bind_buffer(m_filled_rect_vbo);
		//set up the vertex attrib ptrs for the filled rect vbo

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rect_vert), 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(GLfloat) * 3));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(float) * 5));

		bind_vao(m_text_VAO);
		bind_buffer(m_text_vbo);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rect_vert), 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(GLfloat) * 3));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(float) * 5));

		Engine::CheckGLError("Before chart VAO");

		bind_vao(m_chart_VAO);
		Engine::CheckGLError("chart vao 1");
		glCreateBuffers(1, &m_chart_vbo);
		Engine::CheckGLError("chart vao 2");
		glBindBuffer(GL_ARRAY_BUFFER, m_chart_vbo);

		Engine::CheckGLError("After create chart VAO");

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rect_vert), 0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(GLfloat) * 3));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(rect_vert), (void*)(sizeof(float) * 5));

		Engine::CheckGLError("After init chart vao");

		bind_vao(m_candle_VAO);
		glCreateBuffers(1, &m_candle_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_candle_vbo);
		glBufferData(GL_ARRAY_BUFFER, 1 << 26, 0, GL_DYNAMIC_DRAW); //1 << 26 is enough space for 10 years worth of per minute data

		Engine::CheckGLError("After init candle vao & vbo");

		struct candlevert {
			glm::vec3 pos;
		};

		candlevert bl{ glm::vec3(-.5f, -.5f, 0.f)};
		candlevert br{ glm::vec3(.5f, -.5f, 0.f)};
		candlevert tl{ glm::vec3(-.5f, .5f, 0.f)};
		candlevert tr{ glm::vec3(.5f, .5f, 0.f)};

		candlevert rect[6]{tl, bl, tr, tr, bl, br};

		glNamedBufferSubData(m_candle_vbo, 0, 6 * sizeof(candlevert), rect);

		CheckGLError("After init candle vao & vbo data");

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(candlevert), 0);

		glEnableVertexAttribArray(1);
		glVertexAttribDivisor(1, 1); //instanced attrib
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RenderCandleInfo), (void*)(sizeof(candlevert) * 6));

		CheckGLError("After per instance OHLC");

		glEnableVertexAttribArray(2);
		glVertexAttribDivisor(2, 1); //instanced attrib
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RenderCandleInfo), (void*)(sizeof(candlevert) * 6 + 4 * sizeof(GL_FLOAT)));

		CheckGLError("After per instance volume");
	}

	void renderer::write_candle_gpu_data(void* data, size_t size) {
		glNamedBufferSubData(m_candle_vbo, 6 * sizeof(glm::vec3), size, data);
	}

	void renderer::draw_candles(int count, const glm::mat4& projection) {
		bind_vao(m_candle_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_candle_vbo);

		std::shared_ptr<gl_shader> shader;
		shader = Engine::gl_shader::get_shader("candlebody");
		shader->bind();
		shader->set_uniform_mat4f("projection", projection);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);

		shader = Engine::gl_shader::get_shader("candlevolume");
		shader->bind();
		shader->set_uniform_mat4f("projection", projection);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);

		shader = Engine::gl_shader::get_shader("candlewick");
		shader->bind();
		shader->set_uniform_mat4f("projection", projection);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, count);
	}

	renderer::~renderer() {
		WARN("renderer::~renderer() Renderer instance destroyed");
	}

	std::shared_ptr<renderer>& renderer::get_instance() {
		static std::shared_ptr<renderer> instance(new renderer());
		return instance; //returns ref to the shared ptr so a copy op will fire on assignment to shared_ptr but I can assing to a shared_ptr& to access the static original and reset it to delete the instance.
	}

	std::shared_ptr<gl_shader> renderer::active_shader() {
		return gl_shader::s_bound_shader;
	}

	void renderer::bind_buffer(std::shared_ptr<gl_buffer> buffer) {
		buffer->bind();
		m_active_buffer = buffer;
		CheckGLError("renderer::bind_buffer");
	}

	void renderer::bind_shader(std::shared_ptr<gl_shader> shader) {
		shader->bind();
		CheckGLError("renderer::bind_shader");
	}

	GLuint renderer::gen_vao() {
		//generates and binds a Vertex Array object
		GLuint handle = 0;
		glGenVertexArrays(1, &handle);
		bind_vao(handle);
		return handle;
	}

	void renderer::bind_vao(GLuint vao) {
		glBindVertexArray(vao);
		m_active_VAO = vao;
	}

	void renderer::push_rect(RectParams c) {

		CheckGLError("renderer::push_rect before");
		
		if (m_active_VAO != m_rect_VAO) {
			bind_vao(m_rect_VAO);
		}

		if (m_active_buffer != m_rect_vbo) {
			bind_buffer(m_rect_vbo);
		}

		if (c.border != last_border_size) {
			if (last_border_size != -1 && !m_rect_vbo->empty()) {
				WARN("auto draw rects: vbo->content_size() = " << std::dynamic_pointer_cast<vertex_buffer>(m_active_buffer)->content_size() << " bytes; " << std::dynamic_pointer_cast<vertex_buffer>(m_active_buffer)->content_size() / sizeof(rect_data) << " rects");
				draw_rects();
			}//draw rects when border size uniform changes 
			last_border_size = c.border;
			WARN("auto change border size: last_border_size = " << last_border_size << " vbo->empty() = " << m_rect_vbo->empty());
			glLineWidth(static_cast<float>(c.border));
			CheckGLError("renderer::push_rect after glLineWidth");
		}
		//return;

		rect_vert tl{ glm::vec3(c.x,c.y,c.z), glm::vec2(0.f,0.f), c.color };
		rect_vert tr{ glm::vec3(c.x + c.width, c.y, c.z), glm::vec2(1.f,0.f), c.color };
		rect_vert bl{ glm::vec3(c.x, c.y + c.height, c.z ), glm::vec2(0.f, 1.f), c.color };
		rect_vert br{ glm::vec3(c.x + c.width, c.y + c.height, c.z ), glm::vec2(1.f,1.f), c.color };

		rect_data data{
			tl, tr, br, bl
		};
		//std::cout << "renderer::push_rect() m_rect_vbo->push_back(" << std::dec << sizeof(data) << ", " << std::hex << &data << ")" << std::endl;
		m_rect_vbo->push_back(sizeof(data), &data);

		CheckGLError("renderer::push_rect after");
	}

	void renderer::draw_rects() {
		//std::cout << "renderer::draw_rects()" << std::endl;

		//draw code
		CheckGLError("renderer::draw_rects() before glBindVertexArray");

		if (m_active_VAO != m_rect_VAO) {
			bind_vao(m_rect_VAO); //m_rect_vbo buffer should be inferred
			bind_buffer(m_rect_vbo);
		}

		std::shared_ptr<Engine::gl_shader> p_shader = Engine::gl_shader::get_shader("rect");
		if (p_shader != nullptr && p_shader != gl_shader::s_bound_shader) {
			bind_shader(p_shader);
		}
		//std::cout << "screenDim " << m_viewport_size.x << ", " << m_viewport_size.y << std::endl;
		p_shader->set_uniform2f("screenDim", m_viewport_size);

		//glLineWidth(static_cast<float>(last_border_size));
		CheckGLError("renderer::draw_rects() glLineWidth");
		
		//std::cout << "m_rect_vbo->content_size() " << m_rect_vbo->content_size() << " bytes" << std::endl;
		for (int i = 0; i < m_rect_vbo->content_size() / sizeof(rect_data); i++) {
			glDrawArrays(GL_LINE_LOOP, i * 4, 4);
			//std::cout << "renderer::draw_rects() DrawArrays GL_LINE_LOOP, " << std::dec << i * 4  <<", 4" << std::endl;
		}
		CheckGLError("renderer::draw_rects() glDrawArrays");

		//glFinish(); not necessary to sync (await async gpu draw calls to find) since we clear buffer by orphaning
		m_rect_vbo->debug();

		//cleanup after
		m_rect_vbo->clear();
	}

	void renderer::push_filled_rect(FilledRectParams f) {
		//std::cout << "renderer::push_filled_rect rgba = " << std::dec << glm::to_string(rgba) << std::endl;
		CheckGLError("renderer::push_filled_rect before");

		if (m_active_VAO != m_filled_rect_VAO) {
			bind_vao(m_filled_rect_VAO);
		}
		if (m_active_buffer != m_filled_rect_vbo) {
			bind_buffer(m_filled_rect_vbo);
		}

		rect_vert tl{ glm::vec3(f.x,f.y,f.z), glm::vec2(0.f, 0.f), f.color };
		rect_vert tr{ glm::vec3(f.x + f.width,f.y,f.z), glm::vec2(1.f, 0.f), f.color };
		rect_vert bl{ glm::vec3(f.x,f.y + f.height,f.z), glm::vec2(0.f, 1.f), f.color };
		rect_vert br{ glm::vec3(f.x + f.width,f.y + f.height,f.z), glm::vec2(1.f, 1.f), f.color };

		filled_rect_data data{
			tl, bl, tr,
			tr, bl, br
		};

		m_filled_rect_vbo->push_back(sizeof(data), &data);

		//draw_text(std::to_string(z), x + w, y + h, z, 100, 20, 12, glm::vec4(1.f, 1.f, 1.f, 1.f), false, false);
	}

	void renderer::draw_filled_rects() {
		CheckGLError("renderer::draw_filled_rects() before");

		if (m_active_VAO != m_filled_rect_VAO) {
			bind_vao(m_filled_rect_VAO);
		}
		if (m_active_buffer != m_filled_rect_vbo) {
			bind_buffer(m_filled_rect_vbo);
		}

		std::shared_ptr<Engine::gl_shader> p_shader = Engine::gl_shader::get_shader("rect");
		if (p_shader != nullptr && p_shader != active_shader()) {
			bind_shader(p_shader);
		}
		//std::cout << "screenDim " << m_viewport_size.x << ", " << m_viewport_size.y << std::endl;
		p_shader->set_uniform2f("screenDim", m_viewport_size);

		//std::cout << "glDrawArrays(GL_TRIANGLES, 0, " << m_filled_rect_vbo->content_size() / sizeof(rect_vert) << ");" << std::endl;
		glDrawArrays(GL_TRIANGLES, 0, m_filled_rect_vbo->content_size()/sizeof(rect_vert));
		CheckGLError("renderer::draw_filled_Rects() glDrawArrays");

		m_filled_rect_vbo->debug();
		m_filled_rect_vbo->clear();//orphan buffer(implicit sync -> sync safe)

		CheckGLError("renderer::draw_filled_rects() after");
	}

	void renderer::push_text(const TextParams& t) {
		m_text_local_data.push_back(t);
	}

	void renderer::draw_text() {

		if (m_active_VAO != m_text_VAO) {
			bind_vao(m_text_VAO);
		}
		std::shared_ptr<gl_shader> shader = gl_shader::get_shader("text");
		if (active_shader() != shader) {
			bind_shader(shader);
		}
		if (m_active_buffer != m_text_vbo) {
			bind_buffer(m_text_vbo);
		}
		//std::cout << "screenDim " << m_viewport_size.x << ", " << m_viewport_size.y << std::endl;
		shader->set_uniform2f("screenDim", m_viewport_size);
		//slot 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_font_texture);
		shader->set_uniform1i("atlas", 0);

		std::vector<filled_rect_data> frame_text_data{};
		frame_text_data.reserve(50 * sizeof(filled_rect_data));

		for (auto itext : m_text_local_data) {
			frame_text_data.clear();

			float size = (float)itext.size / 48.f;

			glm::ivec2 cursor{ itext.x ,itext.y + itext.size };

			for (auto it = itext.s.cbegin(); it != itext.s.cend(); it++) {
				char k = (*it);
				auto _c = m_characterdata.find(k);

				if ((int)k == (int)'\n') {
					cursor.x = itext.x;
					cursor.y += itext.size;
				}
				else if (_c != m_characterdata.end()) {
					character c = _c->second;
					if (cursor.x + (c.advance - c.bearing.x) * size >= (itext.width + itext.x) && k != (char)32 && itext.wrap) {
						cursor.x = itext.x;
						cursor.y += itext.size + 15.f * size;
					}

					float x1 = cursor.x + c.bearing.x * size;
					float y1 = cursor.y - (c.bearing.y) * size;
					float x2 = x1 + c.size.x * size;
					float y2 = y1 + c.size.y * size;

					glm::vec3 tl{ x1, y1, itext.z };
					glm::vec3 tr{ x2, y1, itext.z };
					glm::vec3 bl{ x1, y2, itext.z };
					glm::vec3 br{ x2, y2, itext.z };

					filled_rect_data data{ //flip the y axis 
						rect_vert{tl, c.uv_topleft, itext.color},
						rect_vert{bl, c.uv_bottomleft, itext.color},
						rect_vert{tr, c.uv_topright, itext.color},
						rect_vert{tr, c.uv_topright, itext.color},
						rect_vert{bl, c.uv_bottomleft, itext.color},
						rect_vert{br, c.uv_bottomright, itext.color}
					};

					//m_text_vbo->push_back(sizeof(data), &data);
					frame_text_data.push_back(data);

					cursor.x += (float)(c.advance - c.bearing.x) * size;
				}
				else
				{
					ERROR("renderer::draw_text: Failed to find char " << (int)k << "(" << k << ")");
				}
			}

			//std::cout << frame_text_data.size() << std::endl;
			m_text_vbo->push_back(frame_text_data.size() * sizeof(filled_rect_data), &frame_text_data[0]);
			//std::cout << "glDrawArrays(GL_TRIANGLES, 0, " << m_text_vbo->content_size() / sizeof(rect_vert) << ")" << std::endl;
		}

		glDrawArrays(GL_TRIANGLES, 0, m_text_vbo->content_size() / sizeof(rect_vert));
		//m_text_vbo->debug();
		m_text_vbo->clear();

		

		//glDepthMask(GL_TRUE);
		//glFinish();
		m_text_local_data.clear();
		//glActiveTexture(0);
	}

	void renderer::draw_chart_rect(const glm::vec2& a, const glm::vec2& b) {
		if (m_active_VAO != m_chart_VAO) {
			bind_vao(m_chart_VAO);
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		glm::vec2 s2 = get_viewport_size();
		//std::cout << b.x << " " << b.y << " / " << s2.x << " " << s2.y << std::endl;
		glm::vec2 p = a / s2;
		glm::vec2 s = b / s2;

		rect_vert tl{ glm::vec3(a.x, a.y, 0.f), glm::vec2(0.f, 1.f), glm::vec4(1.f)};
		rect_vert tr{ glm::vec3(a.x + b.x, a.y, 0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f)};
		rect_vert bl{ glm::vec3(a.x, a.y + b.y, 0.f), glm::vec2(0.f, 0.f), glm::vec4(1.f)};
		rect_vert br{ glm::vec3(a.x + b.x, a.y + b.y, 0.f), glm::vec2(1.f, 0.f), glm::vec4(1.f)};

		filled_rect_data data{
			tl, bl, tr,
			tr, bl, br
		};

		glNamedBufferData(m_chart_vbo, sizeof(filled_rect_data), &data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
	}

	glm::vec2 renderer::get_text_bounds(const std::string& s, int32_t width, int32_t size) {
		if (this == nullptr) {
			return glm::vec2(0);
		}
		float _size = (float)size / 48.f;
		float maxX = 0;

		float lastwidth = 0;
		float lastadvance = 0;
		float lastbearing = 0;

		glm::vec2 cursor{ 0, size };
		for (auto it = s.cbegin(); it != s.cend(); it++) {
			char k = *it;
			auto cd = m_characterdata.find(k);
			if ((int)k == (int)'\n') {
				cursor.y = cursor.y + size + 15.f * _size;
			}
			else if (cd != m_characterdata.end()) {
				float sx = cd->second.advance;
				
				float newX = cursor.x + sx *_size;

				lastwidth = cd->second.size.x * _size;
				lastadvance = cd->second.advance * _size;
				lastbearing = cd->second.bearing.x * _size;

				maxX = std::max(newX - lastadvance + lastwidth - lastbearing, maxX);
				//std::cout << k << " " << maxX << std::endl;
				if (newX > width) {
					cursor.y += size + 15.f * _size;
					cursor.x = 0;
				}
				else
				{
					cursor.x = newX;
				}

				
			}
		}

		return glm::vec2(std::min((float)width, maxX), cursor.y + 15.f * _size);
	}

	glm::vec2 renderer::get_viewport_size() {
		return m_viewport_size;
	}

	void renderer::set_viewport_size(int32_t width, int32_t height) {
		m_viewport_size = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	}

	void renderer::exit() {//static
		if (shutdown) {
			return;
		}
		std::shared_ptr<renderer>& refinst = get_instance();
		refinst.reset();
		shutdown = true;
		WARN("renderer::exit()");
	}

	bool renderer::shutdown = false;

}