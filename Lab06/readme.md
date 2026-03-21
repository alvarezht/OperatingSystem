# Lab006 Parking Lot 

Cada auto hace:
1. Llega y registra evento con timestamp.
2. Espera en `sem_wait` hasta tener cupo.
3. Se estaciona de 1 a 5 segundos.
4. Sale y libera el cupo con `sem_post`.

La GUI consulta periodicamente un snapshot del estado del motor y lo dibuja en pantalla.

## Archivos

- `main.c`: interfaz grafica raylib/raygui.
- `parking_lot.c`: logica del laboratorio (sin GUI).
- `parking_lot.h`: estructuras y funciones publicas del motor.

### En `parking_lot.c`
- Semaforo contador:
  - Declaracion: `static sem_t parking_semaphore;`
  - Inicializacion: `sem_init(...)` dentro de `parking_start(...)`.
  - Bloqueo por cupo: `sem_wait(...)` dentro de `car_thread(...)`.
  - Liberacion de cupo: `sem_post(...)` dentro de `car_thread(...)`.

- Logging thread-safe:
  - Mutex: `static pthread_mutex_t log_mutex ...`
  - Funcion: `log_event(...)`.

- Estadisticas protegidas:
  - Mutex: `static pthread_mutex_t stats_mutex ...`
  - Actualizaciones en `car_thread(...)`.
  - Snapshot para GUI en `parking_get_snapshot(...)`.

- API publica:
  - `parking_start(...)`
  - `parking_is_running(...)`
  - `parking_get_snapshot(...)`
  - `parking_wait_until_done(...)`
  - `parking_shutdown(...)`

### En `main.c`
- Controles GUI:
  - `GuiSliderBar(...)` para capacidad y autos.
  - `GuiButton(...)` para iniciar simulacion.

- Integracion con logica:
  - Inicio: `parking_start(capacity, totalCars)`.
  - Actualizacion en vivo: `parking_get_snapshot(&snapshot)`.
  - Cierre limpio: `parking_shutdown()`.

## Flujo detallado (3 carros, 1 parqueo)

Ejemplo de configuracion:
- Capacidad del parqueo: `1`
- Total de carros: `3` (Car 0, Car 1, Car 2)

Cada carro corre en su propio thread (`car_thread`). El semaforo (`parking_semaphore`) se inicializa con valor `1`, asi que solo un carro puede estar parqueado al mismo tiempo.

### 1) Arranque de la simulacion

- `parking_start(1, 3)` crea 3 threads (uno por carro).
- `sem_init(&parking_semaphore, 0, 1)` deja el contador del semaforo en `1`.
- Cada thread comienza con `sleep_ms(random_between(0, 1000))`, asi que los carros llegan en tiempos aleatorios.

### 2) Llegadas aleatorias y estado WAITING

Supongamos esta secuencia de llegadas:
- Car 0 llega en 120 ms
- Car 1 llega en 300 ms
- Car 2 llega en 650 ms

Cuando cada carro llega:
- Registra evento con `log_event`.
- Entra a seccion critica con `stats_mutex`.
- Incrementa `cars_arrived` y `cars_waiting`.
- Pasa a estado `CAR_WAITING`.

### 3) Uso del semaforo: Wait/Signal real

#### Car 0
- Ejecuta `sem_wait(&parking_semaphore)` cuando el contador vale `1`.
- El semaforo decrementa a `0` y Car 0 entra sin bloquearse.
- Se le asigna el unico slot libre y pasa a `CAR_PARKED`.

#### Car 1
- Llega cuando el contador ya esta en `0`.
- `sem_wait(...)` lo bloquea (queda esperando cupo).
- Su tiempo de espera empieza justo antes del `sem_wait` (`wait_start`).

#### Car 2
- Tambien encuentra contador en `0`.
- `sem_wait(...)` lo bloquea igual que Car 1.

### 4) Salida, liberacion y despertar de otro thread

Cuando Car 0 termina de parquear (1 a 5 segundos):
- Actualiza estado a `CAR_DONE` y libera su slot en seccion critica.
- Ejecuta `sem_post(&parking_semaphore)`.

Efecto de `sem_post`:
- Incrementa el contador del semaforo.
- Si hay threads bloqueados en `sem_wait`, el sistema despierta uno. en dependencia de cual decida el scheduler.
e- Puede que se desbloquea Car 1 primero; Car 2 sigue esperando.

Luego se repite el ciclo:
- Car 1 entra, se parquea, sale, hace `sem_post`.
- Car 2 se desbloquea, entra, se parquea y sale.

### 5) Medicion del tiempo de espera

Para cada carro:
- Inicio de espera: antes de `sem_wait`.
- Fin de espera: justo al retornar de `sem_wait`.
- `waited = now_seconds() - wait_start`.

Ese valor se acumula en:
- `total_wait_time`
- `total_cars_parked`

Y el promedio mostrado en snapshot es:
- `average_wait = total_wait_time / total_cars_parked`.

### 6) Fin de la simulacion

- Cada carro, al salir, incrementa `cars_completed` en seccion critica.
- El ultimo carro que termina marca `simulation_done = 1`.
- `parking_wait_until_done()` espera ese flag y luego hace `join_all_threads_if_needed()` para cerrar todos los threads correctamente.

