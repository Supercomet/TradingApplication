#include <iostream>
#include <memory>
#include "App.h"

int main(int argc, const char* argv)
{
	(void)argc;
	(void)argv;

	std::unique_ptr<App> app = std::make_unique<App>();
	
	app->Run();

	return 0;
}