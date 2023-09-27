// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#include "event_output_v200.h"

// #include "byte_order_utils.h"
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

const int EventOutputV200::size =
    sizeof(int8_t) * VAS_ID_SIZE_LEN_V200 + sizeof(int16_t) + sizeof(int32_t) +
    sizeof(int16_t) + sizeof(int32_t) + sizeof(int32_t) * MAX_OUTPUT_ZONE_V200 +
    sizeof(int32_t) + sizeof(int8_t) * MAX_CLIP_NAME_SIZE_V200 +
    sizeof(int64_t) + sizeof(int64_t) + sizeof(int64_t) + sizeof(uint16_t) +
    sizeof(int8_t) * MAX_OBJECT_INFO_SIZE_V200 +
    sizeof(int8_t) * MAX_EVENT_MSG_SIZE_V200 +
    sizeof(int8_t) * MAX_EVENT_ACTION_SIZE_V200 + sizeof(uint8_t) +
    sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) +
    sizeof(int32_t);

EventOutputV200::EventOutputV200()
{
  data_out = new char[sizeof(EventOutputV200)];
}
EventOutputV200::~EventOutputV200() { delete data_out; }

uint16_t swapByteOrder(uint16_t us)
{
  uint16_t ret_us = (us >> 8) | (us << 8);
  return ret_us;
}

uint32_t swapByteOrder(uint32_t ui)
{
  uint32_t ret_ui = (ui >> 24) | ((ui << 8) & 0x00FF0000) |
                    ((ui >> 8) & 0x0000FF00) | (ui << 24);
  return ret_ui;
}

uint64_t swapByteOrder(uint64_t ull)
{
  int64_t ret_ull =
      (ull >> 56) | ((ull << 40) & 0x00FF000000000000) |
      ((ull << 24) & 0x0000FF0000000000) | ((ull << 8) & 0x000000FF00000000) |
      ((ull >> 8) & 0x00000000FF000000) | ((ull >> 24) & 0x0000000000FF0000) |
      ((ull >> 40) & 0x000000000000FF00) | (ull << 56);
  return ret_ull;
}

char* EventOutputV200::toNetwork()
{
  int offset = 0;
  memcpy(data_out + offset, vasID, sizeof(int8_t) * VAS_ID_SIZE_LEN_V200);
  offset += sizeof(int8_t) * VAS_ID_SIZE_LEN_V200;

  int16_t t_channelId = htons(channelId);
  memcpy(data_out + offset, &t_channelId, sizeof(int16_t));
  offset += sizeof(int16_t);

  int32_t t_appId = htonl(appId);
  memcpy(data_out + offset, &t_appId, sizeof(int32_t));
  offset += sizeof(int32_t);

  int16_t t_eventId = htons(eventId);
  memcpy(data_out + offset, &t_eventId, sizeof(int16_t));
  offset += sizeof(int16_t);

  int32_t t_numZones = htonl(numZones);
  memcpy(data_out + offset, &t_numZones, sizeof(int32_t));
  offset += sizeof(int32_t);

  memcpy(data_out + offset, &zoneId,
         sizeof(int32_t) * MAX_OUTPUT_ZONE_V200); // FIXME
  offset += sizeof(int32_t) * MAX_OUTPUT_ZONE_V200;

  int32_t t_clipSize = htonl(clipSize);
  memcpy(data_out + offset, &t_clipSize, sizeof(int32_t));
  offset += sizeof(int32_t);

  memcpy(data_out + offset, &clipName,
         sizeof(int8_t) * MAX_CLIP_NAME_SIZE_V200);
  offset += sizeof(int8_t) * MAX_CLIP_NAME_SIZE_V200;

  uint64_t t_instanceId = swapByteOrder((uint64_t)instanceId);
  memcpy(data_out + offset, &t_instanceId, sizeof(uint64_t));
  offset += sizeof(int64_t);

  uint64_t t_stTimeStamp = swapByteOrder((uint64_t)stTimeStamp);

  memcpy(data_out + offset, &t_stTimeStamp, sizeof(uint64_t));
  offset += sizeof(int64_t);

  uint64_t t_endTimeStamp = swapByteOrder((uint64_t)endTimeStamp);
  memcpy(data_out + offset, &t_endTimeStamp, sizeof(uint64_t));
  offset += sizeof(int64_t);

  int16_t t_eventPriority = htons(eventPriority);
  memcpy(data_out + offset, &t_eventPriority, sizeof(uint16_t));
  offset += sizeof(uint16_t);

  memcpy(data_out + offset, &objectInfo,
         sizeof(int8_t) * MAX_OBJECT_INFO_SIZE_V200);
  offset += sizeof(int8_t) * MAX_OBJECT_INFO_SIZE_V200;

  memcpy(data_out + offset, &eventMsg,
         sizeof(int8_t) * MAX_EVENT_MSG_SIZE_V200);
  offset += sizeof(int8_t) * MAX_EVENT_MSG_SIZE_V200;

  memcpy(data_out + offset, &eventAction,
         sizeof(int8_t) * MAX_EVENT_ACTION_SIZE_V200);
  offset += sizeof(int8_t) * MAX_EVENT_ACTION_SIZE_V200;

  memcpy(data_out + offset, &resultFlag, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  memcpy(data_out + offset, &objectProperty1, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  memcpy(data_out + offset, &objectProperty2, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  memcpy(data_out + offset, &objectProperty3, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  memcpy(data_out + offset, &objectProperty4, sizeof(uint8_t));
  offset += sizeof(uint8_t);

  int32_t t_numOfSnap = htonl(numOfSnap);
  memcpy(data_out + offset, &t_numOfSnap, sizeof(int32_t));
  offset += sizeof(int32_t);
  return data_out;
}

const int EventInfo::size =
    sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t) +
    sizeof(int64_t) + sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t) +
    sizeof(int16_t) + sizeof(int64_t) + sizeof(int16_t) + sizeof(int16_t) +
    sizeof(int16_t) + sizeof(int16_t) + sizeof(int64_t) + sizeof(int16_t) +
    sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t) + sizeof(int32_t) +
    sizeof(int32_t) + sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t) +
    sizeof(int16_t);

