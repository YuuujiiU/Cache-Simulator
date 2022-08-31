#ifndef CACHE_HPP_
#define CACHE_HPP_

#include <bitset>
#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

// access state:
typedef int AccessState;
#define NA 0  // no action
#define RH 1  // read hit
#define RM 2  // read miss
#define WH 3  // Write hit
#define WM 4  // write miss

struct config {
  size_t L1blocksize;
  size_t L1setsize;
  size_t L1size;
  size_t L2blocksize;
  size_t L2setsize;
  size_t L2size;
};

/* you can define the cache class here, or design your own data structure for L1
   and L2 cache
*/
class LowLevelCache {
 public:
  LowLevelCache() {
    this->tagPos = std::make_pair(0, 0);
    this->setIndexPos = std::make_pair(0, 0);
    this->blockOffsetPos = std::make_pair(0, 0);
    this->wayCount = 0;
    this->wayEvict = 0;
    this->accessState = NA;
    this->tagVal = 0;
    this->setIndexVal = 0;
    this->blockOffsetVal = 0;
    this->tagBitsBuffer = std::vector<std::vector<unsigned long> >();
    this->validBitsBuffer = std::vector<std::vector<bool> >();
    std::cout << "\t\tnew LowLevelCache object created, uninitialized"
              << std::endl;
  }

  LowLevelCache(int blockSize, int associativity, int cacheSize) {
    int tagLength, setIndexLength, blockOffsetLength;

    if (associativity == 0) {
      setIndexLength = log2(cacheSize * pow(2, 10) / blockSize);
    } else {
      setIndexLength =
          log2(cacheSize * pow(2, 10) / (blockSize * associativity));
    }
    blockOffsetLength = log2(blockSize);
    tagLength = 32 - setIndexLength - blockOffsetLength;

    this->tagPos = std::make_pair(0, tagLength);
    this->setIndexPos = std::make_pair(tagLength, setIndexLength);
    this->blockOffsetPos =
        std::make_pair(tagLength + setIndexLength, blockOffsetLength);

    this->wayCount = associativity;
    this->wayEvict = 0;

    for (int i = 0; i < std::pow(2, setIndexLength); i++) {
      this->tagBitsBuffer.push_back(
          std::vector<unsigned long>(this->wayCount, 0));
      this->validBitsBuffer.push_back(std::vector<bool>(this->wayCount, false));
    }

    if (associativity == 0) {
      std::cout << "\t\tFully associative" << std::endl;
    } else if (associativity == 1) {
      std::cout << "\t\tDirectly mapped" << std::endl;
    } else {
      std::cout << "\t\t" << associativity << "-way associative" << std::endl;
    }
    std::cout << "\t\ttag bits: start-" << tagPos.first << " len-"
              << tagPos.second << std::endl;
    std::cout << "\t\tset index bits: start-" << setIndexPos.first << " len-"
              << setIndexPos.second << std::endl;
    std::cout << "\t\tblock Offset bits: start-" << blockOffsetPos.first
              << " len-" << blockOffsetPos.second << std::endl;
    std::cout << "\t\tcache index: " << std::pow(2, setIndexLength)
              << std::endl;
  }

  void decodeAddress(std::bitset<32> addr) {
    std::string addrStr = addr.to_string();
    this->tagVal =
        std::bitset<32>(addrStr.substr(tagPos.first, tagPos.second)).to_ulong();
    this->setIndexVal =
        std::bitset<32>(addrStr.substr(setIndexPos.first, setIndexPos.second))
            .to_ulong();
    this->blockOffsetVal =
        std::bitset<32>(
            addrStr.substr(blockOffsetPos.first, blockOffsetPos.second))
            .to_ulong();
    std::cout << "\t\ttag value: " << this->tagVal << std::endl;
    std::cout << "\t\tset index: " << this->setIndexVal << std::endl;
    std::cout << "\t\tblk offst: " << this->blockOffsetVal << std::endl;
  }

  // Read/Write cache
  void access(std::bitset<32> addr, bool isRead) {
    this->decodeAddress(addr);
    if (isRead == true) {
      this->accessState = RM;
    } else {
      this->accessState = WM;
    }
    for (int way = 0; way < this->wayCount; way++) {
      if ((this->tagBitsBuffer.at(this->setIndexVal).at(way) == this->tagVal) &&
          (this->validBitsBuffer.at(this->setIndexVal).at(way) == true)) {
        if (isRead == true) {
          this->accessState = RH;
        } else {
          this->accessState = WH;
        }
        break;
      }
    }
  }

  /**
   * @brief Add new tag value on read miss.
   * If invalid/empty ways were found, put new tag value in there,
   * If set is full, evict a block based on evict counter.
   *
   */
  void update() {
    for (int way = 0; way < this->wayCount; way++) {
      if (this->validBitsBuffer.at(this->setIndexVal).at(way) == false) {
        this->validBitsBuffer.at(this->setIndexVal).at(way) = true;
        this->tagBitsBuffer.at(this->setIndexVal).at(way) = this->tagVal;
        std::cout << "\t\tWay " << way << " invalid/empty, cache updated"
                  << std::endl;
        return;
      }
    }

    // all ways are valid, set full
    this->tagBitsBuffer.at(this->setIndexVal).at(wayEvict) = this->tagVal;
    std::cout << "\t\tSet full, way " << wayEvict << " evicted, cache updated"
              << std::endl;

    // increment eviction counter
    wayEvict++;
    if (wayEvict == wayCount) {
      wayEvict = 0;
    }
  }

