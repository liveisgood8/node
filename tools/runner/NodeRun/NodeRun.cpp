#include <iostream>

#include <windows.h>


int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	SetEnvironmentVariableA("NODE_DISABLE_COLORS", "1");

	using mainStartFunc_t = int(_stdcall*)(int argc_, char *argv_[]);

	HMODULE handle = LoadLibraryA("node.dll");
	if (!handle)
	{
		std::cerr << "Can not load node.dll" << std::endl;
		std::cerr << "Last error code: " << ::GetLastError() << std::endl;

		system("pause");

		return 1;
	}

	mainStartFunc_t runFunction = reinterpret_cast<mainStartFunc_t>(GetProcAddress(handle, "MainStart"));
	if (!runFunction)
	{
		std::cerr << "Can not get MainStart function address" << std::endl;
		std::cerr << "Last error code: " << ::GetLastError() << std::endl;

		system("pause");

		return 2;
	}

	int result = runFunction(argc, argv);
	return result;
}
