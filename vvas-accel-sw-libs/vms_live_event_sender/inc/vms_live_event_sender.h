// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef VmsLiveEventSender_h
#define VmsLiveEventSender_h
#include <nng/nng.h>
#include <nng/protocol/pipeline0/push.h>
#include <nng/supplemental/util/platform.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "event_output_v200.h"
#include "job.h"

class VmsLiveEventSender
{
private:
  Job _job;
  std::string _url = "tcp://172.16.2.110:5001";
  nng_socket* _sock = nullptr;
  EventOutputV200 _event_output;
  EventInfo _event_info_bytes;

public:
  VmsLiveEventSender(Job job);
  ~VmsLiveEventSender();
  void start();
  void stop();
  void sendEventFromEncodedString(uint8_t* encoded_string, int16_t objectProperty1 = 0, int16_t objectProperty2 = 0,
                                  int16_t objectProperty3 = 0, int16_t objectProperty4 = 0, std::string objectInfo = "",
                                  std::string eventMsg = "", std::string eventAction = "", int32_t appID = 0,
                                  std::string vasID = "", int64_t time_stamp = 0, std::string clipName = "",
                                  int32_t numZones = 0, int32_t zoneId = 0);
  void sendEventFromEncodedJpeg(uint8_t* encoded_jpeg, int16_t objectProperty1 = 0, int16_t objectProperty2 = 0,
                                int16_t objectProperty3 = 0, int16_t objectProperty4 = 0, std::string objectInfo = "",
                                std::string eventMsg = "", std::string eventAction = "", int32_t appID = 0,
                                std::string vasID = "", int64_t time_stamp = 0, std::string clipName = "",
                                int32_t numZones = 0, int32_t zoneId = 0);
  void sendEventFromRawNpArray(uint8_t* raw_np_array, int16_t objectProperty1 = 0, int16_t objectProperty2 = 0,
                               int16_t objectProperty3 = 0, int16_t objectProperty4 = 0, std::string objectInfo = "",
                               std::string eventMsg = "", std::string eventAction = "", int32_t appID = 0,
                               std::string vasID = "", int64_t time_stamp = 0, std::string clipName = "",
                               int32_t numZones = 0, int32_t zoneId = 0);
};

// std::vector<char> serialize(EventOutputV200* event_output);

#endif // VmsLiveEventSender_h
