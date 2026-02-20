#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define NUM_CHILDREN 3

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0) {
        DWORD childPID = GetCurrentProcessId();
        DWORD parentPID = atoi(argv[2]);
        int childNum = atoi(argv[3]);
        
        printf("Child %d: PID=%lu, Parent PID=%lu\n", childNum, childPID, parentPID);
        Sleep(5000);
        
        return 0;
    }
    
    DWORD parentPID = GetCurrentProcessId();
    printf("Parent Process: PID=%lu\n", parentPID);
    
    PROCESS_INFORMATION pi[NUM_CHILDREN];
    STARTUPINFO si[NUM_CHILDREN];
    
    for (int i = 0; i < NUM_CHILDREN; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        ZeroMemory(&pi[i], sizeof(pi[i]));
        
        char cmdLine[256];
        sprintf(cmdLine, "%s child %lu %d", argv[0], parentPID, i + 1);
        
        if (!CreateProcess(
            NULL,     
            cmdLine,   
            NULL,     
            NULL,     
            FALSE,   
            0,      
            NULL,     
            NULL,  
            &si[i], 
            &pi[i])
        ) {
            printf("CreateProcess failed for child %d (%lu)\n", i + 1, GetLastError());
            continue;
        }
        
        printf("Parent Process: Created child %d\n", i + 1);
    }
    
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pi[i].hProcess != NULL) {
            WaitForSingleObject(pi[i].hProcess, INFINITE);
            CloseHandle(pi[i].hProcess);
            CloseHandle(pi[i].hThread);
        }
    }
    
    printf("Parent Process: All children have finished execution.\n");
    
    return 0;
}
