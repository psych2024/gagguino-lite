#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Adafruit_FT6206.h>


#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

#define TS_MINX 160
#define TS_MAXX 319
#define TS_MINY 0
#define TS_MAXY 119


#define LCD_PIXEL_WIDTH 320
#define LCD_PIXEL_HEIGHT 240

#define GRAPH_GRADATIONS 10
#define GRAPH_TEMP_MIN 30
#define GRAPH_TEMP_MAX 120

#define TEXT_SIZE 1

enum GagguinoMode {
    BREW,
    STEAM
};

class LCD
{
private:
    Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    Adafruit_FT6206 touchscreen = Adafruit_FT6206();
    const int MARGIN_LEFT = 5;
    const int MARGIN_BOTTOM = 10;
    const int MARGIN_RIGHT = 10;
    uint16_t touch_count = 0;

    // size of point on LCD: 2x2 pixels
    const int POINT_HEIGHT = 4;
    const int POINT_WIDTH = 3;

    void draw_graph_labels();
    void plot_points(int color);
    int16_t temperature_status_x;
    int16_t temperature_status_y;
    int16_t x_min, x_max, y_min, y_max;
    int16_t x_plot_min, x_plot_max, y_plot_min, y_plot_max;
    int16_t label_positions[GRAPH_GRADATIONS];
    int16_t max_points;
    float* buffer;
    int buff_pos = 0;
    float previous_temp_reading = -1;
    float previous_pid_reading = -1;
    uint32_t plot_count = 0;
    GagguinoMode mode = BREW;

    void display_current_mode(int color);

public:
    LCD();
    ~LCD();
    void init();
    void plot_temperature_reading(float reading, float pid);
    void poll_touchscreen();
    GagguinoMode get_gagguino_mode();
};
