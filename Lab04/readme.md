# Task 1 y 2 - Creación de Procesos

El programa se ejecuta en dos modos dependiendo de los argumentos:

- **Modo Padre**: Sin argumentos ****- crea el proceso hijo
- **Modo Hijo**: Con argumentos `"child"` y el PID del padre

```c
if (argc > 1 && strcmp(argv[1], "child") == 0) {
    // Código del hijo
} else {
    // Código del padre
}
```

El hijo recibe:
- `argv[0]` = nombre del programa
- `argv[1]` = "child"
- `argv[2]` = PID del padre (como string)

## Generación del String para el Nuevo Proceso

El padre construye la línea de comando usando `sprintf`:

```c
char cmdLine[256];
sprintf(cmdLine, "%s child %lu", argv[0], parentPID);
// Resultado: "task1_process_creation.exe child 1234"
```

Este string se pasa a `CreateProcess` y se convierte en `argc` y `argv[]` del proceso hijo.

## WaitForSingleObject

```c
WaitForSingleObject(pi.hProcess, INFINITE);
```

**Para qué sirve:**
- Bloquea el padre hasta que el hijo termine
- Sin esto, el padre terminaría inmediatamente y el hijo quedaría huérfano
- `INFINITE` significa esperar indefinidamente hasta que el proceso hijo termine

## CloseHandle

```c
CloseHandle(pi.hProcess);
CloseHandle(pi.hThread);
```

**Para qué sirve:**
- Libera las referencias al proceso y thread hijo
- Previene fugas de memoria del sistema operativo
- Permite a Windows limpiar completamente los recursos del proceso terminado
- Cada handle consume recursos del kernel, por eso deben cerrarse cuando ya no se necesitan

---

# Task 3 - Comunicación entre Procesos con Pipes


Similar a Task 1, pero el hijo recibe el **handle del pipe** como argumento:

```c
if (argc > 1 && strcmp(argv[1], "child") == 0) {
    HANDLE hReadPipe = (HANDLE)_atoi64(argv[2]);  // Convierte string a handle
    // Código del hijo
}
```

El hijo recibe:
- `argv[0]` = nombre del programa
- `argv[1]` = "child"
- `argv[2]` = Handle del pipe de lectura (como string)

## Generación del String para el Nuevo Proceso

```c
sprintf(cmdLine, "%s child %lld", argv[0], (long long)(intptr_t)hReadPipe);
// Resultado: "task3_ipc_pipes.exe child 140737488355328"
```

El handle se convierte a número y se pasa como string.

## Handles

Un handle es un número que identifica un recurso del sistema ya sea pipe, archivo, proceso u otro. Es como una llave para acceder al recurso.

```c
HANDLE hReadPipe, hWritePipe;
```

- `hReadPipe`: Handle para leer datos del pipe
- `hWritePipe`: Handle para escribir datos al pipe

## SECURITY_ATTRIBUTES

```c
SECURITY_ATTRIBUTES sa;
sa.nLength = sizeof(SECURITY_ATTRIBUTES);
sa.bInheritHandle = TRUE;
sa.lpSecurityDescriptor = NULL;
```

**Para qué sirve:**
- Define la seguridad y herencia de handles
- **nLength**: Tamaño de la estructura (obligatorio)
- **bInheritHandle**: `TRUE` = el proceso hijo puede heredar este handle
- **lpSecurityDescriptor**: `NULL` = usa seguridad por defecto

Sin `bInheritHandle = TRUE`, el hijo no podría usar el pipe.

## CreatePipe

```c
CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
```

**Parámetros:**
1. `&hReadPipe`: Puntero donde se guardará el handle de lectura
2. `&hWritePipe`: Puntero donde se guardará el handle de escritura
3. `&sa`: Security atrributes que permiten herencia
4. `0`: Tamaño del buffer en este caso 0

## SetHandleInformation

```c
SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0);
```

**Para qué sirve:**
- Controla qué handles se heredan al crear el proceso hijo
- **hWritePipe**: Handle a modificar
- **HANDLE_FLAG_INHERIT**: Flag que queremos cambiar
- **0**: Para que el hijo NO tenga acceso a este handle ya que el hijo solo lee

## WriteFile

```c
WriteFile(hWritePipe, message, strlen(message), &bytesWritten, NULL);
```

**Para qué sirve:** Escribe datos en el pipe.

**Parámetros:**
1. `hWritePipe`: Handle del pipe donde escribir
2. `message`: Puntero al buffer con los datos a enviar
3. `strlen(message)`: Número de bytes a escribir
4. `&bytesWritten`: Puntero donde se guardará cuántos bytes se escribieron
5. `NULL`: Sin operación asíncrona

## ReadFile

```c
ReadFile(hReadPipe, buffer, BUFFER_SIZE, &bytesRead, NULL);
```

**Para qué sirve:** Lee datos del pipe.

**Parámetros:**
1. `hReadPipe`: Handle del pipe desde donde leer
2. `buffer`: Buffer donde guardar los datos leídos
3. `BUFFER_SIZE`: Tamaño máximo a leer (capacidad del buffer)
4. `&bytesRead`: Puntero donde se guardará cuántos bytes se leyeron
5. `NULL`: Sin operación asíncrona

## CreateProcess - Cambio Importante

```c
CreateProcess(
    NULL, 
    cmdLine, 
    NULL,
    NULL,
    TRUE,    // Ahora TRUE para heredar handles
    0, 
    NULL, 
    NULL,
    &si,
    &pi
)
```

**TRUE en bInheritHandles** permite que el hijo herede `hReadPipe`.

## WaitForSingleObject

Igual que Task 1: espera a que el hijo termine.

## CloseHandle

```c
CloseHandle(hWritePipe);   // Padre cierra escritura después de enviar
CloseHandle(hReadPipe);    // Ambos cierran lectura al final
CloseHandle(pi.hProcess);
CloseHandle(pi.hThread);
```

Cerrar `hWritePipe` después de escribir señala al lector que no habrá más datos.

---

# Task 4 - Múltiples Procesos Hijos

## Cambios Respecto a Task 1

### Arreglos de Estructuras

En Task 1 había una sola instancia:
```c
PROCESS_INFORMATION pi;
STARTUPINFO si;
```

En Task 4 son arreglos:
```c
PROCESS_INFORMATION pi[NUM_CHILDREN];  // n procesos
STARTUPINFO si[NUM_CHILDREN];          // n configuraciones
```

### Primer For - Creación de Procesos

```c
for (int i = 0; i < NUM_CHILDREN; i++) {
    ZeroMemory(&si[i], sizeof(si[i]));
    si[i].cb = sizeof(si[i]);
    ZeroMemory(&pi[i], sizeof(pi[i]));
    
    sprintf(cmdLine, "%s child %lu %d", argv[0], parentPID, i + 1);
    
    CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si[i], &pi[i]);
    
    printf("Parent Process: Created child %d\n", i + 1);
}
```

**Diferencia con Task 1:**
- Task 1: Crea **1 proceso** una sola vez
- Task 4: Itera n veces para crear **n procesos**
- Cada proceso recibe un número diferente (`i + 1`) como argumento

### Segundo For - Esperar a Todos los Hijos

En Task 1:
```c
WaitForSingleObject(pi.hProcess, INFINITE);
CloseHandle(pi.hProcess);
CloseHandle(pi.hThread);****
```

En Task 4:
```c
for (int i = 0; i < NUM_CHILDREN; i++) {
    if (pi[i].hProcess != NULL) {
        WaitForSingleObject(pi[i].hProcess, INFINITE);
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }
}
```
