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

// UUIDs: You can generate new ones with any online UUID generator
#define CUSTOM_SERVICE_UUID_BASE {0x56, 0x34, 0x12, 0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12, 0x90, 0x78, 0x56, 0x34, 0x12}

#define CUSTOM_CHAR_WRITE_UUID_BASE {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0xA7, 0xB8, 0xC9, 0xDA, 0xEB, 0xFC, 0x01, 0x02, 0x03, 0x04}

#define CUSTOM_CHAR_READ_UUID_BASE {0xB1, 0xC2, 0xD3, 0xE4, 0xF5, 0x06, 0x17, 0x28, 0x39, 0x4A, 0x5B, 0x6C, 0x11, 0x12, 0x13, 0x14}

#define VOLUME_UP (0x00E9)     // Volume Up
#define VOLUME_DOWN (0x00EA)   // Volume Down
#define MUTE (0x00E2)          // Mute
#define PLAY_PAUSE (0x00CD)    // Play/Pause
#define SCAN_NEXT (0x00B5)     // Scan Next Track
#define SCAN_PREVIOUS (0x00B6) // Scan Previous Track
#define STOP (0x00B7)          // Stop

// Modifier Keys
#define KEY_MOD_NONE 0x00
#define KEY_MOD_LCTRL 0x01
#define KEY_MOD_LSHIFT 0x02
#define KEY_MOD_LALT 0x04
#define KEY_MOD_LGUI 0x08
#define KEY_MOD_RCTRL 0x10
#define KEY_MOD_RSHIFT 0x20
#define KEY_MOD_RALT 0x40
#define KEY_MOD_RGUI 0x80

// Letters
#define KEY_A 0x04
#define KEY_B 0x05
#define KEY_C 0x06
#define KEY_D 0x07
#define KEY_E 0x08
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_H 0x0B
#define KEY_I 0x0C
#define KEY_J 0x0D
#define KEY_K 0x0E
#define KEY_L 0x0F
#define KEY_M 0x10
#define KEY_N 0x11
#define KEY_O 0x12
#define KEY_P 0x13
#define KEY_Q 0x14
#define KEY_R 0x15
#define KEY_S 0x16
#define KEY_T 0x17
#define KEY_U 0x18
#define KEY_V 0x19
#define KEY_W 0x1A
#define KEY_X 0x1B
#define KEY_Y 0x1C
#define KEY_Z 0x1D

// Numbers
#define KEY_1 0x1E
#define KEY_2 0x1F
#define KEY_3 0x20
#define KEY_4 0x21
#define KEY_5 0x22
#define KEY_6 0x23
#define KEY_7 0x24
#define KEY_8 0x25
#define KEY_9 0x26
#define KEY_0 0x27

// Special Characters
#define KEY_ENTER 0x28
#define KEY_ESC 0x29
#define KEY_BACKSPACE 0x2A
#define KEY_TAB 0x2B
#define KEY_SPACE 0x2C
#define KEY_MINUS 0x2D
#define KEY_EQUAL 0x2E
#define KEY_LEFTBRACE 0x2F
#define KEY_RIGHTBRACE 0x30
#define KEY_BACKSLASH 0x31
#define KEY_SEMICOLON 0x33
#define KEY_APOSTROPHE 0x34
#define KEY_GRAVE 0x35
#define KEY_COMMA 0x36
#define KEY_DOT 0x37
#define KEY_SLASH 0x38

// Arrow Keys
#define KEY_RIGHT 0x4F
#define KEY_LEFT 0x50
#define KEY_DOWN 0x51
#define KEY_UP 0x52

// Function Keys
#define KEY_F1 0x3A
#define KEY_F2 0x3B
#define KEY_F3 0x3C
#define KEY_F4 0x3D
#define KEY_F5 0x3E
#define KEY_F6 0x3F
#define KEY_F7 0x40
#define KEY_F8 0x41
#define KEY_F9 0x42
#define KEY_F10 0x43
#define KEY_F11 0x44
#define KEY_F12 0x45

