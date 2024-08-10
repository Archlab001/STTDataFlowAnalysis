#include "include/analysis/costAnalysis.h"
#include "include/analysis/multiLevelAnalysis.h"
extern COST::COSTDADA _Cost;
void MultLevelAnalyzer::addLevel(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec,
    MAPPING::Transform &T, ARCH::Level &L, bool doubleBufferFlag) {
  int spatialDimNum = L.getSpatialDimNum();
  for (auto var : coupledVarVec) {
    _allCoupledVarVec.push_back(var);
  }
  _coupledVarVecVec.push_back(coupledVarVec);
  extendT(coupledVarVec, spatialDimNum, T);
  extendCoupledVar(coupledVarVec, spatialDimNum);
  Analyzer analyzer =
      Analyzer(coupledVarVec, T, _I, _W, _O, L, doubleBufferFlag);
  if (!analyzer.constraintCheckAndBuildAnalyzer())
    _validFlags.push_back(false);
  else
    _validFlags.push_back(true);
  if (!_analyzerSet.empty()) {
    if (_analyzerSet[_analyzerSet.size() - 1]
            .getLevel()
            .checkIfNetworkExtended())
      analyzer._subNetworkExtended = true;
  }
  _analyzerSet.emplace_back(analyzer);
}
void MultLevelAnalyzer::addLevel(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec,
    ARCH::Level &L, bool doubleBufferFlag) {
  int spatialDimNum = L.getSpatialDimNum();
  for (auto var : coupledVarVec) {
    _allCoupledVarVec.push_back(var);
  }
  _coupledVarVecVec.push_back(coupledVarVec);
  MAPPING::Transform T(coupledVarVec.size());
  extendT(coupledVarVec, spatialDimNum, T);
  extendCoupledVar(coupledVarVec, spatialDimNum);
  Analyzer analyzer =
      Analyzer(coupledVarVec, T, _I, _W, _O, L, doubleBufferFlag);
  _validFlags.push_back(false);
  if (!_analyzerSet.empty()) {
    if (_analyzerSet[_analyzerSet.size() - 1]
            .getLevel()
            .checkIfNetworkExtended())
      analyzer._subNetworkExtended = true;
  }
  _analyzerSet.emplace_back(analyzer);
}

void MultLevelAnalyzer::extendT(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    int spatialDimNum, MAPPING::Transform &T) {
  if (spatialDimNum == 1) {
    T.addExtraSpatial();
  } else if (spatialDimNum == 0) {
    T.addExtraSpatial();
    T.addExtraSpatial();
  }
  if (coupledVarVec.size() == spatialDimNum) {
    T.addExtraTemporal();
  }
}
void MultLevelAnalyzer::extendCoupledVar(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    int spatialDimNum) {
  if (spatialDimNum == 1) {
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEX")));
  } else if (spatialDimNum == 0) {
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEY")));
    coupledVarVec.insert(coupledVarVec.begin(),
                         std::make_shared<WORKLOAD::Iterator>(
                             WORKLOAD::Iterator(0, 0, "tmpPEX")));
  }
  if (coupledVarVec.size() == 2) {
    coupledVarVec.push_back(
        std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(0, 0, "tmpT")));
  }
}

bool MultLevelAnalyzer::compAndCheckRequiredDataSize(int level) {
  for (auto var : _allCoupledVarVec) {
    var->lock();
  }
  for (int i = 0; i <= level; i++) {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &oneLevelCoupledVarVec =
        _coupledVarVecVec[i];

    for (auto var : oneLevelCoupledVarVec) {
      var->unlock();
    }
  }
  bool ret = _analyzerSet[level].compAndCheckRequiredDataSize();
  for (auto var : _allCoupledVarVec) {
    var->unlock();
  }
  return ret;
}

bool MultLevelAnalyzer::checkRequiredDataSize() {
  int levelNum = _analyzerSet.size();
  for (int i = 0; i < levelNum; i++) {
    if (!compAndCheckRequiredDataSize(i))
      return false;
  }
  return true;
}

