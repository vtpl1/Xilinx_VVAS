// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************
#include "vms_live_event_sender.h"

#include <bitset>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>

#include "cstring"
#include "ctime"
#include "event_output_v200.h"
#include "job.h"
#include "string.h"
#include "time.h"

// #include "turbojpeg.h"
// #define LOGGER(message) std::cout << "[" << __FUNCTION__ << "] " << message
// << std::endl

VmsLiveEventSender::VmsLiveEventSender(Job job) : _job(job)
{
  _event_output.appId = _job.appID;
  _event_output.channelId = _job.channelID;
  _event_output.numOfSnap = 1;
}

VmsLiveEventSender::~VmsLiveEventSender()
{
  if (_sock != nullptr) {
    nng_close(*_sock);
    delete _sock;
    _sock = nullptr;
  }
}

void VmsLiveEventSender::start()
{
  int rv;
  if (_sock == nullptr) {
    _sock = new nng_socket();
    if ((rv = nng_push0_open(_sock)) != 0) {
      std::cout << "nng_req0_open error : " << rv << std::endl;
    }
  }
}

void VmsLiveEventSender::sendEventFromEncodedString(uint8_t* encoded_string, int16_t objectProperty1,
                                                    int16_t objectProperty2, int16_t objectProperty3,
                                                    int16_t objectProperty4, std::string objectInfo,
                                                    std::string eventMsg, std::string eventAction, int32_t appID,
                                                    std::string vasID, int64_t time_stamp, std::string clipName,
                                                    int32_t numZones, int32_t zoneId)
{
  int rv;
  if ((rv = nng_dial(*_sock, _url.c_str(), NULL, 0)) != 0) {
    std::cout << "nng_dial error : " << rv << std::endl;
  } else {
    std::cout << "nng_dial success : " << rv << std::endl;
  }
  std::vector<char> data_event;

  int jpeg_img_size = sizeof(encoded_string) + sizeof(EventInfo);
  for (int i = 0; i < sizeof(_event_output); i++) {
    auto* x = (char*)(&_event_output);
    // data_event.push_back(*( + i)));
  }

  if ((rv = nng_send(*_sock, data_event.data(), 10, 0)) != 0) {
    std::cout << "nng_send error: " << rv << std::endl;
  } else {
    std::cout << "nng_send success: " << rv << std::endl;
  }
}

// std::vector<char> serialize(EventOutputV200* event_output)
// {

// }