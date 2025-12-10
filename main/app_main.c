// app_main.c  -- Restructured with FreeRTOS Tasks for Project Requirements

#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "driver/i2c.h"

#include "esp_rmaker_core.h"
#include "esp_rmaker_standard_types.h"
#include "esp_rmaker_standard_params.h"
#include "esp_rmaker_utils.h"

#include "app_wifi.h"
#include "app_priv.h"

#include "dht.h"
#include "ssd1306.h"

static const char *TAG = "app_main";

/* RainMaker device */
esp_rmaker_device_t *switch_device = NULL;

/* RainMaker parameters */
static esp_rmaker_param_t *temp_param = NULL;
static esp_rmaker_param_t *hum_param  = NULL;
static esp_rmaker_param_t *power_param = NULL;

/* ====== Hardware Pins ====== */
#define DHT_GPIO        2
#define DHT_TYPE        DHT_TYPE_DHT11

#define I2C_MASTER_NUM  0
#define I2C_SDA_GPIO    6
#define I2C_SCL_GPIO    7
#define I2C_FREQ_HZ     400000

#define OLED_ADDR       0x3C

/* Thresholds for alerts */
#define TEMP_HIGH_THRESHOLD  35.0f   // °C
#define HUM_LOW_THRESHOLD    40.0f   // %

/* SSD1306 handle */
static ssd1306_handle_t ssd1306_dev = NULL;

/* ====== Shared Data (protected by mutex) ====== */
static SemaphoreHandle_t sensor_mutex = NULL;

typedef struct {
    float temperature;
    float humidity;
    bool valid;
} sensor_data_t;

static sensor_data_t current_sensor_data = {0};

/* ====== I2C + OLED Initialization ====== */

static void i2c_oled_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C for OLED: SDA=%d, SCL=%d", I2C_SDA_GPIO, I2C_SCL_GPIO);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, OLED_ADDR);
    if (!ssd1306_dev) {
        ESP_LOGE(TAG, "ssd1306_create failed");
        return;
    }
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    ssd1306_refresh_gram(ssd1306_dev);
    ssd1306_draw_string(ssd1306_dev, 0, 0,
                        (const uint8_t *)"Smart Plant", 12, 1);
    ssd1306_refresh_gram(ssd1306_dev);
}

static void oled_show_readings(float temperature, float humidity, bool buzzer_on, const char *status)
{
    if (!ssd1306_dev) {
        return;
    }
    char line1[24];
    char line2[24];
    char line3[24];
    char line4[24];

    snprintf(line1, sizeof(line1), "T:%.1fC H:%.1f%%", temperature, humidity);
    snprintf(line2, sizeof(line2), "Pump: %s", buzzer_on ? "ON" : "OFF");
    snprintf(line3, sizeof(line3), "%s", status);

    ssd1306_clear_screen(ssd1306_dev, 0x00);
    ssd1306_draw_string(ssd1306_dev, 0, 0,  (uint8_t *)line1, 12, 1);
    ssd1306_draw_string(ssd1306_dev, 0, 16, (uint8_t *)line2, 12, 1);
    ssd1306_draw_string(ssd1306_dev, 0, 32, (uint8_t *)line3, 12, 1);
    ssd1306_refresh_gram(ssd1306_dev);
}

/* ====== TASK 1: Sensor Reading Task ====== */
/* Reads DHT11 sensor periodically */

