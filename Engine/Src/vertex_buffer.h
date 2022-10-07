#pragma once
#include "gl_buffer.h"
#define VBUF_OVERWRITE_WARNING 0 
namespace Engine {
    class vertex_buffer :
        public gl_buffer
    {
    private:
        GLuint m_buffer;
        bool _vwarn;
        void warn_mutate_nonbound_buffer();
        GLsizeiptr m_size;
        uint8_t* m_heap;
    public:
        vertex_buffer(GLsizeiptr buffer_size);
        vertex_buffer();
        ~vertex_buffer();

        void clear() override;
        void bind() override;
        void push_back(size_t size, const void* data) override;
        void overwrite(size_t size, GLintptr offs, const void* data) override;
        void debug();
        bool empty() override;

        GLsizeiptr content_size() const;
    };
}