// Other useful keys
#define KEY_DELETE 0x4C
#define KEY_HOME 0x4A
#define KEY_END 0x4D
#define KEY_PAGEUP 0x4B
#define KEY_PAGEDOWN 0x4E

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
    // Consumer Control (Report ID 1)
    0x05, 0x0C, // Usage Page (Consumer Devices)
    0x09, 0x01, // Usage (Consumer Control)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x01, //   Report ID (1)
    0x15, 0x00,
    0x26, 0xFF, 0x03, //   Logical Maximum (0x03FF)
    0x19, 0x00,
    0x2A, 0xFF, 0x03, //   Usage Maximum
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x00,       //   Input (Data, Array)
    0xC0,             // End Collection

    // Keyboard (Report ID 3)
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x03, //   Report ID (3)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0xE0, //   Usage Minimum (224)
    0x29, 0xE7, //   Usage Maximum (231)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data, Var, Abs)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x03, //   Input (Const, Var, Abs)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x65, //   Logical Maximum (101)
    0x05, 0x07, //   Usage Page (Key Codes)
    0x19, 0x00, //   Usage Minimum (0)
    0x29, 0x65, //   Usage Maximum (101)
    0x81, 0x00, //   Input (Data, Array)
    0xC0,       // End Collection

    // Mouse (Report ID 2)
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x85, 0x02, //   Report ID (2)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Buttons)
    0x19, 0x01, //     Usage Minimum (Button 1)
    0x29, 0x03, //     Usage Maximum (Button 3)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data, Var, Abs)
    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x03, //     Input (Const, Var, Abs) – padding
    0x05, 0x01, //     Usage Page (Generic Desktop)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x06, //     Input (Data, Var, Rel)
    0xC0,       //   End Collection
    0xC0        // End Collection
};

/* ───────────────────────── Globals ─────────────────────────────── */
static esp_hidd_dev_t *hid_dev;

/* ───────────────────────── Helpers ─────────────────────────────── */
static void send_consumer(uint16_t usage)
{
    uint8_t rpt[2] = {usage & 0xFF, usage >> 8}; // Little endian

    // Send key press
    esp_hidd_dev_input_set(hid_dev, 0, 1, rpt, sizeof(rpt));
    vTaskDelay(pdMS_TO_TICKS(30));

    // Send key release
    rpt[0] = rpt[1] = 0;
    esp_hidd_dev_input_set(hid_dev, 0, 1, rpt, sizeof(rpt));
}

void send_key(uint8_t modifier, uint8_t keycode)
{
    uint8_t report[8] = {0};

    report[0] = modifier; // Modifier (e.g., SHIFT)
    report[2] = keycode;  // Keycode (e.g., G = 0x0A)

    // Press
    esp_hidd_dev_input_set(hid_dev, 0, 3, report, sizeof(report));
    vTaskDelay(pdMS_TO_TICKS(20));

    // Release
    memset(report, 0, sizeof(report));
    esp_hidd_dev_input_set(hid_dev, 0, 3, report, sizeof(report));
    vTaskDelay(pdMS_TO_TICKS(10)); // delay again
}

void send_mouse(int8_t dx, int8_t dy, uint8_t buttons)
{
    uint8_t mouse_report[4] = {
        0x02,    // Report ID (2)
        buttons, // Button bits
        dx,      // X delta
        dy       // Y delta
    };

    esp_hidd_dev_input_set(hid_dev, 0, mouse_report[0], &mouse_report[1], 3);
}

void ble_hid_task(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "Demo Task");

        esp_hidd_dev_battery_set(hid_dev, 10);
        // send_consumer(VOLUME_DOWN);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        // // send_consumer(VOLUME_UP);
        // send_mouse(10, 0, 0x01); // Move mouse right with left click
        // vTaskDelay(pdMS_TO_TICKS(20));
        // send_mouse(0, 0, 0x00); // Release button
    }
}

void ble_hid_task_start_up(void)
{
    if (s_ble_hid_param.task_hdl)
    {
        // Task already exists
        return;
    }

    xTaskCreate(ble_hid_task, "ble_hid_task", 2 * 4096, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);
}

/* HID / GAP events: start advertising, spawn task on connect */
static void hid_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case ESP_HIDD_START_EVENT:
        esp_hid_ble_gap_adv_start();
        break;
    case ESP_HIDD_CONNECT_EVENT:
        esp_hid_ble_gap_adv_start();
        // xTaskCreate(ble_hid_task, "vol", 4096, NULL, 5, NULL);
        // ble_hid_task_start_up();
        break;
    case ESP_HIDD_DISCONNECT_EVENT:
        esp_hid_ble_gap_adv_start();
        break;
    default:
        break;
    }
}

