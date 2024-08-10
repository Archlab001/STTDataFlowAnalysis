#include "include/analysis/singleLevelAnalysis.h"

void Analyzer::setEdge(std::shared_ptr<WORKLOAD::Iterator> curI) {
  curI->setEdge();
  if (_curSubCoupledVarSet.count(curI)) {
    changeBase();
  }
}

void Analyzer::unsetEdge(std::shared_ptr<WORKLOAD::Iterator> curI) {
  curI->unsetEdge();
  if (_curSubCoupledVarSet.count(curI)) {
    changeBase();
  }
}

void Analyzer::changeBase() {
  int curSubCoupledVarNum = _curSubCoupledVarVec.size();
  int tmp = 0;
  for (int i = 0; i < curSubCoupledVarNum; i++) {
    tmp *= 2;
    tmp += _curSubCoupledVarVec[i]->isEdge();
  }
  _curBaseIndex = tmp;
}

void Analyzer::oneAnalysis() {
  _result = std::make_shared<AnalyzerResult>(AnalyzerResult());
  _result->requiredDataSize[ARCH::INPUT] = _requiredDataSize[ARCH::INPUT];
  _result->requiredDataSize[ARCH::WEIGHT] = _requiredDataSize[ARCH::WEIGHT];
  _result->requiredDataSize[ARCH::OUTPUT] = _requiredDataSize[ARCH::OUTPUT];
  std::vector<int> innerTimeVec;
  std::vector<int> outerTimeVec;
  constructInnerOuterTimeVec(innerTimeVec, outerTimeVec);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> outerVarVec;
  generateVarVec(outerTimeVec, outerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, outerVarVec);

  int stateNum = state.size();
  int varNum = outerVarVec.size();
  if (stateNum == 0) {

    delayAnalysis(innerTimeVec, outerTimeVec);
    accessAnalysis(innerTimeVec, outerTimeVec);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, outerVarVec);
      delayAnalysis(innerTimeVec, outerTimeVec);
      accessAnalysis(innerTimeVec, outerTimeVec);
      changeEdgeByState(0, varNum, i, state, outerVarVec);
    }
  }
  if (_L.checkIfNetworkExtended()) {
    _tensorDimRange = std::vector<std::vector<long long>>(3);
    _tensorDimRange[ARCH::INPUT].push_back(_result->uniqueVolumn[ARCH::INPUT]);
    _tensorDimRange[ARCH::WEIGHT].push_back(
        _result->uniqueVolumn[ARCH::WEIGHT]);
    _tensorDimRange[ARCH::OUTPUT].push_back(
        _result->uniqueVolumn[ARCH::OUTPUT]);
  }
}

void Analyzer::delayAnalysis(std::vector<int> &innerTimeVec,
                             std::vector<int> &outerTimeVec) {
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  int coupleVarNum = _coupledVarVec.size();
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  long long initDelay = 0;
  long long delay = 0;
  long long compCycle = 0;
  long long activePEMultTimeNum = 0;
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  int ret = 0;
  if (stateNum == 0) {
    compOneStateDelay(innerTimeVec, delay, compCycle, initDelay,
                      activePEMultTimeNum);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, innerVarNum, i, state, innerVarVec);
      compOneStateDelay(innerTimeVec, delay, compCycle, initDelay,
                        activePEMultTimeNum);
      changeEdgeByState(0, innerVarNum, i, state, innerVarVec);
    }
  }
  if (_doubleBufferFlag) {
    //_result->delay += ((long long)outerTimeSize - 1) * std::max(delay,
    // initDelay) + initDelay + delay; _result->compCycle += (long
    // long)outerTimeSize * compCycle; _result->activePEMultTimeNum += (long
    // long)outerTimeSize * activePEMultTimeNum; _result->totalPEMultTimeNum +=
    //(((long long)outerTimeSize - 1) * std::max(delay, initDelay) + initDelay +
    // delay) * _L.getPENum(); _result->initTimes += outerTimeSize;
    _result->delay += ((long long)outerTimeSize) * std::max(delay, initDelay);
    _result->compCycle += (long long)outerTimeSize * compCycle;
    _result->activePEMultTimeNum +=
        (long long)outerTimeSize * activePEMultTimeNum;
    _result->totalPEMultTimeNum +=
        (((long long)outerTimeSize) * std::max(delay, initDelay)) *
        _L.getPENum();
    _result->initTimes += outerTimeSize;
  } else {
    delay += initDelay;
    _result->delay += (long long)outerTimeSize * delay;
    _result->compCycle += (long long)outerTimeSize * compCycle;
    _result->activePEMultTimeNum +=
        (long long)outerTimeSize * activePEMultTimeNum;
    _result->totalPEMultTimeNum +=
        (long long)outerTimeSize * delay * _L.getPENum();
    _result->initTimes += outerTimeSize;
  }
}

