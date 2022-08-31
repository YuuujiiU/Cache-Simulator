/*
   Cache Simulator
   Level one L1 and level two L2 cache parameters are read from file
   (block size, line per set and set per cache).
   The 32 bit address is divided into:
   -tag bits (t)
   -set index bits (s)
   -block offset bits (b)

   s = log2(#sets)   b = log2(block size)  t=32-s-b
*/

#include <stdlib.h>

#include <bitset>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "./cache.hpp"

int main(int argc, char* argv[]) {
  config cacheconfig;
  std::ifstream cache_params;
  std::string dummyLine;
  cache_params.open(argv[1]);

  // read config file
  while (!cache_params.eof()) {
    cache_params >> dummyLine;
    cache_params >> cacheconfig.L1blocksize;
    cache_params >> cacheconfig.L1setsize;
    cache_params >> cacheconfig.L1size;
    cache_params >> dummyLine;
    cache_params >> cacheconfig.L2blocksize;
    cache_params >> cacheconfig.L2setsize;
    cache_params >> cacheconfig.L2size;
  }

  // Implement by you:
  // initialize the hirearch cache system with those configs
  // probably you may define a Cache class for L1 and L2, or any data structure
  // you like
  Cache cache = Cache(cacheconfig);

  // // L1 access state variable, can be one of NA, RH, RM, WH, WM;
  // int L1AcceState = 0;
  // // L2 access state variable, can be one of NA, RH, RM, WH, WM;
  // int L2AcceState = 0;

  std::ifstream traces;
  std::ofstream tracesout;
  std::string outname;
  outname = std::string(argv[2]) + ".out";

  traces.open(argv[2]);
  tracesout.open(outname.c_str());

  std::string line;
  std::string accesstype;  // the Read/Write access type from the memory trace;
  std::string xaddr;       // the address from the memory trace store in hex;

  // the address from the memory trace store in unsigned int;
  unsigned int addr;
  // the address from the memory trace store in the std::bitset;
  std::bitset<32> accessaddr;
  int i = 1;

  if (traces.is_open() && tracesout.is_open()) {
    while (getline(traces, line)) {  // read mem access file and access Cache
      std::istringstream iss(line);
      if (!(iss >> accesstype >> xaddr)) {
        break;
      }
      std::stringstream saddr(xaddr);
      saddr >> std::hex >> addr;
      accessaddr = std::bitset<32>(addr);
      std::cout << std::endl;
      std::cout << "======================================" << i << std::endl;
      i++;
      std::cout << "access addr in hex: " << xaddr << std::endl;
      std::cout << "access addr in bits: " << accessaddr << std::endl;

      // access the L1 and L2 Cache according to the trace;
      if (accesstype.compare("R") == 0) {
        // Implement by you:
        //  read access to the L1 Cache,
        //   and then L2 (if required),
        //   update the L1 and L2 access state variable;
        cache.read(accessaddr);
      } else if (accesstype.compare("W") == 0) {
        // Implement by you:
        //  write access to the L1 Cache,
        // and then L2 (if required),
        // update the L1 and L2 access state variable;
        cache.write(accessaddr);
      } else {
        std::cout << "UNSUPPORTED ADDCESS TYPE" << std::endl;
        exit(EXIT_FAILURE);
      }

      // Output hit/miss results for L1 and L2 to the output file
      tracesout << cache.getL1AccessState() << " " << cache.getL2AccessState()
                << std::endl;
      std::cout << "access state: " << cache.getL1AccessState() << " "
                << cache.getL2AccessState() << std::endl;
      cache.resetAccessStates();
    }
    traces.close();
    tracesout.close();
  } else {
    std::cout << "Unable to open trace or traceout file ";
  }

  return 0;
}
