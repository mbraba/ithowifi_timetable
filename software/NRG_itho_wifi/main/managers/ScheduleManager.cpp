#include <time.h>

#include "managers/ScheduleManager.h"
#include "config/ScheduleConfig.h"
#include "config/SystemConfig.h"
#include "config/IthoRemote.h"
#include "IthoQueue.h"
#include "generic_functions.h"
#include "sys_log.h"
#include "enum.h"

ScheduleManager scheduleManager;

#define SCHEDULE_CHECK_INTERVAL_MS 5000
// Anything before 2023-01-01 means the clock has not been synchronised yet.
#define SCHEDULE_MIN_VALID_EPOCH 1672531200

bool ScheduleManager::currentSlot(uint8_t &day, uint8_t &slot)
{
  time_t now = time(nullptr);
  if (now < SCHEDULE_MIN_VALID_EPOCH)
    return false;

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  // tm_wday: 0 = Sunday; schedule day 0 = Monday
  day = static_cast<uint8_t>((timeinfo.tm_wday + 6) % 7);
  slot = static_cast<uint8_t>(timeinfo.tm_hour * 2 + (timeinfo.tm_min >= 30 ? 1 : 0));
  return true;
}

void ScheduleManager::loop()
{
  if (!forceApply && millis() - lastCheck < SCHEDULE_CHECK_INTERVAL_MS)
    return;
  lastCheck = millis();

  if (!scheduleConfig.schedule_enabled)
  {
    // Forget state so the current slot is applied as soon as it is enabled again
    lastLevel = -1;
    forceApply = false;
    return;
  }

  uint8_t day = 0;
  uint8_t slot = 0;
  if (!currentSlot(day, slot))
  {
    if (lastTimeValid)
      W_LOG("SCHED: local time not available, schedule paused");
    lastTimeValid = false;
    lastLevel = -1;
    return;
  }
  if (!lastTimeValid)
  {
    I_LOG("SCHED: local time available, schedule active");
    lastTimeValid = true;
  }

  const uint8_t level = scheduleConfig.getLevel(day, slot);
  const bool force = forceApply;
  forceApply = false;

  // Only act on a change of scheduled level (or an explicit (re)apply), so a
  // speed chosen with a remote stays active until the next transition.
  if (!force && level == lastLevel)
    return;

  lastLevel = level;

  if (level == ScheduleConfig::NONE)
    return; // nothing scheduled for this slot: leave the fan as it is

  apply(level);
}

void ScheduleManager::apply(uint8_t level)
{
  const char *command = nullptr;
  uint16_t pwmSpeed = 0;
  switch (level)
  {
  case ScheduleConfig::LOW_SPEED:
    command = "low";
    pwmSpeed = systemConfig.itho_low;
    break;
  case ScheduleConfig::MEDIUM_SPEED:
    command = "medium";
    pwmSpeed = systemConfig.itho_medium;
    break;
  case ScheduleConfig::HIGH_SPEED:
    command = "high";
    pwmSpeed = systemConfig.itho_high;
    break;
  default:
    return;
  }

  N_LOG("SCHED: schedule transition, setting fan to %s", command);

  if (systemConfig.itho_rf_standalone == 1)
  {
    // RF standalone: send the command with the first configured RF send remote
    int idx = -1;
    for (int i = 0; i < remotes.getMaxRemotes(); i++)
    {
      if (!remotes.isEmptySlot(i) && remotes.getRemoteFunction(i) == RemoteFunctions::SEND)
      {
        idx = i;
        break;
      }
    }
    if (idx < 0)
    {
      W_LOG("SCHED: no RF send remote configured, cannot execute schedule");
      return;
    }
    ithoExecRFCommand(static_cast<uint8_t>(idx), command, SCHEDULE);
  }
  else if (systemConfig.itho_vremoteapi)
  {
    // Virtual remote: the Itho unit handles remote timers itself, a new
    // speed command overrides a running timer.
    ithoExecCommand(command, SCHEDULE);
  }
  else
  {
    // PWM2I2C: a schedule transition overrides timers started by remotes /
    // the API, so clear the timer queue before setting the new base speed.
    ithoQueue.clearQueue();
    ithoSetSpeed(pwmSpeed, SCHEDULE);
  }
}

void ScheduleManager::getStatus(JsonObject obj) const
{
  uint8_t day = 0;
  uint8_t slot = 0;
  bool valid = currentSlot(day, slot);
  obj["time_valid"] = valid;
  if (valid)
  {
    obj["day"] = day;
    obj["slot"] = slot;
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buf[24]{};
    strftime(buf, sizeof(buf), "%a %H:%M", &timeinfo);
    obj["localtime"] = buf;
  }
}
