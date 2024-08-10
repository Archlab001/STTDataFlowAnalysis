#pragma once
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include <string>
#include <vector>
struct Task {
  std::map<ARCH::DATATYPE, WORKLOAD::Tensor> _tensorMap;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _coupledVarVec;
  std::map<std::shared_ptr<WORKLOAD::Iterator>, std::vector<int>>
      _allIteratorCandidate;
  double _ratio;
  Task() : _ratio(1) {}
  auto defineIterator(int range, std::string sym,
                      std::vector<int> candidate = {}) {
    auto iterator =
        std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(range, sym));
    _coupledVarVec.push_back(iterator);
    candidate.push_back(range);
    _allIteratorCandidate[iterator] = candidate;
    for (auto c : candidate) {
      DEBUG::check(c < range || c > 0, DEBUG::EMPTY_TASK,
                   "Task::defineIterator");
    }
    return std::make_shared<WORKLOAD::Polynomial>(iterator);
  }

  void
  defineTensor(ARCH::DATATYPE dataType, std::string sym,
               std::vector<std::shared_ptr<WORKLOAD::Polynomial>> polyVec) {
    WORKLOAD::Tensor curTensor(sym);
    for (auto p : polyVec) {
      curTensor[p];
    }
    _tensorMap[dataType] = curTensor;
  }

  void defineRatio(double ratio) { _ratio = ratio; }

  bool check() {
    if (_coupledVarVec.empty()) {
      return false;
    }
    if (_tensorMap.count(ARCH::INPUT) == 0) {
      return false;
    }
    if (_tensorMap.count(ARCH::WEIGHT) == 0) {
      return false;
    }
    if (_tensorMap.count(ARCH::OUTPUT) == 0) {
      return false;
    }
    return true;
  }
};

struct TaskSet {
  std::vector<Task> taskVec;
  TaskSet() = default;
  void addTask(Task &task) {
    DEBUG::check(task.check(), DEBUG::TASK_INCOMPLETE, "TaskSet::addTask");
    taskVec.push_back(task);
  }
  void check() {
    DEBUG::check(!taskVec.empty(), DEBUG::EMPTY_TASK, "TaskSet::check");
  }
};

struct Accelerator {
  std::vector<ARCH::Level> _LVec;
  std::vector<bool> _inputNetworkFlag;
  std::vector<bool> _weightNetworkFlag;
  std::vector<bool> _outputNetworkFlag;
  int _dataWidth;
  Accelerator() = default;
  std::map<ARCH::DATATYPE, int> _bwMap;
  void setDataWidth(int dataWidth) { _dataWidth = dataWidth; }
  void addLevel(int rowNum, int colNum, bool peFlag = false,
                bool doubleBufferFlag = false, int spatialDimNum = 2) {
    _LVec.emplace_back(rowNum, colNum, _dataWidth, peFlag, doubleBufferFlag,
                       spatialDimNum);
    _inputNetworkFlag.emplace_back(false);
    _weightNetworkFlag.emplace_back(false);
    _outputNetworkFlag.emplace_back(false);
  }
  void addLevel(int rowNum, bool peFlag = false,
                bool doubleBufferFlag = false) {
    _LVec.emplace_back(rowNum, _dataWidth, peFlag, doubleBufferFlag);
    _inputNetworkFlag.emplace_back(false);
    _weightNetworkFlag.emplace_back(false);
    _outputNetworkFlag.emplace_back(false);
  }
  void addLevel(bool peFlag = false, bool doubleBufferFlag = false) {
    _LVec.emplace_back(_dataWidth, peFlag, doubleBufferFlag);
    _inputNetworkFlag.emplace_back(false);
    _weightNetworkFlag.emplace_back(false);
    _outputNetworkFlag.emplace_back(false);
  }

  void addBuffer(int level, ARCH::BufferType bufferType,
                 ARCH::DATATYPE dataType, int bandWidth,
                 long long capacity = LLONG_MAX) {
    _LVec[level].appendBuffer(bufferType, dataType, capacity);
    _bwMap[dataType] = bandWidth;
  }

  void addNetworkGroup(int level, ARCH::DATATYPE dataType,
                       std::vector<std::vector<int>> featureVec) {
    int bandWidth = 0;
    if (_bwMap.count(dataType) != 0) {
      bandWidth = _bwMap[dataType];
    } else if (_bwMap.size() == 1) {
      bandWidth = _bwMap[ARCH::TOTAL];
    } else {
      if (dataType == ARCH::OUTPUT) {
        bandWidth = _bwMap[ARCH::OUTPUT];
      } else {
        bandWidth = _bwMap[ARCH::ALLINPUT];
      }
    }
    if (dataType == ARCH::INPUT) {
      _inputNetworkFlag[level] = true;
    } else if (dataType == ARCH::WEIGHT) {
      _weightNetworkFlag[level] = true;
    } else {
      _outputNetworkFlag[level] = true;
    }

    if (featureVec.size() == 1) {
      _LVec[level].appendNetworkGroup(dataType, bandWidth, featureVec[0]);
    } else {
      _LVec[level].appendNetworkGroup(dataType, bandWidth, featureVec[0],
                                      featureVec[1]);
    }
  }
  bool checkLevel(int levelNum) {
    if (_inputNetworkFlag[levelNum] && _weightNetworkFlag[levelNum] &&
        _outputNetworkFlag[levelNum]) {
      return true;
    } else {
      return false;
    }
  }
  void check() {
    DEBUG::check(!_LVec.empty(), DEBUG::EMPTY_ACCELERATOR,
                 "Accelerator::check");
    int levelNum = _LVec.size();
    for (int i = 0; i < levelNum; i++) {
      DEBUG::check(checkLevel(i), DEBUG::ERROR_LEVEL, "Accelerator::check");
    }
  }
  int getLevelNum() { return _LVec.size(); }
};

struct AcceleratorSet {
  std::vector<Accelerator> acceleratorVec;
  AcceleratorSet() = default;
  void addAcc(Accelerator &acc) {
    acc.check();
    acceleratorVec.push_back(acc);
  }
  void check() {
    DEBUG::check(acceleratorVec.size() != 0, DEBUG::EMPTY_ACCELERATOR_SET,
                 "AcceleratorSet::check");
    int levelNum = acceleratorVec[0].getLevelNum();
    for (auto &acc : acceleratorVec) {
      DEBUG::check(acc.getLevelNum() == levelNum, DEBUG::EMPTY_TASK,
                   "AcceleratorSet::check");
    }
  }
};

struct Target {
  std::vector<std::vector<double>> _t;
  bool _flag;
  Target(AcceleratorSet &accSet)
      : _t(std::vector<std::vector<double>>(
            accSet.acceleratorVec[0]._LVec.size(), std::vector<double>(12, 0))),
        _flag(false) {}
  Target(int levelNum)
      : _t(std::vector<std::vector<double>>(levelNum,
                                            std::vector<double>(12, 0))),
        _flag(false) {}
  void addTarget(int levelIndex, int targetIndex, double ratio = 1) {
    _t[levelIndex][targetIndex] = ratio;
    _flag = true;
  }
  void check() { DEBUG::check(_flag, DEBUG::EMPTY_TARGET, "Target::check"); }
};

void defineTaskSet(TaskSet &taskset);

void defineTarget(Target &target);

void defineAcceleratorSet(AcceleratorSet &accSet);