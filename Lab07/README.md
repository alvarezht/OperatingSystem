# Lab 007 - Condition Variables

## Estado compartido (`Bridge`)
La estructura `Bridge` contiene:

- `lock`: exclusión mutua para proteger estado compartido.
- `can_cross`: variable de condición para bloquear y despertar hilos.
- `on_bridge`: cantidad actual de estudiantes cruzando.
- `current_dir`: dirección activa del puente (`-1` libre, `0` derecha, `1` izquierda).
- `waiting[2]`: hilos esperando por cada dirección.
- `oldest_wait_start[2]`: inicio de espera del más antiguo por dirección.
- `total_wait_seconds` y `crossed_count`: métricas para promedio de espera.

## Sincronización y reglas del puente
`access_bridge()` aplica estas condiciones de entrada dentro de una sección crítica:

- Solo entra si hay capacidad (`on_bridge < 4`).
- Solo entra si la dirección coincide con la dirección activa o el puente está libre.
- Si el puente está libre, puede bloquear temporalmente una dirección para dar prioridad a la opuesta cuando su espera supera el umbral (`PRIORITY_WAIT_SECONDS`).
- Si no puede entrar, el hilo espera con `pthread_cond_wait()`.

Cuando un hilo entra:

- Actualiza contadores de espera.
- Fija dirección activa si el puente estaba libre.
- Incrementa `on_bridge`.
- Acumula su tiempo de espera para la estadística final.

`exit_bridge()`:

- Decrementa `on_bridge`.
- Libera la dirección activa cuando el puente queda vacío.
- Despierta hilos bloqueados con `pthread_cond_broadcast()`.

## Comportamiento de los estudiantes
Cada hilo en `student_thread()`:

- Espera un tiempo aleatorio de llegada entre 0 y 5 segundos.
- Elige dirección aleatoria (`0` derecha, `1` izquierda).
- Solicita acceso con `access_bridge()`.
- Cruza entre 1 y 3 segundos.
- Sale con `exit_bridge()`.

## Registro y métricas
El programa registra eventos de:

- llegada,
- inicio de cruce,
- salida del puente,

incluyendo estado del puente y colas por dirección.

Al final, calcula e imprime:

- tiempo promedio de espera antes de cruzar (`total_wait_seconds / crossed_count`).
