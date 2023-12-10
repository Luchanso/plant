#pragma once
/// By gh/BortEngineerDude for gh/Luchanso

// Just a definition of the time struct

typedef struct {
  uint8_t year; //year of the century, i.e. can have value from 0 to 99
  uint8_t month;
  uint8_t dayOfMonth; //can have value from 1 to 31
  uint8_t dayOfWeek; //can have value from 1 to 7, from monday to sunday
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} time;