void MultLevelAnalyzer::getSubLevelEdge(
    int level, std::map<std::shared_ptr<WORKLOAD::Iterator>,
                        std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap) {
  auto &curLevelCoupledVarVec = _analyzerSet[level].getCoupledVarVec();
  for (auto it = curLevelCoupledVarVec.rbegin();
       it != curLevelCoupledVarVec.rend(); it++) {
    if ((*it)->hasEdge()) {
      subLevelEdgeMap[(*it)->getCoupledIterator()] = *it;
    } else {
      if (subLevelEdgeMap.count((*it))) {
        subLevelEdgeMap.erase((*it));
      }
    }
  }
}
int MultLevelAnalyzer::compBaseIndex(int varNum,
                                     std::vector<std::vector<int>> &state,
                                     int i) {
  int tmp = 0;
  for (int j = 0; j < varNum; j++) {
    tmp *= 2;
    tmp += state[i][j];
  }
  return tmp;
}
int MultLevelAnalyzer::getLevelNum() { return _analyzerSet.size(); }

void MultLevelAnalyzer::changeEdgeByState(
    bool flag, int varNum, int i, std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec) {
  for (int j = 0; j < varNum; j++) {
    if (state[i][j]) {
      if (flag) {
        varVec[j]->setEdge();
      } else {
        varVec[j]->unsetEdge();
      }
    }
  }
}

void MultLevelAnalyzer::generateSublevelBaseResult(
    int level, std::vector<std::shared_ptr<AnalyzerResult>> &subLevelResultVec,
    std::map<std::shared_ptr<WORKLOAD::Iterator>,
             std::shared_ptr<WORKLOAD::Iterator>> &subLevelEdgeMap,
    std::vector<Base> &baseVec) {
  if (subLevelEdgeMap.empty()) {
    recusiveAnalysis(level - 1);
    auto subLevelResult = _analyzerSet[level - 1].getResult();
    subLevelResult->occTimes = _analyzerSet[level].getOccTimes();
    subLevelResultVec.push_back(subLevelResult);
    baseVec.push_back(Base(subLevelResult->delay,
                           _analyzerSet[level - 1].getTensorDimRange()));
  } else {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> curSubCoupledVarVec;
    for (auto &item : subLevelEdgeMap) {
      curSubCoupledVarVec.push_back(item.second);
    }
    _analyzerSet[level].setCurSubCoupledVarVec(curSubCoupledVarVec);
    baseVec = std::vector<Base>(1 << subLevelEdgeMap.size());

    std::vector<std::vector<int>> state;
    WORKLOAD::generateEdgeState(state, curSubCoupledVarVec);
    int stateNum = state.size();
    int varNum = curSubCoupledVarVec.size();
    for (int i = 0; i < stateNum; i++) {
      changeEdgeByState(1, varNum, i, state, curSubCoupledVarVec);
      recusiveAnalysis(level - 1);
      auto subLevelResult = _analyzerSet[level - 1].getResult();
      subLevelResult->occTimes = _analyzerSet[level].getOccTimes();
      subLevelResultVec.push_back(subLevelResult);
      baseVec[compBaseIndex(varNum, state, i)] = Base(
          subLevelResult->delay, _analyzerSet[level - 1].getTensorDimRange());
      changeEdgeByState(0, varNum, i, state, curSubCoupledVarVec);
    }
  }
}

// Performing analysis by recursively constructing base volume and base delay, and conducting bottom-up analysis
void MultLevelAnalyzer::recusiveAnalysis(int level) {
  if (level != 0) {
    std::vector<std::shared_ptr<AnalyzerResult>> subLevelResultVec;
    std::map<std::shared_ptr<WORKLOAD::Iterator>,
             std::shared_ptr<WORKLOAD::Iterator>>
        subLevelEdgeMap;
    getSubLevelEdge(level, subLevelEdgeMap);
    std::vector<Base> baseVec;
    generateSublevelBaseResult(level, subLevelResultVec, subLevelEdgeMap,
                               baseVec);
    _analyzerSet[level].setBase(baseVec);
    _analyzerSet[level].oneAnalysis();
    _analyzerSet[level].setSubLevelResultVec(subLevelResultVec);
    compAndCheckRequiredDataSize(level);
  } else {

    std::vector<Base> baseVec;
    baseVec.push_back({_I.getDimNum(), _W.getDimNum(), _O.getDimNum()});
    _analyzerSet[0].setBase(baseVec);
    _analyzerSet[0].oneAnalysis();
    compAndCheckRequiredDataSize(0);
  }
}

