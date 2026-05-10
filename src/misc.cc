/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-03 15:32:12
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-10 11:16:55
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */


#include "internal_def.h"
#include "misc.h"

namespace gz_rs_fec {

uint8_t gf256_mul(uint8_t a, uint8_t b) {
  uint8_t res = 0;

  for (int i = 0; i < 8; i++) {
    //1. for current step:
    if (b & 1)
      res ^= a;   // 加法 = XOR

    //2.for next step:
    b >>= 1;  
    //移位前最高位是否是1，是1的话意味着移位就会溢出，进而就要取余
    uint8_t hi = a & 0x80; // 最高位
    a <<= 1;
    if (hi)
      a ^= 0x1B; // 0x11B 去掉最高位后的低8位
   
  }
  return res;
}


/**
 * According to Fermat's Litter Theorem, we got:
 *  a^255 ≡ 1 -->  a^254 * a ≡ 1
 * 
 * so: 
 *  a^254 is the inverse element of a
 * 
 * and:
 *  254   =   128 +   64 +   32 +   16 +   8 +   4 +   2
 *  a^254 = a^128 * a^64 * a^32 * a^16 * a^8 * a^4 * a^2
  *  it means left-shift for 7,6,5,4,3,2,1
 */
uint8_t gf256_inverse_element(uint8_t a)
{
  uint8_t res = 1;
  for (uint32_t i = 0; i <7; i++) {
    a = gf256_mul(a, a);
    res = gf256_mul(res, a);
  }
  return res;
}




}