int Analyzer::compStableVolumn(ARCH::DATATYPE dataType,
                               int innerCoupledDimIndex, int coef) {
  int ret;
  if (innerCoupledDimIndex == -1 ||
      coef >= _baseSet[_curBaseIndex].baseData[dataType][innerCoupledDimIndex])
    ret = std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                          _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                          std::multiplies<long long>());
  else {
    ret = 1;
    for (int i = 0; i < _baseSet[_curBaseIndex].baseData[dataType].size(); i++)
      if (i != innerCoupledDimIndex)
        ret *= _baseSet[_curBaseIndex].baseData[dataType][i];
      else
        ret *= coef;
  }
  return ret;
}

int Analyzer::compOneStateStableDelay() {
  long long inputStableVolumn = 0;
  long long weightStableVolumn = 0;
  long long outputStableVolumn = 0;
  if (!_subNetworkExtended) {
    int innerCoupledDimIndexInput = _I.getCoupledDimIndex(INNERTIME);
    int coefInput = _I.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexInput);
    int innerCoupledDimIndexWeight = _W.getCoupledDimIndex(INNERTIME);
    int coefWeight =
        _W.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexWeight);
    int innerCoupledDimIndexOutput = _O.getCoupledDimIndex(INNERTIME);
    int coefOutput =
        _O.getCoupledDimCoef(INNERTIME, innerCoupledDimIndexOutput);
    inputStableVolumn =
        compStableVolumn(ARCH::INPUT, innerCoupledDimIndexInput, coefInput);
    weightStableVolumn =
        compStableVolumn(ARCH::WEIGHT, innerCoupledDimIndexWeight, coefWeight);
    outputStableVolumn =
        compStableVolumn(ARCH::OUTPUT, innerCoupledDimIndexOutput, coefOutput);
  } else {
    inputStableVolumn = _baseSet[_curBaseIndex].baseData[ARCH::INPUT][0];
    weightStableVolumn = _baseSet[_curBaseIndex].baseData[ARCH::WEIGHT][0];
    outputStableVolumn = _baseSet[_curBaseIndex].baseData[ARCH::OUTPUT][0];
  }
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);

  long long inputStableDelay =
      _L.getStableDelay(ARCH::INPUT, inputStableVolumn, PEXRange, PEYRange);
  long long weightStableDelay =
      _L.getStableDelay(ARCH::WEIGHT, weightStableVolumn, PEXRange, PEYRange);
  long long outputStableDelay =
      _L.getStableDelay(ARCH::OUTPUT, outputStableVolumn, PEXRange, PEYRange);
  long long inputCoupledNum =
      _L.getActiveAccessPointNum(ARCH::INPUT, PEXRange, PEYRange);
  long long weightCoupledNum =
      _L.getActiveAccessPointNum(ARCH::WEIGHT, PEXRange, PEYRange);
  long long outputCoupledNum =
      _L.getActiveAccessPointNum(ARCH::OUTPUT, PEXRange, PEYRange);
  if (_curBaseIndex == 0) {
    if (_L.checkIfNetworkExtended()) {
      _result->stableDelay[ARCH::INPUT] = 0;
      _result->stableDelay[ARCH::WEIGHT] = 0;
      _result->stableDelay[ARCH::OUTPUT] = 0;
    } else {
      _result->stableDelay[ARCH::INPUT] =
          std::max(_result->stableDelay[ARCH::INPUT], inputStableDelay);
      _result->stableDelay[ARCH::WEIGHT] =
          std::max(_result->stableDelay[ARCH::WEIGHT], weightStableDelay);
      _result->stableDelay[ARCH::OUTPUT] =
          std::max(_result->stableDelay[ARCH::OUTPUT], outputStableDelay);
    }
    _result->stableDelay[3] = std::max(_result->stableDelay[3],
                                       _baseSet[_curBaseIndex].baseCompCycle);
  }
  long long inputAndWeightStableDelay;
  int dataWidth = _L.getDataWidth();
  if (_L.ifInputWeightSharedBW()) {
    inputAndWeightStableDelay = inputStableDelay + weightStableDelay;
    bool inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
    bool weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
    bool outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
    if (!inputIfStationary && !weightIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] = std::max(
          _result->requiredBandWidth[ARCH::INPUT],
          (long long)std::ceil(dataWidth *
                               (double)(inputStableVolumn * inputCoupledNum +
                                        weightStableVolumn * weightCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));

    else if (!inputIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] = std::max(
          _result->requiredBandWidth[ARCH::INPUT],
          (long long)std::ceil(dataWidth *
                               (double)(inputStableVolumn * inputCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));
    else if (!weightIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] = std::max(
          _result->requiredBandWidth[ARCH::INPUT],
          (long long)std::ceil(dataWidth *
                               (double)(weightStableVolumn * weightCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));
    _result->requiredBandWidth[ARCH::WEIGHT] = 0;
    if (!outputIfStationary)
      _result->requiredBandWidth[ARCH::OUTPUT] = std::max(
          _result->requiredBandWidth[ARCH::OUTPUT],
          (long long)std::ceil(dataWidth *
                               (double)(outputStableVolumn * outputCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));
  } else {
    inputAndWeightStableDelay = std::max(inputStableDelay, weightStableDelay);
    bool inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
    bool weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
    bool outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
    if (!inputIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] = std::max(
          _result->requiredBandWidth[ARCH::INPUT],
          (long long)std::ceil(dataWidth *
                               (double)(inputStableVolumn * inputCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));

    if (!weightIfStationary)
      _result->requiredBandWidth[ARCH::WEIGHT] = std::max(
          _result->requiredBandWidth[ARCH::WEIGHT],
          (long long)std::ceil(dataWidth *
                               (double)(weightStableVolumn * weightCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));

    if (!outputIfStationary)
      _result->requiredBandWidth[ARCH::OUTPUT] = std::max(
          _result->requiredBandWidth[ARCH::OUTPUT],
          (long long)std::ceil(dataWidth *
                               (double)(outputStableVolumn * outputCoupledNum) /
                               (double)_baseSet[_curBaseIndex].baseCompCycle));
  }

  if (_L.checkIfNetworkExtended()) {
    inputAndWeightStableDelay = 0;
    outputStableDelay = 0;
  }
  if (_doubleBufferFlag) {
    return std::max(std::max(inputAndWeightStableDelay, outputStableDelay),
                    _baseSet[_curBaseIndex].baseCompCycle);
  } else {
    return std::max(inputAndWeightStableDelay, outputStableDelay) +
           _baseSet[_curBaseIndex].baseCompCycle;
  }
}

long long Analyzer::compOneStateInitDelay(std::pair<int, int> &PEXRange,
                                          std::pair<int, int> &PEYRange,
                                          int stableDelay) {
  long long inputInitVolumn =
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::INPUT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::INPUT].end(), 1,
                      std::multiplies<long long>());
  long long weightInitVolumn =
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::WEIGHT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::WEIGHT].end(), 1,
                      std::multiplies<long long>());
  long long outputInitVolumn =
      std::accumulate(_baseSet[_curBaseIndex].baseData[ARCH::OUTPUT].begin(),
                      _baseSet[_curBaseIndex].baseData[ARCH::OUTPUT].end(), 1,
                      std::multiplies<long long>());
  long long inputInitDelay =
      _L.getInitOrOutDelay(ARCH::INPUT, inputInitVolumn, PEXRange, PEYRange);
  long long weightInitDelay =
      _L.getInitOrOutDelay(ARCH::WEIGHT, weightInitVolumn, PEXRange, PEYRange);
  long long outputOutDelay =
      _L.getInitOrOutDelay(ARCH::OUTPUT, outputInitVolumn, PEXRange, PEYRange);
  if (_curBaseIndex == 0) {
    _result->initDelay[ARCH::INPUT] =
        std::max(_result->initDelay[ARCH::INPUT], inputInitDelay);
    _result->initDelay[ARCH::WEIGHT] =
        std::max(_result->initDelay[ARCH::WEIGHT], weightInitDelay);
    _result->initDelay[ARCH::OUTPUT] =
        std::max(_result->initDelay[ARCH::OUTPUT], outputOutDelay);
  }
  long long inputAndWeightInitDelay;
  if (_L.ifInputWeightSharedBW()) {
    inputAndWeightInitDelay = inputInitDelay + weightInitDelay;
    bool inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
    bool weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
    bool outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
    if (inputIfStationary || weightIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] =
          std::max(_result->requiredBandWidth[ARCH::INPUT],
                   _L.getInitOrOutBW(ARCH::INPUT, inputInitVolumn, PEXRange,
                                     PEYRange, stableDelay) +
                       _L.getInitOrOutBW(ARCH::WEIGHT, weightInitVolumn,
                                         PEXRange, PEYRange, stableDelay));
    if (outputIfStationary)
      _result->requiredBandWidth[ARCH::OUTPUT] =
          std::max(_result->requiredBandWidth[ARCH::OUTPUT],
                   _L.getInitOrOutBW(ARCH::OUTPUT, outputInitVolumn, PEXRange,
                                     PEYRange, stableDelay));
  } else {
    inputAndWeightInitDelay = std::max(inputInitDelay, weightInitDelay);
    bool inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
    bool weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
    bool outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
    if (inputIfStationary)
      _result->requiredBandWidth[ARCH::INPUT] =
          std::max(_result->requiredBandWidth[ARCH::INPUT],
                   _L.getInitOrOutBW(ARCH::INPUT, inputInitVolumn, PEXRange,
                                     PEYRange, stableDelay));
    if (weightIfStationary)
      _result->requiredBandWidth[ARCH::WEIGHT] =
          std::max(_result->requiredBandWidth[ARCH::WEIGHT],
                   _L.getInitOrOutBW(ARCH::WEIGHT, weightInitVolumn, PEXRange,
                                     PEYRange, stableDelay));
    if (outputIfStationary)
      _result->requiredBandWidth[ARCH::OUTPUT] =
          std::max(_result->requiredBandWidth[ARCH::OUTPUT],
                   _L.getInitOrOutBW(ARCH::OUTPUT, outputInitVolumn, PEXRange,
                                     PEYRange, stableDelay));
  }
  // if (_L.checkIfNetworkExtended())
  //    return 0;
  return inputAndWeightInitDelay + outputOutDelay;
}

long long Analyzer::compOneStateTimeSize(std::vector<int> &timeVec) {
  std::pair<long long, long long> timeRange;
  long long timeSize = 1;
  for (auto timeIndex : timeVec) {
    timeRange = compTRange(timeIndex);
    timeSize *= timeRange.second - timeRange.first + 1;
  }
  return timeSize;
}

long long Analyzer::compOneStateTimeSizeDelay(std::vector<int> &timeVec) {
  int innerTimeSize = INNERTIME->getSize();
  std::pair<long long, long long> timeRange;
  long long timeSize = 1;
  for (auto timeIndex : timeVec) {
    if (timeIndex == 2)
      continue;
    timeRange = compTRange(timeIndex);
    timeSize *= timeRange.second - timeRange.first + 1;
  }
  long long ret = 0;
  timeRange = compTRange(2);
  ret =
      (timeRange.second - timeRange.first + 1) + (timeSize - 1) * innerTimeSize;
  return ret;
}

void Analyzer::compOneStateDelay(std::vector<int> &innerTimeVec,
                                 long long &delay, long long &compCycle,
                                 long long &initDelay,
                                 long long &activePEMultTimeNum) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);

  long long stableDelay = (long long)compOneStateStableDelay();
  compOneStateTimeSize(innerTimeVec);
  delay += stableDelay * (long long)compOneStateTimeSizeDelay(innerTimeVec);

  PEX->lock();
  PEY->lock();
  long long curCompCycle = (long long)_baseSet[_curBaseIndex].baseCompCycle *
                           compOneStateTimeSize(innerTimeVec);
  compCycle += curCompCycle;
  activePEMultTimeNum += ((long long)PEYRange.second - PEYRange.first + 1) *
                         ((long long)PEXRange.second - PEXRange.first + 1) *
                         curCompCycle;
  PEX->unlock();
  PEY->unlock();
  initDelay =
      std::max(compOneStateInitDelay(PEXRange, PEYRange, delay), initDelay);
}

