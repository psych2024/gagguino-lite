#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include "globals.h"

#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

#define TS_MINX 160
#define TS_MAXX 319
#define TS_MINY 0
#define TS_MAXY 119

#define LCD_PIXEL_WIDTH 320
#define LCD_PIXEL_HEIGHT 240

#define CHAR_PIXEL_WIDTH (6 * TEXT_SIZE)
#define CHAR_PIXEL_HEIGHT (8 * TEXT_SIZE)

#define LCD_CHAR_WIDTH (LCD_PIXEL_WIDTH / CHAR_PIXEL_WIDTH)
#define LCD_CHAR_HEIGHT (LCD_PIXEL_HEIGHT / CHAR_PIXEL_HEIGHT)


#define GRAPH_GRADATIONS 10

// size of point on LCD: 2x2 pixels
#define POINT_HEIGHT 4
#define POINT_WIDTH 3

#define TEXT_SIZE 1

class LCD
{
private:
    Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
    Adafruit_FT6206 touchscreen = Adafruit_FT6206();
    const int16_t MARGIN_LEFT = 5;
    const int16_t MARGIN_BOTTOM = 10;
    const int16_t MARGIN_RIGHT = 10;
    uint16_t touch_count = 0;

    uint16_t LOADING_ART_HEIGHT = 0;
    uint16_t LOADING_ART_WIDTH = -1;

    void draw_graph_labels(GagguinoMode mode, int color);
    void plot_points(GagguinoMode mode, int color);
    uint16_t temperature_status_x;
    uint16_t temperature_status_y;
    uint16_t progress_bar_x;
    uint16_t progress_bar_y;
    uint16_t x_min, x_max, y_min, y_max;
    uint16_t x_plot_min, x_plot_max, y_plot_min, y_plot_max;
    uint16_t label_positions[GRAPH_GRADATIONS + 1];
    uint16_t max_points;
    int curr_progress = -1;
    uint16_t buffer[128];
    uint16_t buff_pos = 0;
    uint16_t buff_len = 0;
    float previous_temp_reading = -1;
    uint16_t previous_pid_reading = 0;

public:
    LCD();
    ~LCD();
    void init();
    void draw_loading_screen();
    void draw_loading_progress(int color, uint8_t progress);
    void plot_temp_graph(GagguinoMode mode);
    void clear_screen();
    void update_warmup_status(uint8_t progress);
    void plot_temperature_reading(GagguinoMode mode, float reading, uint16_t pid);
    void poll_touchscreen();
    bool check_toggled_mode();
    void display_mode(GagguinoMode mode, int color);
};
