//
// Created by zaiheshi on 1/20/23.
//

#ifndef XFLY_XFLY_FUNC_H
#define XFLY_XFLY_FUNC_H

void lStrip(char* str);
void rStrip(char* str);
bool initSignals();
[[noreturn]] void masterProcessCycle();
int createDaemon();
#endif //XFLY_XFLY_FUNC_H
