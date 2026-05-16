# Lab 009 — Virtual Address Translation and Page Tables

## Qué es este proyecto

Este es un laboratorio de **Sistemas Operativos** que simula, completamente en **espacio de usuario**, la traducción de direcciones virtuales a físicas mediante **tablas de páginas lineales** y un **allocator de frames** sobre una RAM simulada de 100 frames.

No se ejecuta código privilegiado ni se interactúa con hardware real. El objetivo es entender **cómo funciona la MMU por dentro**:

- Cómo se descompone una dirección virtual en **VPN** y **offset**.
- Cómo una **tabla de páginas** mapea páginas virtuales a **frames físicos**.
- Cómo un **allocator** gestiona un pool finito de frames.
- Cómo se detectan errores de acceso: dirección fuera de rango, VPN inválida o página no mapeada.

---

## Conceptos clave

| Término | Descripción |
|---|---|
| **Dirección virtual (VA)** | Dirección que el proceso ve. En este lab ocupa 16 bits. |
| **Dirección física (PA)** | Dirección real en la RAM simulada. |
| **VPN** | *Virtual Page Number*: índice de la página dentro del espacio virtual del proceso. |
| **Offset** | Desplazamiento dentro de la página (o frame). |
| **Página** | Bloque lógico del espacio virtual del proceso. |
| **Frame** | Bloque físico en RAM. |
| **PTE** | *Page Table Entry*: contiene bit de validez y el PFN asignado. |
| **Tabla de páginas** | Array lineal de PTEs, uno por cada página virtual del proceso. |
| **Allocator** | Función que escanea la RAM y devuelve un frame libre, marcándolo como ocupado. |

---

## Formato de dirección virtual del lab

El enunciado define una dirección virtual de **16 bits** con el siguiente layout:

```text
 15                              8 7                               0
+--------------------------------+--------------------------------+
|           VPN (8 bits)         |          Offset (8 bits)       |
+--------------------------------+--------------------------------+
```

- **Offset** = `VA & 0xFF` (bits `[7:0]`)
- **VPN** = `(VA >> 8) & 0xFF` (bits `[15:8]`)
- **Tamaño de página / frame** = `256` bytes (`2^8`)

Ejemplos:

| VA (hex) | VA (dec) | VPN | Offset |
|---|---|---|---|
| `0x0000` | 0 | 0 | 0 |
| `0x00FF` | 255 | 0 | 255 |
| `0x0100` | 256 | 1 | 0 |
| `0x0F00` | 3840 | 15 | 0 |

---

## Arquitectura del código

```text
+---------------+      +------------------+      +-----------------+
|  Virtual Addr |----->|   Page Table     |----->|  Physical RAM   |
|   (16 bits)   |      | (Linear array)   |      |  (100 frames)   |
+---------------+      +------------------+      +-----------------+
        |                     |                         |
    [VPN | Offset]        [PTE: valid, PFN]        [Frame: state, owner]
```

### Estructuras principales

- `Frame`: representa un frame físico. Campos: `state` (`FREE` / `OCCUPIED`), `owner` (`-1`=libre, `0`=sistema, `1`=proceso 1, `2`=proceso 2).
- `PTE` (*Page Table Entry*): `valid` (bool), `pfn` (índice del frame).
- `PageTable`: array dinámico de `PTE`, `num_pages`, `pid`.

### Funciones principales

| Función | Rol |
|---|---|
| `init_ram(...)` | Inicializa 100 frames con una ocupación aleatoria en `[10, 60]` y garantiza suficientes libres. |
| `print_ram_map(...)` | Dibuja el mapa F/X en 2D (10 por fila) con colores ANSI. |
| `print_ownership_map()` | Dibuja el mapa de propiedad F/S/1/2 con colores. |
| `allocate_frame(owner)` | Busca linealmente un frame libre y lo marca. |
| `load_process(pt, V, pid)` | Reserva `V` frames y llena la tabla de páginas. Hace rollback si falla a mitad de carga. |
| `translate(pt, VA, ...)` | Extrae VPN y offset, valida rangos, consulta PTE y calcula `PA = PFN * 256 + offset`. |
| `print_translation_result(...)` | Imprime una línea formateada por cada dirección traducida. |

---

## Cómo compilar y ejecutar

### Linux / macOS (GCC o Clang)

```bash
make
./vat_sim
```

Con argumentos:

```bash
./vat_sim -s 12345 -v 8 -f addresses.txt
```

O usando argumentos posicionales:

```bash
./vat_sim 8 addresses.txt
```

### Windows (cmd / PowerShell)

Con `build.bat`:

```cmd
build.bat
vat_sim.exe
```

O manualmente con GCC:

```cmd
gcc -Wall -Wextra -std=c11 -o vat_sim.exe main.c
vat_sim.exe -s 12345 -v 8 -f addresses.txt
```

> **Nota**: los códigos ANSI de color funcionan en **Windows Terminal moderno**, VS Code terminal, y terminales con soporte VT. No se requiere `windows.h`.

### Argumentos CLI

