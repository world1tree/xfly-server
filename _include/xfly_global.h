//
// Created by zaiheshi on 1/20/23.
//

#ifndef XFLY_XFLY_GLOBAL_H
#define XFLY_XFLY_GLOBAL_H

#include "signal.h"

// 定义配置项数据结构
typedef struct {
    char ItemName[51]{};
    char ItemContent[501]{};
}ConfigItem, *PConfigItem;

enum class ProcessType {
  masterProcess,
  workerprocess,
};

extern ProcessType processType;
extern sig_atomic_t invalidProcess;
#endif //XFLY_XFLY_GLOBAL_H