void MultLevelAnalyzer::compMultiLevelReuslt(
    std::shared_ptr<AnalyzerResult> resultTreeRoot) {
  compMultiLevelReusltDFS(resultTreeRoot, _analyzerSet.size() - 1);
  int levelNum = _analyzerSet.size();
  for (int i = 0; i < levelNum; i++) {
    auto result = _resultSet[i];
    result->compRate = result->compCycle / result->compRate;
    result->PEUtilRate =
        double(result->activePEMultTimeNum) / result->totalPEMultTimeNum;
    result->totalBandWidth[ARCH::INPUT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::INPUT);
    result->totalBandWidth[ARCH::WEIGHT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::WEIGHT);
    result->totalBandWidth[ARCH::OUTPUT] =
        _analyzerSet[i].compTotalBandWidth(ARCH::OUTPUT);
    if (_analyzerSet[i]._subNetworkExtended) {
      auto subResult = _resultSet[i - 1];
      result->totalVolumn[ARCH::INPUT] = subResult->totalVolumn[ARCH::INPUT];
      result->totalVolumn[ARCH::WEIGHT] = subResult->totalVolumn[ARCH::WEIGHT];
      result->totalVolumn[ARCH::OUTPUT] = subResult->totalVolumn[ARCH::OUTPUT];
      result->reuseVolumn[ARCH::INPUT] =
          result->totalVolumn[ARCH::INPUT] - result->uniqueVolumn[ARCH::INPUT];
      result->reuseVolumn[ARCH::WEIGHT] = result->totalVolumn[ARCH::WEIGHT] -
                                          result->uniqueVolumn[ARCH::WEIGHT];
      result->reuseVolumn[ARCH::OUTPUT] = result->totalVolumn[ARCH::OUTPUT] -
                                          result->uniqueVolumn[ARCH::OUTPUT];
    }
  }
}

void MultLevelAnalyzer::compMultiLevelReusltDFS(
    std::shared_ptr<AnalyzerResult> node, int level) {
  int occTimes = node->occTimes;
  *_resultSet[level] += *node;
  for (auto item : node->subLevelResultVec) {
    item->occTimes *= occTimes;
    compMultiLevelReusltDFS(item, level - 1);
  }
}

void MultLevelAnalyzer::oneAnalysis() {
  if (!constraintCheck())
    return;
  int levelNum = getLevelNum();
  _resultSet.clear();
  for (int i = 0; i < levelNum; i++) {
    _resultSet.push_back(std::make_shared<AnalyzerResult>(AnalyzerResult()));
  }
  recusiveAnalysis(levelNum - 1);
  auto resultTreeRoot = _analyzerSet[levelNum - 1].getResult();
  resultTreeRoot->occTimes = 1;
  compMultiLevelReuslt(resultTreeRoot);
  compEnergy();
  compArea();
  compPower();
  // outputCSV();
}

void MultLevelAnalyzer::outputCSVArrayName(std::string name,
                                           std::ofstream &logFile) {
  logFile << name + "_output,";
  for (int j = 0; j < 2; j++) {
    logFile << name + "_input_" + std::to_string(j) + ",";
  }
}

void MultLevelAnalyzer::outputCSVArrayDoubleValue(double data[3],
                                                  std::ofstream &logFile) {
  logFile << std::to_string(data[2]) + ",";
  for (int j = 0; j < 2; j++) {
    logFile << std::to_string(data[j]) + ",";
  }
}

