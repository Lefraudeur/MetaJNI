#include "suspend_threads.hpp"
#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>	
#include <vector>

std::vector<HANDLE> opened_threads{};

bool suspend_threads()
{
	if (!opened_threads.empty()) return false;

	DWORD threadID = GetThreadId(GetCurrentThread());
	DWORD currentProcessId = GetCurrentProcessId();

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);


	THREADENTRY32 entry{};
	entry.dwSize = sizeof(THREADENTRY32);


	for (BOOL res = Thread32First(snapshot, &entry); res; res = Thread32Next(snapshot, &entry))
	{
		if (entry.th32OwnerProcessID != currentProcessId || entry.th32ThreadID == threadID) continue;

		HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);

		if (!thread) continue;
		SuspendThread(thread);
		opened_threads.push_back(thread);
	}

	CloseHandle(snapshot);

	return true;
}

void resume_threads()
{
	for (HANDLE thread : opened_threads)
	{
		ResumeThread(thread);
		CloseHandle(thread);
	}
	opened_threads.clear();
}
