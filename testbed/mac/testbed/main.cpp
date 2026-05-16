/**
 * @Author: gimphammer@gmail.com
 * @Date: 2026-05-10 17:11:14
 * @LastEditors: gimphammer@gmail.com
 * @LastEditTime: 2026-05-14 19:08:58
 * @Copyright: Copyright (c) 2026 by gimphammer@gmail.com, All rights reserved.
 * @Description: [None]
 */

#include <iostream>
#include "gz_rs_fec.h"
#include <string>
#include <fstream>
#include <filesystem> 
#include "gz_rs_fec.h"
#include <memory>
#include <numeric>
#include <cstdint>
#include <random>
#include <algorithm>
#include <chrono>


//TO IMPLEMENT: set path
//const std::string src_file_path="../../../testdata/lavender_small.jpg";
//const std::string src_file_path="../../../testdata/lavender_small.data";
const std::string src_file_path="../../../testdata/test_video.mp4";
const std::string dst_file_path="./rcv_pic.data";





using namespace gz_rs_fec;
namespace fs = std::filesystem;
using HiResClock = std::chrono::high_resolution_clock;
using DurationNS = std::chrono::duration<int64_t, std::nano>;
constexpr int64_t  kMS2NS = 1000'000;
constexpr int32_t kGroupN = 96;
constexpr int32_t kGroupK = 32;
constexpr int32_t kGroupM = kGroupN - kGroupK; //count for fec
constexpr uint32_t kPkgBufferSize = 1200; //bytes
constexpr uint32_t kGroupSrcBytes = kGroupK * kPkgBufferSize;

static std::random_device g_rd;
static std::mt19937 g_generator(g_rd());
constexpr int32_t kMinSrcPkgCount = std::max((kGroupK - kGroupM), (int32_t)1);
constexpr int32_t kMaxSrcPkgCount = std::max((kGroupK-1), kMinSrcPkgCount);
static std::uniform_int_distribution<int32_t> g_src_dist(kMinSrcPkgCount, kMaxSrcPkgCount);


/**
 * src pick count: 1 ~ k-1
 * fec pick 1 ~ (m - src_count)
 * require: 
 *  1. input: src-vector size = k
 *  2. input: fec vector size = m, m=n-k
 */
bool random_pick_src_and_fec_pkgs(std::vector<int32_t>& src_picks,
                                  std::vector<int32_t>& fec_picks,
                                  uint32_t k,
                                  uint32_t m);