void MultLevelAnalyzer::outputCSV() {
  int levelNum = _analyzerSet.size();
  std::ofstream logFile;
  logFile.open("result.csv", std::ios::app);
  logFile << "level ,";
  logFile << "unique_output,";
  logFile << "reuse_output,";
  logFile << "total_output,";
  logFile << "reuseRate_output,";

  for (int j = 0; j < 2; j++) {
    logFile << std::string("unique_input_") + std::to_string(j) + ",";
    logFile << std::string("reuse_input_") + std::to_string(j) + ",";
    logFile << std::string("total_input_") + std::to_string(j) + ",";
    logFile << std::string("reuseRate_input_") + std::to_string(j) + ",";
  }
  outputCSVArrayName("bufferSize", logFile);
  outputCSVArrayName("requiredBandwidth", logFile);
  outputCSVArrayName("totalBandWidth", logFile);
  outputCSVArrayName("maxInitDela", logFile);

  logFile << "maxInitTimes,";
  outputCSVArrayName("maxStableDelay", logFile);

  logFile << std::string("maxStableCompDelay") + ",";
  logFile << std::string("delay") + ",";
  logFile << std::string("compCycleRate") + ",";
  // logFile << "PEUtilRate" << std::endl;
  logFile << std::string("PEUtilRate") + ",";

  logFile << std::string("activePETimeCount") + ",";

  outputCSVArrayName("toSubVolumn", logFile);
  outputCSVArrayName("bufferSubAccessEnergy", logFile);
  outputCSVArrayName("bufferUpAccessEnergy", logFile);

  logFile << std::string("macEnergy") + ",";

  outputCSVArrayName("networkEnergy", logFile);

  logFile << std::string("innerMostRegWriteEnergy") + ",";
  logFile << std::string("innerMostRegReadEnergy") + ",";

  logFile << std::string("accumulateEnergy") + ",";

  outputCSVArrayName("bufferArea", logFile);
  outputCSVArrayName("networkArea", logFile);
  logFile << std::string("macArea") + ",";
  logFile << std::string("innerMostRegArea") + ",";

  logFile << std::string("accumulateArea") + ",";

  outputCSVArrayName("bufferLeakagePower", logFile);
  outputCSVArrayName("networkLeakagePower", logFile);

  logFile << std::string("macLeakagePower") + ",";
  logFile << std::string("innerMostRegLeakagePower") + ",";

  logFile << std::string("accumulateLeakagePower") << std::endl;

  for (int i = 0; i < levelNum; i++) {
    logFile << std::to_string(i) + ',';
    logFile << std::to_string(_resultSet[i]->uniqueVolumn[2]) + ",";
    logFile << std::to_string(_resultSet[i]->reuseVolumn[2]) + ",";
    logFile << std::to_string(_resultSet[i]->totalVolumn[2]) + ",";
    logFile << std::to_string(float(_resultSet[i]->reuseVolumn[2]) /
                              _resultSet[i]->totalVolumn[2]) +
                   ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->uniqueVolumn[j]) + ",";
      logFile << std::to_string(_resultSet[i]->reuseVolumn[j]) + ",";
      logFile << std::to_string(_resultSet[i]->totalVolumn[j]) + ",";
      logFile << std::to_string(float(_resultSet[i]->reuseVolumn[j]) /
                                _resultSet[i]->totalVolumn[j]) +
                     ",";
    }
    logFile << std::to_string(_resultSet[i]->requiredDataSize[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->requiredDataSize[j]) + ",";
    }
    logFile << std::to_string(_resultSet[i]->requiredBandWidth[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->requiredBandWidth[j]) + ",";
    }
    logFile << std::to_string(_resultSet[i]->totalBandWidth[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->totalBandWidth[j]) + ",";
    }
    logFile << std::to_string(_resultSet[i]->initDelay[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->initDelay[j]) + ",";
    }
    logFile << std::to_string(_resultSet[i]->initTimes) + ",";
    logFile << std::to_string(_resultSet[i]->stableDelay[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->stableDelay[j]) + ",";
    }
    logFile << std::to_string(_resultSet[i]->stableDelay[3]) + ",";
    logFile << std::to_string(_resultSet[i]->delay) + ",";
    logFile << std::to_string(_resultSet[i]->compRate) + ",";
    // logFile << std::to_string(_resultSet[i]->PEUtilRate) << std::endl;
    logFile << std::to_string(_resultSet[i]->PEUtilRate) + ",";
    if (i == 0)
      logFile << std::to_string(_resultSet[i]->totalPEMultTimeNum) + ",";
    else
      logFile << std::to_string(0) + ",";

    logFile << std::to_string(_resultSet[i]->toSubVolumn[2]) + ",";
    for (int j = 0; j < 2; j++) {
      logFile << std::to_string(_resultSet[i]->toSubVolumn[j]) + ",";
    }
    outputCSVArrayDoubleValue(_resultSet[i]->bufferSubAccessEnergy, logFile);
    outputCSVArrayDoubleValue(_resultSet[i]->bufferUpAccessEnergy, logFile);
    logFile << std::to_string(_resultSet[i]->macEnergy) + ",";
    outputCSVArrayDoubleValue(_resultSet[i]->networkEnergy, logFile);
    logFile << std::to_string(_resultSet[i]->innerMostRegWriteEnergy) + ",";
    logFile << std::to_string(_resultSet[i]->innerMostRegReadEnergy) + ",";
    logFile << std::to_string(_resultSet[i]->accumulateEnergy) + ",";

    outputCSVArrayDoubleValue(_resultSet[i]->bufferArea, logFile);
    outputCSVArrayDoubleValue(_resultSet[i]->networkArea, logFile);
    logFile << std::to_string(_resultSet[i]->macArea) + ",";
    logFile << std::to_string(_resultSet[i]->innerMostRegArea) + ",";
    logFile << std::to_string(_resultSet[i]->accumulateArea) + ",";

    outputCSVArrayDoubleValue(_resultSet[i]->bufferLeakagePower, logFile);
    outputCSVArrayDoubleValue(_resultSet[i]->networkLeakagePower, logFile);
    logFile << std::to_string(_resultSet[i]->macLeakagePower) + ",";
    logFile << std::to_string(_resultSet[i]->innerMostRegLeakagePower) + ",";
    logFile << std::to_string(_resultSet[i]->accumulateLeakagePower)
            << std::endl;
  }
  logFile.close();
}

