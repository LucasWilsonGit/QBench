#include "../../Engine/Src/Engine.h"
#include <string>
#include <map>
#include "src/app.h"

int main() 
{
	try {
		{
			DissertationProject::app application(1200, 900);
			while (!application.m_toexit) {
				application.on_update();
			}
		}
		glfwTerminate();
	}
	catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}
	catch (...) {

	}

	

	return 0;
}