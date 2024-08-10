#include "include/util/config.h"
using namespace WORKLOAD;
// define tensor task
void defineTask1(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto k = task.defineIterator(96, "k");
  auto c = task.defineIterator(3, "c");
  auto h = task.defineIterator(55, "h");
  auto w = task.defineIterator(55, "w");
  auto r = task.defineIterator(11, "r");
  auto s = task.defineIterator(11, "s");
  auto n = task.defineIterator(64, "n");

  task.defineTensor(ARCH::INPUT, "I", {n, c, 4 * h + r, 4 * w + s});
  task.defineTensor(ARCH::WEIGHT, "W", {k, c, r, s});
  task.defineTensor(ARCH::OUTPUT, "O", {n, k, h, w});

  taskset.addTask(task);
}

void defineTask2(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto k = task.defineIterator(256, "k");
  auto c = task.defineIterator(48, "c");
  auto h = task.defineIterator(27, "h");
  auto w = task.defineIterator(27, "w");
  auto r = task.defineIterator(55, "r");
  auto s = task.defineIterator(55, "s");
  auto n = task.defineIterator(64, "n");

  task.defineTensor(ARCH::INPUT, "I", {n, c, h + r, w + s});
  task.defineTensor(ARCH::WEIGHT, "W", {k, c, r, s});
  task.defineTensor(ARCH::OUTPUT, "O", {n, k, h, w});

  taskset.addTask(task);
}

void defineTask3(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto k = task.defineIterator(384, "k");
  auto c = task.defineIterator(256, "c");
  auto h = task.defineIterator(13, "h");
  auto w = task.defineIterator(13, "w");
  auto r = task.defineIterator(3, "r");
  auto s = task.defineIterator(3, "s");
  auto n = task.defineIterator(64, "n");

  task.defineTensor(ARCH::INPUT, "I", {n, c, h + r, w + s});
  task.defineTensor(ARCH::WEIGHT, "W", {k, c, r, s});
  task.defineTensor(ARCH::OUTPUT, "O", {n, k, h, w});

  taskset.addTask(task);
}

void defineTask4(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto k = task.defineIterator(384, "k");
  auto c = task.defineIterator(192, "c");
  auto h = task.defineIterator(13, "h");
  auto w = task.defineIterator(13, "w");
  auto r = task.defineIterator(3, "r");
  auto s = task.defineIterator(3, "s");
  auto n = task.defineIterator(64, "n");

  task.defineTensor(ARCH::INPUT, "I", {n, c, h + r, w + s});
  task.defineTensor(ARCH::WEIGHT, "W", {k, c, r, s});
  task.defineTensor(ARCH::OUTPUT, "O", {n, k, h, w});

  taskset.addTask(task);
}

void defineTask5(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto k = task.defineIterator(256, "k");
  auto c = task.defineIterator(96, "c");
  auto h = task.defineIterator(13, "h");
  auto w = task.defineIterator(13, "w");
  auto r = task.defineIterator(3, "r");
  auto s = task.defineIterator(3, "s");
  auto n = task.defineIterator(64, "n");

  task.defineTensor(ARCH::INPUT, "I", {n, c, h + r, w + s});
  task.defineTensor(ARCH::WEIGHT, "W", {k, c, r, s});
  task.defineTensor(ARCH::OUTPUT, "O", {n, k, h, w});

  taskset.addTask(task);
}

void defineTask6(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto i = task.defineIterator(64, "i");
  auto j = task.defineIterator(4096, "j");
  auto k = task.defineIterator(9216, "k");

  task.defineTensor(ARCH::INPUT, "I", {i, k});
  task.defineTensor(ARCH::WEIGHT, "W", {k, j});
  task.defineTensor(ARCH::OUTPUT, "O", {i, j});

  taskset.addTask(task);
}

void defineTask7(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto i = task.defineIterator(64, "i");
  auto j = task.defineIterator(4096, "j");
  auto k = task.defineIterator(4096, "k");

  task.defineTensor(ARCH::INPUT, "I", {i, k});
  task.defineTensor(ARCH::WEIGHT, "W", {k, j});
  task.defineTensor(ARCH::OUTPUT, "O", {i, j});

  taskset.addTask(task);
}

void defineTask8(TaskSet &taskset) {
  Task task;
  task.defineRatio(1.0);
  auto i = task.defineIterator(64, "i");
  auto j = task.defineIterator(1000, "j");
  auto k = task.defineIterator(4096, "k");

  task.defineTensor(ARCH::INPUT, "I", {i, k});
  task.defineTensor(ARCH::WEIGHT, "W", {k, j});
  task.defineTensor(ARCH::OUTPUT, "O", {i, j});

  taskset.addTask(task);
}

// define tensor task set
void defineTaskSet(TaskSet &taskset) {
  defineTask1(taskset);
  defineTask2(taskset);
  defineTask3(taskset);
  defineTask4(taskset);
  defineTask5(taskset);
  defineTask6(taskset);
  defineTask7(taskset);
  defineTask8(taskset);
}

// define accelerator
void defineAccelerator1(AcceleratorSet &accSet, int row, int col) {
  Accelerator acc;
  acc.setDataWidth(16);

  acc.addLevel(row, col, false, true);
  acc.addBuffer(0, ARCH::SRAM, ARCH::INPUT, 256000000, 12800000000);
  acc.addBuffer(0, ARCH::SRAM, ARCH::WEIGHT, 256000000, 12800000000);
  acc.addBuffer(0, ARCH::SRAM, ARCH::OUTPUT, 256000000, 12800000000);
  acc.addNetworkGroup(0, ARCH::INPUT, {{1, 0, 0}});
  acc.addNetworkGroup(0, ARCH::WEIGHT, {{0, 0, 1}});
  acc.addNetworkGroup(0, ARCH::OUTPUT, {{0, 1, 0}});

  accSet.addAcc(acc);
}

// define accelerator set
void defineAcceleratorSet(AcceleratorSet &accSet) {
  defineAccelerator1(accSet, 4, 4);
  defineAccelerator1(accSet, 8, 8);
  defineAccelerator1(accSet, 16, 16);
  defineAccelerator1(accSet, 32, 32);
  defineAccelerator1(accSet, 64, 64);
  defineAccelerator1(accSet, 128, 128);
  defineAccelerator1(accSet, 256, 256);
}

// levelindex targetIndex ratio
// levelindex: define the accelerator level which the desired target parameters
// belong targetIndex: define the desired target parameters
//  0 unique_output
//  1 unique_input_0
//  2 unique_input_1
//  3 bufferSize_output
//  4 bufferSize_input_0
//  5 bufferSize_input_1
//  6 requiredBandwidth_output
//  7 requiredBandwidth_input_0
//  8 requiredBandwidth_input_1
//  9 delay
//  10 energy mj
//  11 area mm^2

// ratio: define parameter weights
// levelindex targetIndex ratio
// levelindex: define the accelerator level which the desired target parameters
// belong targetIndex: define the desired target parameters
void defineTarget(Target &target) { target.addTarget(0, 9, 1); }
