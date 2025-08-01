//
// Created by 赵丹 on 25-8-1.
//

#ifndef LITETVM_RUNTIME_SERIALIZER_H
#define LITETVM_RUNTIME_SERIALIZER_H

#include <dmlc/io.h>
#include <dmlc/serializer.h>

namespace dmlc {
namespace serializer {

template<>
struct Handler<DLDataType> {
    static void Write(Stream* strm, const DLDataType& dtype) {
        Handler<uint8_t>::Write(strm, dtype.code);
        Handler<uint8_t>::Write(strm, dtype.bits);
        Handler<uint16_t>::Write(strm, dtype.lanes);
    }
    static bool Read(Stream* strm, DLDataType* dtype) {
        if (!Handler<uint8_t>::Read(strm, &(dtype->code))) return false;
        if (!Handler<uint8_t>::Read(strm, &(dtype->bits))) return false;
        if (!Handler<uint16_t>::Read(strm, &(dtype->lanes))) return false;
        return true;
    }
};

template<>
struct Handler<DLDevice> {
    static void Write(Stream* strm, const DLDevice& dev) {
        int32_t device_type = static_cast<int32_t>(dev.device_type);
        Handler<int32_t>::Write(strm, device_type);
        Handler<int32_t>::Write(strm, dev.device_id);
    }
    static bool Read(Stream* strm, DLDevice* dev) {
        int32_t device_type = 0;
        if (!Handler<int32_t>::Read(strm, &(device_type))) return false;
        dev->device_type = static_cast<DLDeviceType>(device_type);
        if (!Handler<int32_t>::Read(strm, &(dev->device_id))) return false;
        return true;
    }
};

}// namespace serializer
}// namespace dmlc

#endif//LITETVM_RUNTIME_SERIALIZER_H
