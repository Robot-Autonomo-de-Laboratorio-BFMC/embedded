# Robot Autónomo de Laboratorio - ESP32 FreeRTOS Architecture

![BANNER](https://github.com/nicolas-mangini/esp32-rc-car-mod/assets/72108522/f1f22568-18d2-42b5-bf51-995ad033ce1d)

## Descripción

Sistema de control para auto RC implementado con ESP32, utilizando arquitectura basada en **Mailbox + Task Notifications** con FreeRTOS. El sistema permite controlar motores, dirección y luces mediante comandos recibidos vía UART o interfaz web (Wi-Fi AP).

## Características Principales

- **Arquitectura FreeRTOS**: Sistema multitsk con tareas dedicadas por subsistema
- **Mailbox Pattern**: Comunicación mediante buzones con patrón last-writer-wins (sin colas)
- **Task Notifications**: Notificaciones eficientes para despertar tareas críticas
- **Dual Interface**: Control vía UART (921600 baud) y Web (Wi-Fi AP)
- **Emergency Brake**: Sistema de frenado de emergencia con respuesta <1ms
- **Time-to-Live**: Comandos con expiración automática para seguridad
- **Core Pinning**: Distribución optimizada de tareas entre los dos núcleos del ESP32

### Sistema de Luces

- **Luz trasera**: LED en el paragolpes trasero que se enciende automáticamente al dar marcha atrás
- **Luz delantera**: LED en el paragolpes delantero con tres modos:
  - **ON**: Encendido manual
  - **OFF**: Apagado manual
  - **AUTO**: Control automático mediante LDR (se enciende con poca luz)

## Software - Arquitectura FreeRTOS

### Arquitectura General

El sistema utiliza una arquitectura basada en **Mailboxes + Task Notifications** para comunicación entre subsistemas:

```
┌─────────────┐      ┌─────────────┐
│  LinkRxTask │      │   WebTask   │
│  (UART)     │      │  (Wi-Fi AP) │
└──────┬──────┘      └──────┬──────┘
       │                    │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │   Mailboxes (W)     │
       │  ┌────────────────┐ │
       │  │ Motor Mailbox  │ │
       │  │ Steer Mailbox  │ │
       │  │ Lights Mailbox │ │
       │  │ Supervisor MB  │ │
       │  └────────────────┘ │
       └──────────┬──────────┘
                  │
       ┌──────────▼──────────┐
       │  Control Tasks (R)  │
       │  ┌────────────────┐ │
       │  │  MotorTask     │ │
       │  │  SteerTask     │ │
       │  │  LightsTask    │ │
       │  │  SupervisorTask││
       │  └────────────────┘ │
       └─────────────────────┘
```

### Mailbox Pattern

Cada subsistema tiene un mailbox que almacena el último comando recibido:

```c
typedef struct {
    uint32_t ts_ms;          // Timestamp en milisegundos
    topic_t topic;            // Tópico del mensaje
    command_type_t cmd;       // Tipo de comando
    int32_t value;            // Valor del comando (velocidad, ángulo, etc.)
    uint32_t seq;             // Número de secuencia
    uint32_t ttl_ms;          // Time-to-Live en milisegundos
    bool valid;               // Indica si el mailbox contiene datos válidos
    SemaphoreHandle_t mutex;  // Mutex para operaciones atómicas
} mailbox_t;
```

**Características del Mailbox:**
- **Last-Writer-Wins**: Los comandos antiguos se descartan automáticamente
- **Time-to-Live**: Los comandos expiran después de un tiempo configurado
- **Thread-Safe**: Operaciones protegidas con mutex
- **Zero Backlog**: No hay colas, solo el último comando válido

### Tareas FreeRTOS

#### MotorTask (Core 0, Priority 4)
- Lee comandos del mailbox de motor
- Controla velocidad y dirección del motor de tracción
- Implementa frenado de emergencia con notificaciones directas (<1ms)
- Ejecuta a 100 Hz (10ms de período)

#### SteerTask (Core 0, Priority 3)
- Lee comandos del mailbox de dirección
- Controla el ángulo del servo de dirección
- Valida rango de ángulos (centro, izquierda, derecha)
- Ejecuta a 100 Hz (10ms de período)

#### LightsTask (Core 1, Priority 1)
- Lee comandos del mailbox de luces
- Controla LEDs delantero y trasero
- Modo automático con lectura periódica del LDR
- Ejecuta a 10 Hz (100ms de período)

#### LinkRxTask (Core 1, Priority 4)
- Recibe comandos vía UART (921600 baud)
- Parsea mensajes en formato: `CHANNEL:CMD:VALUE`
- Escribe en los mailboxes correspondientes
- Maneja comandos de emergencia (BRAKE_NOW, STOP)

#### WebTask (Core 1, Priority 2)
- Configura Wi-Fi Access Point
- Sirve página web HTML con controles
- Procesa comandos HTTP (GET/POST)
- Escribe en los mailboxes correspondientes

#### SupervisorTask (Core 1, Priority 2)
- Monitorea el estado del sistema
- Implementa watchdog/heartbeat
- Detecta fallos y activa modos seguros

#### LinkTxTask (Core 1, Priority 2)
- Envía telemetría vía UART
- Transmite estado del sistema periódicamente

### Task Notifications

Las tareas de control utilizan notificaciones de FreeRTOS para despertarse eficientemente:

```c
// En el sender (LinkRxTask, WebTask)
mailbox_write(&motor_mailbox, TOPIC_MOTOR, CMD_SET_SPEED, value, 200);
xTaskNotify(motor_task_handle, 0, eNoAction);

// En MotorTask
void motor_task(void *pvParameters) {
    while (1) {
        // Espera notificación (bloquea hasta recibir)
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        
        // Lee mailbox
        Msg msg;
        if (mailbox_read(&motor_mailbox, &msg) && !expired(&msg)) {
            applyMotor(msg.value);
        } else {
            applySafeStop(); // Comando expirado
        }
    }
}
```

**Ventajas:**
- **Bajo consumo de CPU**: Tareas duermen hasta recibir notificación
- **Respuesta inmediata**: Despertar de tareas es muy rápido (<1ms)
- **Sin polling**: No hay chequeos periódicos innecesarios
- **Minimal overhead**: Menor uso de memoria que colas

### Emergency Brake

Sistema de frenado de emergencia con respuesta <1ms:

```c
// En LinkRxTask
if (channel == CHANNEL_EMERGENCY && cmd == "BRAKE_NOW") {
    motor_task_trigger_emergency(); // Notificación directa
}

// En MotorTask
uint32_t notification = ulTaskNotifyTake(pdTRUE, 0);
if (notification & EMERGENCY_NOTIFICATION_BIT) {
    motor_stop(); // Acción inmediata
}
```

### Interfaces de Control

#### UART (921600 baud)
Formato de mensajes: `CHANNEL:CMD:VALUE`

Ejemplos:
- `CONTROL:SET_SPEED:150` - Establece velocidad a 150
- `CONTROL:SET_STEER:90` - Establece ángulo de dirección a 90°
- `EMERGENCY:BRAKE_NOW:0` - Freno de emergencia
- `LIGHTS:SET_MODE:2` - Establece modo de luces (0=OFF, 1=ON, 2=AUTO)

#### Web Interface (Wi-Fi AP)
- **SSID**: Configurable (default: "ESP32-RC-Car")
- **URL**: `http://192.168.4.1`
- **Controles**: Botones y sliders para velocidad, dirección y luces
- **Responsive**: Funciona en dispositivos móviles

### Composición vs Herencia

El sistema está diseñado siguiendo el principio de **Composition over Inheritance**:
- Cada subsistema (motor, dirección, luces) es independiente
- Comunicación mediante mailboxes (composición)
- No hay jerarquías de clases, solo funciones y estructuras
- Facilita testing y mantenimiento

## Compilación y Uso

### Requisitos

- PlatformIO
- ESP32 Toolchain
- FreeRTOS (incluido en ESP32 Arduino framework)

### Compilación

```bash
pio run -e esp32doit-devkit-v1
```

### Upload

```bash
pio run -e esp32doit-devkit-v1 -t upload
```

### Monitor Serial

```bash
pio device monitor
```

### Configuración

Los parámetros de configuración se definen en `platformio.ini`:
- `UART_BAUD`: Velocidad de UART (default: 921600)
- `SERIAL_BAUD`: Velocidad de Serial Monitor (default: 115200)

## DEMO

https://github.com/nicolas-mangini/esp32-rc-car-mod/assets/72108522/c88e2a87-3ad3-4a92-a44b-5ae4985a6168

## Conclusión

Este proyecto demuestra la integración exitosa de hardware y software utilizando una arquitectura moderna basada en FreeRTOS. El uso de mailboxes con patrón last-writer-wins y task notifications proporciona:

- **Baja latencia**: Respuesta inmediata a comandos críticos
- **Eficiencia**: Mínimo uso de CPU y memoria
- **Confiabilidad**: Sistema robusto con manejo de expiración de comandos
- **Escalabilidad**: Fácil agregar nuevos subsistemas
- **Mantenibilidad**: Código modular y bien estructurado

El resultado final es un sistema de control remoto funcional y versátil, con arquitectura profesional adecuada para aplicaciones embebidas de tiempo real.
