#include <Keyboard.h>

typedef struct {
  int pin;
  int button;
} t_button_pin_conf;

typedef struct {
  int debounce;
  int pressed;
} t_button_state;

// Define how many pins to use as buttons
#define BUTTONS_TOTAL 9
// Debounce counter max value (lower = faster, higher = better debounce)
#define DEBOUNCE_MAX_VALUE 5

// Define board pin <-> button mappings here. See keys macro in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {0, 'w'},
  {1, 'a'},
  {2, 's'},
  {3, 'd'},
  {4, 'u'},
  {5, 'i'},
  {6, 'o'},
  {7, 'j'},
  {8, 'k'}
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
  }  
}


void setup() {
  setup_buttons();
  Keyboard.begin();
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (!digitalRead(button_pin_conf[i].pin)) {
      debounce_inc(i);
      if (button_state[i].debounce == DEBOUNCE_MAX_VALUE && button_state[i].pressed == false) {
        button_state[i].pressed = true;
        Keyboard.press(button_pin_conf[i].button);
      }
    } else {
      debounce_dec(i);
      if (button_state[i].debounce == 0 && button_state[i].pressed == true) {
        button_state[i].pressed = false;
        Keyboard.release(button_pin_conf[i].button);
      }
    }
  }
}
