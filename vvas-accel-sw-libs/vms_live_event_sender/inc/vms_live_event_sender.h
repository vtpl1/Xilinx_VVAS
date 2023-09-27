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

#include <opencv2/core.hpp>
#include <string>
#include <vector>

#include "event_output_v200.h"
#include "job.h"

class VmsLiveEventSender
{
private:
  Job _job;
  std::string _url = "tcp://localhost:5001";
  nng_socket* _sock = nullptr;
  nng_duration _dur = 1000;

public:
  VmsLiveEventSender();
  ~VmsLiveEventSender();
  int sendEventFromEncodedMat(
      cv::Mat mat, int16_t objectProperty1 = 0, int16_t objectProperty2 = 0,
      int16_t objectProperty3 = 0, int16_t objectProperty4 = 0,
      std::string objectInfo = "", std::string eventMsg = "",
      std::string eventAction = "", int32_t appID = 0, std::string vasID = "",
      int64_t time_stamp = 0, std::string clipName = "", int32_t numZones = 0,
      int32_t zoneId = 0, int16_t channelID = 0);
  int sendEventFromEncodedString(
      uint8_t* encoded_string, int16_t objectProperty1 = 0,
      int16_t objectProperty2 = 0, int16_t objectProperty3 = 0,
      int16_t objectProperty4 = 0, std::string objectInfo = "",
      std::string eventMsg = "", std::string eventAction = "",
      int32_t appID = 0, std::string vasID = "", int64_t time_stamp = 0,
      std::string clipName = "", int32_t numZones = 0, int32_t zoneId = 0,
      int16_t channelID = 0);
  void sendEventFromEncodedJpeg(
      uint8_t* encoded_jpeg, int16_t objectProperty1 = 0,
      int16_t objectProperty2 = 0, int16_t objectProperty3 = 0,
      int16_t objectProperty4 = 0, std::string objectInfo = "",
      std::string eventMsg = "", std::string eventAction = "",
      int32_t appID = 0, std::string vasID = "", int64_t time_stamp = 0,
      std::string clipName = "", int32_t numZones = 0, int32_t zoneId = 0);
  void sendEventFromRawNpArray(
      uint8_t* raw_np_array, int16_t objectProperty1 = 0,
      int16_t objectProperty2 = 0, int16_t objectProperty3 = 0,
      int16_t objectProperty4 = 0, std::string objectInfo = "",
      std::string eventMsg = "", std::string eventAction = "",
      int32_t appID = 0, std::string vasID = "", int64_t time_stamp = 0,
      std::string clipName = "", int32_t numZones = 0, int32_t zoneId = 0);
};
// std::vector<char> serialize(EventOutputV200* event_output);

#endif // VmsLiveEventSender_h