EventInfo::EventInfo() { data_out = new char[sizeof(EventInfo)]; }
EventInfo::~EventInfo() { delete data_out; }
char* EventInfo::toNetwork()
{
  int offset = 0;
  int16_t t_topLeftCol = htons(topLeftCol);
  memcpy(data_out + offset, &t_topLeftCol, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_topLeftRow = htons(topLeftRow);
  memcpy(data_out + offset, &t_topLeftRow, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightCol = htons(buttomRightCol);
  memcpy(data_out + offset, &t_buttomRightCol, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightRow = htons(buttomRightRow);
  memcpy(data_out + offset, &t_buttomRightRow, sizeof(int16_t));
  offset += sizeof(int16_t);

  int64_t t_evidenceTimeStamp_1 = htonl(evidenceTimeStamp_1);
  memcpy(data_out + offset, &t_evidenceTimeStamp_1, sizeof(int64_t));
  offset += sizeof(int64_t);

  int16_t t_topLeftCol_1 = htons(topLeftCol_1);
  memcpy(data_out + offset, &t_topLeftCol_1, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_topLeftRow_1 = htons(topLeftRow_1);
  memcpy(data_out + offset, &t_topLeftRow_1, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightCol_1 = htons(buttomRightCol_1);
  memcpy(data_out + offset, &t_buttomRightCol_1, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightRow_1 = htons(buttomRightRow_1);
  memcpy(data_out + offset, &t_buttomRightRow_1, sizeof(int16_t));
  offset += sizeof(int16_t);

  int64_t t_evidenceTimeStamp_2 = htonl(evidenceTimeStamp_2);
  memcpy(data_out + offset, &t_evidenceTimeStamp_2, sizeof(int64_t));
  offset += sizeof(int64_t);

  int16_t t_topLeftCol_2 = htons(topLeftCol_2);
  memcpy(data_out + offset, &t_topLeftCol_2, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_topLeftRow_2 = htons(topLeftRow_2);
  memcpy(data_out + offset, &t_topLeftRow_2, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightCol_2 = htons(buttomRightCol_2);
  memcpy(data_out + offset, &t_buttomRightCol_2, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightRow_2 = htons(buttomRightRow_2);
  memcpy(data_out + offset, &t_buttomRightRow_2, sizeof(int16_t));
  offset += sizeof(int16_t);

  int64_t t_evidenceTimeStamp_3 = htonl(evidenceTimeStamp_3);
  memcpy(data_out + offset, &t_evidenceTimeStamp_3, sizeof(int64_t));
  offset += sizeof(int64_t);

  int16_t t_topLeftCol_3 = htons(topLeftCol_3);
  memcpy(data_out + offset, &t_topLeftCol_3, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_topLeftRow_3 = htons(topLeftRow_3);
  memcpy(data_out + offset, &t_topLeftRow_3, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightCol_3 = htons(buttomRightCol_3);
  memcpy(data_out + offset, &t_buttomRightCol_3, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_buttomRightRow_3 = htons(buttomRightRow_3);
  memcpy(data_out + offset, &t_buttomRightRow_3, sizeof(int16_t));
  offset += sizeof(int16_t);

  int32_t t_rgbWidth = htonl(rgbWidth);
  memcpy(data_out + offset, &t_rgbWidth, sizeof(int32_t));
  offset += sizeof(int32_t);

  int32_t t_rgbHeight = htonl(rgbHeight);
  memcpy(data_out + offset, &t_rgbHeight, sizeof(int32_t));
  offset += sizeof(int32_t);

  int16_t t_vehicleTopLeftCol = htons(vehicleTopLeftCol);
  memcpy(data_out + offset, &t_vehicleTopLeftCol, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_vehicleTopLeftRow = htons(vehicleTopLeftRow);
  memcpy(data_out + offset, &t_vehicleTopLeftRow, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_vehicleWidth = htons(vehicleWidth);
  memcpy(data_out + offset, &t_vehicleWidth, sizeof(int16_t));
  offset += sizeof(int16_t);

  int16_t t_vehicleHeight = htons(vehicleHeight);
  memcpy(data_out + offset, &t_vehicleHeight, sizeof(int16_t));
  offset += sizeof(int16_t);

  return data_out;
}