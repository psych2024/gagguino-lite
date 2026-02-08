#include "lcd.h"
#include "globals.h"

LCD::LCD()
{
}

LCD::~LCD()
{
}

void LCD::init()
{
    LOADING_ART_HEIGHT = 0;
    LOADING_ART_WIDTH = 0;

    tft.begin();
    if (!touchscreen.begin()) {
        Serial.println(F("Touchscreen not found!"));
    } else {
        Serial.println(F("Touchscreen initialized."));
    }
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(TEXT_SIZE);

    int c;
    // consume first newline
    size_t i = 1;
    uint16_t curr_len = 0;
    while ((c = pgm_read_byte_near(ART + i)) != '\0') {
        if (c == '\n') {
            LOADING_ART_HEIGHT++;
            LOADING_ART_WIDTH = max(LOADING_ART_WIDTH, curr_len);
            curr_len = 0;
        } else {
            curr_len++;
        }
        i++;
    }
}

void LCD::display_mode(GagguinoMode mode, int color)
{
    int charWidth = 6 * TEXT_SIZE;
    tft.setTextColor(color);
    if (mode == BREW)
    {
        // MODE: BREW
        tft.setCursor(x_max - charWidth * 10, temperature_status_y);
        tft.print(F("MODE: BREW"));
    }
    else
    {
        // MODE: STEAM
        tft.setCursor(x_max - charWidth * 11, temperature_status_y);
        tft.print(F("MODE: STEAM"));
    }
}

void LCD::draw_graph_labels(GagguinoMode mode, int color)
{
    uint16_t temperature_labels[GRAPH_GRADATIONS + 1];
    uint16_t temp_max, temp_min;
    if (mode == BREW) {
        temp_min = BREW_GRAPH_TEMP_MIN;
        temp_max = BREW_GRAPH_TEMP_MAX;
    } else {
        temp_min = STEAM_GRAPH_TEMP_MIN;
        temp_max = STEAM_GRAPH_TEMP_MAX;
    }
     
    int step_size = ceil((temp_max - temp_min + 1) / float(GRAPH_GRADATIONS + 1));
    for (int i = 0; i <= GRAPH_GRADATIONS; ++i)
    {
        temperature_labels[i] = temp_max - step_size * i;
    }

    // draw ticks
    for (int i = 0; i <= GRAPH_GRADATIONS; ++i)
    {
        tft.drawFastHLine(x_plot_min - 3, label_positions[i], 3, color);
        tft.setCursor(x_min, label_positions[i]);
        tft.print(temperature_labels[i]);
    }
}

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void LCD::plot_points(GagguinoMode mode, int color)
{
    uint16_t temp_max, temp_min;
    if (mode == BREW) {
        temp_min = BREW_GRAPH_TEMP_MIN;
        temp_max = BREW_GRAPH_TEMP_MAX;
    } else {
        temp_min = STEAM_GRAPH_TEMP_MIN;
        temp_max = STEAM_GRAPH_TEMP_MAX;
    }

    for (uint16_t i = 0; i < buff_len; ++i)
    {
        float reading = buffer[(buff_pos + i) % max_points];

        if (reading < temp_min || reading > temp_max)
            continue;
        if (reading == 0)
            continue;

        float max_dist = y_plot_max - y_plot_min;
        uint16_t y_pos = map_float(reading, temp_min, temp_max, 0, max_dist);
        y_pos = max_dist - y_pos;
        y_pos += y_plot_min;
        y_pos = min(y_pos, y_plot_max - POINT_HEIGHT - 1);
        y_pos = max(y_pos, y_plot_min + POINT_HEIGHT + 1);
        tft.fillRect(x_plot_min + i * POINT_WIDTH, y_pos, POINT_WIDTH, POINT_HEIGHT, color);
    }
}

