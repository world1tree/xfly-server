//
// Created by zaiheshi on 1/28/23.
//
#include "xfly_memory.h"
#include <cstring>
Memory* Memory::m_instance = nullptr;     // 类的静态成员变量赋值
void *Memory::AllocMemory(int memCount, bool ifmemset) {
  void* tmpData = (void*)new char[memCount];
  if (ifmemset) {
    memset(tmpData, 0, memCount);
  }
  return tmpData;
}
void Memory::FreeMemory(void *point) {
  delete [] ((char*)point);
}
