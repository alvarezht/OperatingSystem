# Lab08 - Simulador de Scheduling de CPU

Este proyecto implementa una simulacion de planificacion de CPU en C para comparar cuatro algoritmos:

- FIFO (First In, First Out)
- Round Robin (RR)
- SJF (Shortest Job First, no expropiativo)
- SRTF (Shortest Remaining Time First, expropiativo)

El programa ejecuta todos los algoritmos sobre el mismo conjunto de procesos para comparar de forma justa:

- tiempos de espera (waiting time)
- tiempos de retorno (turnaround time)
- promedios de cada metrica
- linea de tiempo tipo Gantt en ASCII

## Estructura del proyecto

- `main.c`
- `build_run.bat`
- `include/`
  - `process.h`
  - `dataset.h`
  - `scheduler.h`
  - `report.h`
- `src/`
  - `process.c`
  - `dataset.c`
  - `scheduler.c`
  - `report.c`

## Explicacion por archivo

### `main.c`
Archivo principal del programa.

Responsabilidades:

1. Construye el dataset base con `build_dataset`.
2. Imprime el dataset compartido.
3. Ejecuta cada algoritmo sobre una copia limpia del dataset:
   - FIFO
   - RR (quantum = 2)
   - SJF
   - SRTF
4. Imprime por algoritmo:
   - Gantt chart
   - estadisticas por proceso y promedios

Puntos importantes:

- Usa `run_one_algorithm(...)` para evitar duplicar codigo.
- Clona el dataset con `copy_process_set(...)` y reinicia metricas con `reset_process_metrics(...)` antes de cada corrida.
- Permite cambiar entre dataset fijo y aleatorio con `DATASET_FIXED` o `DATASET_RANDOM`.

---

### `build_run.bat`
Script de compilacion y ejecucion para Windows.

Responsabilidades:

1. Crea la carpeta `build` si no existe.
2. Detecta compilador disponible en este orden:
   - `gcc`
   - `clang`
   - `cl` (MSVC)
3. Compila todos los modulos `.c`.
4. Ejecuta `build\lab008_scheduling.exe`.

Si no encuentra compilador en PATH, muestra un error y termina con codigo 1.

---


## Carpeta `include/` (interfaces)

### `include/process.h`
Define los tipos de datos base del proyecto.

- `MAX_PROCESSES`: limite maximo de procesos.
- `Process`: representa un proceso con:
  - `id`, `arrival_time`, `burst_time`
  - `remaining_time`
  - `start_time`, `completion_time`
  - `waiting_time`, `turnaround_time`
- `ProcessSet`: arreglo de procesos + contador.

Declara utilidades:

- `copy_process_set(...)`
- `reset_process_metrics(...)`

---

### `include/dataset.h`
Interfaz para crear datasets de procesos.

- `DatasetMode`:
  - `DATASET_FIXED`
  - `DATASET_RANDOM`
- `build_dataset(...)`: construye el conjunto de procesos segun el modo.

---

### `include/scheduler.h`
Interfaz de los algoritmos de planificacion.

- `MAX_TIMELINE`: tamano maximo de la linea de tiempo.
- `ScheduleResult`: guarda la secuencia de ejecucion (`timeline`) y su longitud.

Declara:

- `run_fifo(...)`
- `run_rr(...)`
- `run_sjf(...)`
- `run_srtf(...)`

---

### `include/report.h`
Interfaz para salida por consola.

Declara funciones para mostrar:

- `print_dataset(...)`
- `print_stats(...)`
- `print_gantt(...)`

---

## Carpeta `src/` (implementaciones)

### `src/process.c`
Implementa utilidades de manejo de procesos.

- `copy_process_set(...)`: copia todos los procesos de un conjunto a otro.
- `reset_process_metrics(...)`: reinicia tiempos de simulacion para reutilizar el mismo dataset en distintos algoritmos.

---

### `src/dataset.c`
Implementa la construccion del dataset.

Funciones internas:

- `fill_fixed_dataset(...)`: crea 6 procesos con datos fijos.
- `fill_random_dataset(...)`: crea entre 5 y 15 procesos (acotado por `MAX_PROCESSES`) con:
  - llegadas aleatorias entre 0 y 20
  - burst aleatorio entre 1 y 10

Funcion publica:

- `build_dataset(...)`: selecciona modo fijo o aleatorio.

---

### `src/report.c`
Implementa la visualizacion de resultados.

- `print_dataset(...)`: imprime tabla de procesos (ID, llegada, burst).
- `print_stats(...)`: imprime waiting/turnaround por proceso y promedio.
- `print_gantt(...)`: dibuja la ejecucion en formato ASCII, incluyendo `IDLE` cuando no hay proceso listo.

---

### `src/scheduler.c`
Implementa la logica de planificacion.

Componentes auxiliares:

- Cola simple (`Queue`) para Round Robin.
- `clear_result(...)` y `push_timeline(...)` para construir el Gantt.
- `compute_metrics(...)` para calcular:
  - `turnaround_time = completion_time - arrival_time`
  - `waiting_time = turnaround_time - burst_time`
- `print_arrivals(...)` para registrar eventos de llegada.
- `all_completed(...)` para verificar fin en SRTF.

Algoritmos:

1. `run_fifo(...)`
   - No expropiativo.
   - Atiende por orden de llegada (desempate por ID).

2. `run_rr(...)`
   - Expropiativo por quantum.
   - Usa cola FIFO de listos y reencola procesos no terminados.

3. `run_sjf(...)`
   - No expropiativo.
   - Elige el burst mas corto entre los procesos disponibles.

4. `run_srtf(...)`
   - Expropiativo.
   - En cada unidad de tiempo elige menor tiempo restante.

Durante la simulacion se imprimen eventos:

- llegada
- inicio/reanudacion
- expropiacion (cuando aplica)
- finalizacion

## Flujo general de ejecucion

1. Se crea dataset base (fijo o aleatorio).
2. Se imprime dataset compartido.
3. Para cada algoritmo:
   - se copia dataset
   - se reinician metricas
   - se simula
   - se imprime Gantt
   - se imprimen estadisticas

Esto asegura una comparacion justa entre algoritmos.

## Importante

- El proyecto no usa hilos reales del sistema operativo modela la planificacion de forma discreta en unidades de tiempo.
- El Gantt se guarda como IDs por unidad de tiempo dentro de `ScheduleResult.timeline`.
- Si `MAX_TIMELINE` se queda corto para un caso grande, la salida de timeline se truncaria al limite definido.