/* NimBLE host task (required) */
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host running");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static int custom_write_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    uint8_t buffer[64] = {0};

    printf("---------------------------------------------------------\n");

    int rc = ble_hs_mbuf_to_flat(ctxt->om, buffer, sizeof(buffer), NULL);
    if (rc != 0 || len == 0)
    {
        ESP_LOGW(TAG, "Failed to parse mbuf or empty write.");
        return 0;
    }

    ESP_LOGI(TAG, "Custom Write received (%d bytes): %.*s", len, len, buffer);

    // Convert to null-terminated string
    buffer[len] = '\0';

    // Basic command parsing
    if (strncmp((char *)buffer, "volup", 5) == 0)
    {
        send_consumer(VOLUME_UP);
        send_key(KEY_MOD_LSHIFT, KEY_G); // Sends 'G'
    }
    else if (strncmp((char *)buffer, "voldown", 7) == 0)
    {
        ESP_LOGI(TAG, "Command parsed");
        send_consumer(VOLUME_DOWN);
        send_key(KEY_MOD_NONE, KEY_G); // Sends 'g'
    }
    else if (strncmp((char *)buffer, "mute", 4) == 0)
    {
        send_consumer(MUTE);
        send_key(KEY_MOD_NONE, KEY_ENTER); // Sends Enter key
    }
    else if (strncmp((char *)buffer, "play", 4) == 0)
    {
        send_consumer(PLAY_PAUSE);
    }
    else if (strncmp((char *)buffer, "next", 4) == 0)
    {
        send_consumer(SCAN_NEXT);
    }
    else if (strncmp((char *)buffer, "prev", 4) == 0)
    {
        send_consumer(SCAN_PREVIOUS);
    }
    else if (strncmp((char *)buffer, "stop", 4) == 0)
    {
        ESP_LOGI(TAG, "Command parsed");
        send_consumer(STOP);
    }
    else if (strncmp((char *)buffer, "move", 4) == 0)
    {
        int dx = 0, dy = 0;
        if (sscanf((char *)buffer + 5, "%d %d", &dx, &dy) == 2)
        {
            send_mouse((int8_t)dx, (int8_t)dy, 0);
        }
        else
        {
            ESP_LOGW(TAG, "Invalid format for 'move' command. Use: move <dx> <dy>");
        }
    }
    else if (strncmp((char *)buffer, "click", 5) == 0)
    {
        send_mouse(0, 0, 0x01); // Left click down
        vTaskDelay(pdMS_TO_TICKS(20));
        send_mouse(0, 0, 0x00); // Release
    }
    else if (strncmp((char *)buffer, "rightclick", 5) == 0)
    {
        send_mouse(0, 0, 0x02); // Right click down
        vTaskDelay(pdMS_TO_TICKS(20));
        send_mouse(0, 0, 0x00); // Release
    }
    else
    {
        ESP_LOGI(TAG, "Typing string: %s", buffer);

        for (int i = 0; buffer[i] != '\0'; i++)
        {
            char ch = buffer[i];
            uint8_t keycode = 0;
            uint8_t modifier = KEY_MOD_NONE;

            if (ch >= 'A' && ch <= 'Z')
            {
                keycode = KEY_A + (ch - 'A');
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch >= 'a' && ch <= 'z')
            {
                keycode = KEY_A + (ch - 'a');
            }
            else if (ch >= '1' && ch <= '9')
            {
                keycode = KEY_1 + (ch - '1');
            }
            else if (ch == '0')
            {
                keycode = KEY_0;
            }
            else if (ch == ' ')
            {
                keycode = KEY_SPACE;
            }
            else if (ch == '\n' || ch == '\r')
            {
                keycode = KEY_ENTER;
            }
            else if (ch == '\t')
            {
                keycode = KEY_TAB;
            }
            // Punctuation & Symbols
            else if (ch == '!')
            {
                keycode = KEY_1;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '@')
            {
                keycode = KEY_2;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '#')
            {
                keycode = KEY_3;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '$')
            {
                keycode = KEY_4;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '%')
            {
                keycode = KEY_5;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '^')
            {
                keycode = KEY_6;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '&')
            {
                keycode = KEY_7;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '*')
            {
                keycode = KEY_8;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '(')
            {
                keycode = KEY_9;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == ')')
            {
                keycode = KEY_0;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '-')
            {
                keycode = KEY_MINUS;
            }
            else if (ch == '_')
            {
                keycode = KEY_MINUS;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '=')
            {
                keycode = KEY_EQUAL;
            }
            else if (ch == '+')
            {
                keycode = KEY_EQUAL;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '[')
            {
                keycode = KEY_LEFTBRACE;
            }
            else if (ch == '{')
            {
                keycode = KEY_LEFTBRACE;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == ']')
            {
                keycode = KEY_RIGHTBRACE;
            }
            else if (ch == '}')
            {
                keycode = KEY_RIGHTBRACE;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '\\')
            {
                keycode = KEY_BACKSLASH;
            }
            else if (ch == '|')
            {
                keycode = KEY_BACKSLASH;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == ';')
            {
                keycode = KEY_SEMICOLON;
            }
            else if (ch == ':')
            {
                keycode = KEY_SEMICOLON;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '\'')
            {
                keycode = KEY_APOSTROPHE;
            }
            else if (ch == '"')
            {
                keycode = KEY_APOSTROPHE;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == ',')
            {
                keycode = KEY_COMMA;
            }
            else if (ch == '<')
            {
                keycode = KEY_COMMA;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '.')
            {
                keycode = KEY_DOT;
            }
            else if (ch == '>')
            {
                keycode = KEY_DOT;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '/')
            {
                keycode = KEY_SLASH;
            }
            else if (ch == '?')
            {
                keycode = KEY_SLASH;
                modifier = KEY_MOD_LSHIFT;
            }
            else if (ch == '`')
            {
                keycode = KEY_GRAVE;
            }
            else if (ch == '~')
            {
                keycode = KEY_GRAVE;
                modifier = KEY_MOD_LSHIFT;
            }
            else
            {
                ESP_LOGW(TAG, "Unsupported char: %c", ch);
                continue;
            }

            send_key(modifier, keycode);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    return 0;
}

static int custom_read_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *response = "Hello from ESP32!";
    os_mbuf_append(ctxt->om, response, strlen(response));
    return 0;
}
static const struct ble_gatt_svc_def gatt_custom_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID128_DECLARE(CUSTOM_SERVICE_UUID_BASE),
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             .uuid = BLE_UUID128_DECLARE(CUSTOM_CHAR_WRITE_UUID_BASE),
             .access_cb = custom_write_cb,
             .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,

         },
         // {
         //     .uuid = BLE_UUID128_DECLARE(CUSTOM_CHAR_READ_UUID_BASE),
         //     .access_cb = custom_read_cb,
         //     .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
         // },
         {0} // End
     }},
    {0} // End
};