static void sensor_task(void *arg)
{
    ESP_LOGI(TAG, "TASK 1: Sensor Reading Task started");

    while (1) {
        int16_t temperature = 0;
        int16_t humidity    = 0;

        esp_err_t res = dht_read_data(DHT_TYPE, DHT_GPIO, &humidity, &temperature);
        
        if (res == ESP_OK) {
            // Convert raw values (DHT11 returns values * 10)
            float t = (float)temperature / 10.0f;
            float h = (float)humidity / 10.0f;

            // Update shared data with mutex protection
            if (xSemaphoreTake(sensor_mutex, portMAX_DELAY)) {
                current_sensor_data.temperature = t;
                current_sensor_data.humidity = h;
                current_sensor_data.valid = true;
                xSemaphoreGive(sensor_mutex);
            }

            ESP_LOGI(TAG, "DHT11: Temp=%.1f°C, Humidity=%.1f%%", t, h);
        } else {
            ESP_LOGW(TAG, "DHT read error: %s", esp_err_to_name(res));
            
            if (xSemaphoreTake(sensor_mutex, portMAX_DELAY)) {
                current_sensor_data.valid = false;
                xSemaphoreGive(sensor_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));   // Read every 3 seconds
    }
}

/* ====== TASK 2: Cloud Communication Task ====== */
/* Updates RainMaker cloud with sensor data */

static void cloud_task(void *arg)
{
    ESP_LOGI(TAG, "TASK 2: Cloud Communication Task started");

    // Wait for RainMaker to be ready
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        sensor_data_t data;
        
        // Get sensor data safely
        if (xSemaphoreTake(sensor_mutex, portMAX_DELAY)) {
            data = current_sensor_data;
            xSemaphoreGive(sensor_mutex);
        }

        if (data.valid) {
            // Update temperature parameter
            if (temp_param) {
                esp_rmaker_param_update_and_report(temp_param, 
                    esp_rmaker_float(data.temperature));
            }

            // Update humidity parameter
            if (hum_param) {
                esp_rmaker_param_update_and_report(hum_param, 
                    esp_rmaker_float(data.humidity));
            }

            ESP_LOGI(TAG, "Cloud updated: T=%.1f°C, H=%.1f%%", 
                     data.temperature, data.humidity);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));   // Update cloud every 5 seconds
    }
}

/* ====== TASK 3: Alert & Notification Task ====== */
/* Checks thresholds and sends push notifications */

