#include <Keyboard.h>

// Define how many pins to use as buttons
#define BUTTONS_TOTAL 2
// Anti-jitter counter max value (lower = faster, higher = better anti-jitter)
#define JITTER_MAX_VALUE 5

typedef struct {
  int pin;
  int button;
} t_button_pin_conf;

typedef struct {
  int jitter;
  int pressed;
} t_button_state;


// Define button mappings here. See keys macro in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {2, KEY_LEFT_ARROW},
  {3, KEY_RIGHT_ARROW}
};

t_button_state button_state[BUTTONS_TOTAL];

inline void jitter_inc(int index) {
  if (button_state[index].jitter < JITTER_MAX_VALUE) button_state[index].jitter++;
}

inline void jitter_dec(int index) {
  if (button_state[index].jitter > 0 ) button_state[index].jitter--;
}

void setup_buttons() {
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    pinMode(button_pin_conf[i].pin, INPUT_PULLUP);
    digitalWrite(button_pin_conf[i].pin, HIGH);
    button_state[i].jitter = 0;
    button_state[i].pressed = false;
  }  
}


void setup() {
  setup_buttons();
  Keyboard.begin();
  // Serial is for debugging purposes only
  Serial.begin(9600);
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (!digitalRead(button_pin_conf[i].pin)) {
      jitter_inc(i);
      if (button_state[i].jitter == JITTER_MAX_VALUE && button_state[i].pressed == false) {
        button_state[i].pressed = true;
        Keyboard.press(button_pin_conf[i].button);
        Serial.write("press ");
        Serial.println(button_pin_conf[i].button);
      }
    } else {
      jitter_dec(i);
      if (button_state[i].jitter == 0 && button_state[i].pressed == true) {
        button_state[i].pressed = false;
        Keyboard.release(button_pin_conf[i].button);
        Serial.write("release ");
        Serial.println(button_pin_conf[i].button);
      }
    }
  }
}