/* ───────────────────────── app_main ────────────────────────────── */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_hid_gap_init(ESP_HID_TRANSPORT_BLE));

    /* Advertise as a generic HID */
    ESP_ERROR_CHECK(esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_GENERIC,
                                             "Azmuth"));

    /* Device-level configuration */
    esp_hid_raw_report_map_t map = {.data = consumer_map,
                                    .len = sizeof(consumer_map)};

    esp_hid_device_config_t cfg = {
        .vendor_id = 0x16C0,
        .product_id = 0x05DF,
        .version = 0x0100,
        .device_name = "Azmuth",
        .manufacturer_name = "BitForge",
        .serial_number = "123456",
        .report_maps = &map,
        .report_maps_len = 1};

    ESP_LOGI(TAG, "Registering custom GATT service...");
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_custom_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_custom_svcs));
    ESP_LOGI(TAG, "Custom GATT service registered.");

    ESP_ERROR_CHECK(esp_hidd_dev_init(&cfg,
                                      ESP_HID_TRANSPORT_BLE,
                                      hid_cb, &hid_dev));

    /* Start the NimBLE stack */
    extern void ble_store_config_init(void); /* IDF helper */
    ble_store_config_init();
    ESP_ERROR_CHECK(esp_nimble_enable(ble_host_task));
}
