// OBS-Instance-Manager.cpp : Defines the entry point for the console application.
//

// Last tested with OBS 23.0.2
#include "stdafx.h"
#include <SDKDDKVer.h>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <atlbase.h>
#include "atlstr.h"
#include <atlalloc.h>
#include <shlobj.h>
#include <streambuf>

namespace fs = std::filesystem;
using std::string;
using std::vector;

//  Forward declarations:
DWORD FindProcessId(const std::wstring& processName);
const char* WinGetEnv(const char* name);
HRESULT CreateShortcut(LPCTSTR lpszFileName, LPCTSTR lpszDesc, LPCTSTR lpszShortcutPath, LPCTSTR lpszArguments, int iShowCmd);
string OBSPath;

bool exists(const std::string& file) {
	struct stat buffer;
	return (stat(file.c_str(), &buffer) == 0);
}
string findDefaultOBS() {
	string default32 = "C:\\Program Files\\obs-studio\\bin\\32bit\\obs32.exe";
	string default64 = "C:\\Program Files\\obs-studio\\bin\\64bit\\obs64.exe";
	string default32x86 = "C:\\Program Files (x86)\\obs-studio\\bin\\32bit\\obs32.exe";
	string default64x86 = "C:\\Program Files (x86)\\obs-studio\\bin\\64bit\\obs64.exe";
	// Check for x64 OBS first for possible performance boost.
	if (exists(default64)) { return default64; }
	else if (exists(default32)) { return default32; }
	else if (exists(default32x86)) { return default32x86; }
	else if (exists(default64x86)) { return default64x86; }
	else return "";
}
string pathFromUser() {
	string temp;
	std::cout << "Please enter a path (eg. \"C:\\Programs\\OBS\\obs64.exe\"): ";
	std::cin >> temp;
	std::cout << std::endl;
	return temp;
}
void setDirectoryDefault(){
	char module_name[MAX_PATH];
	GetModuleFileNameA(0, module_name, MAX_PATH);
	PathRemoveFileSpecA(module_name);

	SetCurrentDirectoryA(module_name);
}
bool OBSRunning(){
	return (FindProcessId(L"obs64.exe") || FindProcessId(L"obs.exe"));
}
void saveConfig(){
	std::ofstream configFile;
	configFile.open("config.cfg", std::ofstream::out | std::ios_base::trunc);
	configFile << OBSPath;
	configFile.close();
}

