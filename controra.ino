#include <Keyboard.h>

// Uncomment to use serial port logging instead of real keyboard presses
// #define DEBUG

// ---------- DEBOUNCE

// Debounce counter max value (lower = faster, higher = better debounce).
// Icrease it if the output flicker.
#define DEBOUNCE_MAX_VALUE 3

enum DEBOUNCE_EVENT {DEBOUNCE_EVENT_NONE, DEBOUNCE_EVENT_PRESSED, DEBOUNCE_EVENT_RELEASED};

typedef struct {
  int debounce; // current value of debounce counter
  bool pressed; // pressed state
  DEBOUNCE_EVENT event;
} t_button_state;

// ---------- BUTTON GROUPS

// Number of milliseconds when it's not allowed to press another button in group after previous
// was released. 17 is minimal whole number that is higher than 16.667 miliiseconds (1 game frame).
// Seems to be enough for not to trigger instant input jumps to opposite direction in GGST.
#define BUTTON_GROUP_DELAY_MS 17

enum BUTTON_GROUP_TYPE {BUTTON_GROUP_NONE, BUTTON_GROUP_GHOSTING, BUTTON_GROUP_KNOCKOUTING};

#define BUTTON_GROUP_CONTENT_CLEAR -1 // buffer content when it's empty

typedef struct {
  BUTTON_GROUP_TYPE type;
  // bool has_switch_delay; // always true for now
  int content; // value in buffer
  int intent;
  unsigned long last_change_millis; // value of millis() call when content was changed
} t_button_group;

// ---------- GENERAL DATA TYPES

typedef struct {
  int pin; // pin number on board
  int button; // button value
  t_button_group *button_group;
} t_button_pin_conf;

// ---------- GENERAL CONFIGURATION

// Define how many pins to use as buttons
#define BUTTONS_TOTAL 13

#define BUTTON_GROUPS 2

t_button_group group_direction_vertical = {BUTTON_GROUP_GHOSTING, BUTTON_GROUP_CONTENT_CLEAR, BUTTON_GROUP_CONTENT_CLEAR, 0};
t_button_group group_direction_horizontal = {BUTTON_GROUP_GHOSTING, BUTTON_GROUP_CONTENT_CLEAR, BUTTON_GROUP_CONTENT_CLEAR, 0};


// Button mapping defines.
// Define board pin <-> button mappings here. See key macros in Keyboard.h.
t_button_pin_conf button_pin_conf[BUTTONS_TOTAL] = {
  {0, 'w', &group_direction_vertical},
  {1, 'n', NULL},
  {2, 'j', NULL},
  {3, 'k', NULL},
  {4, 'l', NULL},
  {5, 'p', NULL},
  {6, 'o', NULL},
  {7, 'i', NULL},
  {8, 'u', NULL},
  {9, 'd', &group_direction_horizontal},
  {10, 's', &group_direction_vertical},
  {11, 'a', &group_direction_horizontal},
  {12, 'q', NULL}
};

// ---------- DATA ARRAYS

// Global state buffers
t_button_state button_state[BUTTONS_TOTAL];


// ---------- Convenience wrapper functions

inline void debounce_inc(int button_offset) {
  if (button_state[button_offset].debounce < DEBOUNCE_MAX_VALUE) button_state[button_offset].debounce++;
}

inline void debounce_dec(int button_offset) {
  if (button_state[button_offset].debounce > 0) button_state[button_offset].debounce--;
}

inline DEBOUNCE_EVENT debounce_handle(int button_offset) {
  if (button_state[button_offset].debounce == DEBOUNCE_MAX_VALUE && button_state[button_offset].pressed == false) {
    button_state[button_offset].pressed = true;
    button_state[button_offset].event = DEBOUNCE_EVENT_PRESSED;
  } else if (button_state[button_offset].debounce == 0 && button_state[button_offset].pressed == true) {
    button_state[button_offset].pressed = false;
    button_state[button_offset].event = DEBOUNCE_EVENT_RELEASED;
  } else {
    button_state[button_offset].event = DEBOUNCE_EVENT_NONE;
  }
  return button_state[button_offset].event;
}

// simple button handler
inline void button_handle_simple(int button_offset, DEBOUNCE_EVENT event) {
  if (event == DEBOUNCE_EVENT_PRESSED) {
    #ifndef DEBUG
    Keyboard.press(button_pin_conf[button_offset].button);
    #else
    Serial.print((char)button_pin_conf[button_offset].button);
    Serial.print(".\n");
    #endif
  } else if (event == DEBOUNCE_EVENT_RELEASED) {
    #ifndef DEBUG
    Keyboard.release(button_pin_conf[button_offset].button);
    #else
    Serial.print((char)button_pin_conf[button_offset].button);
    Serial.print("^\n");
    #endif
  }
}