void MultLevelAnalyzer::outputLog(std::ofstream &logFile) {
  int levelNum = _analyzerSet.size();
  for (int i = 0; i < levelNum; i++) {
    if (i != 0)
      logFile << ",";
    logFile << "\"LEVEL" + std::to_string(i) + "\":\n{";
    _analyzerSet[i].outputConfig(logFile);
    _resultSet[i]->outputLog(logFile);
    logFile << "}";
  }
}

void MultLevelAnalyzer::compEnergy() {
  // long long totalCycle = _resultSet[level]->delay;
  int levelNum = _analyzerSet.size();
  // ARCH::Level& _L = _analyzerSet[level].getLevel();

  // for curlevel and the innermost regfile(if has), so levelNum + 1
  std::vector<std::vector<long long>> bufferReadAccessCount(
      3, std::vector<long long>(levelNum + 1, 0));
  std::vector<std::vector<long long>> bufferWriteAccessCount(
      3, std::vector<long long>(levelNum + 1, 0));

  for (int i = 0; i < levelNum; i++) {
    bufferReadAccessCount[ARCH::INPUT][i + 1] =
        _resultSet[i]->uniqueVolumn[ARCH::INPUT];
    bufferWriteAccessCount[ARCH::INPUT][i] =
        _resultSet[i]->toSubVolumn[ARCH::INPUT];
    bufferReadAccessCount[ARCH::WEIGHT][i + 1] =
        _resultSet[i]->uniqueVolumn[ARCH::WEIGHT];
    bufferWriteAccessCount[ARCH::WEIGHT][i] =
        _resultSet[i]->toSubVolumn[ARCH::WEIGHT];
    bufferWriteAccessCount[ARCH::OUTPUT][i + 1] =
        _resultSet[i]->uniqueVolumn[ARCH::OUTPUT];
    bufferReadAccessCount[ARCH::OUTPUT][i] =
        _resultSet[i]->toSubVolumn[ARCH::OUTPUT];
  }

  // for buffer
  for (int i = 0; i < levelNum; i++) {
    ARCH::Level &L = _analyzerSet[i].getLevel();
    _resultSet[i]->bufferSubAccessEnergy[ARCH::INPUT] =
        L.getBufferReadWriteEnergy(
            ARCH::INPUT, bufferReadAccessCount[ARCH::INPUT][i + 1], 0);
    _resultSet[i]->bufferUpAccessEnergy[ARCH::INPUT] =
        L.getBufferReadWriteEnergy(
            ARCH::INPUT, bufferWriteAccessCount[ARCH::INPUT][i + 1], 1);

    _resultSet[i]->bufferSubAccessEnergy[ARCH::WEIGHT] =
        L.getBufferReadWriteEnergy(
            ARCH::WEIGHT, bufferReadAccessCount[ARCH::WEIGHT][i + 1], 0);
    _resultSet[i]->bufferUpAccessEnergy[ARCH::WEIGHT] =
        L.getBufferReadWriteEnergy(
            ARCH::WEIGHT, bufferWriteAccessCount[ARCH::WEIGHT][i + 1], 1);

    _resultSet[i]->bufferSubAccessEnergy[ARCH::OUTPUT] =
        L.getBufferReadWriteEnergy(
            ARCH::OUTPUT, bufferWriteAccessCount[ARCH::OUTPUT][i + 1], 1);
    _resultSet[i]->bufferUpAccessEnergy[ARCH::OUTPUT] =
        L.getBufferReadWriteEnergy(
            ARCH::OUTPUT, bufferReadAccessCount[ARCH::OUTPUT][i + 1], 0);
  }

  // for network
  getNetworkEnergy(ARCH::INPUT);
  getNetworkEnergy(ARCH::WEIGHT);
  getNetworkEnergy(ARCH::OUTPUT);

  ARCH::Level &L0 = _analyzerSet[0].getLevel();

  // for inner reg
  if (!L0.isPE()) {
    _resultSet[0]->innerMostRegWriteEnergy =
        _Cost._regData.lookup(1, 1) * L0.getDataWidth() / 8 *
        (bufferWriteAccessCount[ARCH::INPUT][0] +
         bufferWriteAccessCount[ARCH::WEIGHT][0] +
         bufferWriteAccessCount[ARCH::OUTPUT][0] +
         _resultSet[0]->activePEMultTimeNum);
    _resultSet[0]->innerMostRegReadEnergy =
        _Cost._regData.lookup(1, 0) * L0.getDataWidth() / 8 *
        (bufferReadAccessCount[ARCH::INPUT][0] +
         bufferReadAccessCount[ARCH::WEIGHT][0] +
         bufferReadAccessCount[ARCH::OUTPUT][0] +
         2 * _resultSet[0]->activePEMultTimeNum);
  }

  // for mac
  _resultSet[0]->macEnergy =
      L0.getMacEnergy(_resultSet[0]->activePEMultTimeNum);

  for (int i = 0; i < levelNum; i++) {
    if (i == 0) {
      _resultSet[i]->accumulateEnergy =
          _resultSet[i]->bufferSubAccessEnergy[ARCH::OUTPUT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::OUTPUT] +
          _resultSet[i]->bufferSubAccessEnergy[ARCH::INPUT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::INPUT] +
          _resultSet[i]->bufferSubAccessEnergy[ARCH::WEIGHT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::WEIGHT] +
          _resultSet[i]->networkEnergy[ARCH::INPUT] +
          _resultSet[i]->networkEnergy[ARCH::WEIGHT] +
          _resultSet[i]->networkEnergy[ARCH::OUTPUT] +
          _resultSet[i]->innerMostRegReadEnergy +
          _resultSet[i]->innerMostRegReadEnergy + _resultSet[i]->macEnergy;
    } else {
      _resultSet[i]->accumulateEnergy =
          _resultSet[i]->bufferSubAccessEnergy[ARCH::OUTPUT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::OUTPUT] +
          _resultSet[i]->bufferSubAccessEnergy[ARCH::INPUT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::INPUT] +
          _resultSet[i]->bufferSubAccessEnergy[ARCH::WEIGHT] +
          _resultSet[i]->bufferUpAccessEnergy[ARCH::WEIGHT] +
          _resultSet[i]->networkEnergy[ARCH::INPUT] +
          _resultSet[i]->networkEnergy[ARCH::WEIGHT] +
          _resultSet[i]->networkEnergy[ARCH::OUTPUT] +
          _resultSet[i - 1]->accumulateEnergy;
    }
  }
}

