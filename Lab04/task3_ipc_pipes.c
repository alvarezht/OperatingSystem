
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define BUFFER_SIZE 256

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        HANDLE hReadPipe = (HANDLE)_atoi64(argv[2]);
        char buffer[BUFFER_SIZE];
        DWORD bytesRead;
        
        if (ReadFile(hReadPipe, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0';
            printf("Child Process: Received \"%s\"\n", buffer);
        } else {
            printf("Child Process: Failed to read from pipe (%lu)\n", GetLastError());
        }
        
        CloseHandle(hReadPipe);

        Sleep(5000);
        
        return 0;
    }
    
    printf("Parent Process: PID=%lu\n", GetCurrentProcessId());
    
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        printf("CreatePipe failed (%lu)\n", GetLastError());
        return 1;
    }
    SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    char cmdLine[256];
    sprintf(cmdLine, "%s child %lld", argv[0], (long long)(intptr_t)hReadPipe);
    
    if (!CreateProcess(
        NULL, 
        cmdLine, 
        NULL,
        NULL,
        TRUE, 
        0, 
        NULL, 
        NULL,
        &si,
        &pi) 
    ) {
        printf("CreateProcess failed (%lu)\n", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 1;
    }
    
    const char *message = "Hello from Parent";
    DWORD bytesWritten;
    
    printf("Parent Process: Writing \"%s\"\n", message);
    
    if (!WriteFile(hWritePipe, message, strlen(message), &bytesWritten, NULL)) {
        printf("WriteFile failed (%lu)\n", GetLastError());
    }
    
    CloseHandle(hWritePipe);
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return 0;
}
