#if ARDUINO_USB_MODE
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else
#include <WiFi.h>
#include <esp_wifi.h>

#include "FS.h"
#include "USB.h"
#include "USBMSC.h"
#include "structs.h"
#include "wifi_secret.h"

#if ARDUINO_USB_CDC_ON_BOOT
#define HWSerial Serial0
#else
#define HWSerial Serial
#endif

#ifdef DEBUG
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf((fmt), ##__VA_ARGS__)
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#define DEBUG_PRINT(x)
#endif

struct SettingsST {
  const char ssid[32] = WIFI_SSID;
  const char password[64] = WIFI_PASSWORD;
  const char server_ip[16] = "192.168.1.171";
  const int remote_port = 12345;
};

SettingsST settings;

USBMSC MSC;
WiFiClient client;

uint32_t DISK_SECTOR_COUNT = 0;
uint16_t DISK_SECTOR_SIZE = 512;
static const byte LED_PIN = 2;

init_answer answer = {};
sectors_buffer* g_buffer;

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer,
                       uint32_t bufsize) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  if (bufsize > BUFFER_SIZE) {
    DEBUG_PRINTF("Error write: bufsize: %u\n", bufsize);
  } else if (g_buffer->status == BUFFER_STATUS_NONE ||
             g_buffer->status == BUFFER_STATUS_DONE) {
    g_buffer->lba = lba;
    g_buffer->status = BUFFER_STATUS_NEED_REQUEST_WRITE;
    g_buffer->offset = offset;
    g_buffer->bufsize = bufsize;
    memcpy(g_buffer->buffer, buffer, bufsize);
  }

  while (g_buffer->status == BUFFER_STATUS_NEED_REQUEST_WRITE) {
    delay(1);
  }
  if (g_buffer->status == BUFFER_STATUS_DONE) {
    return bufsize;
  }
  return 0;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer,
                      uint32_t bufsize) {
  if (WiFi.status() != WL_CONNECTED) return 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  uint64_t pos = (uint64_t)lba * DISK_SECTOR_SIZE + offset;
  uint64_t buffer_pos =
      (uint64_t)g_buffer->lba * DISK_SECTOR_SIZE + g_buffer->offset;
  if (buffer_pos <= pos && buffer_pos + g_buffer->bufsize >= pos + bufsize) {
    if (g_buffer->status == BUFFER_STATUS_IN_REQUEST ||
        g_buffer->status == BUFFER_STATUS_NEED_REQUEST_READ) {
      delay(1);
    }
    if (g_buffer->status == BUFFER_STATUS_DONE) {
      memcpy(buffer, g_buffer->buffer + (pos - buffer_pos), bufsize);
      return bufsize;
    }
  }
  while (g_buffer->status == BUFFER_STATUS_IN_REQUEST ||
         g_buffer->status == BUFFER_STATUS_NEED_REQUEST_READ) {
    delay(1);
  }
  if (g_buffer->status == BUFFER_STATUS_NONE ||
      g_buffer->status == BUFFER_STATUS_ERROR ||
      g_buffer->status == BUFFER_STATUS_DONE) {
    g_buffer->lba = lba;
    g_buffer->status = BUFFER_STATUS_NEED_REQUEST_READ;
    g_buffer->bufsize = BUFFER_SIZE;
  }
  while (g_buffer->status == BUFFER_STATUS_IN_REQUEST ||
         g_buffer->status == BUFFER_STATUS_NEED_REQUEST_READ) {
    delay(1);
  }
  if (g_buffer->status == BUFFER_STATUS_DONE) {
    memcpy(buffer, g_buffer->buffer, bufsize);

    return bufsize;
  }

  return 0;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  HWSerial.printf("MSC START/STOP: power: %u, start: %u, eject: %u\r\n",
                  power_condition, start, load_eject);
  return true;
}

static void usbEventCallback(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t* data = (arduino_usb_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
        HWSerial.println(F("USB PLUGGED"));
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        HWSerial.println("USB UNPLUGGED");
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        HWSerial.printf("USB SUSPENDED: remote_wakeup_en: %u\r\n",
                        data->suspend.remote_wakeup_en);
        break;
      case ARDUINO_USB_RESUME_EVENT:
        HWSerial.println(F("USB RESUMED"));
        break;

      default:
        break;
    }
  }
}

