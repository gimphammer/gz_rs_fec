/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-05 17:49:53
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-10 17:36:57
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */




#include "internal_def.h"
#include "gz_rs_fec.h"
#include "misc.h"
#include <iomanip>
#include <iostream>

namespace gz_rs_fec {


Package::Package()
{
  this->buf = nullptr;
  this->data_size = 0;
  this->cap_size  = 0;
  this->idx_in_group = -1;  
}


Package::Package(uint32_t cap_size)
{  
  this->buf = new uint8_t[cap_size];
  this->data_size = 0;
  this->cap_size  = cap_size;
  this->idx_in_group = -1;
}

Package::~Package()
{
  delete[] buf;
  buf = nullptr;
}

Package::Package(const Package& src)
{
  buf = nullptr;
  this->data_size = 0;
  this->cap_size  = 0;
  this->idx_in_group = -1;

  if (src.cap_size) {
    buf = new uint8_t[src.cap_size];
    if (src.data_size) {
      std::memcpy(this->buf, src.buf, src.data_size);
      this->data_size = src.data_size;
    }
    this->cap_size = src.cap_size;    
  }
  this->idx_in_group = src.idx_in_group;

}

Package::Package(Package&& src)
{
  this->buf = src.buf;
  src.buf = nullptr;

  this->data_size = src.data_size;
  src.data_size = 0;

  this->cap_size = src.cap_size;
  src.cap_size = 0;

  this->idx_in_group = src.idx_in_group;
  src.idx_in_group = -1;
}

Package& Package::operator=(const Package& src)
{
  if (&src == this)
    return *this;


  delete[] buf;
  buf = nullptr;
  this->data_size = 0;
  this->cap_size  = 0;
  this->idx_in_group = -1;  

  if (src.cap_size) {
    buf = new uint8_t[src.cap_size];
    if (src.data_size) {
      std::memcpy(this->buf, src.buf, src.data_size);
      this->data_size = src.data_size;
    }
    this->cap_size = src.cap_size;    
  }
  this->idx_in_group = src.idx_in_group;

  return *this;
}


Package& Package::operator=(Package&& src)
{
  if (&src == this)
    return *this;


  delete[] buf;
  buf = nullptr;
  this->data_size = 0;
  this->cap_size  = 0;
  this->idx_in_group = -1;  

  this->buf          = src.buf;
  this->data_size    = src.data_size;
  this->cap_size     = src.cap_size;
  this->idx_in_group = src.idx_in_group;
  
  src.buf       = nullptr;
  src.cap_size  = 0;
  src.data_size = 0;
  src.idx_in_group = -1;

  return *this;
}

RSFECProcessor::RSFECProcessor(uint32_t n, uint32_t k) :
          n_(n),
          k_(k),
          enc_matrix_(CreateEncMatriax(n,k)),
          last_rcv_pattern_(std::vector<bool>(n))
{

}


/**
 * [R] * X 
 * 
 * generate redundant packages m = n - k;
 * 
 */
std::vector<Package> 
RSFECProcessor::Encode(const std::vector<Package>& src_pkgs)
{
  if (!CheckInputPackages(src_pkgs)) {
    std::cout << "[ERR] RSFECProcessor::Encode() input package invalid";
    return {};
  }

  if (src_pkgs.size() != k_){
    std::cout << "[ERR] RSFECProcessor::Encode() input packages count is"
              << src_pkgs.size() << ", not " << k_;
    return {};
  }

  std::vector<Package> ret_pkgs = AllocPackages(src_pkgs[0].data_size, 
                                                  src_pkgs.size());
  
  int32_t data_len = src_pkgs[0].data_size;
  Matrix1DUint8 x(k_);
  

  for (int32_t i=0; i<data_len; i++) {
    EncOneByteForAllOutputPkgs(src_pkgs, i, ret_pkgs);
  }

  return ret_pkgs;
}


std::vector<Package> 
RSFECProcessor::Decode(const std::vector<Package>& rcv_pkgs)
{
  if (!CheckInputPackages(rcv_pkgs)) {
    std::cout << "[ERR] RSFECProcessor::Decode() input package invalid";
    return {};
  }
  
  if (rcv_pkgs.size() < k_) {
    std::cout << "[ERR] RSFECProcessor::Decode(), input package count < k";
    return {};    
  }

  std::vector<bool> current_rcv_pattern(n_);
  for (int32_t i=0; i < rcv_pkgs.size(); i++) {
    int32_t pkg_seq_id = rcv_pkgs[i].idx_in_group;
    current_rcv_pattern[pkg_seq_id] = 1;
  }
  

  if (first_dec_ || current_rcv_pattern != last_rcv_pattern_) {    
    dec_matrix_ = CreateDecMatrix(current_rcv_pattern);
    last_rcv_pattern_ = current_rcv_pattern;

    if (first_dec_)
      first_dec_ = false;
  }

  int32_t pkg_count = static_cast<int32_t>(rcv_pkgs.size());
  std::vector<Package> ret_pkgs = AllocPackages(rcv_pkgs[0].data_size,
                                                pkg_count);

  uint32_t data_len = rcv_pkgs[0].data_size;
  for (uint32_t i=0; i<data_len; i++) {
    DecOneBytesForAllPackage(rcv_pkgs, i, ret_pkgs);
  }
  
  return ret_pkgs;
} //end of Decode


std::vector<Package> RSFECProcessor::AllocPackages(int32_t data_size, 
                                                  int32_t pkg_count)
{
  std::vector<Package> pkgs(pkg_count);

  for (auto pkg : pkgs) {
    pkg.buf = new uint8_t[data_size];
    pkg.data_size = data_size;
    pkg.cap_size = data_size;
  }


  return pkgs;
}



void RSFECProcessor::EncOneByteForAllOutputPkgs(const std::vector<Package>& src_pkgs, 
                                              int32_t data_idx,
                                              std::vector<Package>& enc_pkgs)
{
  //m is the count of redundant pkg count
  int32_t m = n_ - k_; 
  for (int32_t i=0; i<m; i++) {
    uint8_t result = 0;
    for (int32_t j=0; j<k_; j++) {
      result ^= gf256_mul(enc_matrix_[k_ + i][j], src_pkgs[j].buf[data_idx]);
    }
    
    enc_pkgs[i].buf[data_idx] = result;
  }
  return;
}

/**
 * D = Gs^-1 
 * [X] = [D] * [Y] 
 */
void RSFECProcessor::DecOneBytesForAllPackage(const std::vector<Package>& rcv_pkgs, 
                                             int32_t byte_idx, 
                                             std::vector<Package>& dec_pkgs)
{
  int32_t m = n_ - k_;
  for (int32_t i=0; i<m; i++){
    uint8_t result = 0;

    for (int32_t j=0; j<k_; j++) {
      result ^= gf256_mul(dec_matrix_[i][j], rcv_pkgs[j].buf[byte_idx]);
    }
    dec_pkgs[i].buf[byte_idx] = result;    
  }
  return;
}

bool RSFECProcessor::CheckInputPackages(const std::vector<Package>& fec_group)
{
  if (fec_group.empty())
    return false;
  
  int32_t data_len = fec_group[0].data_size;
  for (int32_t i=1; i<fec_group.size(); i++) {
    if (data_len != fec_group[i].data_size)
      return false;
  }

  return true;
}


/**
 *  Completed Enc Matrix is like this:
 *    [I, R]^1  -- where R stands for Reduntant-Matrix
 *  as below:
 *    1, 0, 0, ...., 0
 *    1, 0, 0, ...., 0
 *    ................
 *    0, 0, 0, ...., 1
 * 
 *    c1,c2,c3,....,ck   => x_0+y_i  i<k
 *    d1,d2,d3,....,dk
 *    ................
 *    m1,m2,m3,....,mk   => x_m+y_i  i<k
 * 
 *  but, only R is needed, because we only need redundant package
 *  the orignial package can be send directly.
 * 
 */
Matrix2DUInt8 
RSFECProcessor::CreateEncMatriax(uint32_t n, uint32_t k)
{
  Matrix2DUInt8 enc_mat(n, Matrix1DUint8(k));
  for (uint32_t i=0; i<k; i++) {
    enc_mat[i][i] = 1;
  }

  //construct x_i and y_i used for Cauchy Maxtrix
  uint32_t m = n-k;
  Matrix1DUint8 y(k);
  for (uint32_t i=0; i<k; i++) {
    y[i] = static_cast<uint8_t>(i);
  }

  Matrix1DUint8 x(m);
  for (uint32_t i=0; i<m; i++) {
    x[i] = static_cast<uint8_t>(k+i);
  }

  for (uint32_t i=k; i<n; i++) {
    for (uint32_t j=0; j<k; j++) {
      enc_mat[i][j] = gf256_inverse_element(x[i-k]^y[j]);
    }
  }

#ifdef DEBUG
  DbgPrintMatrix2D(enc_mat);
#endif

  return enc_mat;
}



/**
 *  Basic Formular: * 
 *    Gs * X = Rs
 *    Gs^-1 * Gs * X = Gs^-1 * Rs
 *    X = Gs^-1 * Rs
 * 
 *  Notes:
 *    Gs = G selected
 *    Rs = Receive selected
 * 
 *    Gs contains the lines selected from enc matrix, its dimension is k x k
 *    Every line in Gs is corresponding to the package we received, but no more
 *    than k.
  
 *  Acoording to Guass Elimination:
 *    [I, Gs]  -->  [Gs^1, I]
 *  Than we can got Gs^1, which is just the dec matrix, we rename is as D
 */
Matrix2DUInt8 
RSFECProcessor::CreateDecMatrix(const std::vector<bool>& rcv_indicators)
{  
  //indicate status of all n package, so size should be n_
  //ture mea
  if (rcv_indicators.size() != n_)
    return {};

  //1. get selected lines from enc_matrix to organize the g_selected_mat
  Matrix2DUInt8 g_selected_mat(k_, Matrix1DUint8(k_));
  uint32_t rcv_count=0;
  for (int32_t i=0; i<n_, rcv_count == k_; i++) {
    if (rcv_indicators[i]) {
      g_selected_mat[rcv_count] = enc_matrix_[i];
      rcv_count++;
    }
  }

  if (rcv_count != k_)
    return {};

  //2. us Gauss Elimination to get the 
  Matrix2DUInt8 dec_mat = 
    GetInverseMatrixByGuassianElemination(g_selected_mat);

  return dec_mat;
}

/**
 * Gaussian Elemination is bound to GF2(2^8) and set value of k
 * input mat should be k x k, and invoker should be responsible for dimensions
 * 
 * Note:
 *  [I, A]  -->  [A^-1, I]
 */
Matrix2DUInt8
RSFECProcessor::GetInverseMatrixByGuassianElemination(Matrix2DUInt8& mat)
{
  
  int32_t k = mat.size();
  //1. construct [I, A]
  Matrix2DUInt8 work_mat(k, Matrix1DUint8(k*2));
  for (int32_t line = 0; line < k; line++) {
    work_mat[line][line] = 1;
    
    for (int32_t column = k; column < 2*k; column++) {
      work_mat[line][column] = mat[line][column - k];
    }    
  }

#ifdef DEBUG
  DbgPrintMatrix2D(mat, "input mat for caculating inverse matrix: ");
  DbgPrintMatrix2D(work_mat, "init [I, A] = ");
#endif 


  //2. [I, A] --> [A^-1, I]
  //2.1 trans A to upper triangular matrix
  for (int32_t line = 0; line < k; line++) {
    //prepare line with pivot required
    int32_t target_y_pos = line + k;
    if (0 == work_mat[line][target_y_pos]) {
      //如果pivot位置的元素等于0，则要寻找其他line的这个pivot位置不为0的行，
      //并交换到当前行
      bool find_pivot = false;
      for (int32_t line_2_find = line + 1; line_2_find < k; line_2_find++) {
        if (work_mat[line_2_find][target_y_pos]) {          
          Switch1DMatrix(work_mat[line_2_find], work_mat[line]);
          find_pivot = true;
        }        
      }

      if (!find_pivot) {
        std::cout << "[ERR] no pivot is found. current line idx=" 
                  << line << "\n";
        return {};
      }
    }
  }

  /**
   * 2.2 perform elimination back to eliminate all element which
   *     is not equal to zero
   *    t1 t2 t3 ... t4 t5 t6 | 1, a, b, ..., d, e, f
   *    p1 p2 p3 ... p4 p5 p6 | 0, 1, g, ..., h, i, l
   *                          |     ......
   *    o1 o2 o3 ... o4 o5 o6 | 0, 0, 0, .... 1, x, y
   *    u1 u2 u3 ... u4 u5 u6 | 0, 0, 0, .... 0, 1, z
   *    h1 h2 h3 ... h4 h5 h6 | 0, 0, 0, ..., 0, 0, 1
   */

  //last line is no need to to the elimination...
  //so start from: k - 2
  for (int32_t line = k-2; line >= 0; line--) {
    //根据line行右上角非1元素的个数进行循环。
    //从每行最后一个位置开始，依次往前处理到 1的位置：
    uint8_t* current_line = work_mat[line].data();
    for (int32_t column_idx = 2*k - 1; column_idx > line + k; column_idx--) {
      int32_t down_line_index = column_idx - k;
      uint8_t* down_line = work_mat[down_line_index].data();
      
      uint8_t factor = work_mat[line][column_idx];      
      //process such as: p1 p2 p3 ... p4 p5 p6, show above
      Matrix1DAddition(current_line, down_line, factor, k);

      //process such as: x,y,z shown above
      //set tail element to zero
      work_mat[line][column_idx] = 0;
    }
  }

  Matrix2DUInt8 ret(k, Matrix1DUint8(k));
  for (int32_t i; i<k; i++) {
    std::memcpy(ret[i].data(), work_mat[i].data(), k);
  }
  return ret;
}



void RSFECProcessor::Switch1DMatrix(Matrix1DUint8 &mat_a, Matrix1DUint8 &mat_b)
{
  if (mat_a.size() != mat_b.size()) {
    std::cout << "[ERR] Switch1DMatrix() a.size() != b.size()";
    return;
  }

  uint8_t temp;
  auto a = mat_a.begin(), b = mat_b.begin(); 
  for (; a != mat_a.end(), b != mat_b.end(); a++, b++) {
    temp = *a;
    *a = *b;
    *b = temp;
  }
  return;
}

/**
 * a = a + b*b_tims
 */
void RSFECProcessor::Matrix1DAddition(uint8_t *a, uint8_t*b, 
                                     uint8_t b_times, int32_t count)
{
  for (int32_t i=0; i<count; i++) {
    uint8_t to_add = gf256_mul(b[i], b_times);
    a[i] ^= to_add;
  }
  return;
}

#ifdef DEBUG
/**
 * pattern:
 *  {0x11, 0x12, 0x92, ...., 0x85}
 */

std::string RSFECProcessor::DbgPrintMatrix1D(const Matrix1DUint8& mat,
                                            const std::string& log_prefix, 
                                            bool return_log)
{
  oss_1d_.str("");

  if (!log_prefix.empty()) {
    oss_1d_ << log_prefix;
  }

  oss_1d_ << std::hex << std::setfill('0');
  //auto elem : mat)
  for (auto it=mat.begin(); it!=mat.end(); it++) {
    if (mat.begin() == it) {
      //first value
      oss_1d_ << "{0x" << std::setw(2) << static_cast<uint32_t>(*it)
           << ", ";
    }
    else if (std::next(it) == mat.end()) {
      //last value
      oss_1d_ << "0x" << std::setw(2) << static_cast<uint32_t>(*it)
           << "}\n";
    }
    else {
      //middle value
      oss_1d_ << "0x" << std::setw(2) << static_cast<uint32_t>(*it)
           << ", "; 
    }
  }

  if (return_log) {
    return oss_1d_.str();
  }
  else {
    std::cout << oss_1d_.str();
    return "";
  }

}


/**
 * pattern:
 *  {
 *    {},
 *    {},
 *    {},
 *  }
 */
void RSFECProcessor::DbgPrintMatrix2D(const Matrix2DUInt8& mat,
                                     const std::string& log_prefix)
{
  oss_2d_.str("");

  if (!log_prefix.empty()) {
    oss_2d_ << log_prefix << "\n";
  }
  oss_2d_ << "{\n";

  for (auto it = mat.begin(); it != mat.end(); it++) {
    oss_2d_ << "  " << DbgPrintMatrix1D(*it, "", true) << ",\n";
  }

  oss_2d_ << "}\n";
  std::cout << oss_2d_.str();
}
#endif



}
