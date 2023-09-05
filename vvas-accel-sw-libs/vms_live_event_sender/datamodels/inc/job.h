// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef Job_h
#define Job_h
#include <stdint.h>
#define SRC_URL_SIZE 256
#define SRC_UID_SIZE 16
#define SRC_PASS_SIZE 16
#define JOB_KEY_SIZE 32
#define SCHEDULE_DATA_SIZE 24 * 7

#define EVENT_MSG_SIZE 64
#define EVENT_ACTION_SIZE 128
#define VAS_ID_SIZE 32
class Job
{
public:
  int64_t jobID{0};
  int16_t channelID{0};
  int16_t appID{0};
  uint8_t jobType{0}; // Temporary job or Scheduled job
  int16_t scheduleID{0};
  char vasID[VAS_ID_SIZE]{
      0,
  };
  char srcUrl[SRC_URL_SIZE]{
      0,
  };
  char srcUid[SRC_UID_SIZE]{
      0,
  };
  char srcPass[SRC_PASS_SIZE]{
      0,
  };

  char jobKey[JOB_KEY_SIZE]{
      0,
  };

  // Event priority
  int8_t eventPriority{0}; // forward

  // Event specific msg
  char eventMsg[EVENT_MSG_SIZE]{
      0,
  }; // 64 byte - forward

  // Event Action
  char eventAction[EVENT_ACTION_SIZE]{
      0,
  }; // 128 byte- forward

  int32_t applicationDataSize{0};

public:
  Job() = default;
  ~Job() = default;
};

#endif // Job_h
