#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <ui/ui.h>
#include <Adafruit_INA219.h>
#include <Wire.h>
lv_chart_series_t *lv_chart_series;

const int I2Cbus = 1;
TwoWire I2Ctwo = TwoWire(I2Cbus);

const float backlight_default_brightness = 0.2;

const int tick_count = 15;
const int range_max = 3500;
const int range_max_zoomed = 350;
const int num_points = 60;

bool is_zoomed = true;
bool is_paused = true;

static lv_coord_t coord[num_points] = {0};

static const unsigned long refresh_interval_ms = 1000;
static unsigned long last_refresh = 0;

Adafruit_INA219 ina219;

// #define IS_DEMO ;

void set_status(String status)
{
  lv_label_set_text(ui_StatusLabel, status.c_str());
}

void add_point(int new_point)
{
  if (num_points > 1)
  {
    for (int i = num_points - 1; i > 0; --i)
    {
      coord[i] = coord[i - 1];
    }
    coord[0] = new_point;
  }
  set_status(String(new_point) + "mA");
}

void update_chart()
{
#if defined(IS_DEMO)
  int current_mA = random(200, 300);

#else
  int current_mA = int(abs(ina219.getCurrent_mA()));
#endif
  Serial.println(current_mA);
  add_point(current_mA);
  lv_chart_set_ext_y_array(ui_Chart1, lv_chart_series, coord);
}

void set_zoom()
{
  int range;
  if (is_zoomed)
  {
    range = range_max_zoomed;
    lv_label_set_text(ui_ZoomLabel, "-");
  }
  else
  {
    range = range_max;
    lv_label_set_text(ui_ZoomLabel, "+");
  }
  lv_chart_set_axis_tick(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 10, 5, tick_count, 2, true, 50);
  lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, range);
}

#include <Wire.h>

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting...");

  smartdisplay_init();
  ui_init();

  smartdisplay_lcd_set_backlight(backlight_default_brightness);
  smartdisplay_led_set_rgb(false, false, false);

  // FIX I2C
  // .pio/libdeps/esp32-2432S032C/Adafruit BusIO/Adafruit_I2CDevice.cpp
  // _wire->beginTransmission(0x00);
  // _wire->endTransmission();

#if defined(IS_DEMO)
#else
  I2Ctwo.setPins(21, 22);
  if (!ina219.begin(&I2Ctwo))
  {
    set_status("Failed to find INA219");
    Serial.println("Failed to find INA219");
  }
  else
  {
    set_status("Found INA219");
  }
#endif

  lv_chart_series = lv_chart_add_series(ui_Chart1, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  set_zoom();
  toggle(nullptr);
}

void toggle(lv_event_t *e)
{
  Serial.println("Toggle");
  is_paused = !is_paused;
  if(is_paused) {
      lv_label_set_text(ui_ToggleLabel, "Start");
  } else {
      lv_label_set_text(ui_ToggleLabel, "Stop");
  }  
}

void zoom(lv_event_t *e)
{
  Serial.println("Zoom");
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
      update_chart();
    }
  }
  lv_timer_handler();
}
