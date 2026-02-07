# Calculadora

![Resultado del Proyecto](https://github.com/alvarezht/OperatingSystem/blob/main/002/resultado.png)

### Conversión de String a Float atof

La función `uart_atof` convierte una cadena de texto que representa un número decimal en su valor de punto flotante equivalente.

#### Código Completo

```c
float uart_atofconst char *s{
    float num = 0.0;
    int sign = 1;
    
    int i = 0;
    double divisor = 1.0;

    ifs[i] == '-' {
        sign = -1;
        i++;
    }

    for; s[i] >= '0' && s[i] <= '9'; i++ {
        num = num * 10 + s[i] - '0';
    }

    ifs[i] == '.' {
        i++;
        for; s[i] >= '0' && s[i] <= '9'; i++ {
            num = num * 10 + s[i] - '0';
            divisor *= 10.0;
        }
    }

    return sign * num / divisor;
}
```

#### Algoritmo Explicado

**Paso 1: Inicialización**
```c
float num = 0.0;        // Acumulador de dígitos
int sign = 1;           // Signo del número 1 o -1
int i = 0;              // Índice de la cadena
double divisor = 1.0;   // Divisor para la parte decimal
```

**Paso 2: Detectar Signo**
```c
ifs[i] == '-' {
    sign = -1;
    i++;
}
```
- Verifica si el primer carácter es un signo negativo
- Si lo es, marca el número como negativo y avanza el índice

**Paso 3: Procesar Parte Entera**
```c
for; s[i] >= '0' && s[i] <= '9'; i++ {
    num = num * 10 + s[i] - '0';
}
```
- Acumula los dígitos antes del punto decimal
- Multiplica el acumulador por 10 y suma el nuevo dígito
- `s[i] - '0'` convierte el carácter Alphanumerica su valor numérico


**Paso 4: Procesar Parte Decimal**
```c
ifs[i] == '.' {
    i++;
    for; s[i] >= '0' && s[i] <= '9'; i++ {
        num = num * 10 + s[i] - '0';
        divisor *= 10.0;
    }
}
```
- Si encuentra un punto decimal, procesa los dígitos siguientes
- Continúa acumulando dígitos en `num` como si fueran enteros
- Simultáneamente multiplica `divisor` por 10 por cada dígito
- Esto permite rastrear cuántos lugares decimales hay


**Paso 5: Aplicar Divisor y Signo**
```c
return sign * num / divisor;
```
- Divide el número acumulado por el divisor para obtener el valor correcto
- Aplica el signo
- Ejemplo: `sign=1, num=1234, divisor=100` → `1 * 1234/100` = `12.34`
---

### Conversión de Float a String ftoa

La función `uart_ftoa` convierte un número de punto flotante a su representación en texto.

#### Código Completo

```c
void uart_ftoafloat num, char *buffer, int *precision {
    int i = 0;
    int is_negative = 0;
    
    if num < 0 {
        is_negative = 1;
        buffer[i++] = '-';
        num = -num;
    }
    
    int int_part = intnum;
    float float_part = num - floatint_part;
    
    char int_str[15];
    uart_itoaint_part, int_str;
    
    int j = 0;
    while int_str[j] != '\0' {
        buffer[i++] = int_str[j++];
    }
    
    if *precision > 0 {
        buffer[i++] = '.';
        
        for int k = 0; k < *precision; k++ {
            float_part = float_part * 10;
            int digit = intfloat_part;
            buffer[i++] = '0' + digit;
            float_part -= digit;
        }
    }
    
    buffer[i] = '\0';
}
```

#### Algoritmo Explicado

**Paso 1: Manejo del Signo**
```c
if num < 0 {
    is_negative = 1;
    buffer[i++] = '-';
    num = -num;
}
```
- Detecta si el número es negativo
- Si lo es, escribe `-` al inicio del buffer
- Convierte el número a positivo para facilitar el procesamiento

**Paso 2: Separación de Partes**
```c
int int_part = intnum;
float float_part = num - floatint_part;
```
- **Parte entera**: Casting a `int` trunca los decimales
- **Parte decimal**: Resta la parte entera del número original
- Ejemplo: `3.14` → int_part=3, float_part=0.14

**Paso 3: Convertir Parte Entera**
```c
char int_str[15];
uart_itoaint_part, int_str;

int j = 0;
while int_str[j] != '\0' {
    buffer[i++] = int_str[j++];
}
```
- Usa la función `uart_itoa` integer to Alphanumericpara convertir la parte entera
- Copia el resultado carácter por carácter al buffer final
- Ejemplo: int_part=123 → int_str="123"

**Paso 4: Agregar Punto Decimal**
```c
if *precision > 0 {
    buffer[i++] = '.';
    ...
}
```
- Solo si la precisión es mayor que 0
- Escribe el carácter `.` en el buffer

**Paso 5: Extraer Dígitos Decimales**
```c
for int k = 0; k < *precision; k++ {
    float_part = float_part * 10;
    int digit = intfloat_part;
    buffer[i++] = '0' + digit;
    float_part -= digit;
}
```
Este es el algoritmo más importante:

1. **Multiplicar por 10**: Mueve el siguiente dígito decimal a la posición de las unidades
2. **Extraer dígito**: Casting a `int` obtiene solo la parte entera
3. **Convertir a Alphanumeric**: `'0' + digit` convierte el número a su carácter
4. **Remover dígito**: Resta el dígito extraído para continuar con el resto

**Ejemplo**:
```
Número: 3.14159, precisión: 4
float_part inicial = 0.14159

k=0: float_part = 0.14159 * 10 = 1.4159
     digit = int1.4159 = 1
     buffer[i++] = '0' + 1 = '1'
     float_part = 1.4159 - 1 = 0.4159

k=1: float_part = 0.4159 * 10 = 4.159
     digit = int4.159 = 4
     buffer[i++] = '0' + 4 = '4'
     float_part = 4.159 - 4 = 0.159

k=2: float_part = 0.159 * 10 = 1.59
     digit = int1.59 = 1
     buffer[i++] = '0' + 1 = '1'
     float_part = 1.59 - 1 = 0.59

k=3: float_part = 0.59 * 10 = 5.9
     digit = int5.9 = 5
     buffer[i++] = '0' + 5 = '5'
     float_part = 5.9 - 5 = 0.9

Resultado: "3.1415"
```

**Paso 6: Terminar la Cadena**
```c
buffer[i] = '\0';
```
- Añade el terminador nulo al final
---

### Función PRINT - Salida Formateada

La función `PRINT` es similar a `printf` de la biblioteca estándar de C

#### Código Completo

```c
void PRINTconst char *format, ... {
    va_list args;
    va_startargs, format;
    
    char buffer[32];
    int i = 0;
    
    while format[i] != '\0' {
        if format[i] == '%' {
            i++;
            if format[i] == 'd' {
                int val = va_argargs, int;
                uart_itoaval, buffer;
                uart_putsbuffer;
            } else if format[i] == 'f' {
                float val = va_argargs, double;
                int precision = 6;
                uart_ftoaval, buffer, &precision;
                uart_putsbuffer;
            } else if format[i] == '.' && format[i+1] >= '0' || format[i+1] <= '9' && format[i+2] == 'f' {
                float val = va_argargs, double;
                int precision = format[i+1] - '0';
                uart_ftoaval, buffer, &precision;
                uart_putsbuffer;
                i++;
                i++;
            } else {
                uart_putc'%';
                uart_putcformat[i];
            }
        } else {
            uart_putcformat[i];
        }
        i++;
    }
    
    va_endargs;
}
```

#### Componentes de la Implementación

**1. Argumentos Variables de la libreria `stdarg.h`**

```c
va_list args;          // Declara lista de argumentos
va_startargs, format; // Inicializa la lista después del parámetro 'format'
va_argargs, tipo;    // Extrae el siguiente argumento del tipo especificado
va_endargs;          // Limpia la lista de argumentos
```

Esto permite que `PRINT` acepte un número variable de argumentos, similar a `printf`.

**2. Parseo del String de Formato**

```c
while format[i] != '\0' {
    if format[i] == '%' {
        // Procesar especificador de formato
    } else {
        // Carácter normal, imprimir directamente
        uart_putcformat[i];
    }
    i++;
}
```

El algoritmo recorre la cadena de formato carácter por carácter:
- Caracteres normales se envían directamente a UART
- Carácter `%` indica un especificador de formato

**3. Especificador `%d` - Enteros**

```c
if format[i] == 'd' {
    int val = va_argargs, int;  // Extraer el siguiente argumento como int
    uart_itoaval, buffer;       // Convertir a string
    uart_putsbuffer;            // Enviar a UART
}
```

**Ejemplo**:
```c
PRINT"Resultado: %d\n", 42;
```
- Encuentra `%d`
- Extrae `42` de los argumentos
- Convierte a `"42"` usando `uart_itoa`
- Envía `"42"` por UART

**4. Especificador `%f` - Flotantes precisión por defecto igual a 6**

```c
else if format[i] == 'f' {
    float val = va_argargs, double;  // Nota: float se promociona a double
    int precision = 6;                 // Precisión por defecto
    uart_ftoaval, buffer, &precision;
    uart_putsbuffer;
}
```
**5. Especificador `%.Nf` - Flotantes con Precisión Para Que sea mas Similar A Printf**

```c
else if format[i] == '.' && format[i+1] >= '0' || format[i+1] <= '9' && format[i+2] == 'f' {
    float val = va_argargs, double;
    int precision = format[i+1] - '0';  // Extrae el dígito de precisión
    uart_ftoaval, buffer, &precision;
    uart_putsbuffer;
    i++;  // Saltar el dígito
    i++;  // Saltar la 'f'
}
```

Este código detecta patrones como `%.2f`, `%.3f`, etc.

**Proceso**:
1. Detecta el punto `.`
2. Verifica que el siguiente carácter sea un dígito
3. Verifica que después venga `f`
4. Extrae el número de precisión: `format[i+1] - '0'`
5. Avanza el índice 2 posiciones adicionales

**Ejemplo**:
```c
PRINT"Precio: %.2f\n", 19.99;
// Salida: "Precio: 19.99\n"
```

---

### Función READ - Entrada Formateada

La función `READ` es similar a `scanf`, permitiendo leer valores del usuario según el formato dado.

#### Código Completo

```c
void READconst char *format, void *ptr {
    char buffer[32];
    uart_gets_inputbuffer, 32;
    
    int i = 0;
    while format[i] != '\0' {
        if format[i] == '%' {
            i++;
            if format[i] == 'd' {
                int *p = int *ptr;
                *p = uart_atoibuffer;
                return;
            } else if format[i] == 'f' {
                float *p = float *ptr;
                *p = uart_atofbuffer;
                return;
            }
        }
        i++;
    }
}
```

#### Algoritmo Explicado

**Paso 1: Leer Entrada del Usuario**

```c
char buffer[32];
uart_gets_inputbuffer, 32;
```

La función `uart_gets_input`:
1. Lee caracteres uno por uno desde UART
2. Hace **echo** de cada carácter lo retransmite para que el usuario vea lo que escribe
3. Se detiene al recibir Enter `\n` o `\r`
4. Añade terminador nulo `\0` al final
5. Limita la entrada a `max_length - 1` caracteres

**Ejemplo de interacción**:
```
Programa envía: "Enter number: "
Usuario escribe: 4 2 [Enter]
  
uart_getc recibe: '4'  → echo → usuario ve: '4'
uart_getc recibe: '2'  → echo → usuario ve: '2'
uart_getc recibe: '\n' → echo → usuario ve: nueva línea
  
buffer = "42\0"
```

**Paso 2: Parsear Formato**

```c
int i = 0;
while format[i] != '\0' {
    if format[i] == '%' {
        i++;
        // Procesar especificador
    }
    i++;
}
```

Recorre el string de formato buscando especificadores `%`.

**Paso 3: Especificador `%d` - Leer Entero**

```c
if format[i] == 'd' {
    int *p = int *ptr;      // Convertir puntero genérico a int*
    *p = uart_atoibuffer;   // Convertir string a int y almacenar
    return;
}
```

**Proceso detallado**:
1. **Cast del puntero**: `ptr` es `void*` genérico, se convierte a `int*`
2. **Conversión**: `uart_atoi` convierte el string a número entero
3. **Almacenamiento**: Se guarda el valor en la dirección apuntada por `ptr`
4. **Retorno inmediato**: Solo procesa un valor por llamada

**Paso 4: Especificador `%f` - Leer Flotante**

```c
else if format[i] == 'f' {
    float *p = float *ptr;
    *p = uart_atofbuffer;
    return;
}
```

Similar a `%d`, pero:
1. Cast a `float*`
2. Usa `uart_atof` para conversión
3. Almacena el resultado como float
   
####  Creación del Script build_and_run.sh

 Un solo comando para compilar y ejecutar
Adaptado para poder compilar con flags de --nostdlib
```bash
#!/bin/bash
set -e  # Terminar si hay error

# Limpieza
rm -f *.o *.elf *.bin

# Ensamblado
arm-none-eabi-as -o root.o root.s

# Compilación
arm-none-eabi-gcc -c -o stdio.o stdio.c
arm-none-eabi-gcc -c -o main.o main.c

# Enlazado
arm-none-eabi-gcc -T linker.ld -o calculadora.elf \
    root.o stdio.o main.o -nostdlib -lgcc

# Conversión a binario opcional
arm-none-eabi-objcopy -O binary calculadora.elf calculadora.bin

# Ejecución
qemu-system-arm -M versatilepb -nographic -kernel calculadora.elf
```

---

**Última actualización**: Febrero 2026
