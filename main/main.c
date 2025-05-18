#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define BTN_PIN ((1ULL << 25))
#define LED_PINS ((1ULL << 18) | (1ULL << 19))

static QueueHandle_t gpio_event_queue = NULL; //xQueueHandle

static void IRAM_ATTR isr(void *args) {
    uint32_t num_pin = (uint32_t) args;
    xQueueSendFromISR(gpio_event_queue, &num_pin, NULL);
}

static void task1(void * args) {
    uint32_t num_pin = (uint32_t) args;
    static bool set_bt = 0;
    for (;;) {
        if (xQueueReceive(gpio_event_queue, &num_pin, portMAX_DELAY)) {
            gpio_set_level(GPIO_NUM_18, !gpio_get_level(num_pin));
        }
    }
}

void app_main(void)
{
    gpio_config_t conf_gpio = {};
    conf_gpio.intr_type = GPIO_INTR_DISABLE;
    conf_gpio.mode = GPIO_MODE_OUTPUT;
    conf_gpio.pin_bit_mask = LED_PINS;
    conf_gpio.pull_down_en = 1;
    conf_gpio.pull_up_en = 0;
    gpio_config(&conf_gpio);

    conf_gpio.intr_type = GPIO_INTR_DISABLE;
    conf_gpio.mode = GPIO_MODE_INPUT;
    conf_gpio.pin_bit_mask = BTN_PIN;
    conf_gpio.pull_down_en = 0;
    conf_gpio.pull_up_en = 1;
    gpio_config(&conf_gpio);

    gpio_set_intr_type(GPIO_NUM_25, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_25, isr, (void *) GPIO_NUM_25);

    gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
    
    xTaskCreatePinnedToCore(task1, "task1", configMINIMAL_STACK_SIZE * 2, (void *) GPIO_NUM_25, 3, NULL, 0);
}