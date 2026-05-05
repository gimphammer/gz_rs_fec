/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-03 15:32:09
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-04 18:25:30
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */



#ifndef MISC_H
#define MISC_H

#include <cstdint>

namespace gz_rs_fec {
uint8_t gf256_mul(uint8_t a, uint8_t b);
uint8_t gf256_inverse_element(uint8_t a);


}
#endif
