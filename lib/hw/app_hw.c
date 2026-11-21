#include "app_hw.h"
#include <stdlib.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define BTN_CNT 4
#define TAG "app.btn"

static appbtn_fan_changed_f fan_cb = NULL;
typedef struct
{
    uint8_t idx;
    SemaphoreHandle_t completion;
    bool only_if_off;
} relay_request_t;

static btn_map_t btn_map[BTN_CNT] = {
    {APP_BTN0, GPIO_NUM_NC, 0},
    {APP_BTN1, APP_RELAY1, 1},
    {APP_BTN2, APP_RELAY2, 2},
    {APP_BTN3, APP_RELAY3, 3}};

static uint8_t active_btn_idx = 0;
static portMUX_TYPE state_mux = portMUX_INITIALIZER_UNLOCKED;

static QueueHandle_t idx_queue = NULL;
static TickType_t next = 0;

static void IRAM_ATTR button_handler(void *arg)
{
    TickType_t now = xTaskGetTickCountFromISR();
    if (next > now)
    {
        return;
    }
    next = now + 100 / portTICK_PERIOD_MS;
    relay_request_t request = {
        .idx = (uint8_t)(uintptr_t)arg,
        .completion = NULL,
        .only_if_off = false};
    xQueueSendToBackFromISR(idx_queue, &request, NULL);
}

static void set_relay_task(void *arg)
{
    relay_request_t request;
    gpio_set_level(APP_OE, 1);

    while (1)
    {
        if (xQueueReceive(idx_queue, &request, portMAX_DELAY))
        {
            uint8_t idx = request.idx;
            if (idx >= BTN_CNT)
            {
                if (request.completion != NULL)
                {
                    xSemaphoreGive(request.completion);
                }
                continue;
            }
            ESP_LOGI(TAG, "received idx: %d", idx);

            portENTER_CRITICAL(&state_mux);
            bool should_skip = idx == active_btn_idx ||
                               (request.only_if_off && active_btn_idx != 0);
            portEXIT_CRITICAL(&state_mux);
            if (should_skip)
            {
                if (request.completion != NULL)
                {
                    xSemaphoreGive(request.completion);
                }
                continue;
            }
            for (int i = 1; i < BTN_CNT; ++i)
            {
                gpio_set_level(btn_map[i].relay_pin, 1);
            }

            if (idx != 0)
            {
                ESP_LOGI(TAG, "setting idx: %d", idx);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                gpio_set_level(btn_map[idx].relay_pin, 0);
            }

            portENTER_CRITICAL(&state_mux);
            active_btn_idx = idx;
            portEXIT_CRITICAL(&state_mux);
            if (fan_cb != NULL)
            {
                fan_cb(btn_map[idx].val);
            }
            if (request.completion != NULL)
            {
                xSemaphoreGive(request.completion);
            }
        }
    }
    vTaskDelete(NULL);
}

void apphw_init(appbtn_fan_changed_f f)
{
    fan_cb = f;

    gpio_config_t io_conf;
    uint64_t pin_sel = 0;

    pin_sel = (1ULL << APP_OE);
    pin_sel |= (1ULL << APP_RELAY1);
    pin_sel |= (1ULL << APP_RELAY2);
    pin_sel |= (1ULL << APP_RELAY3);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = pin_sel;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    pin_sel = (1ULL << APP_BTN0);
    pin_sel |= (1ULL << APP_BTN1);
    pin_sel |= (1ULL << APP_BTN2);
    pin_sel |= (1ULL << APP_BTN3);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = pin_sel;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    idx_queue = xQueueCreate(3, sizeof(relay_request_t));
    xTaskCreatePinnedToCore(set_relay_task, "consumer", 2048, NULL, 10, NULL, 0);

    gpio_install_isr_service(0);
    for (int i = 0; i < BTN_CNT; ++i)
    {
        gpio_isr_handler_add(btn_map[i].btn_pin, button_handler, (void *)i);
    }

    gpio_set_level(APP_OE, 0);
    for (int i = 1; i < BTN_CNT; ++i)
    {
        gpio_set_level(btn_map[i].relay_pin, 1);
    }
}

uint8_t apphw_get_state(void)
{
    uint8_t active_idx;
    portENTER_CRITICAL(&state_mux);
    active_idx = active_btn_idx;
    portEXIT_CRITICAL(&state_mux);
    return btn_map[active_idx].val;
}

static bool enqueue_state(uint8_t val, bool only_if_off)
{
    if (val >= BTN_CNT || idx_queue == NULL)
    {
        return false;
    }

    SemaphoreHandle_t completion = xSemaphoreCreateBinary();
    if (completion == NULL)
    {
        return false;
    }

    relay_request_t request = {
        .idx = val,
        .completion = completion,
        .only_if_off = only_if_off};
    BaseType_t queued = xQueueSendToBack(idx_queue, &request, portMAX_DELAY);
    if (queued != pdTRUE)
    {
        vSemaphoreDelete(completion);
        return false;
    }

    xSemaphoreTake(completion, portMAX_DELAY);
    vSemaphoreDelete(completion);
    return true;
}

bool apphw_set_state(uint8_t val)
{
    return enqueue_state(val, false);
}

bool apphw_turn_on_if_off(void)
{
    return enqueue_state(1, true);
}
