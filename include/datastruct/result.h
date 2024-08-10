#pragma once
#include "include/datastruct/mapping.h"
#include <memory>
#include <vector>
struct Base {
  std::vector<std::vector<long long>> baseData;
  long long baseCompCycle;
  Base() {
    baseData = std::vector<std::vector<long long>>(3);
    baseCompCycle = 1;
  }
  Base(int inputDimNum, int weightDimNum, int outputDimNum) {
    baseData = std::vector<std::vector<long long>>(3);
    baseData[0] = std::vector<long long>(inputDimNum, 1);
    baseData[1] = std::vector<long long>(weightDimNum, 1);
    baseData[2] = std::vector<long long>(outputDimNum, 1);
    baseCompCycle = 1;
  }
  Base(long long newBaseCompCycle,
       std::vector<std::vector<long long>> newBaseData) {
    baseData = std::vector<std::vector<long long>>(3);
    baseCompCycle = newBaseCompCycle;
    for (int i = 0; i < 3; i++) {
      baseData[i] = newBaseData[i];
    }
  }
};
struct AnalyzerResult {
  long long uniqueVolumn[3]; // input weight output
  long long totalVolumn[3];
  long long reuseVolumn[3];
  long long requiredDataSize[3];
  long long initDelay[3];
  long long initTimes;
  long long stableDelay[4];
  long long delay;
  long long compCycle;
  long long occTimes;
  double compRate;
  long long activePEMultTimeNum;
  long long totalPEMultTimeNum;
  double PEUtilRate;
  long long totalBandWidth[3];
  long long requiredBandWidth[3];
  std::vector<std::shared_ptr<AnalyzerResult>> subLevelResultVec;
  std::shared_ptr<std::map<std::pair<int, int>, long long>>
      activateCountMapVec[3];

  long long toSubVolumn[3];
  double bufferSubAccessEnergy[3];
  double bufferUpAccessEnergy[3];
  double accumulateEnergy;
  double networkEnergy[3];
  double innerMostRegWriteEnergy;
  double innerMostRegReadEnergy;
  double macEnergy;

  double bufferArea[3];
  double networkArea[3];
  double macArea;
  double accumulateArea;
  double innerMostRegArea;

  double bufferLeakagePower[3];
  double networkLeakagePower[3];
  double macLeakagePower;
  double accumulateLeakagePower;
  double innerMostRegLeakagePower;