inline void button_handle_button_group_ghosting(int button_offset, t_button_group *group) {
  static unsigned long current_millis;
  current_millis = millis();

  if (button_state[button_offset].pressed) {
    if (group->content == BUTTON_GROUP_CONTENT_CLEAR && \
        current_millis - group->last_change_millis > BUTTON_GROUP_DELAY_MS) {
      group->content = button_offset;
      button_handle_simple(button_offset, DEBOUNCE_EVENT_PRESSED);
    }
  } else {
    if (group->content == button_offset) {
      group->last_change_millis = current_millis;
      group->content = BUTTON_GROUP_CONTENT_CLEAR;
      button_handle_simple(button_offset, DEBOUNCE_EVENT_RELEASED);
    }
  }
}

inline void button_handle_button_group_knockouting(int button_offset, DEBOUNCE_EVENT event, t_button_group *group) {
  static unsigned long current_millis;
  current_millis = millis();

  if (event == DEBOUNCE_EVENT_PRESSED) {
    group->intent = button_offset;
    if (group->content != button_offset && group->content != BUTTON_GROUP_CONTENT_CLEAR) {
        button_handle_simple(group->content, DEBOUNCE_EVENT_RELEASED);
        group->content = BUTTON_GROUP_CONTENT_CLEAR;
        group->last_change_millis = current_millis;
    }
  } else if (event == DEBOUNCE_EVENT_RELEASED) {
    if (group->content == button_offset) {
      button_handle_simple(button_offset, DEBOUNCE_EVENT_RELEASED);
      group->content = BUTTON_GROUP_CONTENT_CLEAR;
      group->last_change_millis = current_millis;
    }
    if (group->intent == button_offset) {
      group->intent = BUTTON_GROUP_CONTENT_CLEAR;
    }
  }
  if (group->intent != BUTTON_GROUP_CONTENT_CLEAR && current_millis - group->last_change_millis > BUTTON_GROUP_DELAY_MS) {
    button_handle_simple(group->intent, DEBOUNCE_EVENT_PRESSED);
    group->content = group->intent;
    group->intent = BUTTON_GROUP_CONTENT_CLEAR;
    group->last_change_millis = current_millis;
  }
  if (button_state[button_offset].pressed && current_millis - group->last_change_millis > BUTTON_GROUP_DELAY_MS && \
      group->content == BUTTON_GROUP_CONTENT_CLEAR && group->content == BUTTON_GROUP_CONTENT_CLEAR) {
        button_handle_simple(button_offset, DEBOUNCE_EVENT_PRESSED);
        group->content = button_offset;
        group->last_change_millis = current_millis;
      }
}

void setup() {
  memset(button_state, 0, sizeof(t_button_state) * BUTTONS_TOTAL);
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    pinMode(button_pin_conf[i].pin, INPUT_PULLUP);
    digitalWrite(button_pin_conf[i].pin, HIGH);
  }
  #ifndef DEBUG
  Keyboard.begin();
  #else
  Serial.begin(115200);
  #endif
  if (!digitalRead(button_pin_conf[0].pin)) {
    group_direction_vertical.type = BUTTON_GROUP_NONE;
    group_direction_horizontal.type = BUTTON_GROUP_NONE;
  }
  if (!digitalRead(button_pin_conf[1].pin)) {
    group_direction_vertical.type = BUTTON_GROUP_KNOCKOUTING;
    group_direction_horizontal.type = BUTTON_GROUP_KNOCKOUTING;
  }
}

void loop()
{
  for (int i = 0; i < BUTTONS_TOTAL; ++i) {
    if (!digitalRead(button_pin_conf[i].pin)) {
      debounce_inc(i);
    } else {
      debounce_dec(i);
    }
    DEBOUNCE_EVENT event = debounce_handle(i);
    t_button_group *group = button_pin_conf[i].button_group;
    if (group == NULL || group->type == BUTTON_GROUP_NONE) {
      button_handle_simple(i, event);
    } else {
      switch (group->type)
      {
      case BUTTON_GROUP_GHOSTING:
        button_handle_button_group_ghosting(i, group);
        break;
      case BUTTON_GROUP_KNOCKOUTING:
        button_handle_button_group_knockouting(i, event, group);
        break;
      default:
        break;
      }
    }
  }
}
