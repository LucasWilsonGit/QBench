#pragma once
#include "core.h"
#include <map>
#include <vector>

namespace Engine {
	class gl_shader
	{
	public:
		gl_shader(std::string name, const char* vpath, const char* fpath); //DO NOT CALL, use gl_shader::create_shader, method only public since used by create_shared call
		~gl_shader();

		void bind();
		void unbind();
		const std::string& name();

		void set_uniform1i(const char* name, GLint value);
		void set_uniform1f(const char* name, GLfloat value);
		void set_uniform1b(const char* name, GLboolean value);
		void set_uniform2f(const char* name, const glm::vec2& vec);
		void set_uniform3f(const char* name, const glm::vec3& vec);
		void set_uniform4f(const char* name, const glm::vec4& vec);
		void set_uniform_mat4f(const char* name, const glm::mat4& mat);

		static std::shared_ptr<gl_shader> get_shader(const std::string& name);
		static void create_shader(std::string name, const char* vpath, const char* fpath); 
		static void exit();

		static std::shared_ptr<gl_shader> s_bound_shader; //DONT MODIFY THIS FFS BUT IT CANT BE CONST

	private:
		
		//copy name since it may be scoped and destructed 

		GLuint m_program;
		const std::string m_name;
		bool m_valid;

		std::vector<GLuint> m_shaders;

		static std::map<std::string, std::shared_ptr<gl_shader>> s_shaders;

		GLuint load_shader(const char* path, GLenum shader_type);
		GLint get_uniform_location(const char* name);
	};
}

