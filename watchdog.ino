#include <EEPROM.h>

#define EEPROM_INITIALISATION_KEY 2020202020  // Some value specific to our app
#define DEBUG true
#define POWER_CYCLE_PIN 12  // The pin which when high, causes power cycle

struct Config {
  unsigned long initialisationKey = EEPROM_INITIALISATION_KEY;
  unsigned long initDelayMs = 300000;
  unsigned long heartbeatIntervalMs = 30000;
  unsigned long powerCycleDurationMs = 5000;
  unsigned long maxUnresponsiveCycles = 5;
};

enum Phase {
  UNINITIALIZED,
  INITIALIZING,
  WATCHING,
  POWER_CYCLE,
  MAX_UNRESPONSIVE_STOP,
};

unsigned long time_now = 0;
Phase phase = UNINITIALIZED;
unsigned long unresponsiveCycles = 0;

Config config;
void setup() {
  Serial.begin(115200);
  debug("initialising\n");
  pinMode(POWER_CYCLE_PIN, OUTPUT);
  digitalWrite(POWER_CYCLE_PIN, HIGH);
  EEPROM.get(0, config);
  if (config.initialisationKey != EEPROM_INITIALISATION_KEY) {
    debug("resetting config\n");
    config = Config{};
    EEPROM.put(0, config);
  }
  phase = INITIALIZING;
}

template <typename T>
void debug(const T& data) {
  if (DEBUG) {
    Serial.print(data);
  }
}

void loop() {
  // Check for serial data
  while (Serial.available()) {
    int inByte = Serial.read();
    if (inByte == 'H') {
      unresponsiveCycles = 0;
      if (phase == INITIALIZING) {
        debug("got early heartbeat, entering watching phase\n");
        // If initialising, switch to heartbeat phase
        time_now = millis();
        phase = WATCHING;
      } else if (phase == WATCHING) {
        debug("got heartbeat\n");
        // If heartbeat phase, just update latest time
        time_now = millis();
      } else if (phase == MAX_UNRESPONSIVE_STOP) {
        debug("got heartbeat, re-entering watching phase\n");
        // If heartbeat phase, just update latest time
        time_now = millis();
        phase = WATCHING;
      }
    }
  }

  if (phase == INITIALIZING) {
    // Initialisation phase, just wait to be done
    if (millis() - time_now > config.initDelayMs) {
      // Enter heartbeat phase
      debug("initialisation phase complete\n");
      time_now = millis();
      phase = WATCHING;
    }
  } else if (phase == WATCHING) {
    if (millis() - time_now > config.heartbeatIntervalMs) {
      // Power off
      debug("starting power cycle\n");
      time_now = millis();
      phase = POWER_CYCLE;
      digitalWrite(POWER_CYCLE_PIN, LOW);
    }
  } else if (phase == POWER_CYCLE) {
    if (millis() - time_now > config.powerCycleDurationMs) {
      // Power on and re-enter initialisation phase
      debug("stopping power cycle\n");
      time_now = millis();
      digitalWrite(POWER_CYCLE_PIN, HIGH);
      unresponsiveCycles += 1;
      if (unresponsiveCycles >= config.maxUnresponsiveCycles) {
        debug("max unresponsive power cycles reached, waiting for heartbeat\n");
        phase = MAX_UNRESPONSIVE_STOP;
      } else {
        phase = INITIALIZING;
      }
    }
  }
}
