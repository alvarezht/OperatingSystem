#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>

#define SHARED_MEMORY_NAME "Local\\MySharedMemory"
#define BUFFER_SIZE 256

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        printf("Child Process: PID=%lu\n", GetCurrentProcessId());
        
        Sleep(500);
        
        HANDLE hMapFile = OpenFileMapping(
            FILE_MAP_READ,  
            FALSE,          
            SHARED_MEMORY_NAME);
        
        if (hMapFile == NULL) {
            printf("Child Process: Could not open file mapping object (%lu)\n", GetLastError());
            return 1;
        }
        
        LPVOID pBuf = MapViewOfFile(
            hMapFile,           
            FILE_MAP_READ,      
            0,                  
            0,                  
            BUFFER_SIZE);               
        if (pBuf == NULL) {
            printf("Child Process: Could not map view of file (%lu)\n", GetLastError());
            CloseHandle(hMapFile);
            return 1;
        }
        
        char buffer[BUFFER_SIZE];
        CopyMemory(buffer, pBuf, BUFFER_SIZE);
        printf("Child Process: Read \"%s\"\n", buffer);
        
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        
        return 0;
    }
    
    printf("Parent Process: PID=%lu\n", GetCurrentProcessId());
    
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, 
        NULL,                 
        PAGE_READWRITE,       
        0,                    
        BUFFER_SIZE,          
        SHARED_MEMORY_NAME);  

    if (hMapFile == NULL) {
        printf("Parent Process: Could not create file mapping object (%lu)\n", GetLastError());
        return 1;
    }
    
    LPVOID pBuf = MapViewOfFile(
        hMapFile,       
        FILE_MAP_ALL_ACCESS,
        0,              
        0,              
        BUFFER_SIZE);       
    
    if (pBuf == NULL) {
        printf("Parent Process: Could not map view of file (%lu)\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }
    
    const char *message = "Shared Memory Example";
    printf("Parent Process: Writing \"%s\"\n", message);
    CopyMemory(pBuf, message, strlen(message) + 1);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    char cmdLine[256];
    sprintf(cmdLine, "%s child", argv[0]);
    
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
        printf("CreateProcess failed (%lu)\n", GetLastError());
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return 1;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    printf("Parent Process: Child has finished execution.\n");
    
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return 0;
}