  /**
   * @brief Back invalidate L1 when L2 evicts
   * If the evicted block is updated in L1, do nothing
   * If the evicted block is not in L1, do nothing
   * If the evicted block is in L1 and not updated, its valid bit turns off
   *
   * @param tagVal the L2-evicted block to find in L1
   */
  void backInvalidate(unsigned long tagVal) {
    if (tagVal == this->tagVal) {
      std::cout << "\t\tupdated" << std::endl;
      return;
    }
    for (int set = 0; set < this->tagBitsBuffer.size(); set++) {
      for (int way = 0; way < this->wayCount; way++) {
        if (this->tagBitsBuffer.at(set).at(way) == tagVal) {
          this->validBitsBuffer.at(set).at(way) = false;
          std::cout << "\t\tinvalidated" << std::endl;
          break;
        }
      }
    }
    std::cout << "\t\tnot found" << std::endl;
  }

  unsigned long getTagVal() { return this->tagVal; }

  AccessState getAccessState() { return this->accessState; }

  void resetStates() {
    this->tagVal = 0;
    this->setIndexVal = 0;
    this->blockOffsetVal = 0;
    this->accessState = NA;
  }

 private:
  // first parameter is starting index in the address
  // second parameter is offset
  std::pair<int, int> tagPos;
  std::pair<int, int> setIndexPos;
  std::pair<int, int> blockOffsetPos;

  // equals to associativity, for read traversal
  int wayCount;
  // for eviction when all ways are full
  int wayEvict;

  // temporary states for update access
  unsigned long tagVal;
  unsigned long setIndexVal;
  unsigned long blockOffsetVal;

  AccessState accessState;

  // first layer is to select set
  // second layer is to select way inside a set
  std::vector<std::vector<unsigned long> > tagBitsBuffer;
  std::vector<std::vector<bool> > validBitsBuffer;
};

class Cache {
 public:
  Cache(config cacheConfig) {
    std::cout << "\tL1 Cache" << std::endl;
    this->l1Cache = LowLevelCache(cacheConfig.L1blocksize,
                                  cacheConfig.L1setsize, cacheConfig.L1size);
    std::cout << "\tL2 Cache" << std::endl;
    this->l2Cache = LowLevelCache(cacheConfig.L2blocksize,
                                  cacheConfig.L2setsize, cacheConfig.L2size);
  }

  void read(std::bitset<32> addr) {
    std::cout << "\tL1 Cache Read" << std::endl;
    this->l1Cache.access(addr, true);
    if (this->l1Cache.getAccessState() == RH) {
      std::cout << "\tL1 Cache Read Hit" << std::endl;
      std::cout << "\tL2 Cache No Access" << std::endl;
      return;
    }
    std::cout << "\tL1 Cache Read Miss" << std::endl;
    std::cout << "\tL2 Cache Read" << std::endl;
    this->l2Cache.access(addr, true);
    if (this->l2Cache.getAccessState() == RH) {
      std::cout << "\tL2 Cache Read Hit" << std::endl;
      this->l1Cache.update();
      return;
    }
    std::cout << "\tL2 Cache Read Miss" << std::endl;
    std::cout << "\tL1 Update" << std::endl;
    this->l1Cache.update();
    std::cout << "\tL2 Update" << std::endl;
    this->l2Cache.update();
    std::cout << "\tL1 Back Invalidate" << std::endl;
    this->l1Cache.backInvalidate(l2Cache.getTagVal());
  }

  void write(std::bitset<32> addr) {
    std::cout << "\tL1 Cache Write" << std::endl;
    this->l1Cache.access(addr, false);
    if (this->l1Cache.getAccessState() == WH) {
      std::cout << "\tL1 Cache Write Hit" << std::endl;
      std::cout << "\tL2 Cache No Access" << std::endl;
      return;
    }
    std::cout
        << "\tL1 Cache Write Miss, forward to L2 Cache (Write-no-allocate)"
        << std::endl;
    std::cout << "\tL2 Cache Write" << std::endl;
    this->l2Cache.access(addr, false);
    if (this->l2Cache.getAccessState() == WH) {
      std::cout << "\tL2 Cache Write Hit" << std::endl;
      return;
    }
    std::cout
        << "\tL2 Cache Write Miss, forward to main memory (Write-no-allocate)"
        << std::endl;
  }

  AccessState getL1AccessState() { return this->l1Cache.getAccessState(); }

  AccessState getL2AccessState() { return this->l2Cache.getAccessState(); }

  void resetAccessStates() {
    this->l1Cache.resetStates();
    this->l2Cache.resetStates();
  }

 private:
  LowLevelCache l1Cache;
  LowLevelCache l2Cache;
};

#endif  // CACHE_HPP_
