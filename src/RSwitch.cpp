#include "Application.h"
#include "RSwitch.h"

RSwitch::RSwitch(int switch_gpio, int detect_gpio)
 {
  this->switch_gpio = switch_gpio;
  this->detect_gpio = detect_gpio;
  pinMode(switch_gpio, OUTPUT);
  pinMode(detect_gpio, INPUT_PULLUP);
  last_detect_gpio_validation = millis();
  int stat = digitalRead(detect_gpio);
  Serial.printf("Switch OUTPUT GPIO: %d Detect INPUT GPIO: %d (%s)\n", 
    switch_gpio, detect_gpio, stat==HIGH ? "HIGH" : "LOW");
}

void RSwitch::setCallback(void (*power_status_change_callback)(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status)) {
  this->power_status_change_callback = power_status_change_callback;
}

int RSwitch::getSwitchGPIO() {
  return switch_gpio;
}

int RSwitch::getDetectGPIO() {
  return detect_gpio;
}

char* RSwitch::decodePowerStat(POWER_STATUS power_status) {
  switch (power_status) {
    case POWER_OFF:
     return (char*)"Power OFF";
    case POWER_ON:
     return (char*)"Power ON";
    case SLEEP:
     return (char*)"Sleep";
    case POWERING_ON:
     return (char*)"Powering ON";
    case POWERING_OFF:
     return (char*)"Powering OFF";
    default:
     return (char*)"Unknown";
  }
}

void RSwitch::set_power_status(POWER_STATUS new_power_status) {
  if (power_status != new_power_status) {
    Serial.printf("Power status is changed from %d to %d\n", power_status, new_power_status);
    POWER_STATUS prev_power_status = power_status;
    power_status = new_power_status;
    power_status_changed_mills = millis();
    if (new_power_status == POWER_ON) {
      on_time = power_status_changed_mills;
    } else if (new_power_status == POWER_OFF) {
      off_time = power_status_changed_mills;
    }
    if (power_status_change_callback != nullptr) {
      power_status_change_callback(this, new_power_status, prev_power_status);
    }
  }
}

bool RSwitch::isPowerOn() {
  return power_status == POWER_ON ? true : false;
}

bool RSwitch::isPoweringOn() {
  return power_status == POWERING_ON ? true : false;
}

bool RSwitch::isPoweringOff() {
  return power_status == POWERING_OFF ? true : false;
}

bool RSwitch::isPowerOff() {
  return power_status == POWER_OFF ? true : false;
}

bool RSwitch::isSleep() {
  return power_status == SLEEP ? true : false;
}

bool RSwitch::isUndefined() {
  return power_status == UNDEFINED ? true : false;
}

POWER_STATUS RSwitch::getPowerStatus() {
  //status = digitalRead(gpio);
  //Serial.printf("#### GET: %d - %d\n", gpio, status);
  return power_status;
}

unsigned long RSwitch::getOnTimeSec() {
  return on_time;
}

unsigned long RSwitch::getOffTimeSec() {
  return off_time;
}

unsigned long RSwitch::getOnDurationSec() {
  if (isPowerOn()) {
    return (millis() - on_time) / 1000;
  }
  else {
    return 0L;
  }
}

unsigned long RSwitch::getOffDurationSec() {
  if (isPowerOff()) {
    return (millis() - off_time) / 1000L;
  }
  else {
    return 0L;
  }
}

void RSwitch::short_push() {
  digitalWrite(switch_gpio, HIGH);
  Serial.println("==> short_push - start");
  delay(short_delay);
  Serial.println("==> short_push - end");
  digitalWrite(switch_gpio, LOW);
}

void RSwitch::long_push() {
  digitalWrite(switch_gpio, HIGH);
  Serial.println("==> long_push - start");
  delay(long_delay);
  Serial.println("==> long_push - end");
  digitalWrite(switch_gpio, LOW);
}

void RSwitch::detect_power_status() {
  detect_power_status_old();
}

