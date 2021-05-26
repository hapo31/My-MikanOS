#pragma once

#include <stdint.h>

#include "message.hpp"

void InitializeBenchMark();
void TaskBenchMark(uint64_t taskid, int64_t data);
