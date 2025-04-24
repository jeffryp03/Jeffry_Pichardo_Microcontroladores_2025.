#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "stm32f4xx_hal.h"

// GPIO
#define LED_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_5
#define BUTTON_GPIO_Port GPIOC
#define BUTTON_Pin GPIO_PIN_13

// Variables
TaskHandle_t xButtonTaskHandle = NULL;
TaskHandle_t xBlinkTaskHandle = NULL;

TimerHandle_t xBlinkTimer;
volatile TickType_t buttonPressTick = 0;
volatile TickType_t pressDuration = 0;
volatile BaseType_t buttonPressed = pdFALSE;

// ===== Timer callback para el parpadeo del LED =====
void vBlinkTimerCallback(TimerHandle_t xTimer) {
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

// ===== ISR del botón (simulado por EXTI) =====
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin == BUTTON_Pin) {
        if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin) == GPIO_PIN_RESET) {
            // Botón presionado
            buttonPressTick = xTaskGetTickCountFromISR();
            buttonPressed = pdTRUE;
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        } else {
            // Botón liberado
            if (buttonPressed) {
                TickType_t now = xTaskGetTickCountFromISR();
                pressDuration = now - buttonPressTick;
                buttonPressed = pdFALSE;

                HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
                // Notificar a la tarea que debe iniciar parpadeo
                vTaskNotifyGiveFromISR(xBlinkTaskHandle, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

// ===== Tarea de parpadeo =====
void vBlinkTask(void *pvParameters) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Configurar tiempo de parpadeo
        TickType_t blinkTime = pressDuration;
        TickType_t startTime = xTaskGetTickCount();

        // Iniciar timer con intervalo de 200ms
        xTimerChangePeriod(xBlinkTimer, pdMS_TO_TICKS(200), 0);
        xTimerStart(xBlinkTimer, 0);

        while ((xTaskGetTickCount() - startTime) < blinkTime) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Evitar bloqueo completo
        }

        xTimerStop(xBlinkTimer, 0);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    }
}

// ===== Tarea principal (puede estar vacía si no se usa) =====
void vMainTask(void *pvParameters) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Inactivo
    }
}

// ===== MAIN =====
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init(); // Tu función de inicialización GPIO

    // Crear timer para parpadeo
    xBlinkTimer = xTimerCreate("BlinkTimer", pdMS_TO_TICKS(200), pdTRUE, 0, vBlinkTimerCallback);

    // Crear tareas
    xTaskCreate(vBlinkTask, "BlinkTask", 128, NULL, 2, &xBlinkTaskHandle);
    xTaskCreate(vMainTask, "MainTask", 128, NULL, 1, &xButtonTaskHandle);

    // Iniciar RTOS
    vTaskStartScheduler();

    while (1);
}