void Analyzer::accessAnalysis(std::vector<int> &innerTimeVec,
                              std::vector<int> &outerTimeVec) {
  PEX->lock();
  PEY->lock();
  int outerTimeSize = compOneStateTimeSize(outerTimeVec);
  PEX->unlock();
  PEY->unlock();
  compOneDataVolumn(ARCH::OUTPUT, _accessO, innerTimeVec, outerTimeSize, _O);
  compOneDataVolumn(ARCH::INPUT, _accessI, innerTimeVec, outerTimeSize, _I);
  compOneDataVolumn(ARCH::WEIGHT, _accessW, innerTimeVec, outerTimeSize, _W);
}

std::pair<long long, long long> Analyzer::compTRange(int row) {
  long long colNumT = _T.getColNum();
  std::shared_ptr<WORKLOAD::Polynomial> dim =
      std::make_shared<WORKLOAD::Polynomial>();
  for (int i = 0; i < colNumT; i++) {
    if (_T(row, i) == 1)
      dim = dim + _coupledVarVec[i];
  }
  std::pair<long long, long long> range = dim->getRange();
  return range;
}

void Analyzer::compOneStateVolumn(
    long long &uniqueVolumn, long long &totalVolumn, long long &toSubVolumn,
    std::vector<int> &innerTimeVec, ARCH::DATATYPE dataType,
    WORKLOAD::Tensor &curTensor,
    std::shared_ptr<std::map<std::pair<int, int>, long long>>
        activateCountMap) {
  std::pair<int, int> PEXRange = compTRange(0);
  std::pair<int, int> PEYRange = compTRange(1);
  long long pexSize = ((long long)PEYRange.second - PEYRange.first + 1);
  long long peySize = ((long long)PEXRange.second - PEXRange.first + 1);
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  PEX->lock();
  PEY->lock();
  totalVolumn +=
      (long long)compOneStateTimeSize(innerTimeVec) *
      std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                      _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                      std::multiplies<long long>()) *
      pexSize * peySize;
  PEX->unlock();
  PEY->unlock();
  if (_L.checkIfStationary(dataType)) {
    long long baseVolumn =
        std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                        _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                        std::multiplies<long long>());
    uniqueVolumn = std::max(
        uniqueVolumn,
        baseVolumn * _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange));
    toSubVolumn = std::max(toSubVolumn, baseVolumn * pexSize * peySize);
    _L.updateNetworkActivateCountMap(dataType, PEXRange, PEYRange, baseVolumn,
                                     activateCountMap, 1);
  } else {
    PEX->lock();
    PEY->lock();
    if (_L.checkIfUnlockPEDim(0, dataType))
      PEX->unlock();
    if (_L.checkIfUnlockPEDim(1, dataType))
      PEY->unlock();
    int innerCoupledDimIndex = curTensor.getCoupledDimIndex(INNERTIME);
    int coef = curTensor.getCoupledDimCoef(INNERTIME, innerCoupledDimIndex);
    long long baseVolumn =
        std::accumulate(_baseSet[_curBaseIndex].baseData[dataType].begin(),
                        _baseSet[_curBaseIndex].baseData[dataType].end(), 1,
                        std::multiplies<long long>());
    if (_subNetworkExtended || innerCoupledDimIndex == -1 ||
        coef >=
            _baseSet[_curBaseIndex].baseData[dataType][innerCoupledDimIndex]) {
      long long singlePEVolumn =
          compOneStateTimeSize(innerTimeVec) * baseVolumn;
      uniqueVolumn += singlePEVolumn *
                      _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
      toSubVolumn += singlePEVolumn * pexSize * peySize;
      _L.updateNetworkActivateCountMap(dataType, PEXRange, PEYRange,
                                       singlePEVolumn, activateCountMap);
    } else {
      std::vector<int> tmpTimeVec;
      tmpTimeVec.push_back(2);
      int innerMostRange = compOneStateTimeSize(tmpTimeVec);
      long long innerMostVolumn = 1;
      for (int i = 0; i < _baseSet[_curBaseIndex].baseData[dataType].size();
           i++) {
        if (i != innerCoupledDimIndex)
          innerMostVolumn *= _baseSet[_curBaseIndex].baseData[dataType][i];
        else
          innerMostVolumn *= coef;
      }
      tmpTimeVec.clear();
      for (auto timeIndex : innerTimeVec)
        if (timeIndex != 2)
          tmpTimeVec.push_back(timeIndex);
      int innerNotMostRange = compOneStateTimeSize(tmpTimeVec);
      long long tmp =
          (long long)baseVolumn +
          (long long)innerMostVolumn * ((long long)innerMostRange - 1);
      // uniqueVolumn += tmp * innerNotMostRange * pexSize * peySize;
      long long singlePEVolumn = tmp * innerNotMostRange;
      uniqueVolumn += singlePEVolumn *
                      _L.getActiveAccessPointNum(dataType, PEXRange, PEYRange);
      toSubVolumn += singlePEVolumn * pexSize * peySize;
      _L.updateNetworkActivateCountMap(dataType, PEXRange, PEYRange,
                                       singlePEVolumn, activateCountMap);
    }
    PEX->unlock();
    PEY->unlock();
  }
}