int main(int argc, const char * argv[]) {


  std::cout << "kMinSrcPkgCount=" << kMinSrcPkgCount 
            << ", kMaxSrcPkgCount=" << kMaxSrcPkgCount<<"\n";

  std::fstream read_fs(src_file_path, std::ios::binary | std::ios::in);
  if (!read_fs.is_open()) {
    std::cout << "[ERR][APP] src file open failed. file:" << src_file_path << "\n";
    return -1;
  }
  read_fs.seekg(0, std::ios::end);
  size_t src_file_size = read_fs.tellg();
  read_fs.seekg(0, std::ios::beg);

  if (fs::exists(dst_file_path)) {
    fs::remove(dst_file_path);
  }
  std::fstream write_fs(dst_file_path, std::ios::binary | std::ios::out);
  if (!write_fs.is_open()) {
    //no close of read_fs
    std::cout << "[ERR][APP] dst file open failed. file:" << dst_file_path << "\n";
    return -1;
  }

  //1、打开src文件，获取文件尺寸。
  //2、打开dst文件（删除可能存在的文件）
  //3、确定(n,k)
  //4、确定尾包填充尺寸
  //5、开始编码，模拟丢包
  //6、解包、写文件。  

  auto fec_processor = std::make_unique<RSFECProcessor>(kGroupN, kGroupK);
  std::vector<Package> src_pkgs = 
          RSFECProcessor::AllocPackages(kPkgBufferSize, kGroupK);

  std::vector<Package> rcv_pkgs = 
          RSFECProcessor::AllocPackages(kPkgBufferSize, kGroupK);


  std::vector<int32_t> src_picks(kGroupK);
  std::vector<int32_t> fec_picks(kGroupM);
  
  auto start = std::chrono::high_resolution_clock::now();
  
  
  //Normal Package Process
  DurationNS total_enc_cost_ns;
  DurationNS total_dec_cost_ns;
  int64_t enc_count = 0;
  int64_t dec_count = 0;
  
  uint32_t normal_group_count  = src_file_size / kGroupSrcBytes;
  for (int32_t i = 0; i<normal_group_count; i++) {
    //read data from file
    int32_t step_read_bytes = 0;
    for (int32_t j = 0; j<kGroupK; j++) {
      read_fs.read((char*)src_pkgs[j].buf, kPkgBufferSize);
      step_read_bytes = read_fs.gcount();
    }

    auto start_ts = HiResClock::now();
    std::vector<Package> fec_pkgs = fec_processor->Encode(src_pkgs);
    auto end_ts = HiResClock::now();
    total_enc_cost_ns += end_ts - start_ts;
    enc_count++;
    
    //simulate package lost
    src_picks.resize(kGroupK);
    fec_picks.resize(kGroupM);
    if (!random_pick_src_and_fec_pkgs(src_picks, fec_picks, kGroupK, kGroupM)) {
      std::cout << "[ERR][APP] Pos-A, random_pick_src_and_fec_pkgs failed. \n";
      return -1;
    }

    ///////////////////////////////////////////////////
    
//    src_picks = {0,1,2,4,5,6,7};
//    fec_picks = {2};
    ///////////////////////////////////////

    
    if (src_picks.size() + fec_picks.size() != kGroupK) {
      std::cout << "[ERR][APP] Pos-A src rcv count("<<src_picks.size()
                << "), fec rcv count("<<fec_picks.size()<<"), total count is not "
                << kGroupK <<". application abort!\n";
      return -1;
    }

    //simulate lost senario
    int32_t pkg_rcv_count = 0;
    for (auto index : src_picks) {
      int32_t rcv_pkg_idx = pkg_rcv_count++;
      rcv_pkgs[rcv_pkg_idx] = src_pkgs[index];
      rcv_pkgs[rcv_pkg_idx].idx_in_group = index;
    }

    for (auto index: fec_picks) {
      int32_t rcv_pkg_idx = pkg_rcv_count++;
      rcv_pkgs[rcv_pkg_idx] = fec_pkgs[index];
      rcv_pkgs[rcv_pkg_idx].idx_in_group = index + kGroupK; //fec pkg index starts from k
    }
    
    //at receive endian
    start_ts = HiResClock::now();
    std::vector<Package> dec_pkgs = fec_processor->Decode(rcv_pkgs);
    end_ts = HiResClock::now();
    dec_count++;
    total_dec_cost_ns += end_ts - start_ts;
    
    //write to file
    for (auto pkg: dec_pkgs) {
      write_fs.write((char*)pkg.buf, pkg.data_size);
    }
  }
  
  //statistic for performance
  int64_t ms_per_enc = total_enc_cost_ns.count() / (enc_count*kMS2NS);
  int64_t ms_per_dec = total_dec_cost_ns.count() / (dec_count*kMS2NS);
  std::cout << "Statistics for Normal Pkg: \n"
            << "  (n,k) = ("<<kGroupN<<", "<<kGroupK<<"), data block size=" << kPkgBufferSize<<"\n"
            << "  ms per enc: " << ms_per_enc << ", process count="<<enc_count<<"\n"
            << "  ms per dec: " << ms_per_dec << ", process count="<<dec_count<<"\n";

  //Tail Package Process
  int32_t tail_count = 0;
  int32_t tail_bytes = 0;
  while(1) {
    read_fs.read((char*)src_pkgs[tail_count++].buf, kPkgBufferSize);
    int32_t read_bytes = read_fs.gcount();
    tail_bytes += read_bytes;
    if ( read_bytes != kPkgBufferSize) {
      std::fill_n(src_pkgs[tail_count-1].buf + read_bytes, 
                  (kPkgBufferSize-read_bytes), 0);
      break;
    }
  }
  if (tail_count > kGroupK) {
    std::cout << "[ERR][APP] tail_count("<<tail_count<<") > k("<<kGroupK
              <<"), abort!!\n";
    return -1;
  }

  if (kGroupK - tail_count) {
    for (int32_t i=tail_count; i<kGroupK; i++) {
      std::fill_n(src_pkgs[i].buf, kPkgBufferSize, 0);
    }
  }

  std::vector<Package> fec_pkgs = fec_processor->Encode(src_pkgs);

  src_picks.resize(kGroupK);
  fec_picks.resize(kGroupM);
  if (!random_pick_src_and_fec_pkgs(src_picks, fec_picks, kGroupK, kGroupM)) {
    std::cout << "[ERR][APP] Pos-B, random_pick_src_and_fec_pkgs failed. \n";
    return -1;
  }

  if (src_picks.size() + fec_picks.size() != kGroupK) {
    std::cout << "[ERR][APP] Pos-B src rcv count("<<src_picks.size()
              << "), fec rcv count("<<fec_picks.size()<<"), total count is not "
              << kGroupK <<". application abort!\n";
    return -1;
  }

  //simulate lost senario
  int32_t pkg_rcv_count = 0;
  for (auto index : src_picks) {
    int32_t rcv_pkg_idx = pkg_rcv_count++;
    rcv_pkgs[rcv_pkg_idx] = src_pkgs[index];
    rcv_pkgs[rcv_pkg_idx].idx_in_group = index;
  }

  for (auto index: fec_picks) {
    int32_t rcv_pkg_idx = pkg_rcv_count++;
    rcv_pkgs[rcv_pkg_idx] = fec_pkgs[index];
    rcv_pkgs[rcv_pkg_idx].idx_in_group = index + kGroupK; //fec pkg index starts from k
  }
  
  //at receive endian

  std::vector<Package> dec_pkgs = fec_processor->Decode(rcv_pkgs);

  for (auto &pkg: dec_pkgs) {
    if (!tail_bytes)
      break;

    int32_t write_bytes = (tail_bytes > kPkgBufferSize)
                          ? kPkgBufferSize : tail_bytes;

    
    tail_bytes = (tail_bytes > kPkgBufferSize)
                 ? tail_bytes - kPkgBufferSize : 0;
    write_fs.write((char*)pkg.buf, write_bytes);
  }

  // insert code here...
  std::cout << "Hello, World!\n";
  read_fs.close();
  write_fs.close();
  return EXIT_SUCCESS;
}


bool random_pick_src_and_fec_pkgs(std::vector<int32_t> &src_picks,/* size = k*/
                                  std::vector<int32_t> &fec_picks,/* size = n-k */
                                  uint32_t k,
                                  uint32_t m)
{


  std::iota(src_picks.begin(), src_picks.end(), 0);
  std::iota(fec_picks.begin(), fec_picks.end(), 0);
  
  uint32_t src_count = g_src_dist(g_generator);
  uint32_t fec_count = k - src_count;
  
  std::shuffle(src_picks.begin(), src_picks.end(), g_generator);
  std::shuffle(fec_picks.begin(), fec_picks.end(), g_generator);

  src_picks.resize(src_count);
  fec_picks.resize(fec_count);

  std::sort(src_picks.begin(), src_picks.end());
  std::sort(fec_picks.begin(), fec_picks.end());
  
  return true;
}