//----------------------------
// MAIN
//----------------------------
int main(int argc, TCHAR argv[])
{
	if (OBSRunning())
	{
		std::cout << "OBS is already running!" << std::endl;
		std::cout << "Continuing with the process may cause issues." << std::endl;
	}

	// Get OBS install directory, if not found then ask user.
	bool OBSGood = false;
	std::ifstream configFile("config.cfg");
	if (configFile.good()){
		getline(configFile, OBSPath);
		if (exists(OBSPath)) { OBSGood = true; }
	}

	if (!OBSGood) {
		string opt;
		OBSPath = findDefaultOBS();
		std::cout << "No OBS path picked, or has changed." << std::endl;
		if (OBSPath != "") {
			std::cout << "We've detected: " << OBSPath << std::endl;
			char response;
			do
			{
				std::cout << "Is this correct? [y/n]: ";
				std::cin >> response;
				std::cout << std::endl;
				tolower(response);
			} while (!std::cin.fail() && response != 'y' && response != 'n');

			if (response == 'y') {
				std::cout << "Saved." << std::endl;
			}
			else if (response == 'n') {
				OBSPath = pathFromUser();
			}
		}
		else {
			OBSPath = pathFromUser();
		}
		std::cout << std::endl;
		saveConfig();
	}

	std::cout << "------------" << std::endl;
	std::cout << "Commands: " << std::endl;
	std::cout << "list -- Lists existing obs-studio instances." << std::endl;
	std::cout << "add <name> -- Copies [Current] obs-studio settings to a new instance." << std::endl;
	std::cout << "rename <name> -- Changes [Current] obs-studio name." << std::endl;
	std::cout << "remove <name> -- Removes instance settings (No undo!)." << std::endl;
	std::cout << "switch <name> -- Switches to given obs-studio instance." << std::endl;
	std::cout << "shortcut desktop -- Creates a shortcut in specified folder." << std::endl;
	std::cout << "start -- Starts OBS" << std::endl;
	std::cout << "inst <dir> -- Tells the program where OBS is located, for the 'start' command" << std::endl;
	// --------------------
	// Add plugin_config to be moved as well
	// profiler_data
	// --------------------


	// Files that differ between instances:
	// ./basic/*
	// ./global.ini
	std::string path = WinGetEnv("APPDATA"); // Get AppData path string
	std::string curPath = path + "\\" + "obs-studio";


	while (true) {
		string command = "";
		std::cout << std::endl << "Enter a command: ";
		while (command == "") {
			std::getline(std::cin, command);
		}

		// --------------------
		// LIST COMMAND
		// --------------------
		if (command.find("list") != string::npos) {
			vector<std::string> obsfolders = vector<std::string>();
			for (const auto& entry : fs::directory_iterator(path)) {// Foreach file in directory
				std::string fld = entry.path().string();
				std::string foldername = fld.substr(fld.find_last_of("\\") + 1, fld.length());
				if (foldername.find("obs-studio") != std::string::npos) {
					if (foldername.length() > 10) {
						foldername = foldername.substr(11, foldername.length());
						std::cout << "- " << foldername << std::endl; // Other OBS Studio profiles
					}
					else {
						std::string curName;
						std::ifstream fl(curPath + "\\name.obsinstance");
						std::stringstream cNbuffer;
						cNbuffer << fl.rdbuf();
						curName = cNbuffer.str();
						fl.close();

						if (curName.length() < 1)
						{
							std::cout << "----------------------------------------" << std::endl;
							std::cout << "--    Active profile has no name!     --" << std::endl;
							std::cout << "-- Use 'rename <name>' to give it one --" << std::endl;
							std::cout << "----------------------------------------" << std::endl;
						}
						else {
							std::cout << curName << "\t[Currently Active OBS-Studio]" << std::endl; // This one does not have text after it, so it's active.
						}
					}
				}
			}
		}
		// --------------------
		// START COMMAND
		// --------------------
		else if (command.find("start") != string::npos) {
			// DEBUG std::cout << '"' << OBSPath.c_str() << '"' << std::endl;
			std::cout << "Starting OBS... ";

			string launchString = "start \"\" \"" + OBSPath + '"';
			string OBSFolder = OBSPath.substr(0, OBSPath.find_last_of("\\"));

			USES_CONVERSION_EX;
			LPWSTR lpOBSPath = A2W_EX(OBSPath.c_str(), OBSPath.length());

			LPWSTR lpOBSFolder = A2W_EX(OBSFolder.c_str(), OBSFolder.length());
			SetCurrentDirectory(lpOBSFolder);

			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			if (!CreateProcess(NULL, lpOBSPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
				printf("CreateProcess failed (%d).\n", GetLastError());
			}

			WaitForSingleObject(pi.hProcess, INFINITE);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			setDirectoryDefault();
			std::cout << std::endl;
		}
		// --------------------
		// RENAME COMMAND
		// --------------------
		else if (command.find("rename") != string::npos) {
			std::string obsNewName = command.erase(0, 7); // Remove "rename "
			std::replace(obsNewName.begin(), obsNewName.end(), ' ', '-'); // Replace spaces in name with hypens

			std::ofstream file(curPath + "\\name.obsinstance");
			file << obsNewName;
			file.close();

			std::cout << "Successfully changed name to '" << obsNewName << "'!" << std::endl;
		}
		// --------------------
		// ADD COMMAND
		// --------------------
		else if (command.find("add") != string::npos) {
			std::string obsNewName = command.erase(0, 4); // Remove "add "
			std::replace(obsNewName.begin(), obsNewName.end(), ' ', '-'); // Replace spaces in name with hypens
			std::string newPath = path + "\\" + "obs-studio-" + obsNewName;



			if (!fs::exists(newPath)) {
				fs::create_directory(newPath);
				std::cout << "--> obs-studio-" << obsNewName << " created in %AppData% [1/3]" << std::endl;

				fs::copy_file(curPath + "\\global.ini", newPath + "\\global.ini", fs::copy_options::overwrite_existing | fs::copy_options::recursive);	// Copy ./global.ini
				fs::copy(curPath + "\\basic", newPath + "\\basic", fs::copy_options::overwrite_existing | fs::copy_options::recursive);					// Copy ./basic/*
				std::cout << "--> Current settings copied into new directory [2/3]" << std::endl;

				std::ofstream file(newPath + "\\name.obsinstance");
				file << obsNewName;
				file.close();
				std::cout << "--> Instance finalised [3/3]" << std::endl;
			}
			else {
				std::cout << "The requested OBS instance already exists (" << obsNewName << ")" << std::endl;
				std::cout << "You can use \"remove " << obsNewName << "\" to remove it." << std::endl;
			}
		}
		// --------------------
		// REMOVE COMMAND
		// --------------------
		else if (command.find("remove") != string::npos) {
			std::string obsRemoveName = command.erase(0, 7); // Remove "switch "
			std::replace(obsRemoveName.begin(), obsRemoveName.end(), ' ', '-'); // Replace spaces in name with hypens
			std::string obsRemovePath = path + "\\" + "obs-studio-" + obsRemoveName;


			if (fs::exists(obsRemovePath)) {
				std::cout << "Are you sure you want to delete " << obsRemoveName << " permanently?" << std::endl;
				char response;
				do
				{
					std::cout << "Are you sure? [y/n]: ";
					std::cin >> response;
					std::cout << std::endl;
					tolower(response);
				} while (!std::cin.fail() && response != 'y' && response != 'n');
				if (response == 'y') {
					std::cout << "Removing " << obsRemoveName << "..." << std::endl;
					std::string execCommand = "rmdir /S/Q \"" + obsRemovePath + "\"";
					system(execCommand.c_str());
					std::cout << "Removed!" << std::endl;
				}
				else {
					std::cout << "Cancelled." << std::endl;
				}
			} else
				std::cout << obsRemoveName << " was not found!" << std::endl;


		}
		// --------------------
		// SWITCH COMMAND
		// --------------------
		else if (command.find("switch") != string::npos) {
		if (OBSRunning()) {
			std::cout << "Please close OBS before switching to prevent corruption!" << std::endl;
		}
		else {
			std::string obsWantedName = command.erase(0, 7); // Remove "switch "
			std::replace(obsWantedName.begin(), obsWantedName.end(), ' ', '-'); // Replace spaces in name with hypens

			// Get current OBS name
			std::string curName;
			std::ifstream fl(curPath + "\\name.obsinstance");
			std::stringstream cNbuffer;
			cNbuffer << fl.rdbuf();
			curName = cNbuffer.str();
			fl.close();

			if (curName.length() < 1)
			{
				std::cout << "----------------------------------------" << std::endl;
				std::cout << "--    Active profile has no name!     --" << std::endl;
				std::cout << "-- Use 'rename <name>' to give it one --" << std::endl;
				std::cout << "----------------------------------------" << std::endl;
			}
			else {
				std::string wantedOBSPath = path + "\\" + "obs-studio-" + obsWantedName;
				std::string newOBSPath = path + "\\" + "obs-studio-" + curName;
				//CONTINUE ONLY IF WANTED OBS EXISTS!
				if (fs::exists(wantedOBSPath)) {
					std::cout << "Switching from: " << curName << std::endl;
					std::cout << "To: " << obsWantedName << std::endl << std::endl;

					std::cout << "Unmounting current instance [1/2]" << std::endl;

					bool moveComplete = false;

					while (!moveComplete) {

						if (!std::filesystem::exists(newOBSPath)) {
							// MOVING OLD OBS FROM FOLDER
							std::filesystem::create_directory(newOBSPath);
							std::cout << "--> obs-studio-" << curName << " created in %AppData% [1/3]" << std::endl;

							std::string ACTIVEglobal = curPath + "\\global.ini";
							std::string ACTIVEinstance = curPath + "\\name.obsinstance";
							std::string ACTIVEbasic = curPath + "\\basic";

							std::filesystem::copy_file(ACTIVEglobal, newOBSPath + "\\global.ini", std::filesystem::copy_options::overwrite_existing);						// Copy ./global.ini
							std::filesystem::copy_file(ACTIVEinstance, newOBSPath + "\\name.obsinstance", std::filesystem::copy_options::overwrite_existing);				// Copy ./name.obsinstance
							std::filesystem::copy(ACTIVEbasic, newOBSPath + "\\basic", fs::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);	// Copy ./basic/*
							std::cout << "--> Current settings copied into new directory [2/3]" << std::endl;

							std::filesystem::remove(ACTIVEglobal);					// Remove ./global.ini
							std::filesystem::remove(ACTIVEinstance);					// Remove ./name.obsinstance
							std::string basicRemoveCommand = "rmdir /S/Q \"" + ACTIVEbasic + "\"";
							system(basicRemoveCommand.c_str());										// Remove ./basic/* and ./basic itself
							std::cout << "--> Removed old settings from active OBS instance [3/3]" << std::endl;


							std::cout << "Mounting wanted instance [2/2]" << std::endl;


							// MOVING NEW OBS TO FOLDER
							fs::copy_file(wantedOBSPath + "\\global.ini", curPath + "\\global.ini", fs::copy_options::overwrite_existing);				// Copy ./global.ini
							fs::copy_file(wantedOBSPath + "\\name.obsinstance", curPath + "\\name.obsinstance", fs::copy_options::overwrite_existing);	// Copy ./name.obsinstance
							fs::copy(wantedOBSPath + "\\basic", curPath + "\\basic", fs::copy_options::overwrite_existing | fs::copy_options::recursive);	// Copy ./basic/*
							std::cout << "--> Settings copied from wanted OBS instance [1/2]" << std::endl;

							std::string execCommand = "rmdir /S/Q \"" + wantedOBSPath + "\"";
							system(execCommand.c_str());
							std::cout << "--> Wanted OBS instance folder removed [2/2]" << std::endl;


							std::cout << "Switching complete!" << std::endl;

							moveComplete = true;
						}
						else
						{
							std::string execCommand = "rmdir /S/Q \"" + newOBSPath + "\"";
							system(execCommand.c_str());
						}
					}
				}
				else
					std::cout << "Requested OBS instance could not be found! Use 'list' to see existing OBS instances" << std::endl;
			}
		}
		}
		// --------------------
		// INST COMMAND
		// --------------------
		else if (command.find("inst") != string::npos) {
			OBSPath = command.substr(command.find(" ")+1, command.length());
			std::cout << "Set OBS Path to: " << OBSPath << std::endl;
			saveConfig();
		}


		// --------------------
		// SHORTCUT COMMAND *WIP*
		// --------------------
		//else if (command.find("shortcut") != string::npos) {
		//	CoInitialize(NULL);
		//	std::cout << argv[0];
		//	std::basic_string<TCHAR> fn = TEXT("OBS Instance Manager");
		//	std::basic_string<TCHAR> fd = TEXT("TechNobo's OBS Instance Manager");
		//	std::cout << CreateShortcut(/*fn.c_str()*/CA2W("OBS Instance Manager"), CA2W("TechNobo's OBS Instance Manager"), CA2W(argv[0]), NULL, 1);
		//}
	}
    return 0;
}


DWORD FindProcessId(const std::wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

const char * WinGetEnv(const char * name)
{
	const DWORD buffSize = 65535;
	static char buffer[buffSize];
	if (GetEnvironmentVariableA(name, buffer, buffSize))
	{
		return buffer;
	}
	else
	{
		return 0;
	}
}
/**********************************************************************
* Function......: CreateShortcut
* Parameters....: lpszFileName - string that specifies a valid file name
*          lpszDesc - string that specifies a description for a
							 shortcut
*          lpszShortcutPath - string that specifies a path and
									 file name of a shortcut
* Returns.......: S_OK on success, error code on failure
* Description...: Creates a Shell link object (shortcut)
**********************************************************************/
HRESULT CreateShortcut(/*in*/ LPCTSTR lpszFileName,
	/*in*/ LPCTSTR lpszDesc,
	/*in*/ LPCTSTR lpszShortcutPath,
	/*in*/ LPCTSTR lpszArguments,
	/*in*/ int iShowCmd
)
{
	//::CoInitialize(NULL);
	HRESULT hRes = E_FAIL;
	DWORD dwRet = 0;

	ATL::CComPtr<IShellLink> ipShellLink;
	// buffer that receives the null-terminated string

	// for the drive and path

	TCHAR szPath[MAX_PATH];
	// buffer that receives the address of the final

	//file name component in the path

	LPTSTR lpszFilePart;
	WCHAR wszTemp[MAX_PATH];

	// Retrieve the full path and file name of a specified file

	dwRet = GetFullPathName(lpszFileName,
		sizeof(szPath) / sizeof(TCHAR),
		szPath, &lpszFilePart);
	if (!dwRet)
		return hRes;

	// Get a pointer to the IShellLink interface

	hRes = CoCreateInstance(CLSID_ShellLink,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IShellLink,
		(void**)& ipShellLink);

	if (SUCCEEDED(hRes))
	{
		// Get a pointer to the IPersistFile interface

		CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

		// Set the path to the shortcut target and add the description

		hRes = ipShellLink->SetPath(szPath);
		if (FAILED(hRes))
			return hRes;

		hRes = ipShellLink->SetDescription(lpszDesc);
		if (FAILED(hRes)) {
			return hRes;
		}


		hRes = ipShellLink->SetArguments(lpszArguments);
		if (FAILED(hRes)) {
			return hRes;
		}

		hRes = ipShellLink->SetShowCmd(iShowCmd);
		if (FAILED(hRes)) {
			return hRes;
		}

		// IPersistFile is using LPCOLESTR, so make sure

		// that the string is Unicode

#if !defined _UNICODE
		MultiByteToWideChar(CP_ACP, 0,
			lpszShortcutPath, -1, wszTemp, MAX_PATH);
#else
		wcsncpy_s(wszTemp, lpszShortcutPath, MAX_PATH);
#endif

		// Write the shortcut to disk

		hRes = ipPersistFile->Save(wszTemp, TRUE);
	}
	//::CoUninitialize();
	return hRes;
}