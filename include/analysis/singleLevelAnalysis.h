#pragma once

#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/result.h"
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include "include/util/timeline.h"
#include <algorithm>
#include <numeric>
#include <set>
#include <vector>
class Analyzer {
private:
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _coupledVarVec;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _oriCoupledVarVec;
  MAPPING::Transform _T;
  WORKLOAD::Tensor _I;
  WORKLOAD::Tensor _W;
  WORKLOAD::Tensor _O;
  WORKLOAD::Tensor _oriI;
  WORKLOAD::Tensor _oriW;
  WORKLOAD::Tensor _oriO;
  bool _edgePEFlag;
  ARCH::Level &_L;
  MAPPING::Access _accessI;
  MAPPING::Access _accessW;
  MAPPING::Access _accessO;
  std::shared_ptr<AnalyzerResult> _result;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecI;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecW;
  std::shared_ptr<std::vector<std::vector<int>>> _reuseVecO;
  std::map<ARCH::DATATYPE, std::shared_ptr<std::vector<std::vector<int>>>>
      _reuseVecMap;
  std::shared_ptr<WORKLOAD::Iterator> PEX;
  std::shared_ptr<WORKLOAD::Iterator> PEY;
  std::shared_ptr<WORKLOAD::Iterator> INNERTIME;
  bool _doubleBufferFlag;
  bool _stationaryDoubleBufferFlag;
  long long _requiredDataSize[3];
  std::vector<std::vector<long long>> _tensorDimRange;
  std::vector<Base> _baseSet;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _curSubCoupledVarVec;
  std::set<std::shared_ptr<WORKLOAD::Iterator>> _curSubCoupledVarSet;
  int _curBaseIndex;

  std::pair<long long, long long> compTRange(int row);
  bool checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType);
  void constructSingleDataTypeTimeSet(int flag,
                                      std::set<int> &singleDataTypeSet,
                                      ARCH::DATATYPE dataType);
  void constructInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                  std::vector<int> &outerTimeVec);
  long long compOneStateTimeSize(std::vector<int> &timeVec);
  long long compOneStateTimeSizeDelay(std::vector<int> &innerTimeVec);
  void generateVarVec(std::vector<int> &timeVec,
                      std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec);
  void compOneDataVolumn(ARCH::DATATYPE dataType, MAPPING::Access &access,
                         std::vector<int> &innerTimeVec, int outerTimeSize,
                         WORKLOAD::Tensor &curTensor);
  int compStableVolumn(ARCH::DATATYPE dataType, int innerCoupledDimIndex,
                       int coef);
  int compOneStateStableDelay();
  long long compOneStateInitDelay(std::pair<int, int> &PEXRange,
                                  std::pair<int, int> &PEYRange,
                                  int stableDelay);
  void compOneStateDelay(std::vector<int> &innerTimeVec, long long &delay,
                         long long &compCycle, long long &initDelay,
                         long long &activePEMultTimeNum);
  void accessAnalysis(std::vector<int> &innerTimeVec,
                      std::vector<int> &outerTimeVec);
  void delayAnalysis(std::vector<int> &innerTimeVec,
                     std::vector<int> &outerTimeVec);
  void setEdge(std::shared_ptr<WORKLOAD::Iterator> curI);
  void unsetEdge(std::shared_ptr<WORKLOAD::Iterator> curI);
  void changeBase();
  void
  compOneStateVolumn(long long &uniqueVolumn, long long &totalVolumn,
                     long long &toSubVolumn, std::vector<int> &innerTimeVec,
                     ARCH::DATATYPE dataType, WORKLOAD::Tensor &curTensor,
                     std::shared_ptr<std::map<std::pair<int, int>, long long>>
                         activateCountMap);
  void
  changeEdgeByState(bool flag, int varNum, int i,
                    std::vector<std::vector<int>> &state,
                    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec);

