//
// Created by richard on 3/6/25.
//

#ifndef LITETVM_SUPPORT_SCALARS_H
#define LITETVM_SUPPORT_SCALARS_H

namespace litetvm {
namespace support {


// 2^15 * (1 + 1023/1024)
// See https://en.wikipedia.org/wiki/Half-precision_floating-point_format
constexpr double kMaxFloat16 = 65504.0;

// 2^127 * (1 + 127/128)
// See https://en.wikipedia.org/wiki/Bfloat16_floating-point_format
constexpr double kMaxBFloat16 = 3.895313892515354759047080037148786688e38;

// 2^8 * (1 + 6/8)
// See https://arxiv.org/pdf/2209.05433.pdf
constexpr double kMaxE4M3 = 448;

// 2^15 * (1 + 3/4)
// See https://arxiv.org/pdf/2209.05433.pdf
constexpr double kMaxE5M2 = 57344;

}// namespace support
}// namespace litetvm

#endif//LITETVM_SUPPORT_SCALARS_H
