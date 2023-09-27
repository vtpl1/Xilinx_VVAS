// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef event_output_v2_0_h
#define event_output_v2_0_h
#include <stdint.h>

#include <vector>

#define VAS_ID_SIZE_LEN_V200 32

#define MAX_OUTPUT_ZONE_V200 16
#define MAX_CLIP_NAME_SIZE_V200 64
#define MAX_OBJECT_INFO_SIZE_V200 32
#define MAX_EVENT_MSG_SIZE_V200 32
#define MAX_EVENT_ACTION_SIZE_V200 256

class EventOutputV200
{
private:
  char* data_out;

public:
  EventOutputV200();
  ~EventOutputV200();
  static const int size;

  int8_t vasID[VAS_ID_SIZE_LEN_V200]{
      0,
  };
  int16_t channelId{0};
  int32_t appId{0};
  int16_t eventId{0};
  int32_t numZones{0};
  int32_t zoneId[MAX_OUTPUT_ZONE_V200]{
      0,
  };
  int32_t clipSize{0};
  int8_t clipName[MAX_CLIP_NAME_SIZE_V200]{
      0,
  };
  int64_t instanceId{0};
  int64_t stTimeStamp{0};
  int64_t endTimeStamp{0};
  // Event priority
  uint16_t eventPriority{0};
  int8_t objectInfo[MAX_OBJECT_INFO_SIZE_V200]{
      0,
  };
  // Event specific msg
  int8_t eventMsg[MAX_EVENT_MSG_SIZE_V200]{
      0,
  };
  // Event Action
  int8_t eventAction[MAX_EVENT_ACTION_SIZE_V200] = {0};
  uint8_t resultFlag{0};
  uint8_t objectProperty1{0};
  uint8_t objectProperty2{0};
  uint8_t objectProperty3{0};
  uint8_t objectProperty4{0};
  //   int16_t objectProperty5{0};
  //   int16_t objectProperty6{0};
  //   int16_t objectProperty7{0};
  //   int16_t objectProperty8{0};
  //   int16_t objectProperty9{0};
  //   int16_t objectProperty10{0};
  //   int16_t objectProperty11{0};
  //   int16_t objectProperty12{0};
  //   int16_t objectProperty13{0};
  //   int16_t objectProperty14{0};
  //   int16_t objectProperty15{0};
  int32_t numOfSnap{0}; // size(4) + imgbuff
  char* toNetwork();
};

class EventInfo
{
private:
  char* data_out;

public:
  EventInfo();
  ~EventInfo();
  static const int size;

  int16_t topLeftCol{0};
  int16_t topLeftRow{0};
  int16_t buttomRightCol{0};
  int16_t buttomRightRow{0};
  int64_t evidenceTimeStamp_1{0};
  int16_t topLeftCol_1{0};
  int16_t topLeftRow_1{0};
  int16_t buttomRightCol_1{0};
  int16_t buttomRightRow_1{0};
  int64_t evidenceTimeStamp_2{0};
  int16_t topLeftCol_2{0};
  int16_t topLeftRow_2{0};
  int16_t buttomRightCol_2{0};
  int16_t buttomRightRow_2{0};
  int64_t evidenceTimeStamp_3{0};
  int16_t topLeftCol_3{0};
  int16_t topLeftRow_3{0};
  int16_t buttomRightCol_3{0};
  int16_t buttomRightRow_3{0};
  int32_t rgbWidth{0};
  int32_t rgbHeight{0};
  int16_t vehicleTopLeftCol{0};
  int16_t vehicleTopLeftRow{0};
  int16_t vehicleWidth{0};
  int16_t vehicleHeight{0};
  char* toNetwork();
};

#endif // event_output_v2_0_h
