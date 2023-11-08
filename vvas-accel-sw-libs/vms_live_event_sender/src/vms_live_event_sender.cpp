// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************
#include "vms_live_event_sender.h"

#include <bitset>
#include <iostream>
#include <netinet/in.h>
#include <nng/nng.h>
// #include <nng/supplemental/util/platform.h>
#include <sstream>
#include <tuple>
#include <vector>

#include "cstring"
#include "ctime"
#include "event_output_v200.h"
#include "job.h"
#include "string.h"
#include "time.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

// #include "turbojpeg.h"
// #define LOGGER(message) std::cout << "[" << __FUNCTION__ << "] " << message
// << std::endl

VmsLiveEventSender::VmsLiveEventSender() { _job.jobID = 1; }

VmsLiveEventSender::~VmsLiveEventSender()
{
  if (_sock != nullptr) {
    nng_close(*_sock);
    delete _sock;
    _sock = nullptr;
  }
}

int VmsLiveEventSender::sendEventFromEncodedMat(
    cv::Mat mat, int16_t objectProperty1, int16_t objectProperty2,
    int16_t objectProperty3, int16_t objectProperty4, std::string objectInfo,
    std::string eventMsg, std::string eventAction, int32_t appID,
    std::string vasID, int64_t time_stamp, std::string clipName,
    int32_t numZones, int32_t zoneId, int16_t channelID)
{
  int ret;
  if (_sock == nullptr) {
    _sock = new nng_socket();
    ret = nng_push0_open(_sock);
    if (ret != 0) {
      std::cout << "nng_push0_open failed, error_code: " << ret << std::endl;
      return ret;
    }
  }

  // int ret = nng_socket_set_ms(*_sock, NNG_OPT_RECVTIMEO, _dur);
  if (ret != 0) {
    std::cout << "nng_socket_set_ms failed, error_code:" << ret << std::endl;
    return ret;
  }

  ret = nng_dial(*_sock, _url.c_str(), NULL, 0);
  if (ret != 0) {
    std::cout << "nng_dial failed, error_code:" << ret << std::endl;
    return ret;
  }

  EventOutputV200 _event_output;
  EventInfo _event_info;

  _event_output.numOfSnap = 1;
  _event_output.appId = appID;
  _event_output.channelId = channelID;
  _event_output.stTimeStamp = time_stamp;
  _event_output.endTimeStamp = time_stamp;
  // std::cout << "channel_id: " << _event_output.channelId
  // << " app_id: " << _event_output.appId << std::endl;

  char* event_output_net = _event_output.toNetwork();
  char* event_info_net = _event_info.toNetwork();

  std::vector<uchar> img_buff;
  img_buff.resize(2 * 1024 * 1024);
  std::vector<int> param(2);
  param[0] = cv::IMWRITE_JPEG_QUALITY;
  param[1] = 80; // default(95) 0-100

  cv::imencode(".jpg", mat, img_buff, param);
  uint32_t img_buff_size = img_buff.size();

  uint32_t send_buff_size = sizeof(uint32_t) + EventOutputV200::size +
                            sizeof(uint32_t) + img_buff_size + EventInfo::size;
  uint32_t send_buff_size_net = htonl((send_buff_size - sizeof(uint32_t)));

  char* send_buff = new char[send_buff_size];

  int offset = 0;

  memcpy(send_buff + offset, &send_buff_size_net, sizeof(uint32_t));
  offset += sizeof(uint32_t);

  memcpy(send_buff + offset, event_output_net, EventOutputV200::size);
  offset += EventOutputV200::size;

  uint32_t img_buff_size_net = htonl(img_buff_size + EventInfo::size);
  memcpy(send_buff + offset, &img_buff_size_net, sizeof(uint32_t));
  offset += sizeof(uint32_t);

  memcpy(send_buff + offset, img_buff.data(), img_buff.size());
  offset += img_buff.size();

  memcpy(send_buff + offset, event_info_net, EventInfo::size);
  offset += EventInfo::size;

  ret = nng_send(*_sock, send_buff, send_buff_size, 0);
  std::cout << " xilinx data sent " << send_buff_size << std::endl;
  delete send_buff;

  if (ret != 0) {
    std::cout << "nng_send failed, error_code:" << ret << std::endl;
    return ret;
  }
  return 0;
}

int VmsLiveEventSender::sendEventFromEncodedString(
    uint8_t* encoded_string, int16_t objectProperty1, int16_t objectProperty2,
    int16_t objectProperty3, int16_t objectProperty4, std::string objectInfo,
    std::string eventMsg, std::string eventAction, int32_t appID,
    std::string vasID, int64_t time_stamp, std::string clipName,
    int32_t numZones, int32_t zoneId, int16_t channelID)
{
}
// void VmsLiveEventSender::sendEventInfo(
//   int16_t topLeftCol,int16_t topLeftRow,int16_t buttomRightCol,int16_t
//   buttomRightRow, int64_t evidenceTimeStamp_1,int16_t topLeftCol_1,int16_t
//   topLeftRow_1, int16_t buttomRightCol_1, int16_t buttomRightRow_1,int64_t
//   evidenceTimeStamp_2
// )
// {
//   char *data = _event_info_bytes.toNetwork();
// }
// std::vector<char> serialize(EventOutputV200* event_output)
// {

// }