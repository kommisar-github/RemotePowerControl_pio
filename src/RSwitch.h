#ifndef SN_RSWITCH_H
#define SN_RSWITCH_H

#define SET_POWER_OFF 0
#define SET_POWER_ON 1

#define VALIDATE_SWITCH_STATUS_PERIOD_MINS 5
#define VALIDATE_LONG_PULSE_SEC 6
#define LONG_PULSE_DURATION_DETECT_SEC 3
#define PULSE_PRECESION_MILLS 20

enum POWER_STATUS
{
  UNDEFINED = -1,
  POWER_OFF = 0,
  POWER_ON = 1,
  SLEEP = 2,
  POWERING_OFF = 10,
  POWERING_ON = 11
};

class RSwitch
{
public:
  RSwitch(int switch_gpio, int detect_gpio);
  void setCallback(void (*power_status_change_callback)(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status));
  bool isPowerOn();
  bool isPowerOff();
  bool isPoweringOn();
  bool isPoweringOff();
  bool isSleep();
  bool isUndefined();
  POWER_STATUS getPowerStatus();
  unsigned long getOnTimeSec();
  unsigned long getOffTimeSec();
  unsigned long getOnDurationSec();
  unsigned long getOffDurationSec();
  void setSwitch(bool);
  void longPress();
  int getSwitchGPIO();
  int getDetectGPIO();
  char* decodePowerStat(POWER_STATUS power_status);
  void handleClient();
  void debug_info();

private:
  int switch_gpio;
  int detect_gpio;
  POWER_STATUS power_status = UNDEFINED;
  unsigned long on_time = 0;
  unsigned long off_time = 0;
  void (*power_status_change_callback)(RSwitch *src, POWER_STATUS new_status, POWER_STATUS old_status) = nullptr;
  int short_delay = 800;
  int long_delay = 10000;
  void short_push();
  void long_push();
  void set_power_status(POWER_STATUS);

  void detect_power_status();
  void detect_power_status_old();
  void detect_power_status_new();
  unsigned long power_status_changed_mills = 0L;
  unsigned long last_detect_gpio_validation;
  unsigned long verify_time_prev = 0L;
  unsigned long state_change_time = 0L;
  int stat_current = -1;
  int stat_prev = -1;
  unsigned long_pulse = 0;
  unsigned short_pulse = 0;
  int stat_changed = 0;
  bool use_stat = true;  // Make validation of detect_gpio on the controller init

};

#endif
