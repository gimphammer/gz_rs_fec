/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-01 12:21:50
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-10 17:18:52
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */


#ifndef INTERNAL_DEF_H
#define INTERNAL_DEF_H

#include <cstdint>
#include <vector>

namespace gz_rs_fec {

using Matrix1DUint8 = std::vector<uint8_t>;
using Matrix2DUInt8 = std::vector<Matrix1DUint8>;

// x^8+x^4+x^3+x^2+1
extern const uint16_t kPrimitivePolynomial;
extern const uint8_t kGF256MulTable[256][256]; 
extern const uint8_t kMIETable[256]; //mie = multiplicative inverse element


// uint16_t GaloisFieldMultiply(uint16_t a, uint16_t b);

//Finite Field GF(2^8) is default supported
// uint8_t gf256_mul(uint8_t a, uint8_t b);

}
#endif
