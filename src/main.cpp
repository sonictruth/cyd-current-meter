#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <ui/ui.h>
#include <Adafruit_INA219.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// FIX I2C
// .pio/libdeps/esp32-2432S032C/Adafruit BusIO/Adafruit_I2CDevice.cpp
// _wire->beginTransmission(0x00);
// _wire->endTransmission();
//
// FIX spi bus
//  const spi_bus_config_t spi_bus_config = {
//      .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT,
// .pio/libdeps/esp32-2432S032C/esp32_smartdisplay/src/lvgl_st7789.c

#define IS_DEMO ;
typedef struct
{
  unsigned long time;
  unsigned short current;
} sensor_readings_t;

// Main sensor readings
#define maximum_readings 4000
sensor_readings_t readings[maximum_readings]; // RTC_DATA_ATTR
int sensor_reading_index = 0;
unsigned short miliAmps_last = 0;
const unsigned short tolerance_miliAmps = 10;

// BLE
#define SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"
#define SENSOR_CHARACTERISTIC_UUID "19b10001-e8f2-537e-4f6c-d104768a1214"

BLEServer *ble_server = NULL;
BLECharacteristic *ble_characteristic = NULL;

bool ble_device_connected = false;
bool ble_device_connected_old = false;

lv_chart_series_t *lv_chart_series;

const int I2Cbus = 1;
TwoWire I2Ctwo = TwoWire(I2Cbus);

const float backlight_default_brightness = 0.2;

const int chart_tick_count = 15;
const int chart_range_max = 3500;
const int chart_range_max_zoomed = 350;
const int chart_num_points = 60;

bool is_zoomed = true;
bool is_paused = true;

static lv_coord_t chart_coord[chart_num_points] = {0};

static const unsigned long refresh_interval_ms = 1000;
static unsigned long last_refresh = 0;

Adafruit_INA219 ina219;

void set_status(String status)
{
  Serial.println(status);
  lv_label_set_text(ui_StatusLabel, status.c_str());
}
class ble_server_callbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    ble_device_connected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    ble_device_connected = false;
  }
};

void ble_setup()
{

  BLEDevice::init("CYD_CURRENT_METER");

  ble_server = BLEDevice::createServer();
  ble_server->setCallbacks(new ble_server_callbacks());

  BLEService *pService = ble_server->createService(SERVICE_UUID);

  ble_characteristic = pService->createCharacteristic(
      SENSOR_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  ble_characteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  set_status("Waiting conn...");
}

void ble_loop()
{
  if (!ble_device_connected && ble_device_connected_old)
  {
    set_status("Device disconnected.");
    delay(500);
    ble_server->startAdvertising();
    set_status("Start advertising");
    ble_device_connected_old = ble_device_connected;
  }
  if (ble_device_connected && !ble_device_connected_old)
  {
    ble_device_connected_old = ble_device_connected;
    set_status("Device Sync");
    /*
    for (int i = 0; i < sensor_reading_index; i++)
    {
      ble_characteristic->setValue((uint8_t *)&readings[i], sizeof(sensor_readings_t));
      ble_characteristic->notify();
      Serial.printf("Syncing size: %d", sizeof(sensor_readings_t));
      delay(50);
    }
    */
    set_status("Sync done");
  }
  else if (ble_device_connected)
  {
    Serial.println("Sync");
    ble_characteristic->setValue((uint8_t *)&readings[sensor_reading_index], sizeof(sensor_readings_t));
    ble_characteristic->notify();
  }
}

void clear_readings()
{
  for (int i = 0; i < maximum_readings; i++)
  {
    readings[i].time = 0;
    readings[i].current = 0;
  }
}

void update_chart(short new_point)
{
  for (int j = chart_num_points - 1; j > 0; --j)
  {
    chart_coord[j] = chart_coord[j - 1];
  }
  chart_coord[0] = new_point;
  lv_chart_set_ext_y_array(ui_Chart1, lv_chart_series, chart_coord);
}

void update_sensor_reading()
{
  unsigned short miliAmps;
#if defined(IS_DEMO)
  miliAmps = random(100, 300);
#else
  miliAmps = int(abs(ina219.getmiliAmps()));
#endif

  int lower_bound = miliAmps_last - tolerance_miliAmps;
  int upper_bound = miliAmps_last + tolerance_miliAmps;

  if (miliAmps < lower_bound || miliAmps > upper_bound)
  {
    readings[sensor_reading_index].time = millis();
    readings[sensor_reading_index].current = miliAmps;
    sensor_reading_index = (sensor_reading_index + 1);
    if (sensor_reading_index >= maximum_readings)
    {
      sensor_reading_index = 0;
    }
  }
  update_chart(miliAmps);
  set_status(String(miliAmps) + "mA");
}

void set_zoom()
{
  int range;
  if (is_zoomed)
  {
    range = chart_range_max_zoomed;
    lv_label_set_text(ui_ZoomLabel, "-");
  }
  else
  {
    range = chart_range_max;
    lv_label_set_text(ui_ZoomLabel, "+");
  }
  lv_chart_set_axis_tick(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 10, 5, chart_tick_count, 2, true, 50);
  lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, range);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting...");

  smartdisplay_init();
  ui_init();

  smartdisplay_lcd_set_backlight(backlight_default_brightness);
  smartdisplay_led_set_rgb(false, false, false);

#if defined(IS_DEMO)
#else
  I2Ctwo.setPins(21, 22);
  if (!ina219.begin(&I2Ctwo))
  {
    set_status("Failed to find INA219");
  }
  else
  {
    set_status("Found INA219");
  }
#endif
  clear_readings();
  lv_chart_series = lv_chart_add_series(ui_Chart1, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  set_zoom();
  toggle(nullptr);
  ble_setup();
}

void toggle(lv_event_t *e)
{
  is_paused = !is_paused;
  if (is_paused)
  {
    lv_label_set_text(ui_ToggleLabel, "Start");
  }
  else
  {
    lv_label_set_text(ui_ToggleLabel, "Stop");
  }
}

void zoom(lv_event_t *e)
{
  is_zoomed = !is_zoomed;
  set_zoom();
}

void loop()
{
  if (millis() - last_refresh >= refresh_interval_ms)
  {
    last_refresh += refresh_interval_ms;
    if (!is_paused)
    {
      update_sensor_reading();
    }
    ble_loop();
  }
  lv_timer_handler();
}
