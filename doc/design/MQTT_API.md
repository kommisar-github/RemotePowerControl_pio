# MQTT API — topic grammar, payloads, HA mapping

## Abstract

**TL;DR:** The MQTT contract between RemotePowerControl and Home Assistant — topic grammar, set/get payload byte-layout, retained-vs-not rules, Last Will, keepalive, and the info-JSON telemetry schema.

**Load when:** MQTT, topic, home assistant, HA, PubSubClient, retained, last will, LWT, announcement, keepalive, power/set, power/get, power/state, power/status, info, alive, payload format, switch index, json telemetry, broker, subscribe, resubscribe, reconnect, buffer size

**Key facts:**
- Topics: `<area>/<host>/power/{state,status}/<idx>` (retained), `.../power/{set,get}`, `<area>/alive(/get)`, `<area>/<host>/info(/get)`, `<area>/keepalive/<host>`, `<area>/announcement/<host>` (=LWT topic).
- set payload byte layout: payload[0]=action ('0' off / '1' on / '3' force), payload[2]=1-based switch char; index = payload[2]-'1'.
- state/status publish retained=true; get/set/keepalive/announcement non-retained — a retained set would re-fire the relay on reconnect.
- LWT: connect="connected", will="disconnected" on the announcement topic (WILLQOS=0); HA marks offline via the will.
- PubSubClient buffer MQTT_MAX_PACKET_SIZE=1024; resubscribe() is mandatory on every reconnect or the device goes deaf.

**Owner:** `/mqtt` (Primary per DOC_OWNERSHIP_MATRIX.md)
**Related:** `POWER_STATE.md`, `ARCHITECTURE.md`, `BUILD.md`

---

## Topic grammar

> Body authored in Phase 0 wave 0.4 (`/mqtt`) from the current `src/MqttClient.*` + `main.cpp` MQTT section. This stub carries the Abstract so the matrix references resolve at bootstrap.

## set/get payload byte-layout

## Retained / LWT / keepalive rules

## info JSON schema

## Home Assistant mapping
