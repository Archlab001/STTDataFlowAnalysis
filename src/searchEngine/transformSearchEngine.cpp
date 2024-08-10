#include "include/searchEngine/transformSearchEngine.h"

namespace DSE {
void TransformSearchEngine::setMatrixAsOne(int dimNum, MAPPING::Transform &T,
                                           std::vector<int> &permute,
                                           int start) {
  for (int i = start; i < dimNum; i++) {
    T.setValue(i, permute[i], 1);
  }
}

// generate transform matrices
void TransformSearchEngine::generateTransformMatrix(
    int dimNum, int spatialDimNum, std::vector<int> &permute,
    std::vector<MAPPING::Transform> &Tvec) {
  //       int dimNum = _coupledVarVec.size();
  int count = 0;

  if (spatialDimNum == 0) {
    Tvec.emplace_back(dimNum);
    setMatrixAsOne(dimNum, Tvec[count++], permute, 0);
  } else if (spatialDimNum == 1) {
    int PEXindex = permute[0];
    if (dimNum == 1) {
      Tvec.emplace_back(dimNum);
      Tvec[count++].setValue(0, PEXindex, 1);
    } else {
      int majorTimeIndex = permute[1];
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, majorTimeIndex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 2);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, majorTimeIndex, 1);
      Tvec[count].setValue(1, PEXindex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 2);
    }
  } else {
    int PEXindex = permute[0];
    int PEYindex = permute[1];

    if (dimNum == 2) {
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count++].setValue(1, PEYindex, 1);
    } else {
      int majorTimeIndex = permute[2];
      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEXindex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEYindex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 3);

      Tvec.emplace_back(dimNum);
      Tvec[count].setValue(0, PEXindex, 1);
      Tvec[count].setValue(1, PEYindex, 1);
      Tvec[count].setValue(2, majorTimeIndex, 1);
      Tvec[count].setValue(2, PEXindex, 1);
      Tvec[count].setValue(2, PEYindex, 1);
      setMatrixAsOne(dimNum, Tvec[count++], permute, 3);
    }
  }
}

// generate transform matrices(multi thread version)
void generateAllTransformMatrixMultiThread(MultiThreadArgs args) {
  args._count = 0;
  std::vector<MAPPING::Transform> TVecTmp;
  do {
    TransformSearchEngine::generateTransformMatrix(args._coupledVarVec.size(),
                                                   args._spatialDimNum,
                                                   args._permute, TVecTmp);
    for (auto &T : TVecTmp) {
      if (args._multanalysis.changeT(args._level, args._coupledVarVec,
                                     args._spatialDimNum, T, true)) {
        args._TVec.push_back(T);
      }
    }
    args._count += TVecTmp.size();
    TVecTmp.clear();
  } while (
      std::next_permutation(args._permute.begin() + 1, args._permute.end()));
}

// generate all transform matrices
void TransformSearchEngine::generateAllTransformMatrix(
    int level, MultLevelAnalyzer &multanalysis,
    std::vector<MultLevelAnalyzer> &multanalysisVec) {
  int dimNum = _coupledVarVec.size();

  assert(dimNum != 0);

  if (dimNum <= 1) { 
    // no multi thread

    std::vector<MAPPING::Transform> TVecTmp;
    std::vector<int> permute{0};
    TransformSearchEngine::generateTransformMatrix(dimNum, _spatialDimNum,
                                                   permute, TVecTmp);
    for (auto &T : TVecTmp) {
      if (multanalysis.changeT(level, _coupledVarVec, _spatialDimNum, T,
                               true)) {
        _TVec.push_back(T);
      }
    }
    TransformSearchEngine::totalCount += TVecTmp.size();
  } else {  
    // multi thread
    std::vector<MAPPING::Transform> TVecTmp;
    std::vector<std::thread> threadVec;
    std::vector<std::vector<MAPPING::Transform>> _TVecVec(dimNum);
    std::vector<MultiThreadArgs> argVec;
    std::vector<std::vector<int>> permuteVec(dimNum);
    std::vector<long long> countVec(dimNum, 0);

    for (int i = 0; i < dimNum; i++) {
      auto &permute = permuteVec[i];
      permute = std::vector<int>(dimNum, 0);
      permute[0] = i;
      std::iota(permute.begin() + 1, permute.end(), 0);
      for (int j = 1; j < dimNum; j++) {
        if (permute[j] >= permute[0])
          permute[j]++;
      }
      argVec.emplace_back(permute, _spatialDimNum, _coupledVarVec, level,
                          _TVecVec[i], countVec[i], multanalysisVec[i]);
      threadVec.emplace_back(generateAllTransformMatrixMultiThread, argVec[i]);
    }
    for (int i = 0; i < dimNum; i++) {
      threadVec[i].join();
    }

    for (int i = 0; i < dimNum; i++) {
      TransformSearchEngine::totalCount += argVec[i]._count;
      for (auto &TVecThread : argVec[i]._TVec) {
        _TVec.push_back(TVecThread);
      }
    }
  }
}

// call the analyzer
void Generator::startAnalysis(
    MultLevelAnalyzer &multanalysis,
    std::vector<std::shared_ptr<MultiLevelTransformSearchResult>> &mltsResult,
    int count, std::ofstream &logFile, bool logFlag, bool firstFlag) {
  int levelNum = _transformSearchEngineSet.size();
  for (int i = 0; i < levelNum; i++) {
    auto &transformSearchEngine = _transformSearchEngineSet[i];
    // change to next transform matrix
    transformSearchEngine.changeT(i, multanalysis);
  }
  // std::cout << multanalysis.constraintCheck() << std::endl;
  multanalysis.oneAnalysis();
  multanalysis.constructSearchResult(
      mltsResult, MultiLevelTransformSearchEngine::_resultCount);
  MultiLevelTransformSearchEngine::_resultCount++;
  if (logFlag) {
    if (!firstFlag)
      logFile << ",\n";
    logFile << "\"ANSWER:"
            << std::to_string(MultiLevelTransformSearchEngine::_resultCount)
            << "\":{";
    multanalysis.outputLog(logFile);
    logFile << "}";
  }
}
} // namespace DSE
