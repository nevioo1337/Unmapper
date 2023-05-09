#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <Shlobj.h>

#include <MinHook.h>

std::string GetDesktopPath() {
	char* pValue;
	std::string path;
	size_t len;
	errno_t err = _dupenv_s(&pValue, &len, "USERPROFILE");
	if (err == 0 && pValue != NULL) {
		path = pValue;
		path += "\\Desktop\\";
	}
	return path;
}
std::string dumpDirBase = GetDesktopPath() + "UnmapperDump\\";

int wpmCounter = 0;
std::string wpmDumpDir = dumpDirBase + "WPM\\";
typedef void(WINAPI* TWriteProcessMemory)(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
TWriteProcessMemory pWriteProcessMemory = nullptr;
void WINAPI WriteProcessMemoryDetour(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) {
	std::filesystem::create_directories(wpmDumpDir);
	std::string fileName = wpmDumpDir + std::to_string(wpmCounter) + "_0x" + std::to_string((int)lpBaseAddress) + ".bin";
	
	std::cout << "Dumping " << nSize << " bytes to " << fileName << std::endl;
	
	FILE* file;
	fopen_s(&file, fileName.c_str(), "wb");
	fwrite(lpBuffer, 1, nSize, file);
	fclose(file);
	
	wpmCounter++;
	return pWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

void Unmapper() {
	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);
	
    MH_Initialize();
	std::cout << "Unmapper loaded\n";
	
	MH_CreateHookApi(L"kernel32.dll", "WriteProcessMemory", &WriteProcessMemoryDetour, reinterpret_cast<LPVOID*>(&pWriteProcessMemory));
	std::cout << "Hooked WriteProcessMemory\n";

	MH_EnableHook(MH_ALL_HOOKS);
	std::cout << "Hooks enabled\n\n";
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Unmapper();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