void Analyzer::compOneDataVolumn(ARCH::DATATYPE dataType,
                                 MAPPING::Access &access,
                                 std::vector<int> &innerTimeVec,
                                 int outerTimeSize,
                                 WORKLOAD::Tensor &curTensor) {
  ARCH::NETWORKTYPE networkType = _L.getNetworkType(dataType);
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> innerVarVec;
  generateVarVec(innerTimeVec, innerVarVec);
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, innerVarVec);
  long long uniqueVolumn = 0;
  long long totalVolumn = 0;
  long long toSubVolumn = 0;
  int stateNum = state.size();
  int innerVarNum = innerVarVec.size();
  std::shared_ptr<std::map<std::pair<int, int>, long long>> activateCountMap =
      std::make_shared<std::map<std::pair<int, int>, long long>>();
  if (stateNum == 0) {
    compOneStateVolumn(uniqueVolumn, totalVolumn, toSubVolumn, innerTimeVec,
                       dataType, curTensor, activateCountMap);
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, innerVarNum, i, state, innerVarVec);
      compOneStateVolumn(uniqueVolumn, totalVolumn, toSubVolumn, innerTimeVec,
                         dataType, curTensor, activateCountMap);
      changeEdgeByState(0, innerVarNum, i, state, innerVarVec);
    }
  }
  if (!activateCountMap->empty()) {
    for (auto item : (*activateCountMap)) {
      if ((*(_result->activateCountMapVec[dataType])).count(item.first) > 0)
        (*(_result->activateCountMapVec[dataType]))[item.first] +=
            item.second * outerTimeSize;
      else
        (*(_result->activateCountMapVec[dataType]))[item.first] =
            item.second * outerTimeSize;
    }
  }
  uniqueVolumn *= outerTimeSize;
  totalVolumn *= outerTimeSize;
  toSubVolumn *= outerTimeSize;
  _result->toSubVolumn[dataType] += toSubVolumn;
  _result->totalVolumn[dataType] += totalVolumn;
  _result->uniqueVolumn[dataType] += uniqueVolumn;
  _result->reuseVolumn[dataType] += totalVolumn - uniqueVolumn;
}

