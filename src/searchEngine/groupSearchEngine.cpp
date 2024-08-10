#include "include/searchEngine/groupSearchEngine.h"

namespace DSE {

// combination generation algorithm
// n represents the total number of elements
// k represents the size of the combinations to generate
// groupVec is the vector that stores the generated combinations
void GroupSearchEngine::combine(int n, int k, std::vector<Group> &groupVec) {
  std::vector<int> temp;
  for (int i = 1; i <= k; ++i) {
    temp.push_back(i);
  }
  temp.push_back(n + 1); // n + 1 is a termination condition

  int j = 0;
  while (j < k) {
    groupVec.emplace_back(std::vector<int>(temp.begin(), temp.begin() + k));
    j = 0;
    while (j < k && temp[j] + 1 == temp[j + 1]) {
      temp[j] = j + 1;
      ++j;
    }
    ++temp[j];
  }
}
// construct tree grouping record 
// perGroupNum : how many itertors in one group, ex: perGroupNum={1,7} means that level0 has 1 itertor and level1 have 7 itertors
// varNum: total iterator num
// candidate: unallocated iterator index
void GroupSearchEngine::constructGroup(std::vector<int> &perGroupNum,
                                       int levelIndex, int varNum,
                                       Group &rootGroup,
                                       std::vector<int> &candidate) {
  int maxLevelIndex = perGroupNum.size();
  combine(varNum, perGroupNum[levelIndex], rootGroup._subGroupVec);
  for (auto &group : rootGroup._subGroupVec) {
    std::set<int> notSubCandidate;
    for (int i = 0; i < perGroupNum[levelIndex]; i++) {
      group._indexVec[i] = candidate[group._indexVec[i] - 1];
      notSubCandidate.insert(group._indexVec[i]);
    }
    std::vector<int> subCandidate;
    for (auto num : candidate) {
      if (notSubCandidate.count(num) == 0)
        subCandidate.push_back(num);
    }
    if (levelIndex < maxLevelIndex - 1)
      constructGroup(perGroupNum, levelIndex + 1,
                     varNum - perGroupNum[levelIndex], group, subCandidate);
  }
}
// recursively traverse the tree grouping record and construct coupledVarVecVec
// rootGroup: root of the tree grouping record 
// levelIndex: accelerator level index
// coupledVarVecVec: the coupledVarVec of each level
void GroupSearchEngine::buildCoupleVarVec(
    Group &rootGroup, int levelIndex,
    std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
        &coupledVarVecVec,
    std::ofstream &logFile, bool logFlag) {
  // recursive termination
  if (levelIndex == _LVec.size()) {
    int levelNum = _LVec.size();
    if (logFlag) {
      if (!_firstFlag)
        logFile << ",\n";
      else
        _firstFlag = false;

      logFile << " \"Group Search: ";
      for (int i = 0; i < levelNum; i++) {
        auto vec = coupledVarVecVec[i];
        logFile << "Level " + std::to_string(i) << ' ';
        for (auto num : vec) {
          logFile << num->getSym() << ' ';
        }
      }
      logFile << "\":{" << std::endl;
    }
    // for (int i = 0; i < levelNum; i++)
    //{
    //    auto vec = coupledVarVecVec[i];
    //    std::cout << "Level" + std::to_string(i) << ' ';
    //    for (auto num : vec)
    //    {
    //        std::cout << num->getSym() << ' ';
    //    }
    //}
    // std::cout << std::endl;
    totalCount += 1;
    DSE::MultiLevelTransformSearchEngine multiLevelTransformSearchEngine(_I, _W,
                                                                         _O);

    for (int i = 0; i < levelNum; i++) {
      multiLevelTransformSearchEngine.addLevel(coupledVarVecVec[i], _LVec[i]);
    }
    // start transformSearchEngine
    multiLevelTransformSearchEngine.oneSearch(logFile, logFlag);
    if (logFlag)
      logFile << "}" << std::endl;
    multiLevelTransformSearchEngine.constructGroupSearchResult(
        _groupSearchResult, coupledVarVecVec);

  } else {
    for (auto &group : rootGroup._subGroupVec) {
      std::vector<std::shared_ptr<WORKLOAD::Iterator>> coupledVarVec;
      for (auto index : group._indexVec)
        coupledVarVec.push_back(_varVec[index]);
      coupledVarVecVec.push_back(coupledVarVec);
      buildCoupleVarVec(group, levelIndex + 1, coupledVarVecVec, logFile,
                        logFlag);
      coupledVarVecVec.pop_back();
    }
  }
}

// perGroupNum : how many itertors in one group, ex: perGroupNum={1,7} means that level0 has 1 itertor and level1 have 7 itertors
// varNum: how many iterators are still waiting for allocation
// levelNum: accelerator level num
void GroupSearchEngine::recusiveCompPerGroupNum(std::vector<int> &perGroupNum,
                                                int varNum, int levelNum,
                                                std::ofstream &logFile,
                                                bool logFlag) {
  if (levelNum == 1) {
    // if (perGroupNum.size() != 0 && perGroupNum[perGroupNum.size() - 1] >
    // varNum)
    //    return;
    if (perGroupNum.size() != 0 &&
        varNum < std::max(1, _spatialNumVec[perGroupNum.size()]))
      return;
    perGroupNum.push_back(varNum);
    for (auto num : perGroupNum) {
      std::cout << num << ' ';
    }
    std::cout << std::endl;
    Group rootGroup;
    std::vector<int> candidate(_varVec.size(), 0);
    std::iota(candidate.begin(), candidate.end(), 0);
    // construct the rootGroup according to perGroupNum
    constructGroup(perGroupNum, 0, _varVec.size(), rootGroup, candidate);
    std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
        coupledVarVecVec;
    // use rootGroup build coupleVarVecVec
    buildCoupleVarVec(rootGroup, 0, coupledVarVecVec, logFile, logFlag);
    perGroupNum.pop_back();
  } else {
    // for (int i = perGroupNum.size() == 0 ? 1 : perGroupNum[perGroupNum.size()
    // - 1];
    //    i <= varNum - levelNum + 1; i++)
    for (int i = std::max(1, _spatialNumVec[perGroupNum.size()]);
         i <= varNum - levelNum + 1; i++) {
      perGroupNum.push_back(i);
      recusiveCompPerGroupNum(perGroupNum, varNum - i, levelNum - 1, logFile,
                              logFlag);
      perGroupNum.pop_back();
    }
  }
}

// entry of GroupSearchEngine
void GroupSearchEngine::oneSearch(std::ofstream &logFile, bool logFlag) {
  if (_varVec.size() < _LVec.size())
    return;
  MultiLevelTransformSearchEngine multiLevelTransformSearchEngine(_I, _W, _O);
  std::vector<int> perGroupNum;
  _firstFlag = true;
  if (logFlag)
    logFile << "{" << std::endl;
  recusiveCompPerGroupNum(perGroupNum, _varVec.size(), _LVec.size(), logFile,
                          logFlag);
  if (logFlag)
    logFile << "}" << std::endl;
}

void GroupSearchEngine::sortResult(int flag) {
  if (flag == 0) {
    std::sort(_groupSearchResult.begin(), _groupSearchResult.end(),
              cmpResultByTotalCycle);
  }
  for (int i = 0; i < _groupSearchResult.size(); i++) {
    printf("%lld ",
           _groupSearchResult[i]
               ->_multiLevelTransformSearchResult->_transformSearchResult[1]
               ->_result->delay);

    std::vector<std::shared_ptr<TransformSearchResult>> &tr1 =
        _groupSearchResult[i]
            ->_multiLevelTransformSearchResult->_transformSearchResult;
    long long unique1 = 0;
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 3; k++) {
        unique1 += tr1[j]->_result->uniqueVolumn[k];
      }
    }
    // printf("%lld\n", unique1);
    // printf("%lld\n", tr1[0]->_result->requiredDataSize[0] +
    // tr1[0]->_result->requiredDataSize[1] +
    // tr1[0]->_result->requiredDataSize[2]);
  }
}
void GroupSearchEngine::outputTopResult(std::ofstream &ofile, int num) {
  int levelSize =
      _groupSearchResult[0]
          ->_multiLevelTransformSearchResult->_transformSearchResult.size();
  sortResult();
  num = std::min(num, int(_groupSearchResult.size()));
  ofile << "{\n";
  long long curDelay = 0;
  for (int i = 0; i < num; i++) {
    if (i != 0)
      ofile << ",\n";
    if (curDelay == _groupSearchResult[i]
                        ->_multiLevelTransformSearchResult
                        ->_transformSearchResult[levelSize - 1]
                        ->_result->delay)
      continue;
    curDelay = _groupSearchResult[i]
                   ->_multiLevelTransformSearchResult
                   ->_transformSearchResult[levelSize - 1]
                   ->_result->delay;
    _groupSearchResult[i]->outputLog(ofile, i, _LVec.size());
  }
  ofile << "}";
}
} // namespace DSE
