// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#include "event_output_v200.h"

// #include "byte_order_utils.h"
#include <netinet/in.h>

const int EventOutputV200::size = sizeof(int16_t) + sizeof(int64_t) + sizeof(int32_t) + sizeof(int32_t) +
                                  sizeof(int32_t) + sizeof(int32_t) + sizeof(uint8_t) + sizeof(uint8_t) +
                                  sizeof(uint8_t);

char* EventOutputV200::toNetwork()
{
  int offset = 0;

  int16_t t_channelId = htons(channelId);
  int32_t t_appId = htonl(appId);
  int64_t t_instanceId = htonl(instanceId);
  // htonl();

  //   check_and_write_network<decltype(start_timestamp)>(data_out, offset,
  //   start_timestamp); check_and_write_network<decltype(movement)>(data_out,
  //   offset, movement); check_and_write_network<decltype(ipfilter)>(data_out,
  //   offset, ipfilter);
  //   check_and_write_network<decltype(recordingfilter)>(data_out, offset,
  //   recordingfilter);
  //   check_and_write_network<decltype(criticalityfilter)>(data_out, offset,
  //   criticalityfilter); data_out[offset++] = is_sensitive_video_allowed;
  //   data_out[offset++] = as_fast_as_possible;
  //   data_out[offset++] = is_redundant_recording;
  return data_out;
}