void Analyzer::generateVarVec(
    std::vector<int> &timeVec,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  int colNumT = _T.getColNum();
  for (auto row : timeVec) {
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        varVec.push_back(_coupledVarVec[i]);
    }
  }
}

bool Analyzer::checkValidInnerDim(int varIndex, ARCH::DATATYPE dataType) {
  std::shared_ptr<std::vector<std::vector<int>>> reuseVecSet =
      _reuseVecMap[dataType];
  int m = (*reuseVecSet).size();
  int n = (*reuseVecSet)[0].size();
  for (auto reuseVecItem : (*reuseVecSet)) {
    int flag = 1;
    for (int j = 0; j < n; j++) {
      if (j == varIndex) {
        if (reuseVecItem[j] != 1) {
          flag = 0;
        }
      } else {
        if (reuseVecItem[j] != 0) {
          flag = 0;
        }
      }
    }
    if (flag) {
      return true;
    }
  }
  return false;
}
void Analyzer::constructSingleDataTypeTimeSet(int flag,
                                              std::set<int> &singleDataTypeSet,
                                              ARCH::DATATYPE dataType) {
  int coupleVarNum = _coupledVarVec.size();
  if (flag) {
    for (int i = 2; i < coupleVarNum; i++) {
      if (checkValidInnerDim(i, dataType)) {
        singleDataTypeSet.insert(i);
      } else {
        break;
      }
    }
  } else {
    for (int i = 2; i < coupleVarNum; i++) {
      singleDataTypeSet.insert(i);
    }
  }
}

