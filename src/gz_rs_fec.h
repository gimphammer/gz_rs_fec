/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-01 11:51:00
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-01 12:19:35
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: API definition
 */


#ifndef RS_FEC_GZ_H
#define RS_FEC_GZ_H

#include <cstdint>
#include <vector>

namespace gz_rs_fec {

struct RSBuffer
{
  uint8_t *data;
  int32_t size;
};


class ReedSolomonFECProcessor
{
public:
  ReedSolomonFECProcessor() = delete;
  ReedSolomonFECProcessor(int32_t n, int32_t k);
  virtual ~ReedSolomonFECProcessor();

  std::vector<RSBuffer> Encode(const std::vector<RSBuffer>& input_datas);
  std::vector<RSBuffer> Decode(const std::vector<RSBuffer>& input_datas);

private:
  int32_t mem_temp;
  
};

}





#endif