  AnalyzerResult() { reset(); }
  void reset() {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] = 0;
      totalVolumn[i] = 0;
      reuseVolumn[i] = 0;
      requiredDataSize[i] = 0;
      initDelay[i] = 0;
      stableDelay[i] = 0;
      totalBandWidth[i] = 0;
      requiredBandWidth[i] = 0;
      activateCountMapVec[i] =
          std::make_shared<std::map<std::pair<int, int>, long long>>();
      bufferSubAccessEnergy[i] = 0;
      bufferUpAccessEnergy[i] = 0;
      toSubVolumn[i] = 0;
      networkEnergy[i] = 0;
      bufferArea[i] = 0;
      networkArea[i] = 0;
      bufferLeakagePower[i] = 0;
      networkLeakagePower[i] = 0;
    }
    innerMostRegLeakagePower = 0;
    accumulateLeakagePower = 0;
    macLeakagePower = 0;
    innerMostRegArea = 0;
    accumulateArea = 0;
    macArea = 0;
    macEnergy = 0;
    innerMostRegWriteEnergy = 0;
    innerMostRegReadEnergy = 0;
    accumulateEnergy = 0;
    stableDelay[3] = 0;
    initTimes = 0;
    delay = 0;
    occTimes = 0;
    compCycle = 0;
    compRate = 0;
    activePEMultTimeNum = 0;
    totalPEMultTimeNum = 0;
    PEUtilRate = 0;
    subLevelResultVec.clear();
  }

  AnalyzerResult &operator+=(AnalyzerResult &other) {
    for (int i = 0; i < 3; i++) {
      uniqueVolumn[i] += other.uniqueVolumn[i] * other.occTimes;
      totalVolumn[i] += other.totalVolumn[i] * other.occTimes;
      reuseVolumn[i] += other.reuseVolumn[i] * other.occTimes;
      toSubVolumn[i] += other.toSubVolumn[i] * other.occTimes;
      requiredDataSize[i] =
          std::max(requiredDataSize[i], other.requiredDataSize[i]);
      initDelay[i] = std::max(initDelay[i], other.initDelay[i]);
      stableDelay[i] = std::max(stableDelay[i], other.stableDelay[i]);
      totalBandWidth[i] = 0;
      requiredBandWidth[i] =
          std::max(requiredBandWidth[i], other.requiredBandWidth[i]);
      networkEnergy[i] = 0;
      for (auto item : (*(other.activateCountMapVec[i]))) {
        if ((*(activateCountMapVec[i])).count(item.first) > 0)
          (*(activateCountMapVec[i]))[item.first] += item.second;
        else
          (*(activateCountMapVec[i]))[item.first] = item.second;
      }
      bufferSubAccessEnergy[i] = 0;
      bufferUpAccessEnergy[i] = 0;
    }
    macEnergy = 0;
    innerMostRegWriteEnergy = 0;
    innerMostRegReadEnergy = 0;
    stableDelay[3] = std::max(stableDelay[3], other.stableDelay[3]);
    initTimes = std::max(initTimes, other.initTimes);
    delay = std::max(delay, other.delay);
    compRate += other.delay * other.occTimes;
    compCycle += other.compCycle * other.occTimes;
    occTimes += other.occTimes;
    activePEMultTimeNum += other.activePEMultTimeNum * other.occTimes;
    totalPEMultTimeNum += other.totalPEMultTimeNum * other.occTimes;
    PEUtilRate = 0;
    accumulateEnergy = 0;
    return *this;
  }
  void outputLogDataAccess(ARCH::DATATYPE dataType, std::ofstream &logFile) {
    std::string tensorSym;
    if (dataType == ARCH::OUTPUT)
      tensorSym = "output";
    else {
      tensorSym = "input_";
      tensorSym += std::to_string(dataType - ARCH::INPUT);
    }
    logFile << "\"unique_" << tensorSym << "\":\""
            << std::to_string(uniqueVolumn[dataType]);
    logFile << "\"\n,";
    logFile << "\"reuse_" << tensorSym << "\":\""
            << std::to_string(reuseVolumn[dataType]);
    logFile << "\"\n,";
    logFile << "\"total_" << tensorSym << "\":\""
            << std::to_string((totalVolumn[dataType]));
    logFile << "\"\n,";
    logFile << "\"reuseRate_" << tensorSym << "\":\""
            << std::to_string(float(reuseVolumn[dataType]) /
                              totalVolumn[dataType]);
    logFile << "\"\n,";
  }

  void outputLogDoubleArray(std::string name, double data[3],
                            std::ofstream &logFile) {
    logFile << "\"" + name + "_output" + "\":\""
            << std::to_string(data[2]) + "\",\n";
    for (int j = 0; j < 2; j++) {
      logFile << "\"" + name + "_input_" + std::to_string(j) + "\":\""
              << std::to_string(data[j]) + "\",\n";
    }
  }

  void outputLog(std::ofstream &logFile) {
    outputLogDataAccess(ARCH::OUTPUT, logFile);
    outputLogDataAccess(ARCH::INPUT, logFile);
    outputLogDataAccess(ARCH::WEIGHT, logFile);
    logFile << "\"bufferSize_output\":\""
            << std::to_string(requiredDataSize[2]) + "\",\n";
    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"bufferSize_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(requiredDataSize[j]) + "\",\n";
    }
    logFile << "\"requiredBandWidth_output\":\""
            << std::to_string(requiredBandWidth[2]) + "\",\n";
    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"requiredBandWidth_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(requiredBandWidth[j]) + "\",\n";
    }

    logFile << "\"totalBandWidth_output\":\""
            << std::to_string(totalBandWidth[2]) + "\",\n";
    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"totalBandWidth_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(totalBandWidth[j]) + "\",\n";
    }
    logFile << std::string("\"maxInitDelay_output_") + "\":\""
            << std::to_string(initDelay[ARCH::OUTPUT]) + "\",\n";
    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"maxInitDelay_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(initDelay[j]) + "\",\n";
    }

    logFile << "\"maxInitTimes\":\"" << std::to_string(initTimes) + "\",\n";
    logFile << "\"maxStableDelay_output\":\""
            << std::to_string(stableDelay[2]) + "\",\n";

    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"maxStableDelay_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(stableDelay[j]) + "\",\n";
    }

    logFile << std::string("\"maxStableCompDelay\"") + ":\""
            << std::to_string(stableDelay[3]) + "\",\n";
    logFile << std::string("\"delay\"") + ":\""
            << std::to_string(delay) + "\",\n";
    logFile << std::string("\"compCycleRate\"") + ":\""
            << std::to_string(compRate) + "\",\n";
    // logFile << "\"PEUtilRate\":\"" << std::to_string(PEUtilRate) << "\"\n";
    logFile << "\"PEUtilRate\":\"" << std::to_string(PEUtilRate) << "\",\n";
    logFile << "\"activePETimeCount\":\"" << std::to_string(activePEMultTimeNum)
            << "\",\n";

    logFile << "\"toSubVolumn_output\":\""
            << std::to_string(toSubVolumn[2]) + "\",\n";

    for (int j = 0; j < 2; j++) {
      logFile << std::string("\"toSubVolumn_input_") + std::to_string(j) +
                     "\":\""
              << std::to_string(toSubVolumn[j]) + "\",\n";
    }

    outputLogDoubleArray("bufferSubAccessEnergy", bufferSubAccessEnergy,
                         logFile);
    outputLogDoubleArray("bufferUpAccessEnergy", bufferUpAccessEnergy, logFile);
    logFile << "\"macEnergy\":\"" << std::to_string(macEnergy) << "\",\n";
    outputLogDoubleArray("networkEnergy", networkEnergy, logFile);
    logFile << "\"innerMostRegWriteEnergy\":\""
            << std::to_string(innerMostRegWriteEnergy) << "\",\n";
    logFile << "\"innerMostRegReadEnergy\":\""
            << std::to_string(innerMostRegReadEnergy) << "\",\n";
    logFile << "\"accumulateEnergy\":\"" << std::to_string(accumulateEnergy)
            << "\",\n";

    outputLogDoubleArray("bufferArea", bufferArea, logFile);
    outputLogDoubleArray("networkArea", networkArea, logFile);
    logFile << "\"macArea\":\"" << std::to_string(macArea) << "\",\n";
    logFile << "\"innerMostRegArea\":\"" << std::to_string(innerMostRegArea)
            << "\",\n";
    logFile << "\"accumulateArea\":\"" << std::to_string(accumulateArea)
            << "\",\n";

    outputLogDoubleArray("bufferLeakagePower", bufferLeakagePower, logFile);
    outputLogDoubleArray("networkLeakagePower", networkLeakagePower, logFile);
    logFile << "\"macLeakagePower\":\"" << std::to_string(macLeakagePower)
            << "\",\n";
    logFile << "\"innerMostRegLeakagePower\":\""
            << std::to_string(innerMostRegLeakagePower) << "\",\n";
    logFile << "\"accumulateLeakagePower\":\""
            << std::to_string(accumulateLeakagePower) << "\"\n";
  }
};

