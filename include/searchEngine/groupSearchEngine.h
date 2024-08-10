#pragma once
#include "include/searchEngine/transformSearchEngine.h"
namespace DSE {
// a tree grouping record, where each level of the tree corresponds to the allocation of iterator groups to each hardware level
class Group {
public:
  std::vector<int> _indexVec;
  std::vector<Group> _subGroupVec;
  Group() {}
  Group(std::vector<int> indexVec) : _indexVec(indexVec) {}
};
// groupSearchEngine is used to explore all grouping schemes and invoke the groupSearchEngine
class GroupSearchEngine {
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _varVec;
  std::vector<int> _spatialNumVec;
  std::vector<ARCH::Level> _LVec;
  bool _firstFlag;

public:
  std::vector<std::shared_ptr<GroupSearchResult>> _groupSearchResult;

  static long long totalCount;
  GroupSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                    WORKLOAD::Tensor &O,
                    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec)
      : _I(I), _W(W), _O(O), _varVec(varVec), _firstFlag(false) {}
  void addLevel(ARCH::Level &L) {
    _LVec.emplace_back(L);
    _spatialNumVec.push_back(L.getSpatialDimNum());
  }


  void combine(int n, int k, std::vector<Group> &groupVec);

  void constructGroup(std::vector<int> &perGroupNum, int levelIndex, int varNum,
                      Group &rootGroup, std::vector<int> &candidate);

  void buildCoupleVarVec(
      Group &rootGroup, int levelIndex,
      std::vector<std::vector<std::shared_ptr<WORKLOAD::Iterator>>>
          &coupledVarVecVec,
      std::ofstream &logFile, bool logFlag);

  void recusiveCompPerGroupNum(std::vector<int> &perGroupNum, int varNum,
                               int levelNum, std::ofstream &logFile,
                               bool logFlag);

  void oneSearch(std::ofstream &logFile, bool logFlag);

  static bool cmpResultByTotalCycle(std::shared_ptr<GroupSearchResult> &r1,
                                    std::shared_ptr<GroupSearchResult> &r2) {
    int levelSize =
        r1->_multiLevelTransformSearchResult->_transformSearchResult.size();
    std::vector<std::shared_ptr<TransformSearchResult>> &tr1 =
        r1->_multiLevelTransformSearchResult->_transformSearchResult;
    std::vector<std::shared_ptr<TransformSearchResult>> &tr2 =
        r2->_multiLevelTransformSearchResult->_transformSearchResult;
    if (tr1[levelSize - 1]->_result->delay < tr2[levelSize - 1]->_result->delay)
      return true;
    else if (tr1[levelSize - 1]->_result->delay ==
             tr2[levelSize - 1]->_result->delay) {
      long long unique1 = 0;
      long long unique2 = 0;
      for (int i = 0; i < levelSize; i++) {
        for (int j = 0; j < 3; j++) {
          unique1 += tr1[i]->_result->uniqueVolumn[j];
          unique2 += tr2[i]->_result->uniqueVolumn[j];
        }
      }
      if (unique1 < unique2)
        return true;
      else
        return false;
    } else
      return false;
  }
  void sortResult(int flag = 0);
  void outputTopResult(std::ofstream &ofile, int num = 100);
};
} // namespace DSE