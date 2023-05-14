#include <Keyboard.h>

// #define DEBUG

enum INPUT_BUFFER_TYPE {INPUT_BUFFER_NONE, INPUT_BUFFER_HORIZONTAL, INPUT_BUFFER_VERTICAL, INPUT_BUFFER_TOTAL};
#define INPUT_BUFFER_CONTENT_CLEAR 0
#define INPUT_BUFFER_DELAY_VALUE 250

typedef struct {
  int content;
  int delay;
} t_input_buffer_state;

t_input_buffer_state input_buffer_content[INPUT_BUFFER_TOTAL];

typedef struct {
  int pin;
  int button;  
  INPUT_BUFFER_TYPE input_buffer_type;
} t_button_pin_conf;

typedef struct {
  int debounce;
  int pressed;
} t_button_state;

// Define how many pins to use as buttons
#define BUTTONS_TOTAL 13
// Debounce counter max value (lower = faster, higher = better debounce)
#define DEBOUNCE_MAX_VALUE 1

// Define board pin <-> button mappings here. See keys macro in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {0, 'w', INPUT_BUFFER_VERTICAL},
  {1, 'j', INPUT_BUFFER_NONE},
  {2, 'z', INPUT_BUFFER_NONE},
  {3, KEY_RETURN, INPUT_BUFFER_NONE},
  {4, 't', INPUT_BUFFER_NONE},
  {5, 'o', INPUT_BUFFER_NONE},
  {6, 'k', INPUT_BUFFER_NONE},
  {7, 'i', INPUT_BUFFER_NONE},
  {8, 'u', INPUT_BUFFER_NONE},
  {9, 'd', INPUT_BUFFER_HORIZONTAL},
  {10, 's', INPUT_BUFFER_VERTICAL},
  {11, 'a', INPUT_BUFFER_HORIZONTAL},
  {12, 'q', INPUT_BUFFER_NONE}
};

t_button_state button_state[BUTTONS_TOTAL];

inline void debounce_inc(int index) {
  if (button_state[index].debounce < DEBOUNCE_MAX_VALUE) button_state[index].debounce++;
}

inline void debounce_dec(int index) {
  if (button_state[index].debounce > 0) button_state[index].debounce--;
}

void setup_buttons() {
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    pinMode(button_pin_conf[i].pin, INPUT_PULLUP);
    digitalWrite(button_pin_conf[i].pin, HIGH);
    button_state[i].debounce = 0;
    button_state[i].pressed = false;
    memset(input_buffer_content, 0, sizeof(t_input_buffer_state) * INPUT_BUFFER_TOTAL);
  }  
}


void setup() {
  setup_buttons();
  Keyboard.begin();
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void button_handle(int button_offset, bool pressed) {
  if (pressed) {
      button_state[button_offset].pressed = true;
      Keyboard.press(button_pin_conf[button_offset].button);
  } else {
      button_state[button_offset].pressed = false;
      Keyboard.release(button_pin_conf[button_offset].button);
  }
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (!digitalRead(button_pin_conf[i].pin)) {
      debounce_inc(i);
      if (button_state[i].debounce == DEBOUNCE_MAX_VALUE && button_state[i].pressed == false) {
        if (button_pin_conf[i].input_buffer_type == INPUT_BUFFER_NONE) {
          button_handle(i, true);
        } else {
          if (input_buffer_content[button_pin_conf[i].input_buffer_type].delay > 0) {
            input_buffer_content[button_pin_conf[i].input_buffer_type].delay--;
            continue;
          }
          if (input_buffer_content[button_pin_conf[i].input_buffer_type].content == INPUT_BUFFER_CONTENT_CLEAR && input_buffer_content[button_pin_conf[i].input_buffer_type].delay == 0) {
            input_buffer_content[button_pin_conf[i].input_buffer_type].content = button_pin_conf[i].button;
            button_handle(i, true);
          }
        }
      }
    } else {
      debounce_dec(i);
      if (button_state[i].debounce == 0 && button_state[i].pressed == true) {
        if (button_pin_conf[i].input_buffer_type == INPUT_BUFFER_NONE) {
          button_handle(i, false);
        } else {
          if (input_buffer_content[button_pin_conf[i].input_buffer_type].content == button_pin_conf[i].button) {
            input_buffer_content[button_pin_conf[i].input_buffer_type].content = INPUT_BUFFER_CONTENT_CLEAR;
            input_buffer_content[button_pin_conf[i].input_buffer_type].delay = INPUT_BUFFER_DELAY_VALUE;
            button_handle(i, false);
          }
        }
      }
    }
  }
  #ifdef DEBUG
  Serial.print("HOR: [");
  Serial.print((char)input_buffer_content[INPUT_BUFFER_HORIZONTAL].content);
  Serial.print("] VER: [");
  Serial.print((char)input_buffer_content[INPUT_BUFFER_VERTICAL].content);
  Serial.print("]\nPRESSED: ");
  for (int i=0; i<BUTTONS_TOTAL; i++) {
    if (button_state[i].pressed == true) {
      Serial.print(", ");
      Serial.print((char)button_pin_conf[i].button);
    }
  }
  Serial.print("\n");
  delay(100);
  #endif
}
