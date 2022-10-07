#include "gl_shader.h"
#include <fstream>
#include <vector>

namespace Engine {
	std::shared_ptr<gl_shader> gl_shader::s_bound_shader;
	std::map<std::string, std::shared_ptr<gl_shader>> gl_shader::s_shaders{};

	GLuint gl_shader::load_shader(const char* path, GLenum shader_type) {
		/*
		Given a relative path from the SolutionDir and a shader type (e.g. GL_VERTEX_SHADER) loads the shader source and compiles
		returns -1 if failed
		returns the shader GLuint driver handle if succesful 

		read:
		www.khronos.org/opengl/wiki/Shader_Compilation
		*/
		
		GLuint shader = glCreateShader(shader_type);

		std::string shader_source = "";
		{
			std::ifstream ifs;
			ifs.exceptions(std::ios_base::badbit | std::ios_base::failbit); //set badbit and fialbit exceptions

			try {
				ifs.open(path);

				auto size = ifs.gcount();

				ifs.clear();
				ifs.seekg(0, std::ios_base::beg);

				shader_source = std::string{ std::istreambuf_iterator<char>{ifs}, {} };
			}
			catch (const std::ifstream::failure& e) {
				ERROR("gl_shader.cpp::" << path << " - " << e.code() << ": " << e.what());
			}
			//scoped objects cleaned up by lifetime

			ifs.close();
			//clean up file handle
		}

		const char* k = shader_source.c_str();
		glShaderSource(shader, 1, &k, 0);

		glCompileShader(shader);

		GLint status = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			GLint length = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

			std::vector<GLchar> log(length);
			glGetShaderInfoLog(shader, length, &length, &log[0]);

			glDeleteShader(shader);

			ERROR("gl_shader.cpp::gl_shader::load_shader: " << path << ": " << &log[0]);

			return -1;
		}
		else
		{
			return shader;
		}
	}

	void gl_shader::create_shader(std::string name, const char* vpath, const char* fpath) {
		std::shared_ptr<gl_shader> shader = std::make_shared<gl_shader>(name, vpath, fpath);
		s_shaders.emplace(std::pair<std::string, std::shared_ptr<gl_shader>>(name, shader));
	}

	gl_shader::gl_shader(std::string name, const char* vpath, const char* fpath) : m_program(glCreateProgram()), m_valid(false), m_shaders({}), m_name(name) {
		
		GLuint vertex_shader = load_shader(vpath, GL_VERTEX_SHADER);
		if (vertex_shader == -1) {
			ERROR("gl_shader.cpp::gl_shader() Failed to load vertex shader " << vpath);
		}
		else
		{
			GLuint fragment_shader = load_shader(fpath, GL_FRAGMENT_SHADER);
			if (fragment_shader == -1) {
				ERROR("gl_shader.cpp::gl_shader() Failed to load fragment shader " << vpath);
			}
			else
			{

				glAttachShader(m_program, vertex_shader);
				glAttachShader(m_program, fragment_shader);
				glLinkProgram(m_program);

				//after linking (success or no) detach the shaders from the gl state machine (We're done with them) 
				glDetachShader(m_program, vertex_shader);
				glDetachShader(m_program, fragment_shader);

				GLint linked = 0;
				glGetProgramiv(m_program, GL_LINK_STATUS, (int*)&linked);
				if (linked == GL_FALSE) {
					ERROR("gl_shader::gl_shader link error (status " << linked << ")");

					GLint len = 0;
					GLsizei msglen = 0;
					glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &len);
					//std::cout << len << std::endl;

					GLchar *msg = new GLchar[len];
					glGetProgramInfoLog(m_program, len, &msglen, msg); //dont care about the size returned, cout deals with that crap, guess I should worry about debugging internal gl driver memcpy to the vector and the vector not resizing... (bad code!!)
					ERROR("gl_shader.cpp::gl_shader() link error: " << msg);

					glDeleteProgram(m_program);
				}
				else
				{
					//if all shader load succesful and link succesful, set valid to true 
					m_valid = true;
					WARN("Shader " << m_name << " generated with id " << m_program);
				}
				glDeleteShader(vertex_shader);
				glDeleteShader(fragment_shader);
			}
		}
	}

	gl_shader::~gl_shader() {
		/*for (auto shdr : m_shaders) {
			//glDetachShader(m_program, shdr);
			
			//todo: check shader deleted
		}*/
		if (s_bound_shader.get() == this) {
			unbind();
		}
		glDeleteProgram(m_program);
		WARN("gl_shader.cpp::gl_shader::~gl_shader() destroyed shader '" << m_name << "' object at " << std::hex << this);
	}

	void gl_shader::bind() {
		if (m_valid == false) {
			ERROR("gl_shader.cpp::gl_shader::bind() Attempt to bind invalid shader program (loaded incorrectly)");
			return;
		}

		CheckGLError("gl_shader::bind before glUseProgram(m_program)");
		glUseProgram(m_program);
		std::shared_ptr<gl_shader> shader = get_shader(this->name());
		s_bound_shader = shader;
		CheckGLError("gl_shader::bind glUseProgram(m_program)");
		//std::cout << "gl_shader.cpp: bound shader '" << m_name << "' with ID " << m_program << std::endl;
	}

	void gl_shader::unbind() {
		glUseProgram(0);
		s_bound_shader = nullptr;
	}

	const std::string& gl_shader::name() {
		return m_name;
	}

	GLint gl_shader::get_uniform_location(const char* name) {
		if (s_bound_shader.get() != this) {
			WARN("gl_shader.cpp: Accessing uniform '" << name <<  "' of non-bound shader object '" << m_name);
		}

		CheckGLError("gl_shader::get_uniform_location before glGetUniformLocation call");

		GLint x = glGetUniformLocation(m_program, name);
		if (x == -1) {
			ERROR("gl_shader.cpp:: Could not find uniform '" << name << "' in shader '" << s_bound_shader->name() << "'" << " glError: " << glGetError());
			return -1;
		}
		else
		{
			return x;
		}

		CheckGLError("gl_shader::get_uniform_location after glGetUniformLocation call");
	}

	void gl_shader::set_uniform1i(const char* name, GLint val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform1i(loc, val);
		}
	}

	void gl_shader::set_uniform1f(const char* name, GLfloat val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform1f(loc, val);
		}
	}

	void gl_shader::set_uniform1b(const char* name, GLboolean val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform1i(loc, val);
		}
	}

	void gl_shader::set_uniform2f(const char* name, const glm::vec2& val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform2f(loc, val.x, val.y);
		}
	}

	void gl_shader::set_uniform3f(const char* name, const glm::vec3& val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform3f(loc, val.x, val.y, val.z);
		}
	}

	void gl_shader::set_uniform4f(const char* name, const glm::vec4& val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniform4f(loc, val.x, val.y, val.z, val.w);
		}
	}

	void gl_shader::set_uniform_mat4f(const char* name, const glm::mat4& val) {
		GLint loc = get_uniform_location(name);
		if (loc != -1) {
			glUniformMatrix4fv(loc, 1, false, glm::value_ptr(val));
		}
	}

	std::shared_ptr<gl_shader> gl_shader::get_shader(const std::string& name) {
		auto it = s_shaders.find(name);
		if (it != s_shaders.end()) {
			//std::cout << "gl_shader::get_shader() found shader '" << name << "'" << std::endl;
			return (*it).second;
		}
		else
		{
			ERROR("gl_shader::get_shader(): Failed to find shader with name '" << name << "'");
		}
	}

	void gl_shader::exit() {
		s_shaders.clear();
	}
}