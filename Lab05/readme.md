# Lab005 Threads

## Resultados

### Windows
![Resultado Windows](https://raw.githubusercontent.com/alvarezht/OperatingSystem/main/Lab05/resultado_windows.png)

### Linux
![Resultado Linux](https://raw.githubusercontent.com/alvarezht/OperatingSystem/main/Lab05/resultado_linux.png)

Este proyecto implementa un analizador completo de logs web en C con multihilo para Linux y Windows.

En este lab vamos a procesar archivos de log de texto grandes en paralelo y calcular:

1. Solicitudes agrupadas por direcciones IP únicas.
2. URL más visitada.
3. Total de errores HTTP con códigos de estado en el rango 400-599.

La implementación incluye:

- Versión para Linux usando pthreads.
- Versión para Windows usando hilos de WinAPI.
- Contadores basados en tablas hash para agregación eficiente.
- Etapa de fusión de resultados por hilo.
- Comparación de tiempos entre ejecución de 1 hilo y ejecución multihilo.

## Estructura del Proyecto Implementada

- `main_linux.c`: punto de entrada para Linux con pthreads.
- `main_windows.c`: punto de entrada para Windows con WinAPI.
- `log_processor.h`: estructuras de datos compartidas y declaraciones de funciones.
- `log_processor.c`: parseo, tablas hash, procesamiento por chunks y lógica de merge.
- `Makefile`: objetivos de compilación y ejecución para Linux.
- `run.sh`: script auxiliar para compilar y ejecutar en Linux.
- `run.bat`: script auxiliar para compilar y ejecutar en Windows.
- `access.log`: archivo de log de ejemplo/generado.
- `access_log_file_generator.py`: script auxiliar para generar logs de prueba.

## Estructuras y Funciones Principales

Esta sección describe para qué sirve cada estructura de datos y cada función principal del proyecto, incluyendo el propósito de sus argumentos.

### Struct `HashEntry`

Representa una entrada individual dentro de una tabla hash.

- `key`: clave almacenada en la entrada. En este proyecto se usa para IPs o URLs.
- `count`: cantidad acumulada asociada a esa clave.
- `next`: puntero a la siguiente entrada en la misma cubeta, usado para colisiones.

### Struct `HashMap`

Representa una tabla hash completa usada para contar ocurrencias.

- `buckets`: arreglo de listas enlazadas donde se almacenan las entradas.
- `bucket_count`: número total de cubetas disponibles.
- `size`: cantidad de claves únicas almacenadas.

### Struct `LogStats`

Agrupa todas las estadísticas calculadas durante el análisis del log.

- `ip_counts`: tabla hash con el conteo por dirección IP.
- `url_counts`: tabla hash con el conteo por URL.
- `http_errors`: total de respuestas HTTP con códigos entre 400 y 599.
- `parsed_lines`: total de líneas válidas procesadas.

### Struct `ThreadTask`

Describe el trabajo asignado a un hilo en Linux y Windows.

- `filename`: nombre del archivo de log que se va a procesar.
- `start_offset`: byte inicial del bloque asignado al hilo.
- `end_offset`: byte final del bloque asignado al hilo.
- `ok`: indicador de éxito o fallo del procesamiento del hilo.
- `stats`: estadísticas locales producidas por ese hilo.

## Funciones Compartidas (`log_processor.c` / `log_processor.h`)

### `hash_map_init(HashMap *map, size_t bucket_count)`

Inicializa una tabla hash vacía.

- `map`: estructura que se va a preparar.
- `bucket_count`: cantidad de cubetas que tendrá la tabla.

### `hash_map_free(HashMap *map)`

Libera toda la memoria asociada a una tabla hash.

- `map`: tabla hash que se desea liberar.

### `hash_map_increment(HashMap *map, const char *key, long delta)`

Incrementa el contador de una clave. Si la clave no existe, la crea.

- `map`: tabla hash donde se actualizará el valor.
- `key`: clave a incrementar, por ejemplo una IP o una URL.
- `delta`: cantidad que se sumará al contador.

### `hash_map_size(const HashMap *map)`

Devuelve cuántas claves únicas contiene la tabla hash.

- `map`: tabla hash que se desea consultar.

### `log_stats_init(LogStats *stats)`

Inicializa una estructura de estadísticas y prepara sus tablas hash internas.

- `stats`: estructura de estadísticas que se va a preparar.

### `log_stats_free(LogStats *stats)`

Libera los recursos internos de una estructura de estadísticas.

- `stats`: estructura de estadísticas a liberar.

### `process_log_chunk(const char *filename, long start_offset, long end_offset, LogStats *out_stats)`

Procesa un fragmento del archivo log comprendido entre dos offsets y acumula los resultados en una estructura local.

- `filename`: archivo que contiene el log.
- `start_offset`: byte donde empieza el fragmento asignado.
- `end_offset`: byte donde termina el fragmento asignado.
- `out_stats`: estructura donde se guardarán las estadísticas calculadas.

### `merge_log_stats(LogStats *dest, const LogStats *src)`

Fusiona estadísticas parciales dentro de una estructura final.

- `dest`: estructura destino que acumula el resultado global.
- `src`: estructura origen con resultados parciales de un hilo.

### `get_unique_ip_count(const LogStats *stats)`

Obtiene el número total de IPs únicas procesadas.

- `stats`: estructura de estadísticas que se desea consultar.

### `get_most_visited_url(const LogStats *stats, long *count)`

Busca la URL con mayor cantidad de visitas.

- `stats`: estructura de estadísticas donde se buscará la URL más visitada.
- `count`: puntero donde se devuelve la cantidad de visitas de esa URL.

## Funciones Principales de Linux (`main_linux.c`)

### `get_file_size(const char *filename)`

Obtiene el tamaño total del archivo en bytes.

- `filename`: nombre del archivo cuyo tamaño se quiere conocer.

### `print_report(const LogStats *stats, double seconds, int threads)`

Imprime el resumen final del análisis.

- `stats`: estadísticas que se mostrarán.
- `seconds`: tiempo total de ejecución medido.
- `threads`: cantidad de hilos usados en esa ejecución.

### `parse_thread_count(const char *value)`

Convierte el argumento de entrada a una cantidad válida de hilos dentro de los límites definidos.

- `value`: texto recibido desde la línea de comandos.

### `worker_run(void *arg)`

Función que ejecuta cada hilo para procesar su fragmento del archivo.

- `arg`: puntero a la estructura `ThreadTask` asociada al hilo.

### `run_analysis(const char *filename, int threads, LogStats *out_stats, double *out_seconds)`

Coordina toda la ejecución del análisis: crea hilos, espera su finalización, fusiona estadísticas y mide el tiempo.

- `filename`: archivo log a procesar.
- `threads`: cantidad de hilos a utilizar.
- `out_stats`: estructura donde se dejará el resultado final.
- `out_seconds`: puntero donde se almacenará el tiempo de ejecución.

### `main(int argc, char **argv)`

Punto de entrada del programa en Linux.

- `argc`: cantidad de argumentos recibidos.
- `argv`: arreglo con los argumentos de la línea de comandos.

## Funciones Principales de Windows (`main_windows.c`)

### `get_file_size(const char *filename)`

Obtiene el tamaño total del archivo en bytes.

- `filename`: nombre del archivo cuyo tamaño se quiere conocer.

### `print_report(const LogStats *stats, double seconds, int threads)`

Imprime el resumen final del análisis.

- `stats`: estadísticas que se mostrarán.
- `seconds`: tiempo total de ejecución medido.
- `threads`: cantidad de hilos usados en esa ejecución.

### `parse_thread_count(const char *value)`

Convierte el argumento de entrada a una cantidad válida de hilos dentro de los límites definidos.

- `value`: texto recibido desde la línea de comandos.

### `worker_run(LPVOID arg)`

Función que ejecuta cada hilo usando la convención de WinAPI.

- `arg`: puntero a la estructura `ThreadTask` asociada al hilo.

### `elapsed_seconds(LARGE_INTEGER a, LARGE_INTEGER b, LARGE_INTEGER freq)`

Convierte la diferencia entre dos lecturas del contador de alto rendimiento en segundos.

- `a`: instante inicial.
- `b`: instante final.
- `freq`: frecuencia del contador del sistema.

### `run_analysis(const char *filename, int threads, LogStats *out_stats, double *out_seconds)`

Coordina toda la ejecución del análisis: crea hilos, espera su finalización, fusiona estadísticas y mide el tiempo.

- `filename`: archivo log a procesar.
- `threads`: cantidad de hilos a utilizar.
- `out_stats`: estructura donde se dejará el resultado final.
- `out_seconds`: puntero donde se almacenará el tiempo de ejecución.

### `main(int argc, char **argv)`

Punto de entrada del programa en Windows.

- `argc`: cantidad de argumentos recibidos.
- `argv`: arreglo con los argumentos de la línea de comandos.

## Cómo Funciona

### 1) Procesamiento Paralelo por Chunks

El archivo se divide por rangos de bytes:

- Cada hilo recibe un desplazamiento inicial y final.
- Para inicios distintos de cero, el hilo salta la primera línea parcial para evitar duplicados.
- Cada hilo parsea líneas completas mientras el inicio de la línea actual esté dentro de su rango.

Esto garantiza que cada línea del log se cuente una sola vez.

### 2) Agregación Local por Hilo

Cada hilo construye estadísticas locales privadas:

- Tabla hash para conteos de IP.
- Tabla hash para conteos de URL.
- Contador de errores HTTP.
- Contador de líneas parseadas.

El uso de acumuladores privados por hilo elimina la contención por bloqueos durante el parseo.

### 3) Etapa de Fusión (Merge)

Cuando todos los hilos finalizan:

- El hilo principal fusiona cada tabla hash local en estadísticas globales finales.
- Se suman contadores de errores HTTP y líneas parseadas.
- La URL más visitada se selecciona a partir de los conteos finales de URL.

## Formato de Log Soportado

Formato esperado de cada línea de entrada:

```text
IP - - [timestamp] "METHOD /path" STATUS
```

Ejemplo:

```text
192.168.1.2 - - [10/Feb/2024:10:20:31] "POST /login" 403
```

## Ejecución

## Linux

Compilar:

```bash
make
```

Ejecutar archivo por defecto y 4 hilos:

```bash
./log_analyzer_linux access.log 4
```

O usar:

```bash
chmod +x run.sh
./run.sh 4 access.log
```

## Windows

Ejecutar script:

```bat
run.bat 4 access.log
```


## Salida del Programa

Ambos ejecutables imprimen:

- Reporte base de 1 hilo.
- Reporte multihilo usando el número de hilos solicitado.

Métricas principales mostradas:

- Líneas procesadas.
- Total de IP únicas.
- URL más visitada y cantidad de accesos.
- Errores HTTP.
- Tiempo de ejecución.


## Generar un Nuevo Log de Prueba

Usa el script Python incluido:

```bash
python access_log_file_generator.py
```

Esto genera `access.log` con 5000 entradas de ejemplo.

## Notas de Rendimiento

- Las tablas hash ofrecen inserción/actualización promedio cercana a O(1) para contadores.
- Las estadísticas privadas por hilo evitan sincronización pesada durante el parseo.
- El costo de merge suele ser mucho menor que el costo del parseo total del archivo.
- Archivos muy pequeños pueden no beneficiarse de muchos hilos debido al overhead.

