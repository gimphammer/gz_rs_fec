/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-03 15:25:34
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-05 14:43:29
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */

#include <iostream>
#include <sstream>
#include "misc.h"
#include <iomanip>

//#define SHOW_LOG

void genGF256_mul_table();
void testGF256_mul();
void generateInverseElementTable();

int main(int argc, const char * argv[]) {

  // genGF256_mul_table();
  // testGF256_mul();
  generateInverseElementTable();
  return EXIT_SUCCESS;
}


void genGF256_mul_table() {
  
  std::ostringstream oss;
  oss << "GF(2^8) multiplication table, 256x256: \n";
    
  const int total_steps = 256*256;
  int last_permillage = 0;
  
  for (int i=0; i<256; i++)
  {
    if (0==i%8) {
      oss << std::dec;
      oss << "//line:" << i << "\n";
    }
    
    oss << std::hex << std::setfill('0');
    oss << "{";
    for (int j=0; j<256; j++) {
      uint8_t res = gz_rs_fec::gf256_mul(static_cast<uint8_t>(i),
                                         static_cast<uint8_t>(j));

      //文本输出
      if (255 == j) {
        oss << "0x" << std::setw(2)  <<(int)res;
      }
      else {
        oss << "0x" << std::setw(2)  <<(int)res <<", ";
      }
      
      int cur_permillage = (i*256 + j + 1)*1000/total_steps;
      if (cur_permillage > last_permillage) {
        last_permillage = cur_permillage;
        double percentage = cur_permillage * 100 / 1000.0f;
        std::cout << "\r";
        std::cout << "progress: " << percentage << "%          ";
      }
    }

    //文本输出
    if (0xFF == i) {
      oss << "}\n";
    }
    else {
      oss << "},\n";
    }
   
    
  }

  // insert code here...
  std::cout << oss.str();
  std::cout << "---------==============----------\n";
}


void testGF256_mul()
{
  uint8_t a = 223;  //0b1101'1111
  uint8_t b = 12;   //0b0000'1100

  uint8_t res = gz_rs_fec::gf256_mul(a,b);
  int32_t int_res = res;
  
  
  
  std::cout << "In GF(2^8): " << (int)a << "x" << (int)b
            << "=0x"
            << std::hex << std::setfill('0')
            << std::setw(4) << (int)res << "\n";

  
  
  return;
}

void generateInverseElementTable()
{
  std::ostringstream oss;
  oss << "GF(2^8) inverse element: \n";
  oss << "{\n";
  oss << std::hex << std::setfill('0');
  for (int i=0; i<256; i++) {
    uint8_t res = gz_rs_fec::gf256_inverse_element(static_cast<uint8_t>(i));

    if (0 == i%16){
      oss << std::dec;
      oss << "//start index: " << i << "\n";
      oss << std::hex << std::setfill('0');
      oss << "{0x" << std::setw(2) << (int)res << ", ";
    }
    else if (15 == i%16)
      oss << "0x" << std::setw(2) << (int)res << "},\n";
    else
      oss << "0x" << std::setw(2) << (int)res << ", ";

  }
  oss << "}\n";

  std::cout << oss.str();

  int aaaa = 1;
  return;
}
