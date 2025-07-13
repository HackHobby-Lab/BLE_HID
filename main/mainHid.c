/*  Minimal BLE HID Consumer-Control demo for ESP32-S3 (NimBLE)
 *  Sends VOL-UP / VOL-DOWN alternately every 2 s.
 *  Build: idf.py menuconfig → Component-config → Bluetooth → NimBLE
 */
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_hidd.h"
#include "esp_hid_gap.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"

static const char *TAG = "HID_MIN";

#define VOLUME_UP       (0x00E9)    // Volume Up
#define VOLUME_DOWN     (0x00EA)    // Volume Down
#define MUTE            (0x00E2)    // Mute
#define PLAY_PAUSE      (0x00CD)    // Play/Pause
#define SCAN_NEXT       (0x00B5)    // Scan Next Track
#define SCAN_PREVIOUS   (0x00B6)    // Scan Previous Track
#define STOP            (0x00B7)    // Stop


typedef struct
{
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    uint8_t *buffer;
} local_param_t;

static local_param_t s_ble_hid_param = {0};


/* ───────────────────────── Report Map (Consumer-Control) ──────────────────── */
static const uint8_t consumer_map[] = {
    0x05, 0x0C,                    // Usage Page (Consumer Devices)
    0x09, 0x01,                    // Usage (Consumer Control)
    0xA1, 0x01,                    // Collection (Application)
    0x85, 0x01,                    //   Report ID (1)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x26, 0xFF, 0x03,              //   Logical Maximum (0x03FF)
    0x19, 0x00,                    //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,              //   Usage Maximum (0x03FF)
    0x75, 0x10,                    //   Report Size (16 bits)
    0x95, 0x01,                    //   Report Count (1)
    0x81, 0x00,                    //   Input (Data, Array)
    0xC0                           // End Collection
};


/* ───────────────────────── Globals ─────────────────────────────── */
static esp_hidd_dev_t *hid_dev;

/* ───────────────────────── Helpers ─────────────────────────────── */
static void send_consumer(uint16_t usage)
{
    uint8_t rpt[2] = { usage & 0xFF, usage >> 8 };  // Little endian

    // Send key press
    esp_hidd_dev_input_set(hid_dev, 0, 1, rpt, sizeof(rpt));
    vTaskDelay(pdMS_TO_TICKS(30));

    // Send key release
    rpt[0] = rpt[1] = 0;
    esp_hidd_dev_input_set(hid_dev, 0, 1, rpt, sizeof(rpt));
}

void ble_hid_demo_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "Demo Task");

        esp_hidd_dev_battery_set(hid_dev, 10);
        send_consumer(VOLUME_DOWN);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        send_consumer(VOLUME_UP);

    }
}


void ble_hid_task_start_up(void)
{
    if (s_ble_hid_param.task_hdl) {
        // Task already exists
        return;
    }

     xTaskCreate(ble_hid_demo_task, "ble_hid_demo_task", 2 * 4096, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);


}


/* HID / GAP events: start advertising, spawn task on connect */
static void hid_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id) {
    case ESP_HIDD_START_EVENT:
        esp_hid_ble_gap_adv_start();                            break;
    case ESP_HIDD_CONNECT_EVENT:
        // xTaskCreate(ble_hid_demo_task, "vol", 4096, NULL, 5, NULL); 
        ble_hid_task_start_up();                                break;
    case ESP_HIDD_DISCONNECT_EVENT:
        esp_hid_ble_gap_adv_start();                            break;
    default:                                                    break;
    }
}

/* NimBLE host task (required) */
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host running");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ───────────────────────── app_main ────────────────────────────── */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_hid_gap_init(ESP_HID_TRANSPORT_BLE));

    /* Advertise as a generic HID */
    ESP_ERROR_CHECK(esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_GENERIC,
                                             "Azmuth"));

    /* Device-level configuration */
    esp_hid_raw_report_map_t map = { .data = consumer_map,
                                     .len  = sizeof(consumer_map) };

    esp_hid_device_config_t cfg  = {
        .vendor_id         = 0x16C0,
        .product_id        = 0x05DF,
        .version           = 0x0100,
        .device_name       = "Azmuth",
        .manufacturer_name = "BitForge",
        .serial_number     = "123456",
        .report_maps       = &map,
        .report_maps_len   = 1
    };

    ESP_ERROR_CHECK(esp_hidd_dev_init(&cfg,
                                      ESP_HID_TRANSPORT_BLE,
                                      hid_cb, &hid_dev));

    /* Start the NimBLE stack */
    extern void ble_store_config_init(void);  /* IDF helper */
    ble_store_config_init();
    ESP_ERROR_CHECK(esp_nimble_enable(ble_host_task));
}