void RSwitch::detect_power_status_new() {

  int stat = digitalRead(detect_gpio);
  unsigned long now = millis();
  // unsigned long verify_time_period = now - verify_time_prev;

  // Delay state changing analyze for PULSE_PRECESION_MILLS
  if (stat != stat_prev) {
    // state_changed = true;
    state_change_time = millis();
    stat_prev = stat;
    return;
  }
  else {
    if (now - state_change_time < PULSE_PRECESION_MILLS) {
      return;
    }
  }

  unsigned long pulse_duration = now - state_change_time /*verify_time_prev*/;

  // Identify pulsing
  if (stat != stat_current) {
       if (pulse_duration > LONG_PULSE_DURATION_DETECT_SEC * 1000) {
        long_pulse++;
        Serial.printf("## %d LONG pulses changes (%d pulses) detected. now: %ld state_change: %ld last duration: %ld ms\n", long_pulse, long_pulse/2, now, state_change_time, pulse_duration);
      }
      else {
        short_pulse++;
        Serial.printf("## %d SHORT pulses changes (%d pulses) detected. now: %ld state_change: %ld last duration: %ld ms\n", short_pulse, short_pulse/2, now, state_change_time, pulse_duration);
      }
      // verify_time_prev = now;
      stat_current = stat;
      stat_changed++;
  }

  if (stat_changed > 0) {
    if (short_pulse > 5) {
      Serial.printf("## %d SHORT pulses change (%d pulses) detected - SET SLEEP mode.\n", short_pulse, short_pulse/2);
      // 3 flashing detected (6 pulses)
      if (power_status != SLEEP) {
        set_power_status(SLEEP);
      }
      short_pulse = 0;
      long_pulse = 0;   // reset long_pulse since it always set as 1st before starting pulsing
      stat_changed = 0;
      last_detect_gpio_validation = now;
    }
    if (long_pulse && pulse_duration > VALIDATE_LONG_PULSE_SEC * 1000) {  // pulse duration longer than 6 sec
      Serial.printf("## %d LONG pulses changes (%d pulses) detected - Validate status.\n", long_pulse, long_pulse/2);
      long_pulse = 0;
      use_stat = true;
      stat_changed = 0;
    }
  }
  else {
    // Validate status periodically
    if (now - last_detect_gpio_validation > VALIDATE_SWITCH_STATUS_PERIOD_MINS*60*1000) {
      use_stat = true;
      Serial.println("## Periodic status validation.");
    }
  }

  if (use_stat) {
    use_stat = false;
    last_detect_gpio_validation = now;
    // Uses pull-up optocopler output to have HIGH level when Power is OFF (PLED+ has LOW level)
    // LOW level means power is ON (PLED+ is HIGH level)
    if (stat == LOW) {
      if (power_status != POWER_ON) {
          Serial.printf("###### GPIO: %d stat == LOW && power_status != POWER_ON\n", detect_gpio);
          set_power_status(POWER_ON);
      }
      else {
        Serial.printf("###### GPIO: %d stat == LOW && power_status = %d - NO ACTION\n", detect_gpio, power_status);
      }
    }
    else {
      if (power_status != POWER_OFF) {
          Serial.printf("###### GPIO: %d stat == HIGH && power_status != POWER_OFF\n", detect_gpio);
          set_power_status(POWER_OFF);
      }
      else {
        Serial.printf("###### GPIO: %d stat == HIGH && power_status = %d - NO ACTION\n", detect_gpio, power_status);
      }
    }
  }

}

void RSwitch::detect_power_status_old() {

  int stat = digitalRead(detect_gpio);
  unsigned long time = millis();
  unsigned long period = time - verify_time_prev;

  // Identify pulsing
  if (stat != stat_prev) {
    if(period > PULSE_PRECESION_MILLS) { // xx ms precesion 
      if (period > LONG_PULSE_DURATION_DETECT_SEC * 1000) {
        long_pulse++;
        Serial.printf("## %d LONG pulses changes (%d pulses) detected. time: %ld period: %ld ms\n", long_pulse, long_pulse/2, time, (time-verify_time_prev));
      }
      else {
        short_pulse++;
        Serial.printf("## %d SHORT pulses changes (%d pulses) detected. time: %ld period: %ld ms\n", short_pulse, short_pulse/2, time, (time-verify_time_prev));
      }
      verify_time_prev = time;
      stat_prev = stat;
      stat_changed++;
    }
  }

  unsigned long pulse_duration = time - verify_time_prev;

  if (stat_changed > 0) {
    if (short_pulse > 5) {
      Serial.printf("## %d SHORT pulses change (%d pulses) detected - SET SLEEP mode.\n", short_pulse, short_pulse/2);
      // 3 flashing detected (6 pulses)
      if (power_status != SLEEP) {
        set_power_status(SLEEP);
      }
      short_pulse = 0;
      long_pulse = 0;   // reset long_pulse since it always set as 1st before starting pulsing
      stat_changed = 0;
      last_detect_gpio_validation = time;
    }
    if (long_pulse && pulse_duration > VALIDATE_LONG_PULSE_SEC * 1000) {  // pulse duration longer than 6 sec
      Serial.printf("## %d LONG pulses changes (%d pulses) detected - Validate status.\n", long_pulse, long_pulse/2);
      long_pulse = 0;
      use_stat = true;
      stat_changed = 0;
    }
  }
  else {
    // Validate status periodically (skip during SLEEP — mid-pulse GPIO read would corrupt state)
    if (time - last_detect_gpio_validation > VALIDATE_SWITCH_STATUS_PERIOD_MINS*60*1000 && power_status != SLEEP) {
      use_stat = true;
      Serial.println("## Periodic status validation.");
    }
  }

  if (use_stat) {
    use_stat = false;
    last_detect_gpio_validation = time;
    // Uses pull-up optocopler output to have HIGH level when Power is OFF (PLED+ has LOW level)
    // LOW level means power is ON (PLED+ is HIGH level)
    if (stat == LOW) {
      if (power_status != POWER_ON) {
          Serial.printf("###### GPIO: %d stat == LOW && power_status != POWER_ON\n", detect_gpio);
          set_power_status(POWER_ON);
      }
      else {
        Serial.printf("###### GPIO: %d stat == LOW && power_status = %d - NO ACTION\n", detect_gpio, power_status);
      }
    }
    else {
      if (power_status != POWER_OFF) {
          Serial.printf("###### GPIO: %d stat == HIGH && power_status != POWER_OFF\n", detect_gpio);
          set_power_status(POWER_OFF);
      }
      else {
        Serial.printf("###### GPIO: %d stat == HIGH && power_status = %d - NO ACTION\n", detect_gpio, power_status);
      }
    }
  }

}