public:
  bool _subNetworkExtended;
  Analyzer(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
           MAPPING::Transform &T, WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
           WORKLOAD::Tensor &O, ARCH::Level &L, bool doubleBufferFlag)
      : _oriCoupledVarVec(coupledVarVec), _T(T), _oriI(I), _oriW(W), _oriO(O),
        _L(L), _doubleBufferFlag(doubleBufferFlag), _curBaseIndex(0),
        _edgePEFlag(false) {
    for (int i = 0; i < 3; i++)
      _requiredDataSize[0] = 0;
    reset();
    _subNetworkExtended = false;
  }
  ARCH::Level &getLevel() { return _L; }

  void buildAnalyzer() {
    if (!_accessI.isScalar())
      _reuseVecI = compReuseVec(_T, _accessI);
    else
      _reuseVecI = scalarReuseVec(_coupledVarVec.size());
    if (!_accessW.isScalar())
      _reuseVecW = compReuseVec(_T, _accessW);
    else
      _reuseVecW = scalarReuseVec(_coupledVarVec.size());
    if (!_accessO.isScalar())
      _reuseVecO = compReuseVec(_T, _accessO);
    else
      _reuseVecO = scalarReuseVec(_coupledVarVec.size());

    _reuseVecMap[ARCH::INPUT] = _reuseVecI;
    _reuseVecMap[ARCH::WEIGHT] = _reuseVecW;
    _reuseVecMap[ARCH::OUTPUT] = _reuseVecO;
  }
  void changeT(MAPPING::Transform &T) { _T.deepCopy(T); }
  void setCurSubCoupledVarVec(std::vector<std::shared_ptr<WORKLOAD::Iterator>>
                                  curSubCoupledVarVec = {});
  void setBase(std::vector<Base> baseSet);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &getCoupledVarVec();
  void oneAnalysis();
  bool compAndCheckRequiredDataSize();
  int compTotalBandWidth(ARCH::DATATYPE dataType);
  std::shared_ptr<AnalyzerResult> getResult();
  int getOccTimes();
  void setSubLevelResultVec(
      std::vector<std::shared_ptr<AnalyzerResult>> &subLevelResultVec);
  std::vector<std::vector<long long>> getTensorDimRange() {
    return _tensorDimRange;
  }

  bool checkAndSplitIterator(std::shared_ptr<WORKLOAD::Iterator> PEIterator,
                             int dim) {
    std::pair<int, int> PERange = compTRange(dim);
    if (!_L.checkPEDimRange(PERange, 1 - dim)) {
      if (PEIterator->isEdge())
        return false;
      _edgePEFlag = true;
      int lowerBound = PEIterator->getLowBound();
      int upperBound = PEIterator->getUpBound();
      int iteratorRange = upperBound - lowerBound + 1;
      int peRange = _L.getPEDimRange(1 - dim);
      int quotient = iteratorRange / peRange;
      int res = iteratorRange - peRange * quotient;
      std::shared_ptr<WORKLOAD::Iterator> outer;
      std::shared_ptr<WORKLOAD::Iterator> inner;
      if (res == 0) {
        outer = std::make_shared<WORKLOAD::Iterator>(
            WORKLOAD::Iterator(0, quotient - 1, PEIterator->getSym() + "_o"));
        inner = std::make_shared<WORKLOAD::Iterator>(
            WORKLOAD::Iterator(0, peRange - 1, PEIterator->getSym() + "_i"));
      } else {
        inner = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
            0, peRange - 1, 0, res - 1, PEIterator->getSym() + "_i"));
        outer = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
            0, quotient - 1, inner, PEIterator->getSym() + "_o"));
      }
      if (dim == 0)
        PEX = inner;
      else
        PEY = inner;
      int coupledVarNum = _coupledVarVec.size();
      for (int i = 0; i < coupledVarNum; i++) {
        if (_coupledVarVec[i] == PEIterator) {
          _coupledVarVec[i] = inner;
          break;
        }
      }
      _coupledVarVec.push_back(outer);
      //_coupledVarVec.insert(_coupledVarVec.begin() + 2, outer);
      _T.addExtraTemporal();
      // change Tensor
      _I.splitIterator(PEIterator, outer, inner, peRange);
      _W.splitIterator(PEIterator, outer, inner, peRange);
      _O.splitIterator(PEIterator, outer, inner, peRange);
      return true;
    } else {
      return true;
    }
  }

  void reset() {
    _edgePEFlag = false;
    _coupledVarVec = _oriCoupledVarVec;
    _O = _oriO;
    _I = _oriI;
    _W = _oriW;
  }

  bool constraintCheckAndBuildAnalyzer() {
    if (_edgePEFlag)
      reset();
    int colNum = _T.getColNum();
    for (int i = 0; i < colNum; i++) {
      if (_T(0, i) == 1)
        PEX = _coupledVarVec[i];
      if (_T(1, i) == 1)
        PEY = _coupledVarVec[i];
    }
    for (int i = 0; i < colNum; i++) {
      if (_T(2, i) == 1 && _T(0, i) != 1 && _T(1, i) != 1)
        INNERTIME = _coupledVarVec[i];
    }
    if (!_T.check())
      return false;
    if (PEX->hasEdge())
      return false;
    if (PEY->hasEdge())
      return false;
    std::pair<int, int> PEXRange = compTRange(0);
    std::pair<int, int> PEYRange = compTRange(1);
    // if (!_L.checkPEDimRange(PEXRange, 1))
    //    return false;
    // if (!_L.checkPEDimRange(PEYRange, 0))
    //    return false;
    if (!checkAndSplitIterator(PEX, 0))
      return false;
    if (!checkAndSplitIterator(PEY, 1))
      return false;
    _accessI = MAPPING::constructAccessMatrix(_I, _coupledVarVec);
    _accessW = MAPPING::constructAccessMatrix(_W, _coupledVarVec);
    _accessO = MAPPING::constructAccessMatrix(_O, _coupledVarVec);
    buildAnalyzer();
    if (!_L.checkNetworkReuseValid(ARCH::INPUT, _reuseVecI))
      return false;
    if (!_L.checkNetworkReuseValid(ARCH::WEIGHT, _reuseVecW))
      return false;
    if (!_L.checkNetworkReuseValid(ARCH::OUTPUT, _reuseVecO))
      return false;
    return true;
  }
  void getTimeLine() { TIMELINE::getTimeLine(_coupledVarVec, _T, _I, _W, _O); }
  MAPPING::Transform getT() { return _T; }
  void outputReuseVec(std::shared_ptr<std::vector<std::vector<int>>> reuseVec,
                      std::ofstream &logFile) {
    std::string ret;
    logFile << "{";
    int reuseVecNum = reuseVec->size();
    if (reuseVecNum == 0)
      return;
    int dimNum = (*reuseVec)[0].size();
    for (int i = 0; i < reuseVecNum; i++) {
      if (i != 0)
        logFile << ',';
      logFile << "\"" << std::to_string(i) << "\":\"";
      for (int j = 0; j < dimNum; j++) {
        logFile << std::to_string((*reuseVec)[i][j]) + ' ';
      }
      logFile << "\"\n";
    }
    logFile << "}";
    return;
  }
  void outputConfig(std::ofstream &logFile) {
    logFile << "\"Coupled Var\":\n";
    logFile << "{";
    int count = 0;
    for (auto &var : _coupledVarVec) {
      if (count != 0)
        logFile << ',';
      logFile << "\"" << std::to_string(count) << "\":\"";
      count++;
      logFile << var->to_string();
      logFile << "\"\n";
    }
    logFile << "},";
    logFile << "\"Transform Matrix:\":\n";
    _T.outputT(logFile);
    logFile << ",";
    logFile << "\"Reuse Vector Input:\":";
    outputReuseVec(_reuseVecI, logFile);
    logFile << "\n,";
    logFile << "\"Reuse Vector Weight:\":";
    outputReuseVec(_reuseVecW, logFile);
    logFile << "\n,";
    logFile << "\"Reuse Vector Output:\":";
    outputReuseVec(_reuseVecO, logFile);
    logFile << "\n,";
    logFile << "\"Arch Config:\":\n";
    _L.outputLevel(logFile);
    logFile << ",";
  }
};