void Analyzer::constructInnerOuterTimeVec(std::vector<int> &innerTimeVec,
                                          std::vector<int> &outerTimeVec) {
  int coupleVarNum = _coupledVarVec.size();
  int inputIfStationary = _L.checkIfStationary(ARCH::INPUT);
  int weightIfStationary = _L.checkIfStationary(ARCH::WEIGHT);
  int outputIfStationary = _L.checkIfStationary(ARCH::OUTPUT);
  std::set<int> inputSet;
  std::set<int> weightSet;
  std::set<int> outputSet;
  constructSingleDataTypeTimeSet(inputIfStationary, inputSet, ARCH::INPUT);
  constructSingleDataTypeTimeSet(weightIfStationary, weightSet, ARCH::WEIGHT);
  constructSingleDataTypeTimeSet(outputIfStationary, outputSet, ARCH::OUTPUT);
  std::set<int> innerTimeSet;
  std::set<int> outerTimeSet;
  for (int i = 2; i < coupleVarNum; i++) {
    if (inputSet.count(i) && weightSet.count(i) && outputSet.count(i)) {
      innerTimeSet.insert(i);
    } else {
      break;
    }
  }

  for (int i = 2; i < coupleVarNum; i++) {
    if (!innerTimeSet.count(i)) {
      outerTimeSet.insert(i);
    }
  }
  for (auto it = innerTimeSet.rbegin(); it != innerTimeSet.rend(); it++) {
    innerTimeVec.push_back(*it);
  }
  for (auto it = outerTimeSet.rbegin(); it != outerTimeSet.rend(); it++) {
    outerTimeVec.push_back(*it);
  }
}

