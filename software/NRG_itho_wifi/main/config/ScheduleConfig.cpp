#include <string>

#include <Arduino.h>
#include "globals.h"

#include "config/ScheduleConfig.h"

ScheduleConfig scheduleConfig;

// default constructor
ScheduleConfig::ScheduleConfig()
{
  reset();
  configLoaded = false;
} // ScheduleConfig

// default destructor
ScheduleConfig::~ScheduleConfig()
{
} //~ScheduleConfig

void ScheduleConfig::reset()
{
  schedule_enabled = 0;
  for (uint8_t d = 0; d < SCHEDULE_DAYS; d++)
  {
    memset(days[d], '0', SCHEDULE_SLOTS_PER_DAY);
    days[d][SCHEDULE_SLOTS_PER_DAY] = '\0';
  }
  strlcpy(config_struct_version, SCHEDULE_CONFIG_VERSION, sizeof(config_struct_version));
}

bool ScheduleConfig::set(JsonObject obj)
{
  bool updated = false;

  if (!obj["version_of_program"].isNull())
  {
    if (obj["version_of_program"] != SCHEDULE_CONFIG_VERSION)
    {
      return false;
    }
  }

  if (!obj["schedule_enabled"].isNull())
  {
    updated = true;
    schedule_enabled = obj["schedule_enabled"].as<uint8_t>() ? 1 : 0;
  }

  if (obj["days"].is<JsonArray>())
  {
    JsonArray arr = obj["days"].as<JsonArray>();
    uint8_t d = 0;
    for (JsonVariant v : arr)
    {
      if (d >= SCHEDULE_DAYS)
        break;
      const char *str = v.as<const char *>();
      if (str == nullptr)
      {
        d++;
        continue;
      }
      // Copy and sanitize: anything outside '0'..'3' or missing becomes '0'
      for (uint8_t s = 0; s < SCHEDULE_SLOTS_PER_DAY; s++)
      {
        char c = str[s];
        if (c == '\0')
        {
          memset(&days[d][s], '0', SCHEDULE_SLOTS_PER_DAY - s);
          break;
        }
        days[d][s] = (c >= '0' && c <= '3') ? c : '0';
      }
      days[d][SCHEDULE_SLOTS_PER_DAY] = '\0';
      d++;
    }
    updated = true;
  }

  return updated;
}

void ScheduleConfig::get(JsonObject obj) const
{
  obj["schedule_enabled"] = schedule_enabled;
  JsonArray arr = obj["days"].to<JsonArray>();
  for (uint8_t d = 0; d < SCHEDULE_DAYS; d++)
  {
    arr.add(days[d]);
  }
  obj["version_of_program"] = config_struct_version;
}

uint8_t ScheduleConfig::getLevel(uint8_t day, uint8_t slot) const
{
  if (day >= SCHEDULE_DAYS || slot >= SCHEDULE_SLOTS_PER_DAY)
    return NONE;
  char c = days[day][slot];
  if (c < '0' || c > '3')
    return NONE;
  return static_cast<uint8_t>(c - '0');
}