void receiveInit() {
  remote_request request = {0, 0};
  if (client.write((uint8_t*)&request, sizeof(request)) != sizeof(request)) {
    client.stop();
    return;
  }
  client.flush();

  client.setTimeout(1000);  // ms
  if (client.readBytes((uint8_t*)&answer, sizeof(answer)) != sizeof(answer)) {
    DEBUG_PRINTLN("Init read failed");
    client.stop();
    return;
  }

  DISK_SECTOR_COUNT = answer.sectors_count;
  DISK_SECTOR_SIZE = answer.sector_size;

  DEBUG_PRINTF("Received drive sectors count: %u\n",
               (uint32_t)DISK_SECTOR_COUNT);
  DEBUG_PRINTF("Received drive sector size: %u\n", (uint32_t)DISK_SECTOR_SIZE);
}

void setup() {
  g_buffer = new sectors_buffer();

  DEBUG_PRINTLN(F("Starting..."));
  HWSerial.begin(115200);
  HWSerial.setDebugOutput(true);
  Serial.begin(115200);

  // while (!Serial) {;}
  uint32_t timeStart = millis();
  DEBUG_PRINT(F("Attempting to connect to SSID: "));
  DEBUG_PRINTLN(settings.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.begin(settings.ssid, settings.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    DEBUG_PRINT(".");
    if (millis() - timeStart > 10000) {
      break;
    }
  }
  DEBUG_PRINTLN(F("Connected to WiFi"));
  DEBUG_PRINT(F("SSID: "));
  DEBUG_PRINTLN(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  DEBUG_PRINT(F("IP Address: "));
  DEBUG_PRINTLN(ip);
  long rssi = WiFi.RSSI();
  DEBUG_PRINT(F("signal strength (RSSI):"));
  DEBUG_PRINT(rssi);
  DEBUG_PRINTLN(F(" dBm"));
  client.setNoDelay(true);
  while (!client.connected()) {
    if (client.connect(settings.server_ip, settings.remote_port)) {
      if (client.connected()) {
        DEBUG_PRINTLN(F("Connected to server"));
        receiveInit();
      }
    }
    if (client.connected() && DISK_SECTOR_COUNT != 0) {
      break;
    }
  }
  client.setNoDelay(true);
  USB.onEvent(usbEventCallback);
  MSC.vendorID("ESP32");         // max 8 chars
  MSC.productID("USB_MSC_MPZ");  // max 16 chars
  MSC.productRevision("1.0");    // max 4 chars
  MSC.onStartStop(onStartStop);
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.mediaPresent(true);
  MSC.begin(DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);
  USB.begin();
}

bool connected = true;
bool connected2 = true;

void loop() {
  while (!client.connected()) {
    if (connected) {
      connected = false;
      DEBUG_PRINTLN(F("Disconnected from server"));
    }
    if (client.connect(settings.server_ip, settings.remote_port)) {
      if (client.connected()) {
        DEBUG_PRINTLN(F("Connected to server"));
        g_buffer->status = BUFFER_STATUS_NONE;
        receiveInit();
        connected = true;
        break;
      }
    }
  }
  if (g_buffer->status == BUFFER_STATUS_NEED_REQUEST_READ) {
    remote_request request = {
        g_buffer->lba * DISK_SECTOR_SIZE + g_buffer->offset, g_buffer->bufsize};
    if (client.write((byte*)&request, sizeof(request)) != sizeof(request)) {
      client.stop();
    } else {
      client.flush();
      g_buffer->status = BUFFER_STATUS_IN_REQUEST;
      g_buffer->recv_offset = 0;
      g_buffer->recv_len = g_buffer->bufsize;
    }
  }
  if (g_buffer->status == BUFFER_STATUS_NEED_REQUEST_WRITE) {
    DEBUG_PRINTLN(F("Sending data to server2"));
    if (client.write((uint8_t*)g_buffer, sizeof(*g_buffer)) !=
        sizeof(*g_buffer)) {
      client.stop();
    } else {
      client.flush();
      g_buffer->status = BUFFER_STATUS_DONE;
    }
  }
  if (g_buffer->status == BUFFER_STATUS_IN_REQUEST) {
    uint32_t recv = client.readBytes(g_buffer->buffer + g_buffer->recv_offset,
                                     g_buffer->recv_len);
    if (recv == g_buffer->recv_len) {
      g_buffer->status = BUFFER_STATUS_DONE;
    } else if (recv < g_buffer->recv_len) {
      g_buffer->recv_len -= recv;
      g_buffer->recv_offset += recv;
    } else {
      g_buffer->status = BUFFER_STATUS_ERROR;
      DEBUG_PRINTLN(F("Error read"));
    }
  }
}
#endif