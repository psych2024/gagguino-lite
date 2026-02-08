#pragma once

#define WARMUP_THRESHOLD_CELCIUS 94
#define PID_BLEND_DELTA_LO 1.5
#define PID_BLEND_DELTA_HI 0.5

#define BREW_KP 30
#define BREW_KI 2
#define BREW_KD 20
#define BREW_GRAPH_TEMP_MIN 80
#define BREW_GRAPH_TEMP_MAX 120

#define STEAM_KP 100
#define STEAM_KI 10
#define STEAM_KD 0
#define STEAM_GRAPH_TEMP_MIN 100
#define STEAM_GRAPH_TEMP_MAX 140
#define STEAM_TEMP_SETPOINT 135.f

#define EMA_ALPHA 0.1

#define PID_PERIOD_MS 500
#define PWM_WINDOW_MS 1000
#define REPORTING_PERIOD_MS 1000

enum GagguinoMode {
    INVALID,
    WARMUP,
    BREW,
    STEAM
};

// Coffee art
const char ART[] PROGMEM = 
R"rawliteral(
                  (
                   )    (
           __...(------)-....__
       .-""      )   (         ""-.
  .-'``|--._          )        __.-|
 / .--.|  `""---.........---""`    |
 / /   |                           |
 | |   |                           |
  \ \  |                           |
   `\`\|                           |
     \ \                           /
     (__\                         /
  ..---""`\                      /`""---.
.-'        \                    /        "-.
:            `-.__        __.-'             :
:               ) ""---"" (                 :
 '._            `"--...--"`               _.'
   \""--..__                     __..--""/
    '._   """----.........----"""   _...'
       `""--..,,___________,,..--""`

)rawliteral";