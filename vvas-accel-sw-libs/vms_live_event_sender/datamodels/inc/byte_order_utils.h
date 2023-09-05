// *****************************************************
//    Copyright 2023 Videonetics Technology Pvt Ltd
// *****************************************************

#pragma once
#ifndef byte_order_utils_h
#define byte_order_utils_h
// #include <Poco/ByteOrder.h>
#include <vector>
template <typename R> R check_and_read_network(std::vector<uint8_t>& data_in, int32_t& offset)
{
  // NOLINTNEXTLINE(google-readability-casting,cppcoreguidelines-pro-bounds-pointer-arithmetic)
  //   R ret = Poco::ByteOrder::fromNetwork(static_cast<R>(*(R*)(data_in.data()
  //   + offset))); offset += sizeof(R);
  R ret;
  return ret;
}

template <typename T> void check_and_write_network(std::vector<uint8_t>& data_out, int32_t& offset, T val)
{
  // NOLINTNEXTLINE(google-readability-casting,cppcoreguidelines-pro-bounds-pointer-arithmetic)
  //   *(T*)(data_out.data() + offset) = Poco::ByteOrder::toNetwork(val);
  offset += sizeof(T);
}
#endif // byte_order_utils_h