struct TransformSearchResult {
  MAPPING::Transform _T;
  std::shared_ptr<AnalyzerResult> _result;
  TransformSearchResult(MAPPING::Transform &T,
                        std::shared_ptr<AnalyzerResult> &result)
      : _result(result), _T(T) {}
  void outputLog(std::ofstream &logFile) {
    logFile << "\"Transform Matrix:\":\n";
    _T.outputT(logFile);
    logFile << ",";
    _result->outputLog(logFile);
  }
};

struct MultiLevelTransformSearchResult {
  std::vector<std::shared_ptr<TransformSearchResult>> _transformSearchResult;
  long long _index;
  MultiLevelTransformSearchResult(long long index) : _index(index){};
  void addResult(MAPPING::Transform T,
                 std::shared_ptr<AnalyzerResult> &result) {
    _transformSearchResult.push_back(
        std::make_shared<TransformSearchResult>(T, result));
  }
  void outputLog(std::ofstream &logFile) {
    int levelNum = _transformSearchResult.size();
    for (int j = 0; j < levelNum; j++) {
      if (j != 0)
        logFile << ",";
      logFile << "\"LEVEL" + std::to_string(j) + "\":\n{";
      _transformSearchResult[j]->outputLog(logFile);
      logFile << "}";
    }
  }
};

struct GroupSearchResult {
  double score;
  std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
      _coupledVarVecVec;
  std::shared_ptr<MultiLevelTransformSearchResult>
      _multiLevelTransformSearchResult;
  GroupSearchResult(
      std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
          coupledVarVecVec,
      std::shared_ptr<MultiLevelTransformSearchResult>
          multiLevelTransformSearchResult)
      : _multiLevelTransformSearchResult(multiLevelTransformSearchResult),
        _coupledVarVecVec(coupledVarVecVec), score(0) {}
  void outputLog(std::ofstream &logFile, int index, int levelNum) {
    logFile << " \"Group Search: "
            << std::to_string(_multiLevelTransformSearchResult->_index);
    for (int i = 0; i < levelNum; i++) {
      auto vec = _coupledVarVecVec[i];
      logFile << "Level " + std::to_string(i) << ' ';
      for (auto num : vec) {
        logFile << num->getSym() << ' ';
      }
    }
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
    for (auto iteratorVec : _coupledVarVecVec) {
      for (auto iterator : iteratorVec) {
        coupledVarVec.push_back(iterator);
      }
    }
    logFile << "\":{" << std::endl;
    logFile << "\"Coupled Var\":\n";
    logFile << "{";
    int count = 0;
    for (auto &var : coupledVarVec) {
      if (count != 0)
        logFile << ',';
      logFile << "\"" << std::to_string(count) << "\":\"";
      count++;
      logFile << var->to_string();
      logFile << "\"\n";
    }
    logFile << "},";
    _multiLevelTransformSearchResult->outputLog(logFile);
    logFile << "}";
  }
};