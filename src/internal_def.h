/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-01 12:21:50
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-03 19:08:52
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */


#ifndef INTERNAL_DEF_H
#define INTERNAL_DEF_H

#include <cstdint>

namespace gz_rs_fec {


// x^8+x^4+x^3+x^2+1
extern const uint16_t kPrimitivePolynomial;
extern const uint16_t kMultiplicationTable[256][256]; 
extern const uint16_t kMIETable[256]; //mie = multiplicative inverse element


// uint16_t GaloisFieldMultiply(uint16_t a, uint16_t b);

//Finite Field GF(2^8) is default supported
// uint8_t gf256_mul(uint8_t a, uint8_t b);

}
#endif
