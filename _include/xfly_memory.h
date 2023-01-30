//
// Created by zaiheshi on 1/28/23.
//

#ifndef XFLY_XFLY_MEMORY_H
#define XFLY_XFLY_MEMORY_H
class Memory {
private:
  static Memory *m_instance;
private:
  Memory()=default;
  class GCClass {
  public:
    ~GCClass() {
      if (Memory::m_instance) {
        delete Memory::m_instance;
        Memory::m_instance = nullptr;
      }
    }
  };
public:
  ~Memory()=default;
  static Memory *getInstance() {
    if (m_instance == nullptr) {
      m_instance = new Memory{};
      static GCClass gc; // 这里必须这么写，否则存在内存泄漏，因为m_instance这段内存无法释放, 除非程序结尾手动delete
    }
    return m_instance;
  }
  void *AllocMemory(int memCount, bool ifmemset);
  void FreeMemory(void* point);
};

#endif // XFLY_XFLY_MEMORY_H
