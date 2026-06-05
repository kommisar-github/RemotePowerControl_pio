  TODO:
    > When recconnect MQTT send all switch status
    > Add WEB Interface: When device is off, Status and Buttons to be changed to Unavailable/grey status
    > Add button WiFi/Setup portal
    > Add setup persistant
    > Add Telephone / IVR interface

  VERIFY:
    > When device is restarted or powered-off, LAST WILL is sent and change the channel status in HA
    > When device is restarted or powered-off UI will change the all channel status status to UNAVAILABLE
    > Why if when PC was ON and power-detect pin is disconnected, the power-on status remains on D0 but not on D5?