| Opción | Descripción | Default |
|---|---|---|
| `-s <seed>` o `seed=<seed>` | Semilla para reproducir el patrón de RAM. | `time(NULL)` |
| `-v <V>` o primer argumento posicional | Número de páginas virtuales del proceso 1 (`1..90`). | `8` |
| `-f <file>` o segundo argumento posicional | Ruta al archivo de direcciones virtuales. | `addresses.txt` |

---

## Flujo de ejecución paso a paso

1. **Parseo de CLI**: se leen seed, `V` y archivo de entrada.
2. **Validación**: `V` debe estar entre 1 y 256, no puede superar `NUM_FRAMES` (100), y no puede superar 90 porque la RAM siempre reserva al menos 10 frames ocupados.
3. **Inicialización de RAM**: se barajan los 100 frames y se marcan entre 10 y 60 como `OCCUPIED` (sistema). Se repite hasta que `FREE >= max(10, V)` (tope de 1000 iteraciones).
4. **Impresión del mapa RAM**: se muestra una grilla 10x10 con `F` (verde) y `X` (rojo).
5. **Carga del Proceso 1**: se asignan `V` frames libres y se construye su tabla de páginas.
6. **Cálculo de V2**: `V2 = min(V, remaining_free / 2)`. Esto garantiza que el segundo proceso no agote la memoria.
7. **Carga del Proceso 2**: se asignan `V2` frames distintos del pool compartido.
8. **Mapa de propiedad**: se imprime una segunda grilla mostrando quién posee cada frame: `F`, `S`, `1`, `2`.
9. **Resumen de carga**: se listan los PFNs asignados a cada proceso.
10. **Traducción batch**: se lee `addresses.txt`, se traduce cada dirección para el **Proceso 1**, y se imprime éxito o error.
11. **Limpieza**: se liberan tablas de páginas y frames al salir.

---

## Ejemplo de salida esperada

A continuación una salida **simulada** con `seed=12345` y `V=8`. Los PFNs cambian en cada ejecución; lo importante es la **consistencia matemática**.

```text
PHYSICAL RAM (100 frames) after random init (seed=12345):
FREE=82 OCCUPIED=18

  0:X   1:F   2:F   3:X   4:F   5:F   6:F   7:F   8:F   9:F
 10:F  11:X  12:F  13:F  14:F  15:F  16:F  17:F  18:F  19:F
 ... (todas las filas 0..99) ...

Process 2 sizing: remaining_free=82, V=8 -> V2=min(V, remaining_free/2)=8

FRAME OWNERSHIP MAP (100 frames):
  0:S   1:1   2:1   3:S   4:1   5:1   6:1   7:1   8:1   9:1
 10:1  11:S  12:1  13:1  14:2   ...

Legend: F=Free  S=System  1=Process1  2=Process2
Counts: FREE=66 SYSTEM=18 PROC1=8 PROC2=8

Load process 1 (PID=1): V=8 -> VPN 0..7 mapped to PFNs [1 2 4 5 6 7 8 9]
Load process 2 (PID=2): V=8 -> VPN 0..7 mapped to PFNs [10 12 13 14 15 16 17 18]

Translating addresses for Process 1 (PID=1) from file 'addresses.txt':
VA=0x0000 (0)     VPN=0x00 OFF=0x00  PFN=1    PA=0x0100 (256)
VA=0x00FF (255)   VPN=0x00 OFF=0xFF  PFN=1    PA=0x01FF (511)
VA=0x0100 (256)   VPN=0x01 OFF=0x00  PFN=2    PA=0x0200 (512)
VA=0x0200 (512)   VPN=0x02 OFF=0x00  PFN=4    PA=0x0400 (1024)
VA=0x0F00 (3840)  ERROR=VPN_OUT_OF_RANGE (vpn=15, V=8)
VA=70000          ERROR=VA_OUT_OF_RANGE
```

> Los valores de PFN y PA dependen del orden en que `allocate_frame()` recorre la RAM. Lo que se verifica es que `PA = PFN * 256 + offset` sea correcto y que los errores coincidan con las reglas del enunciado.

---

## Explicación de los extras

### 1. Visualización 2D a color de los 100 frames

Se utilizan **códigos ANSI de color** para distinguir estados sin depender únicamente de letras:

- **Verde** (`\x1b[32m`) → Frame libre.
- **Rojo** (`\x1b[31m`) → Frame ocupado por el sistema (holes iniciales).
- **Azul** (`\x1b[34m`) → Frame asignado al **Proceso 1**.
- **Amarillo** (`\x1b[33m`) → Frame asignado al **Proceso 2**.

El mapa se presenta en bloques de **10 frames por fila**, facilitando la correlación visual con una matriz 10×10.

### 2. Segundo proceso compartiendo el mismo pool de RAM

El código soporta **dos tablas de páginas** que consumen frames del **mismo pool global** de 100 frames:

- El **Proceso 1** carga `V` páginas.
- El **Proceso 2** carga `V2 = min(V, remaining_free / 2)` páginas. Esta fórmula documentada asegura que:
  - No se exceda la memoria disponible.
  - Quede margen para futuras asignaciones.
  - Se demuestre explícitamente la propiedad compartida del recurso físico.

El mapa de propiedad (`S`, `1`, `2`, `F`) deja claro quién es dueño de cada frame, lo cual es esencial para entender **multiprogramación con memoria compartida**.

--
