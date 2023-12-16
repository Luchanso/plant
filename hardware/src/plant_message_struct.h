#pragma once

#ifndef __PLANT_MESSAGE_STRUCT
#error "Do not include this file"
#endif

struct plantMessage {
  plantMessageCode code;
  uint8_t payloadSize;
  uint8_t *payload;
};
