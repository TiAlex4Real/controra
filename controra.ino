#include <Keyboard.h>

// Uncomment to use serial port logging
// #define DEBUG

// Directions buffer defines. Used to prohibit simultaneous presses
// of two opposite directions (up <-> down, left <-> right).
#define DIRECTIONS_TOTAL 3
enum DIRECTION_TYPE {DIRECION_NONE, DIRECTION_HORIZONTAL, DIRECTION_VERTICAL};
#define DIRECTION_BUFFER_CONTENT_CLEAR 0 // buffer content when it's empty (pressing of any direction is allowed)
#define DIRECTION_BUFFER_DELAY_MAX_VALUE 100 // number of cycles when it's not allowed to put anything into buffer

typedef struct {
  int content;
  int delay;
} t_direction_buffer_state;

// Common buttons defines.
// Define how many pins to use as buttons
#define BUTTONS_TOTAL 13
// Debounce counter max value (lower = faster, higher = better debounce)
#define DEBOUNCE_MAX_VALUE 1

typedef struct {
  int pin;
  int button;  
  DIRECTION_TYPE direction_type;
} t_button_pin_conf;

typedef struct {
  int debounce;
  int pressed;
} t_button_state;

// Button mapping defines.
// Define board pin <-> button mappings here. See key macros in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {0, 'w', DIRECTION_VERTICAL},
  {1, 'j', DIRECION_NONE},
  {2, 'z', DIRECION_NONE},
  {3, KEY_RETURN, DIRECION_NONE},
  {4, 't', DIRECION_NONE},
  {5, 'o', DIRECION_NONE},
  {6, 'k', DIRECION_NONE},
  {7, 'i', DIRECION_NONE},
  {8, 'u', DIRECION_NONE},
  {9, 'd', DIRECTION_HORIZONTAL},
  {10, 's', DIRECTION_VERTICAL},
  {11, 'a', DIRECTION_HORIZONTAL},
  {12, 'q', DIRECION_NONE}
};

// Global state buffers
t_direction_buffer_state direction_buffer_state[DIRECTIONS_TOTAL];
t_button_state button_state[BUTTONS_TOTAL];

// Convenience wrapper functions
inline void debounce_inc(int index) {
  if (button_state[index].debounce < DEBOUNCE_MAX_VALUE) button_state[index].debounce++;
}

inline void debounce_dec(int index) {
  if (button_state[index].debounce > 0) button_state[index].debounce--;
}

inline void button_handle(int button_offset, bool pressed) {
  if (pressed) {
      button_state[button_offset].pressed = true;
      #ifndef DEBUG
      Keyboard.press(button_pin_conf[button_offset].button);
      #endif
  } else {
      button_state[button_offset].pressed = false;
      #ifndef DEBUG
      Keyboard.release(button_pin_conf[button_offset].button);
      #endif
  }
}

void setup_buttons() {
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    pinMode(button_pin_conf[i].pin, INPUT_PULLUP);
    digitalWrite(button_pin_conf[i].pin, HIGH);
    button_state[i].debounce = 0;
    button_state[i].pressed = false;
  }  
}

void setup() {
  memset(direction_buffer_state, 0, sizeof(t_direction_buffer_state) * DIRECTIONS_TOTAL);
  memset(button_state, 0, sizeof(t_button_state) * BUTTONS_TOTAL);
  setup_buttons();
  Keyboard.begin();
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (direction_buffer_state[button_pin_conf[i].direction_type].content == DIRECTION_BUFFER_CONTENT_CLEAR && \
        direction_buffer_state[button_pin_conf[i].direction_type].delay > 0) {
      direction_buffer_state[button_pin_conf[i].direction_type].delay--;
      if (direction_buffer_state[button_pin_conf[i].direction_type].delay == 0)
      continue;
    }
    if (!digitalRead(button_pin_conf[i].pin)) {
      debounce_inc(i);
      if (button_state[i].debounce == DEBOUNCE_MAX_VALUE && button_state[i].pressed == false) {
        if (button_pin_conf[i].direction_type == DIRECION_NONE) {
          button_handle(i, true);
        } else {
          if (direction_buffer_state[button_pin_conf[i].direction_type].content == DIRECTION_BUFFER_CONTENT_CLEAR && \
              direction_buffer_state[button_pin_conf[i].direction_type].delay == 0) {
            direction_buffer_state[button_pin_conf[i].direction_type].content = button_pin_conf[i].button;
            button_handle(i, true);
          }
        }
      }
    } else {
      debounce_dec(i);
      if (button_state[i].debounce == 0 && button_state[i].pressed == true) {
        if (button_pin_conf[i].direction_type == DIRECION_NONE) {
          button_handle(i, false);
        } else {
          if (direction_buffer_state[button_pin_conf[i].direction_type].content == button_pin_conf[i].button) {
            direction_buffer_state[button_pin_conf[i].direction_type].content = DIRECTION_BUFFER_CONTENT_CLEAR;
            direction_buffer_state[button_pin_conf[i].direction_type].delay = DIRECTION_BUFFER_DELAY_MAX_VALUE;
            button_handle(i, false);
          }
        }
      }
    }
  }
  #ifdef DEBUG
  Serial.print("PRESSED: [");
  for (int i=0; i<BUTTONS_TOTAL; i++) {
    Serial.print(button_state[i].pressed ? (char)button_pin_conf[i].button : ' ');
  }
  Serial.print("] HOR: [");
  Serial.print((char)direction_buffer_state[DIRECTION_HORIZONTAL].content);
  Serial.print(' ');
  Serial.print(direction_buffer_state[DIRECTION_HORIZONTAL].delay);
  Serial.print("] VER: [");
  Serial.print((char)direction_buffer_state[DIRECTION_VERTICAL].content);
  Serial.print(' ');
  Serial.print(direction_buffer_state[DIRECTION_VERTICAL].delay);
  Serial.print("]\n");
  delay(100);
  #endif
}
