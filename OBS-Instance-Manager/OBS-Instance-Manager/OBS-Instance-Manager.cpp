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
using std::string;
using std::vector;

//  Forward declarations:
DWORD FindProcessId(const std::wstring& processName);
int getdir(string dir, vector<string> &files);

int main()
{
	if (FindProcessId(L"obs64.exe") || FindProcessId(L"obs.exe"))
	{
		std::cout << "OBS is already running!\n";
		std::cout << "Continuing with the process may cause issues.\n";	
	}
	std::cout << "------------\n";
	std::cout << "Commands: \n";
	std::cout << "list -- Lists existing obs-studio instances.\n";
	std::cout << "add <name> -- Copies obs-studio settings to a new instance.\n";
	std::cout << "remove <name> -- Removes instance settings.\n";
	std::cout << "switch <name> -- Switches to given obs-studio instance.\n";


	// Files that differ between instances:
	// ./basic/*
	// ./global.ini

	string command;
	std::cout << "Enter a command: ";
	std::cin >> command;

	if (command.find("list") != string::npos) {
		string dir = string(getenv("APPDATA"));
		vector<string> files = vector<string>();

		getdir(dir, files);
		for (unsigned int i = 0; i < files.size(); i++)
			cout << files[i] << endl;
	}


	std::cout << command;


	system("pause");
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

int getdir(string dir, vector<string> &files)
{
	DIR *dp;
	struct dirent *dirp;

	if ((dp = opendir(dir.c_str())) == NULL)
	{
		cout << "Error(" << errno << ") opening " << dir << endl;
		return errno;
	}
	while ((dirp = readdir(dp)) != NULL)
		files.push_back(string(dirp->d_name));
	closedir(dp);
	return 0;
}