void RSwitch::longPress() {
    if (isPowerOff()) {
      Serial.println("LONG PRESS: POWERING_ON");
      //long_push();
      set_power_status(POWERING_ON);
    }
    else if (isPowerOn()) {
      Serial.println("LONG PRESS: POWERING_OFF");
      //long_push();
      set_power_status(POWERING_OFF);
    } else {
      Serial.println("LONG PRESS: UNKNOWN");
      //long_push();
      set_power_status(UNDEFINED);
    }
    long_push();
    last_detect_gpio_validation = millis(); // reset timer
}

void RSwitch::setSwitch(bool on_off_status) {

  if (on_off_status == SET_POWER_ON) {
    if (isPowerOff() || isSleep() || isUndefined()) {
      Serial.println("SET_POWER_ON");
      short_push();
      if (isSleep()) {
        Serial.println("Waking-up: Simulating power-off -> power-on");
        // In case the powering-on from sleeping mode, simulate the previous 
        // status was like power-off. Otherwise the verify_power_status()
        // will work incorrectly because of detect_gpio was pulsing during
        // the sleep mode and stat_prev changed all the time.
        this->stat_changed = 0;
        this->short_pulse = 0;
        this->stat_prev = HIGH; // HIGH means power-off (pull-up resistor)
        this->verify_time_prev = millis() - LONG_PULSE_DURATION_DETECT_SEC * 1000;
      }
      set_power_status(POWERING_ON);
      last_detect_gpio_validation = millis(); // reset timer
      
    }
    else {
      Serial.printf("SET_POWER_ON but power status is %d already\n", power_status);
    }
  }
  else {
    if (isPowerOn()) {
      Serial.println("SET_POWER_OFF");
      short_push();
      set_power_status(POWERING_OFF);
      last_detect_gpio_validation = millis(); // reset timer
    }
    else {
      Serial.printf("SET_POWER_OFF but power status is %d already\n", power_status);
    }
  }
  /*
  this->status = status;
  digitalWrite(gpio, status);
  //Serial.printf("#### SET: %d - %d\n", gpio, status);
  if(status) {
    // Set on_time_start only if it is not already set, otherwise the ON Time will be unwanted extended after WiFi/MQTT reconnect 
    if (on_time_start == 0L) {
      on_time_start = millis();
      if (max_on_time > 0L) {
        Serial.printf("## START MAX ON TIME: %ld sec timer\n", max_on_time/1000);
      }
    }
  }
  else {
    if (max_on_time > 0L && on_time_start > 0L) {
      Serial.printf("## STOP MAX ON TIME: %ld sec timer\n", max_on_time/1000);
    }
    on_time_start = 0L;
  }
  */
}

void RSwitch::handleClient() {
  detect_power_status();
  /*
   if (max_on_time > 0L && status) {
     unsigned long t = millis();
     if (t > on_time_start + max_on_time) {
       Serial.printf("## MAX ON TIME OVER - AUTO SWITCH OFF\n");
       setSwitch(false);
       switch_set_callback(false);
     }
   }
  */
}

void RSwitch::debug_info() {
  int stat = digitalRead(detect_gpio);
  Serial.println("\nRSwitch.cpp:\n================");
  Serial.printf("Switch OUTPUT GPIO: %d\n", switch_gpio);
  Serial.printf("Detect INPUT GPIO: %d\n", detect_gpio);
  Serial.printf("     stat_curent: %d (%s)\n", stat_current, stat_current==HIGH ? "HIGH" : "LOW");
  Serial.printf("       stat  now: %d (%s)\n", stat, stat==HIGH ? "HIGH" : "LOW");
  Serial.printf("       stat_prev: %d (%s)\n", stat_prev, stat_prev==HIGH ? "HIGH" : "LOW");
  Serial.printf("power_status: %d - %s\n", power_status, decodePowerStat(power_status));
  Serial.printf("  long_pulse: %u\n", long_pulse);
  Serial.printf(" short_pulse: %u\n", short_pulse);
  Serial.printf("stat_changed: %u\n", stat_changed);
  Serial.printf("use_stat: %s\n", use_stat ? "TRUE" : "FALSE");
  Serial.printf("                        NOW: %lu\n", millis());

  Serial.printf("           verify_time_prev: %lu\n", verify_time_prev);
  Serial.printf("          state_change_time: %lu\n", state_change_time);
  Serial.printf(" power_status_changed_mills: %lu\n", power_status_changed_mills);
  Serial.printf("last_detect_gpio_validation: %lu\n", last_detect_gpio_validation);
  Serial.printf("                    on_time: %lu\n", on_time);
  Serial.printf("                   off_time: %lu\n", off_time);
  //Serial.printf("aaa: %lu\n", aaa);
}
