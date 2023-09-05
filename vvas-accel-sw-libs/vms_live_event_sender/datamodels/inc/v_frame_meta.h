
#pragma once
#ifndef v_frame_meta
#define v_frame_meta
#include <stdint.h>

class VFrameMeta
{
public:
  uint32_t mediaType{0};
  uint32_t frameType;
  uint32_t bitRate;
  uint32_t fps;
  uint64_t timeStamp;
  uint8_t isMotion;
  uint8_t streamType;
  uint16_t channelID;
};
#endif // v_frame_meta