void MultLevelAnalyzer::compArea() {
  int levelNum = _analyzerSet.size();
  for (int i = 0; i < levelNum; i++) {
    ARCH::Level &L = _analyzerSet[i].getLevel();
    _resultSet[i]->bufferArea[ARCH::INPUT] =
        L.getBufferAreaAndLeakagePower(ARCH::INPUT, 2);
    _resultSet[i]->bufferArea[ARCH::WEIGHT] =
        L.getBufferAreaAndLeakagePower(ARCH::WEIGHT, 2);
    _resultSet[i]->bufferArea[ARCH::OUTPUT] =
        L.getBufferAreaAndLeakagePower(ARCH::OUTPUT, 2);
  }

  getNetworkArea(ARCH::INPUT);
  getNetworkArea(ARCH::WEIGHT);
  getNetworkArea(ARCH::OUTPUT);

  ARCH::Level &L0 = _analyzerSet[0].getLevel();
  _resultSet[0]->macArea = L0.getMacCost(1);

  // for inner reg
  if (!L0.isPE()) {
    _resultSet[0]->innerMostRegArea = _Cost._regData.lookup(1, 2) *
                                      L0.getDataWidth() / 8 * L0.getRowNum() *
                                      L0.getColNum() * 3;
  }

  for (int i = 0; i < levelNum; i++) {

    if (i == 0) {
      _resultSet[i]->accumulateArea = _resultSet[i]->bufferArea[ARCH::INPUT] +
                                      _resultSet[i]->bufferArea[ARCH::OUTPUT] +
                                      _resultSet[i]->bufferArea[ARCH::WEIGHT] +
                                      _resultSet[i]->networkArea[ARCH::INPUT] +
                                      _resultSet[i]->networkArea[ARCH::OUTPUT] +
                                      _resultSet[i]->networkArea[ARCH::WEIGHT] +
                                      _resultSet[i]->macArea +
                                      _resultSet[i]->innerMostRegArea;
    } else {
      ARCH::Level &L = _analyzerSet[i].getLevel();
      _resultSet[i]->accumulateArea =
          _resultSet[i]->bufferArea[ARCH::INPUT] +
          _resultSet[i]->bufferArea[ARCH::OUTPUT] +
          _resultSet[i]->bufferArea[ARCH::WEIGHT] +
          _resultSet[i]->networkArea[ARCH::INPUT] +
          _resultSet[i]->networkArea[ARCH::OUTPUT] +
          _resultSet[i]->networkArea[ARCH::WEIGHT] + _resultSet[i]->macArea +
          _resultSet[i - 1]->accumulateArea * L.getRowNum() * L.getColNum();
    }
  }
}

