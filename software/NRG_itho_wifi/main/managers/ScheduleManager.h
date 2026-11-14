#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Executes the weekly fan schedule (see config/ScheduleConfig.h).
//
// The schedule only sends a command when the scheduled level changes (a
// "transition"), or when the schedule is (re)enabled / saved. In between,
// any speed set by a remote, the API or the web interface stays active until
// its timer expires or until the next schedule transition overrides it.
class ScheduleManager
{
public:
  // Call periodically from the system control task.
  void loop();

  // Re-apply the level of the current slot on the next loop() call.
  void requestApply() { forceApply = true; }

  // Status info for the web interface.
  void getStatus(JsonObject obj) const;

  // Current schedule position. Returns false when local time is not known yet.
  static bool currentSlot(uint8_t &day, uint8_t &slot);

private:
  volatile bool forceApply{true};
  int16_t lastLevel{-1};
  unsigned long lastCheck{0};
  bool lastTimeValid{false};

  void apply(uint8_t level);
};

extern ScheduleManager scheduleManager;
