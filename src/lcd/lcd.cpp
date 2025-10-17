#include "lcd.h"

LCD::LCD()
{
}

LCD::~LCD()
{
}

void LCD::init()
{
    tft.begin();
    if (!touchscreen.begin()) {
        Serial.println("Touchscreen not found!");
    } else {
        Serial.println("Touchscreen initialized.");
    }
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(TEXT_SIZE);
    tft.println("Gagguino Lite v1");
    tft.print("Temperature: ");
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
    for (int i = 0; i < GRAPH_GRADATIONS; ++i)
    {
        float fraction = i / float(GRAPH_GRADATIONS - 1);
        label_positions[i] = y_min + round(fraction * graph_height);
    }

    // determine plot area
    x_plot_min = x_min + 30;
    x_plot_max = x_max;
    y_plot_min = y_min;
    y_plot_max = y_max;

    // draw graph lines
    // draw x axis
    tft.drawLine(x_plot_min, y_plot_max, x_plot_max, y_plot_max, ILI9341_WHITE);
    // draw y axis
    tft.drawLine(x_plot_min, y_plot_min, x_plot_min, y_plot_max, ILI9341_WHITE);

    max_points = (x_plot_max - x_plot_min) / float(POINT_WIDTH);
    buffer = (float *)malloc(sizeof(float) * max_points);
    memset(buffer, 0, sizeof(float) * max_points);
    draw_graph_labels();
    display_current_mode(ILI9341_WHITE);
}

void LCD::display_current_mode(int color)
{
    int charWidth = 6 * TEXT_SIZE;
    tft.setTextColor(color);
    if (mode == BREW)
    {
        // MODE: BREW
        tft.setCursor(x_max - charWidth * 10, temperature_status_y);
        tft.print("MODE: BREW");
    }
    else
    {
        // MODE: STEAM
        tft.setCursor(x_max - charWidth * 11, temperature_status_y);
        tft.print("MODE: STEAM");
    }
}

void LCD::draw_graph_labels()
{
    int temperature_labels[GRAPH_GRADATIONS];
    for (int i = 0; i < GRAPH_GRADATIONS; ++i)
    {
        temperature_labels[i] = GRAPH_TEMP_MAX - 10 * i;
    }

    // draw ticks
    for (int i = 0; i < GRAPH_GRADATIONS; ++i)
    {
        tft.drawFastHLine(x_plot_min - 3, label_positions[i], 3, ILI9341_WHITE);
        tft.setCursor(x_min, label_positions[i]);
        tft.print(temperature_labels[i]);
    }
}

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void LCD::plot_points(int color)
{
    for (int i = 0; i < max_points; ++i)
    {
        int idx = (buff_pos + i) % max_points;
        if (plot_count <= max_points)
            idx = i;
        float reading = buffer[idx];
        if (reading == 0)
            continue;

        float max_dist = y_plot_max - y_plot_min;
        int y_pos = map_float(reading, GRAPH_TEMP_MIN, GRAPH_TEMP_MAX, 0, max_dist);
        y_pos = max_dist - y_pos;
        y_pos += y_plot_min;
        y_pos = min(y_pos, y_plot_max - POINT_HEIGHT - 1);
        y_pos = max(y_pos, y_plot_min + POINT_HEIGHT + 1);
        tft.fillRect(x_plot_min + i * POINT_WIDTH, y_pos, POINT_WIDTH, POINT_HEIGHT, color);
    }
}

void LCD::plot_temperature_reading(float reading, float pid)
{
    if (reading < GRAPH_TEMP_MIN + POINT_HEIGHT + 1 || reading > GRAPH_TEMP_MAX - POINT_HEIGHT - 1)
        return;
    // round to 2 decimal places
    reading = roundf(reading * 100) / 100;
    pid = roundf(pid);

    // clear old graph
    plot_points(ILI9341_BLACK);
    plot_count++;

    // write new reading to buffer
    buffer[buff_pos] = reading;
    buff_pos = (buff_pos + 1) % max_points;

    plot_points(ILI9341_WHITE);

    // update temperature display
    tft.setCursor(temperature_status_x, temperature_status_y);
    tft.setTextColor(ILI9341_BLACK);
    tft.print(previous_temp_reading);
    tft.print(" ");
    tft.print("PID: ");
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
        TS_Point p = touchscreen.getPoint();
        
        touch_count++;
    }
}

GagguinoMode LCD::get_gagguino_mode() {
    if (touch_count > 0) {
        display_current_mode(ILI9341_BLACK);
        if (mode == BREW)
            mode = STEAM;
        else
            mode = BREW;
        display_current_mode(ILI9341_WHITE);

        touch_count = 0;
    }
    
    return mode;
}