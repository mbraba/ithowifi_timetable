#pragma once

#define SCHEDULE_CONFIG_VERSION "001"

#define SCHEDULE_DAYS 7
#define SCHEDULE_SLOTS_PER_DAY 48 // 30 minute granularity

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

// Weekly fan schedule.
//
// Each day is stored as a string of SCHEDULE_SLOTS_PER_DAY characters, one per
// 30 minute slot starting at 00:00. Each character is a schedule level:
//   '0' = no scheduled speed (leave the fan as it is)
//   '1' = low, '2' = medium, '3' = high
// Day index 0 is Monday, 6 is Sunday.
class ScheduleConfig
{
public:
  enum Level : uint8_t
  {
    NONE = 0,
    LOW_SPEED = 1,
    MEDIUM_SPEED = 2,
    HIGH_SPEED = 3
  };

  uint8_t schedule_enabled;
  char days[SCHEDULE_DAYS][SCHEDULE_SLOTS_PER_DAY + 1];
  char config_struct_version[4];

  mutable bool configLoaded;

  ScheduleConfig();
  ~ScheduleConfig();

  bool set(JsonObject);
  void get(JsonObject) const;
  void reset();

  // Level for a given day (0 = Monday) and slot (0..47), NONE when out of range.
  uint8_t getLevel(uint8_t day, uint8_t slot) const;

protected:
}; // ScheduleConfig

extern ScheduleConfig scheduleConfig;
