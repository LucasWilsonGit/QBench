#pragma once
#include "core.h"
namespace Engine {
	class gl_buffer
	{
	protected:
		GLintptr m_head; //end of buffer contents
		GLsizeiptr m_content_size; //size(bytes) of buffer data
		static GLuint s_bound_buffer;
	public:
		gl_buffer();
		~gl_buffer();

		virtual void clear() = 0;
		virtual void bind() = 0;
		virtual void push_back(size_t data_size, const void* data) = 0;
		virtual void overwrite(size_t size, GLintptr offs, const void* data) = 0;
		virtual bool empty() = 0;
	};
}

