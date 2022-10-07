#include "gl_buffer.h"

namespace Engine {
	gl_buffer::gl_buffer() : m_head(0), m_content_size(0) {}
	gl_buffer::~gl_buffer() {}
	GLuint gl_buffer::s_bound_buffer = 0;


}