bool Analyzer::compAndCheckRequiredDataSize() {
  if (_L.checkIfNetworkExtended())
    return true;
  _tensorDimRange = std::vector<std::vector<long long>>(3);
  _tensorDimRange[ARCH::OUTPUT] = _oriO.getEveryDimRange();
  _tensorDimRange[ARCH::INPUT] = _oriI.getEveryDimRange();
  _tensorDimRange[ARCH::WEIGHT] = _oriW.getEveryDimRange();
  int dataWidth = _L.getDataWidth();
  _requiredDataSize[ARCH::OUTPUT] =
      std::accumulate(_tensorDimRange[ARCH::OUTPUT].begin(),
                      _tensorDimRange[ARCH::OUTPUT].end(), 1,
                      std::multiplies<long long>()) *
      (long long)(_doubleBufferFlag ? 2 : 1) * (double)dataWidth / 8;
  _requiredDataSize[ARCH::INPUT] =
      std::accumulate(_tensorDimRange[ARCH::INPUT].begin(),
                      _tensorDimRange[ARCH::INPUT].end(), 1,
                      std::multiplies<long long>()) *
      (long long)(_doubleBufferFlag ? 2 : 1) * (double)dataWidth / 8;
  _requiredDataSize[ARCH::WEIGHT] =
      std::accumulate(_tensorDimRange[ARCH::WEIGHT].begin(),
                      _tensorDimRange[ARCH::WEIGHT].end(), 1,
                      std::multiplies<long long>()) *
      (long long)(_doubleBufferFlag ? 2 : 1) * (double)dataWidth / 8;
  //_requiredDataSize[ARCH::OUTPUT] = _O.getVolumn();
  //_requiredDataSize[ARCH::INPUT] = _I.getVolumn();
  //_requiredDataSize[ARCH::WEIGHT] = _W.getVolumn();
  _L.setFreeBufferCapacity(_requiredDataSize[ARCH::INPUT],
                           _requiredDataSize[ARCH::WEIGHT],
                           _requiredDataSize[ARCH::OUTPUT]);
  if (_L.checkIfBufferTotal()) {
    long long tmp = _requiredDataSize[ARCH::INPUT] +
                    _requiredDataSize[ARCH::WEIGHT] +
                    _requiredDataSize[ARCH::OUTPUT];
    if (!_L.checkBufferSize(tmp, ARCH::TOTAL))
      return false;
  } else if (_L.checkIfBufferInputALL()) {
    long long tmp = _requiredDataSize[ARCH::INPUT] +
                    _requiredDataSize[ARCH::WEIGHT] +
                    _requiredDataSize[ARCH::OUTPUT];
    if (!_L.checkBufferSize(tmp, ARCH::ALLINPUT))
      return false;
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::OUTPUT], ARCH::OUTPUT))
      return false;
  } else {
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::INPUT], ARCH::INPUT))
      return false;
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::WEIGHT], ARCH::WEIGHT))
      return false;
    if (!_L.checkBufferSize(_requiredDataSize[ARCH::OUTPUT], ARCH::OUTPUT))
      return false;
  }
  return true;
}

