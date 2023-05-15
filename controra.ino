#include <Keyboard.h>

// Uncomment to use serial port logging instead of real keyboard presses
// #define DEBUG

// Directions buffer defines. Used to prohibit simultaneous presses
// of two opposite directions (up <-> down, left <-> right).
#define DIRECTIONS_TOTAL 2
enum DIRECTION_TYPE {DIRECION_NONE = -1, DIRECTION_HORIZONTAL, DIRECTION_VERTICAL};

// Number of milliseconds when it's not allowed to press direction button after previous
// was released. 17 is minimal whole number that is higher than 16.667 miliiseconds (1 game frame).
// Seems to be enough for not to trigger instant input jumps to opposite direction in GGST.
#define DIRECTION_BUFFER_DELAY_MS 17

typedef struct {
  int content; // key value in buffer ('w', 'a', KEY_RETURN, etc).
  unsigned long clear_millis; // value of millis() call when buffered button was released
} t_direction_buffer_state;

#define DIRECTION_BUFFER_CONTENT_CLEAR 0 // buffer content when it's empty (pressing of any direction is allowed)

// Common buttons defines.
// Define how many pins to use as buttons
#define BUTTONS_TOTAL 13
// Debounce counter max value (lower = faster, higher = better debounce).
// Icrease it if the output flicker.
#define DEBOUNCE_MAX_VALUE 3

typedef struct {
  int pin; // pin number on board
  int button; // button value 
  DIRECTION_TYPE direction_type;
} t_button_pin_conf;

typedef struct {
  int debounce; // current value of debounce counter
  bool pressed; // pressed state
} t_button_state;

// Button mapping defines.
// Define board pin <-> button mappings here. See key macros in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {0, 'w', DIRECTION_VERTICAL},
  {1, 'j', DIRECION_NONE},
  {2, ' ', DIRECION_NONE},
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

// Main button handler
inline void button_handle(int button_offset, bool pressed) {
  if (pressed) {
    button_state[button_offset].pressed = true;
    #ifndef DEBUG
    Keyboard.press(button_pin_conf[button_offset].button);
    #else
    Serial.print((char)button_pin_conf[button_offset].button);
    Serial.print(".\n");
    #endif
  } else {
    button_state[button_offset].pressed = false;
    #ifndef DEBUG
    Keyboard.release(button_pin_conf[button_offset].button);
    #else
    Serial.print((char)button_pin_conf[button_offset].button);
    Serial.print("^\n");
    #endif
  }
}

// Directions buffer handler. Used in combination with button_handle().
// Returns true if it's allowed to press or release button at the moment.
bool directions_buffer_handle(int button_offset, bool pressed) {
  static int direction_type, button;
  static t_direction_buffer_state *direction_buffer_state_loc;
  static unsigned long current_millis;
  
  direction_type = button_pin_conf[button_offset].direction_type;
  button = button_pin_conf[button_offset].button;
  if (direction_type != DIRECION_NONE) {
    direction_buffer_state_loc = &direction_buffer_state[direction_type];
    current_millis = millis();
    if (pressed) {
      if (direction_buffer_state_loc->content == DIRECTION_BUFFER_CONTENT_CLEAR && \
          current_millis - direction_buffer_state_loc->clear_millis > DIRECTION_BUFFER_DELAY_MS) {
        direction_buffer_state_loc->content = button;
        return true;
      }
    } else {
      if (direction_buffer_state_loc->content == button) {
        direction_buffer_state_loc->clear_millis = current_millis;
        direction_buffer_state_loc->content = DIRECTION_BUFFER_CONTENT_CLEAR;
        return true;
      }
    }
  }
  return false;
}

void setup() {
  memset(direction_buffer_state, 0, sizeof(t_direction_buffer_state) * DIRECTIONS_TOTAL);
  memset(button_state, 0, sizeof(t_button_state) * BUTTONS_TOTAL);
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    pinMode(button_pin_conf[i].pin, INPUT_PULLUP);
    digitalWrite(button_pin_conf[i].pin, HIGH);
  }
  Keyboard.begin();
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (!digitalRead(button_pin_conf[i].pin)) {
      debounce_inc(i);
      if (button_state[i].debounce == DEBOUNCE_MAX_VALUE && button_state[i].pressed == false) {
        if (button_pin_conf[i].direction_type == DIRECION_NONE) {
          button_handle(i, true);
        } else {
          if (directions_buffer_handle(i, true)) {
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
          if (directions_buffer_handle(i, false)) {
            button_handle(i, false);
          }
        }
      }
    }
  }
}