void LCD::plot_temperature_reading(GagguinoMode mode, float reading, uint16_t pid)
{
    // round to 2 decimal places
    reading = roundf(reading * 100) / 100;
    pid = roundf(pid);

    // clear old graph
    plot_points(mode, ILI9341_BLACK);

    // write new reading to buffer
    buffer[(buff_pos + buff_len) % max_points] = static_cast<uint16_t>(reading);
    if (buff_len < max_points)
        buff_len++;
    else
        buff_pos = (buff_pos + 1) % max_points;

    plot_points(mode, ILI9341_WHITE);

    // update temperature display
    tft.setCursor(temperature_status_x, temperature_status_y);
    tft.setTextColor(ILI9341_BLACK);
    tft.print(previous_temp_reading);
    tft.print(" ");
    tft.print(F("PID: "));
    tft.print(previous_pid_reading);
    tft.print("%");

    tft.setCursor(temperature_status_x, temperature_status_y);
    tft.setTextColor(ILI9341_WHITE);
    tft.print(reading);
    tft.print(" ");
    tft.print("PID: ");
    tft.print(pid);
    tft.print("%");

    previous_temp_reading = reading;
    previous_pid_reading = pid;
}

void LCD::poll_touchscreen()
{
    if (touchscreen.touched())
    {
        touch_count++;
    }
}

void LCD::draw_loading_screen() {
    int margin_y = (LCD_CHAR_HEIGHT - LOADING_ART_HEIGHT) / 2;
    int margin_x = (LCD_CHAR_WIDTH - LOADING_ART_WIDTH) / 2;
    
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(0,0);
    tft.setTextWrap(false);

    // consume first newline
    size_t curr = 1;
    for (int i = 0; i < margin_y; ++i) tft.println();
    for (uint16_t i = 0; i < LOADING_ART_HEIGHT; ++i) {
        for (int j = 0; j < margin_x; ++j) tft.print(' ');
        char c;
        while ((c = pgm_read_byte_near(ART + curr++)) != '\n') {
            if (c == '\0') {
                tft.println();
                return;
            }
            tft.print(c);
        }
        tft.println();
    }
}


void LCD::draw_loading_progress(int color, uint8_t progress) {
    char buff[8];
    snprintf(buff, sizeof(buff), "%d%%", progress);

    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(buff, 0, 0, &x1, &y1, &w, &h);

    progress_bar_x = (tft.width()  - (int16_t)w) / 2;
    progress_bar_y = tft.height() - 4 * CHAR_PIXEL_HEIGHT;
    tft.setCursor(progress_bar_x, progress_bar_y);

    tft.setTextColor(color);
    tft.print(buff);
}

bool LCD::check_toggled_mode() {
    if (touch_count > 0) {
        touch_count = 0;
        return true;
    }
    return false;
}

void LCD::plot_temp_graph(GagguinoMode mode) {
    this->clear_screen();

    tft.setCursor(0,0);
    tft.setTextColor(ILI9341_WHITE);
    tft.println(F("Gagguino Lite v1"));
    tft.print(F("Temperature: "));
    temperature_status_x = tft.getCursorX();
    temperature_status_y = tft.getCursorY();
    tft.print("\n");

    // get current y value as y-max for graph
    // this is working region for graph
    y_min = tft.getCursorY() + 5;
    y_max = LCD_PIXEL_HEIGHT - MARGIN_BOTTOM;
    x_min = MARGIN_LEFT;
    x_max = LCD_PIXEL_WIDTH - MARGIN_RIGHT;

    int graph_height = y_max - y_min;

    // calculate tick positions
    for (int i = 0; i <= GRAPH_GRADATIONS; ++i)
    {
        float fraction = i / float(GRAPH_GRADATIONS);
        label_positions[i] = y_min + round(fraction * graph_height);
    }

    // determine plot area
    x_plot_min = x_min + 32;
    x_plot_max = x_max;
    y_plot_min = y_min;
    y_plot_max = y_max;

    // draw graph lines
    // draw x axis
    tft.drawLine(x_plot_min, y_plot_max, x_plot_max, y_plot_max, ILI9341_WHITE);
    // draw y axis
    tft.drawLine(x_plot_min, y_plot_min, x_plot_min, y_plot_max, ILI9341_WHITE);

    max_points = min(128, (x_plot_max - x_plot_min) / float(POINT_WIDTH));
    memset(buffer, 0, sizeof(buffer));
    buff_len = 0;
    buff_pos = 0;
    draw_graph_labels(mode, ILI9341_WHITE);
    display_mode(mode, ILI9341_WHITE);
}

void LCD::clear_screen() {
    tft.fillScreen(ILI9341_BLACK);
}

void LCD::update_warmup_status(uint8_t progress) {
    progress = max(0, progress);
    progress = min(100, progress);
    draw_loading_progress(ILI9341_BLACK, curr_progress);
    curr_progress = progress;
    draw_loading_progress(ILI9341_WHITE, curr_progress);
}