int Analyzer::compTotalBandWidth(ARCH::DATATYPE dataType) {
  return _L.getNetworkBandWidth(dataType);
}

std::shared_ptr<AnalyzerResult> Analyzer::getResult() { return _result; }
int Analyzer::getOccTimes() {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
  for (auto &var : _coupledVarVec) {
    if (!_curSubCoupledVarSet.count(var)) {
      varVec.push_back(var);
    }
  }
  std::vector<std::vector<int>> state;
  WORKLOAD::generateEdgeState(state, varVec);
  int stateNum = state.size();
  int varNum = varVec.size();

  std::vector<int> timeVec;
  int TColNum = _T.getColNum();
  for (int i = 2; i < TColNum; i++) {
    timeVec.push_back(i);
  }
  int ret = 0;
  if (stateNum == 0) {
    PEX->lock();
    PEY->lock();
    ret += compOneStateTimeSize(timeVec);
    PEX->unlock();
    PEY->unlock();
  } else {
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, varVec);
      std::pair<int, int> PEXRange = compTRange(0);
      std::pair<int, int> PEYRange = compTRange(1);
      PEX->lock();
      PEY->lock();
      ret += compOneStateTimeSize(timeVec) *
             (PEYRange.second - PEYRange.first + 1) *
             (PEXRange.second - PEXRange.first + 1);
      PEX->unlock();
      PEY->unlock();
      changeEdgeByState(0, varNum, i, state, varVec);
    }
  }
  return ret;
}
void Analyzer::setSubLevelResultVec(
    std::vector<std::shared_ptr<AnalyzerResult>> &subLevelResultVec) {
  _result->subLevelResultVec = subLevelResultVec;
}
void Analyzer::changeEdgeByState(
    bool flag, int varNum, int i, std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  for (int j = 0; j < varNum; j++) {
    if (state[i][j]) {
      if (flag) {
        setEdge(varVec[j]);
      } else {
        unsetEdge(varVec[j]);
      }
    }
  }
}
void Analyzer::setCurSubCoupledVarVec(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> curSubCoupledVarVec) {
  _curSubCoupledVarVec = curSubCoupledVarVec;
  for (auto &item : _curSubCoupledVarVec) {
    _curSubCoupledVarSet.insert(item);
  }
}

void Analyzer::setBase(std::vector<Base> baseSet) {
  _baseSet = baseSet;
  _curBaseIndex = 0;
}

std::vector<std::shared_ptr<WORKLOAD::Iterator>> &Analyzer::getCoupledVarVec() {
  return _coupledVarVec;
}