static void alert_task(void *arg)
{
    ESP_LOGI(TAG, "TASK 3: Alert & Notification Task started");

    // Wait for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(10000));

    bool last_alert_state = false;

    while (1) {
        sensor_data_t data;
        bool alert_needed = false;
        char alert_msg[64] = {0};
        
        // Get sensor data safely
        if (xSemaphoreTake(sensor_mutex, portMAX_DELAY)) {
            data = current_sensor_data;
            xSemaphoreGive(sensor_mutex);
        }

        if (data.valid) {
            // Check if temperature is too high
            bool temp_high = (data.temperature >= TEMP_HIGH_THRESHOLD);
            
            // Check if humidity is too low
            bool hum_low = (data.humidity <= HUM_LOW_THRESHOLD);

            if (temp_high && hum_low) {
                alert_needed = true;
                snprintf(alert_msg, sizeof(alert_msg), 
                         "Too hot & too dry! T=%.1f°C H=%.1f%%", 
                         data.temperature, data.humidity);
            } else if (temp_high) {
                alert_needed = true;
                snprintf(alert_msg, sizeof(alert_msg), 
                         "Temperature too high! %.1f°C", 
                         data.temperature);
            } else if (hum_low) {
                alert_needed = true;
                snprintf(alert_msg, sizeof(alert_msg), 
                         "Humidity too low! %.1f%%", 
                         data.humidity);
            }

            // Send notification only when state changes (avoid spam)
            if (alert_needed && !last_alert_state) {
                ESP_LOGW(TAG, "ALERT: %s", alert_msg);
                
                // Send push notification via RainMaker
                esp_rmaker_raise_alert(alert_msg);
                
                last_alert_state = true;
            } else if (!alert_needed && last_alert_state) {
                ESP_LOGI(TAG, "Conditions normal again");
                last_alert_state = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

/* ====== TASK 4: Display Update Task ====== */
/* Updates OLED display with current status */

static void display_task(void *arg)
{
    ESP_LOGI(TAG, "TASK 4: Display Update Task started");

    while (1) {
        sensor_data_t data;
        char status[24] = "Initializing...";
        
        // Get sensor data safely
        if (xSemaphoreTake(sensor_mutex, portMAX_DELAY)) {
            data = current_sensor_data;
            xSemaphoreGive(sensor_mutex);
        }

        if (data.valid) {
            bool temp_high = (data.temperature >= TEMP_HIGH_THRESHOLD);
            bool hum_low = (data.humidity <= HUM_LOW_THRESHOLD);

            if (temp_high && hum_low) {
                snprintf(status, sizeof(status), "Too hot & dry!");
            } else if (temp_high) {
                snprintf(status, sizeof(status), "Temp too high!");
            } else if (hum_low) {
                snprintf(status, sizeof(status), "Humidity low!");
            } else {
                snprintf(status, sizeof(status), "Plant is OK :)");
            }

            bool buzzer_state = app_driver_get_state();
            oled_show_readings(data.temperature, data.humidity, buzzer_state, status);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Update display every second
    }
}

/* ====== RainMaker Write Callback ====== */

static esp_err_t write_cb(const esp_rmaker_device_t *device,
                          const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val,
                          void *priv_data,
                          esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Write via: %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *param_name = esp_rmaker_param_get_name(param);

    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        bool new_state = val.val.b;
        ESP_LOGI(TAG, "Pump control -> %s", new_state ? "ON" : "OFF");

        app_driver_set_state(new_state);
        esp_rmaker_param_update_and_report(param, val);
        return ESP_OK;
    }

    return ESP_OK;
}

/* ====== Main Function ====== */

void app_main(void)
{
    ESP_LOGI(TAG, "=== Smart Plant Watering System Starting ===");

    /* 1) Initialize buzzer driver */
    app_driver_init();

    /* 2) Initialize NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* 3) Create mutex for sensor data */
    sensor_mutex = xSemaphoreCreateMutex();
    if (!sensor_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex!");
        abort();
    }

    /* 4) Initialize Wi-Fi */
    app_wifi_init();

    /* 5) Initialize RainMaker node */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg,
                                                     "ESP RainMaker Device",
                                                     "Smart Plant");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialize node. Aborting!");
        vTaskDelay(pdMS_TO_TICKS(5000));
        abort();
    }

    /* 6) Create Switch device (acts as pump control) */
    switch_device = esp_rmaker_device_create("Smart Plant",
                                             ESP_RMAKER_DEVICE_SWITCH,
                                             NULL);

    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);

    /* Add name parameter */
    esp_rmaker_device_add_param(switch_device,
        esp_rmaker_name_param_create("name", "Smart Plant"));

    /* Add power parameter (pump control) */
    power_param = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_POWER);
    esp_rmaker_device_add_param(switch_device, power_param);
    esp_rmaker_device_assign_primary_param(switch_device, power_param);

    /* Add temperature parameter (read-only) */
    temp_param = esp_rmaker_temperature_param_create("temperature", 0.0f);
    esp_rmaker_device_add_param(switch_device, temp_param);

    /* Add humidity parameter (read-only) */
    hum_param = esp_rmaker_param_create("humidity",
                                        "esp.param.humidity",
                                        esp_rmaker_float(0.0f),
                                        PROP_FLAG_READ);
    esp_rmaker_device_add_param(switch_device, hum_param);

    /* Add device to node */
    esp_rmaker_node_add_device(node, switch_device);

    /* 7) Enable RainMaker insights (for dashboard) */
    esp_rmaker_node_add_attribute(node, "serial_num", "123456");
    
    /* 8) Start RainMaker */
    ESP_ERROR_CHECK(esp_rmaker_start());

    /* 9) Start Wi-Fi provisioning */
    app_wifi_start(POP_TYPE_RANDOM);

    /* 10) Initialize I2C and OLED */
    i2c_oled_init();

    /* 11) Create FreeRTOS Tasks */
    ESP_LOGI(TAG, "Creating FreeRTOS tasks...");

    xTaskCreate(sensor_task, "Sensor_Task", 4096, NULL, 5, NULL);
    xTaskCreate(cloud_task, "Cloud_Task", 4096, NULL, 4, NULL);
    xTaskCreate(alert_task, "Alert_Task", 4096, NULL, 3, NULL);
    xTaskCreate(display_task, "Display_Task", 3072, NULL, 2, NULL);

    ESP_LOGI(TAG, "=== All tasks created. System running. ===");
}