#pragma once
#include <cstdint>

void InitializeAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapased();
void StopLAPICTimer();
