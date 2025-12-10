// app_driver.c  -- Shah + RainMaker simple version
// Uses GPIO3 to drive the buzzer (active LOW: 0 = ON, 1 = OFF)

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "app_priv.h"

static const char *TAG = "app_driver";

/* ====== GPIO definitions ====== */
/* Buzzer wiring:
 *  - Buzzer +  -> 3V3
 *  - Buzzer -  -> GPIO3
 * So output is ACTIVE LOW:
 *  - GPIO LOW  -> buzzer ON
 *  - GPIO HIGH -> buzzer OFF
 */
#define OUTPUT_GPIO   3

/* Internal power state for the RainMaker "Switch" */
static bool g_power_state = DEFAULT_POWER;

/* Local helper to actually drive the GPIO */
static void set_power_state(bool target)
{
    int level = target ? 0 : 1;   // true -> ON -> 0; false -> OFF -> 1
    gpio_set_level(OUTPUT_GPIO, level);
    ESP_LOGI(TAG, "Buzzer %s (GPIO%d level=%d)",
             target ? "ON" : "OFF", OUTPUT_GPIO, level);
}

/* Called from RainMaker write callback (app_main.c) */
esp_err_t app_driver_set_state(bool state)
{
    if (g_power_state != state) {
        g_power_state = state;
        set_power_state(g_power_state);
    }
    return ESP_OK;
}

/* Called from app_main.c (for OLED display / logic) */
bool app_driver_get_state(void)
{
    return g_power_state;
}

/* Init buzzer GPIO */
esp_err_t app_driver_init(void)
{
    ESP_LOGI(TAG, "Initialising buzzer on GPIO%d", OUTPUT_GPIO);

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << OUTPUT_GPIO),
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Start with DEFAULT_POWER (from app_priv.h, usually false)
    set_power_state(g_power_state);

    ESP_LOGI(TAG, "app_driver_init done");
    return ESP_OK;
}