/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-05 17:49:51
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-10 16:55:22
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */



#ifndef MATRIX_H
#define MATRIX_H

#include <cstdint>
#include <vector>
#include <memory>
#include <sstream>

namespace gz_rs_fec {

struct Package {
  Package() = delete;
  Package(uint32_t cap_size);
  ~Package();

  //copy and move cto
  Package(const Package& src);
  Package(Package&& src);

  //copy and move argument
  Package& operator=(const Package& src);
  Package& operator=(Package&& src);

  int32_t  idx_in_group; //only fec dec need this to organize dec-matrix
  uint8_t* buf;
  uint32_t data_size; //current bytes used for data, data_size <= cap_size
  uint32_t cap_size; //capacity size is the max buffer used
  // bool     own_buf;
};

using Matrix1DUint8 = std::vector<uint8_t>;
using Matrix2DUInt8 = std::vector<Matrix1DUint8>;

/**
 * 
 */
class RSMatrixGF256
{
public:
  RSMatrixGF256() = delete;
  //require: 
  //1. n > k, caller should be responsive for this
  //2. k >= 2 (k=1 is meanless)
  RSMatrixGF256(uint32_t n, uint32_t k);
  virtual ~RSMatrixGF256() = default;

  /**
   * all packages should have the same size
   * important item:
   * 1. encode matrix update coderoutine
   * 2. matrix multiplication
   * 
   * @return redundant package only
   */
  //TO IMPLEMENT
  std::vector<Package> Encode(const std::vector<Package>& src_pkg);
  
  /** 
   * 1. dec matrix update coderoutine
   * 2. matrix multiplication
   */
  //TO IMPLEMENT
  std::vector<Package> Decode(const std::vector<Package>& rcv_pkgs);

  //TO IMPLEMENT
  static std::vector<Package> AllocPackages(int32_t data_size, int32_t pkg_count);

  /**
   * for internal unit test
   */
  bool ModuleTest();
private:
  Matrix2DUInt8 
  CreateEncMatriax(uint32_t n, uint32_t k);
  
  //Ex. [1,1,0,1,0,1,1,1,0......]
  //1 means received, 0 means lost
  Matrix2DUInt8 
  CreateDecMatrix(const std::vector<bool>& rcv_indicators);

  /**
   * input: G' which is corresponding to the received package
   * output: decoding matrix 
   * requirement: 
   *    input k x k
   *    output k x k
   */
  inline Matrix2DUInt8 
  GetInverseMatrixByGuassianElemination(Matrix2DUInt8& mat);

  //TO IMPLEMENT
  // inline Matrix1DUint8 MatrixMul(Matrix1DUint8 &left, 
  //                                Matrix1DUint8 &right);


  inline void Switch1DMatrix(Matrix1DUint8 &mat_a, Matrix1DUint8 &mat_b);

  
  /**
   * a = a + b*b_tims
   */
  inline void Matrix1DAddition(uint8_t *a, uint8_t*b, uint8_t b_times, int32_t count);

  //TO IMPLEMENT
  // inline uint8_t Matrix1DDotProduction(uint8_t *a, uint8_t*b, int32_t count);

  /**
   * input: pos_idx of data to encode, and src_pkgs
   * output:
   * require: enc_pkgs count = m, src_pkg count = k
   */
  inline void EncOneByteForAllOutputPkgs(const std::vector<Package>& src_pkgs, 
                                         int32_t byte_idx,
                                        std::vector<Package>& enc_pkgs);

  inline void DecOneBytesForAllPackage(const std::vector<Package>& rcv_pkgs, 
                                       int32_t byte_idx, 
                                       std::vector<Package>& dec_pkgs);

  /**
   * @return true mean valid, false means invalid
   * 
   */
  bool CheckInputPackages(const std::vector<Package>& fec_group);

#ifdef DEBUG
  std::string DbgPrintMatrix1D(const Matrix1DUint8& mat, 
                               const std::string& log_prefix = "", 
                               bool return_log = false);
  void DbgPrintMatrix2D(const Matrix2DUInt8& mat, 
                        const std::string& log_prefix = "");
#endif

  

private:  
  //decided by init parameter: (n,k)
  Matrix2DUInt8 enc_matrix_; 

  //decided by received package index
  Matrix2DUInt8 dec_matrix_;
  //Ex. [1,1,0,1,0] means 3th and 5th package is lost
  //    if it's different from current, a new dec-matrix will be create
  std::vector<bool> last_rcv_pattern_; //size = n
  bool first_dec_;

  //(n, k)
  uint32_t n_;
  uint32_t k_;

#ifdef DEBUG
  std::ostringstream oss_1d_;
  std::ostringstream oss_2d_;
#endif
};

}

#endif 