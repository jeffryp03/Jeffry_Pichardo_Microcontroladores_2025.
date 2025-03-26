#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "portmacro.h"

// Definiciones de hardware (puertos y pines)
#define BUTTON_PIN    0    // Número de pin del botón
#define LED_PIN       1    // Número de pin del LED

// Variables para almacenar el tiempo de pulsación
volatile uint32_t buttonPressTime = 0;
volatile uint32_t buttonReleaseTime = 0;

// Temporizadores
TimerHandle_t xButtonTimer;
TimerHandle_t xLedBlinkTimer;

// Prototipos de funciones
void vButtonTimerCallback(TimerHandle_t xTimer);
void vLedBlinkTimerCallback(TimerHandle_t xTimer);
void vButtonISRHandler(void);  // ISR que será llamada cuando se presione el botón

// Función para inicializar los periféricos (botón, LED, etc.)
void setupHardware(void) {
    // Inicializar el LED y el botón (configuración de pines)
    // Este código dependerá de tu plataforma específica
}

// Tarea para manejar el parpadeo del LED
void vLedBlinkTask(void *pvParameters) {
    const TickType_t xDelay = 100 / portTICK_PERIOD_MS;  // Intervalo de parpadeo (100 ms)

    while (1) {
        if (buttonPressTime > 0) {
            digitalWrite(LED_PIN, HIGH);  // Enciende el LED
            vTaskDelay(buttonPressTime / portTICK_PERIOD_MS);  // Mantén el LED encendido
            digitalWrite(LED_PIN, LOW);   // Apaga el LED
            vTaskDelay(xDelay);  // Espera entre parpadeos
        }
    }
}

// Función de callback del temporizador que mide la pulsación del botón
void vButtonTimerCallback(TimerHandle_t xTimer) {
    // Comienza a medir el tiempo de la presión del botón
    if (digitalRead(BUTTON_PIN) == HIGH) {
        // El botón está presionado, arrancamos el temporizador del botón
        buttonPressTime++;
    } else {
        // El botón fue liberado, detenemos el temporizador del botón
        buttonReleaseTime = buttonPressTime;
        buttonPressTime = 0;
        // Notificamos al temporizador del parpadeo del LED
        xTimerStart(xLedBlinkTimer, 0);
    }
}

// Función de callback del temporizador para parpadear el LED
void vLedBlinkTimerCallback(TimerHandle_t xTimer) {
    // Este temporizador controla el parpadeo del LED en función del tiempo de pulsación
    if (buttonReleaseTime > 0) {
        // Parpadeo del LED durante el tiempo que el botón estuvo presionado
        // Realiza las tareas de parpadeo aquí (como encender y apagar el LED)
        for (uint32_t i = 0; i < buttonReleaseTime / 1000; i++) {
            digitalWrite(LED_PIN, HIGH);  // Enciende el LED
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Mantén el LED encendido
            digitalWrite(LED_PIN, LOW);   // Apaga el LED
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Mantén el LED apagado
        }
    }
}

// ISR de interrupción para el botón
void vButtonISRHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Aquí iría el código que gestiona la interrupción del botón
    // Este código debe ser específico para tu hardware (ej., lectura de pin)

    if (digitalRead(BUTTON_PIN) == HIGH) {
        // Si el botón está presionado, inicia el temporizador del botón
        xTimerStartFromISR(xButtonTimer, &xHigherPriorityTaskWoken);
    } else {
        // Si el botón fue liberado, detén el temporizador del botón
        xTimerStopFromISR(xButtonTimer, &xHigherPriorityTaskWoken);
    }

    // Hacer un yield si es necesario
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Tarea principal
void main(void) {
    setupHardware();

    // Crear el temporizador que captura el tiempo del botón
    xButtonTimer = xTimerCreate("Button Timer", pdMS_TO_TICKS(1), pdTRUE, (void *)0, vButtonTimerCallback);
    
    // Crear el temporizador para el parpadeo del LED
    xLedBlinkTimer = xTimerCreate("LED Blink Timer", pdMS_TO_TICKS(1), pdFALSE, (void *)0, vLedBlinkTimerCallback);

    // Crear la tarea que manejará el parpadeo del LED
    xTaskCreate(vLedBlinkTask, "LED Blink Task", 128, NULL, 1, NULL);

    // Arrancar el RTOS
    vTaskStartScheduler();

    // El sistema no debería llegar aquí
    while (1);
}
