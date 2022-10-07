#include "vertex_buffer.h"
#include <string>
#include <sstream>

namespace Engine {
	vertex_buffer::vertex_buffer(GLsizeiptr size) : _vwarn(true), m_size(size), gl_buffer() {
		WARN("CUSTOM SIZE VERTEX_BUFFER CALL");
		//generates a buffer with $(size) memory, defaults to 2^18 bytes  
		//todo: allow for resizing buffer 
		glGenBuffers(1, &m_buffer);

		GLuint prev_buf = s_bound_buffer;

		m_size = size;

		bind();
		glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW); //a bit yucky to default assign a buffer with 2^18 bytes of memory for GL_DYNAMIC_DRAW usage
		//but it sizes the buffer to an acceptable worst case buffer size (accepts ~1500 rects(6 verts, 26 bytes each))

		//m_heap = new uint8_t[m_size]; //2^18 bytes of memory

		if (prev_buf) {
			glBindBuffer(GL_ARRAY_BUFFER, prev_buf);
		}
	}

	vertex_buffer::vertex_buffer() :_vwarn(true), m_size(1 << 18), gl_buffer() {
		glGenBuffers(1, &m_buffer);

		GLuint prev_buf = s_bound_buffer;

		bind();
		glBufferData(GL_ARRAY_BUFFER, m_size, NULL, GL_DYNAMIC_DRAW);

		//m_heap = new uint8_t[1 << 18]; //2^18 bytes of memory

		if (prev_buf) {
			glBindBuffer(GL_ARRAY_BUFFER, prev_buf);
		}
	}

	vertex_buffer::~vertex_buffer() {
		glDeleteBuffers(1, &m_buffer);
		WARN("vertex buffer deleted");

		//delete[] m_heap;
	}

	void vertex_buffer::warn_mutate_nonbound_buffer() {
		if (s_bound_buffer != m_buffer) {
			WARN("vertex_buffer::clear: call to buffer modifying function of non-bound buffer object");
			bind();
		}
	}

	void vertex_buffer::clear() {
		m_head = 0;
		warn_mutate_nonbound_buffer();
		glBufferData(GL_ARRAY_BUFFER, m_size, NULL, GL_DYNAMIC_DRAW);//orphan the buffer
		//memset(m_heap, 0, m_content_size);
		m_content_size = 0;
	}

	void vertex_buffer::bind() {
		GLint x;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &x);

		if (x == 0) {
			ERROR("vertex buffer::bind: No currently bound VAO");
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			s_bound_buffer = m_buffer;
		}
	}

	void vertex_buffer::push_back(size_t size, const void* data) {
		//std::cout << "vertex_buffer.cpp::vertex_buffer::push_back(" << std::dec << size << ", " << std::hex << data << ")" << std::endl << std::dec;
		m_content_size += size;
		_vwarn = false;
		overwrite(size, m_head, data);
		//memcpy( m_heap + m_head, data, size);
		_vwarn = true;
		m_head += size;
	}

	void vertex_buffer::overwrite(size_t size, GLintptr offs, const void* data) {
		//std::cout << "vertex_buffer.cpp::vertex_buffer::overwrite(" << size << ", " << offs << ", " << std::hex << data << ")" << " buffer size " << std::dec << m_size << " bytes " << std::dec << std::endl;
		warn_mutate_nonbound_buffer();

		//CheckGLError("vertex_buffer::overwrite before glBufferSubData");
		glBufferSubData(GL_ARRAY_BUFFER, offs, size, data);
		/*std::string s = "vertex_buffer::overwrite after glBufferSubData(GL_ARRAY_BUFFER, ";
		std::stringstream ss;
		ss << std::hex << data;
		s = s + std::to_string(offs) + ", " + std::to_string(size) + ", " + ss.str() + ")";
		CheckGLError(s.c_str());*/

		if (_vwarn) {
			WARN("vertex_buffer: overwrite {size=" << size << ", offs=" << offs << "data addr=" << data);
		}

		if (size + offs >= m_size) {
			ERROR("vertex buffer size exceeded maximum limits of " << m_size << " bytes)");
		}
	}

	void vertex_buffer::debug() {
		return;
		//
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, m_content_size, m_heap); //according to the spec, this syncs so can take ~30ms, calls to debug should be wrapped in compiler IF macros checking for DEBUG definition
		CheckGLError("glGetBufferSubData");
		//std::cout << "copy buffer data to member heap of vbuf for debug" << std::endl;

		//std::cout << "vertex_buffer<" << this << ">{size = " << m_size << ", content_size = " << m_content_size << ", head = " << m_head << "}" << std::endl;
		for (int offs = 0; offs < m_content_size; offs++) {
			//std::cout << std::hex << +(m_heap[offs]) << " ";
			if ( (offs+1) % 4 == 0) {
				//std::cout << std::endl;
			}
		}
		//std::cout << std::endl;
	}

	bool vertex_buffer::empty() {
		return m_content_size == 0;
	}

	GLsizeiptr vertex_buffer::content_size() const {
		return m_content_size;
	}
}