void MultLevelAnalyzer::compPower() {
  int levelNum = _analyzerSet.size();
  for (int i = 0; i < levelNum; i++) {
    ARCH::Level &L = _analyzerSet[i].getLevel();
    _resultSet[i]->bufferLeakagePower[ARCH::INPUT] =
        L.getBufferAreaAndLeakagePower(ARCH::INPUT, 3);
    _resultSet[i]->bufferLeakagePower[ARCH::WEIGHT] =
        L.getBufferAreaAndLeakagePower(ARCH::WEIGHT, 3);
    _resultSet[i]->bufferLeakagePower[ARCH::OUTPUT] =
        L.getBufferAreaAndLeakagePower(ARCH::OUTPUT, 3);
  }

  getNetworkLeakagePower(ARCH::INPUT);
  getNetworkLeakagePower(ARCH::WEIGHT);
  getNetworkLeakagePower(ARCH::OUTPUT);

  ARCH::Level &L0 = _analyzerSet[0].getLevel();
  _resultSet[0]->macLeakagePower = L0.getMacCost(2);

  // for inner reg
  if (!L0.isPE()) {
    _resultSet[0]->innerMostRegLeakagePower =
        _Cost._regData.lookup(1, 3) * L0.getDataWidth() / 8 * L0.getRowNum() *
        L0.getColNum() * 3;
  }

  for (int i = 0; i < levelNum; i++) {

    if (i == 0) {
      _resultSet[i]->accumulateLeakagePower =
          _resultSet[i]->bufferLeakagePower[ARCH::INPUT] +
          _resultSet[i]->bufferLeakagePower[ARCH::OUTPUT] +
          _resultSet[i]->bufferLeakagePower[ARCH::WEIGHT] +
          _resultSet[i]->networkLeakagePower[ARCH::INPUT] +
          _resultSet[i]->networkLeakagePower[ARCH::OUTPUT] +
          _resultSet[i]->networkLeakagePower[ARCH::WEIGHT] +
          _resultSet[i]->macLeakagePower +
          _resultSet[i]->innerMostRegLeakagePower;
    } else {
      ARCH::Level &L = _analyzerSet[i].getLevel();
      _resultSet[i]->accumulateLeakagePower =
          _resultSet[i]->bufferLeakagePower[ARCH::INPUT] +
          _resultSet[i]->bufferLeakagePower[ARCH::OUTPUT] +
          _resultSet[i]->bufferLeakagePower[ARCH::WEIGHT] +
          _resultSet[i]->networkLeakagePower[ARCH::INPUT] +
          _resultSet[i]->networkLeakagePower[ARCH::OUTPUT] +
          _resultSet[i]->networkLeakagePower[ARCH::WEIGHT] +
          _resultSet[i]->macLeakagePower +
          _resultSet[i - 1]->accumulateLeakagePower * L.getRowNum() *
              L.getColNum();
    }
  }
}