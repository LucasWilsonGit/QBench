#pragma once
#define GLFW_EXPOSE_NATIVE_WIN32
#include "Glad/glad.h"
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h" //allows for native COM interface
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"//for printing glm vectors with cout
#include "glm/gtc/type_ptr.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H
#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include "luainclude.h"

#ifndef __CORE_H

#define __CORE_H 1
#define FOREGROUND_WHITE (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define CONSOLECOLOR(x) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), x);
#define ERROR(x) CONSOLECOLOR(FOREGROUND_RED); std::cout << x << std::endl; CONSOLECOLOR(FOREGROUND_WHITE); DebugBreak();
#define WARN(x) CONSOLECOLOR(FOREGROUND_RED | FOREGROUND_GREEN); std::cout << x << std::endl; CONSOLECOLOR(FOREGROUND_WHITE);
#define BIT(x) 1 << (x - 1)
#define DEBUG 1//unused atm since we have an error hook with glfw and don't need to paste CHECKERROR() macros (todo: implement CHECKERROR() macros) 

namespace Engine {
	void CheckGLError(const char*); //defined in renderer.cpp
	bool BigEndian();
}

#endif



