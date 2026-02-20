#include <windows.h>
#include <stdio.h>
#include <tchar.h>

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        DWORD childPID = GetCurrentProcessId();
        DWORD parentPID = atoi(argv[2]);
        
        printf("Child Process: PID=%lu, Parent PID=%lu\n", childPID, parentPID);
        Sleep(5000);
        
        return 0;
    }
    
    DWORD parentPID = GetCurrentProcessId();
    printf("Parent Process: PID=%lu\n", parentPID);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    char cmdLine[256];
    sprintf(cmdLine, "%s child %lu", argv[0], parentPID);
    
    if (!CreateProcess(
        NULL, 
        cmdLine, 
        NULL, 
        NULL,  
        FALSE, 
        0,  
        NULL,
        NULL,
        &si,
        &pi)
    ) {
        printf("CreateProcess failed (%lu).\n", GetLastError());
        return 1;
    }
        
    WaitForSingleObject(pi.hProcess, INFINITE);

    printf("Parent Process: Child has finished execution.\n");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return 0;
}