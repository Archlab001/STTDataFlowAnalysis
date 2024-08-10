#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/searchEngine/groupSearchEngine.h"
namespace DSE {


struct TileCandidateCombine {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
  std::vector<int> sizeVec;
};

// TileSearchEngine is used to explore all partitioning schemes and invoke the groupSearchEngine
class TileSearchEngine {
private:
  WORKLOAD::Tensor _oriI;
  WORKLOAD::Tensor _oriW;
  WORKLOAD::Tensor _oriO;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _oriCoupledVarVec;

  WORKLOAD::Tensor _I;
  WORKLOAD::Tensor _W;
  WORKLOAD::Tensor _O;
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> _coupledVarVec;

  std::map<std::shared_ptr<WORKLOAD::Iterator>, std::vector<int>>
      _allIteratorCandidate;
  std::vector<ARCH::Level> _LVec;
  std::vector<std::shared_ptr<GroupSearchResult>> _groupSearchResult;

public:
  TileSearchEngine(WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
                   WORKLOAD::Tensor &O,
                   std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec)
      : _oriI(I), _oriW(W), _oriO(O), _oriCoupledVarVec(varVec) {
    reset();
  }

  // add candidate to iterator
  void addCancidate(std::shared_ptr<WORKLOAD::Iterator> iterator,
                    int candidate) {
    if (_allIteratorCandidate.count(iterator) == 0) {
      _allIteratorCandidate[iterator] = std::vector<int>();
    }
    _allIteratorCandidate[iterator].push_back(candidate);
  }

  void reset() {
    _coupledVarVec = _oriCoupledVarVec;
    _O = _oriO;
    _I = _oriI;
    _W = _oriW;
  }

  // split the iterators
  void split(std::shared_ptr<WORKLOAD::Iterator> iterator, int size) {
    int lowerBound = iterator->getLowBound();
    int upperBound = iterator->getUpBound();
    int iteratorRange = upperBound - lowerBound + 1;
    if (size == iteratorRange)
      return;
    int quotient = iteratorRange / size;
    int res = iteratorRange - size * quotient;
    std::shared_ptr<WORKLOAD::Iterator> outer;
    std::shared_ptr<WORKLOAD::Iterator> inner;
    if (res == 0) {
      outer = std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, quotient - 1, iterator->getSym() + "_o"));
      inner = std::make_shared<WORKLOAD::Iterator>(
          WORKLOAD::Iterator(0, size - 1, iterator->getSym() + "_i"));
    } else {
      inner = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
          0, size - 1, 0, res - 1, iterator->getSym() + "_i"));
      outer = std::make_shared<WORKLOAD::Iterator>(WORKLOAD::Iterator(
          0, quotient - 1, inner, iterator->getSym() + "_o"));
    }
    int coupledVarNum = _coupledVarVec.size();
    for (int i = 0; i < coupledVarNum; i++) {
      if (_coupledVarVec[i] == iterator) {
        _coupledVarVec[i] = inner;
        break;
      }
    }
    _coupledVarVec.push_back(outer);
    // change Tensor
    _I.splitIterator(iterator, outer, inner, size);
    _W.splitIterator(iterator, outer, inner, size);
    _O.splitIterator(iterator, outer, inner, size);
  }

  // retrieve all combinations of partitioning schemes
  void combine(std::vector<TileCandidateCombine> &tileCandidateCombineVec,
               TileCandidateCombine &curCandidateCombine,
               std::vector<std::shared_ptr<WORKLOAD::Iterator>> &varVec,
               int index) {
    if (index == varVec.size()) {
      tileCandidateCombineVec.push_back(curCandidateCombine);
    } else {
      auto candidateVec = _allIteratorCandidate[varVec[index]];
      for (auto candidata : candidateVec) {
        curCandidateCombine.varVec.push_back(varVec[index]);
        curCandidateCombine.sizeVec.push_back(candidata);
        combine(tileCandidateCombineVec, curCandidateCombine, varVec,
                index + 1);
        curCandidateCombine.varVec.pop_back();
        curCandidateCombine.sizeVec.pop_back();
      }
    }
  }

  // entry of search
  void oneSearch(std::ofstream &logFile, bool logFlag) {
    std::vector<TileCandidateCombine> tileCandidateCombineVec;
    TileCandidateCombine curCandidateCombine;
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
    for (auto &item : _allIteratorCandidate) {
      varVec.push_back(item.first);
    }

    combine(tileCandidateCombineVec, curCandidateCombine, varVec, 0);
    int num = varVec.size();
    for (auto tileCandidateCombine : tileCandidateCombineVec) {
      // for (int i = 0; i < num; i++)
      //{
      //    std::cout << tileCandidateCombine.varVec[i]->getSym() << ' ' <<
      //    tileCandidateCombine.sizeVec[i] << ' ';
      //}
      // std::cout << std::endl;
      reset();
      for (int i = 0; i < num; i++) {
        split(tileCandidateCombine.varVec[i], tileCandidateCombine.sizeVec[i]);
      }

      DSE::GroupSearchEngine groupSearchEngine(_I, _W, _O, _coupledVarVec);
      for (auto &L : _LVec) {
        groupSearchEngine.addLevel(L);
      }
      groupSearchEngine.oneSearch(logFile, logFlag);
      for (auto result : groupSearchEngine._groupSearchResult) {
        // if
        // (result->_multiLevelTransformSearchResult->_transformSearchResult[2]
        //         ->_result->delay > 10000000)
        //   continue;
        _groupSearchResult.push_back(result);
      }
      std::cout << groupSearchEngine._groupSearchResult.size() << std::endl;
      std::cout << _groupSearchResult.size() << std::endl;
      std::cout << _I.to_string() << ' ';
      std::cout << _W.to_string() << ' ';
      std::cout << _O.to_string() << ' ';
      std::cout << std::endl;
    }
  }
  void oneSearch() {
    std::ofstream logFile;
    oneSearch(logFile, false);
  }

  // add accelerator level
  void addLevel(ARCH::Level &L) { _LVec.emplace_back(L); }
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
      return false;
    } else
      return false;
  }
  static bool cmpResultByScore(std::shared_ptr<GroupSearchResult> &r1,
                               std::shared_ptr<GroupSearchResult> &r2) {
    if (r1->score < r2->score)
      return true;
    else
      return false;
  }
  // compute score for each dataflow
  void cmpScore(Target &target) {
    for (auto &r : _groupSearchResult) {
      std::vector<std::shared_ptr<TransformSearchResult>> &tr =
          r->_multiLevelTransformSearchResult->_transformSearchResult;
      r->score = 0;
      int levelNum = target._t.size();
      for (int i = 0; i < levelNum; i++) {
        r->score += target._t[i][0] * tr[i]->_result->uniqueVolumn[0];
        r->score += target._t[i][1] * tr[i]->_result->uniqueVolumn[1];
        r->score += target._t[i][2] * tr[i]->_result->uniqueVolumn[2];
        r->score += target._t[i][3] * tr[i]->_result->requiredDataSize[0];
        r->score += target._t[i][4] * tr[i]->_result->requiredDataSize[1];
        r->score += target._t[i][5] * tr[i]->_result->requiredDataSize[2];
        r->score += target._t[i][6] * tr[i]->_result->requiredBandWidth[0];
        r->score += target._t[i][7] * tr[i]->_result->requiredBandWidth[1];
        r->score += target._t[i][8] * tr[i]->_result->requiredBandWidth[2];
        r->score += target._t[i][9] * tr[i]->_result->delay;
        r->score +=
            target._t[i][10] * (tr[i]->_result->accumulateEnergy / 1000000000 +
                                tr[i]->_result->accumulateLeakagePower *
                                    tr[i]->_result->delay / 200000000);
        r->score += target._t[i][11] * tr[i]->_result->accumulateArea / 1000000;
      }
    }
  }
  void sortResult(Target &target) {
    cmpScore(target);
    std::sort(_groupSearchResult.begin(), _groupSearchResult.end(),
              cmpResultByScore);
    int levelSize =
        _groupSearchResult[0]
            ->_multiLevelTransformSearchResult->_transformSearchResult.size();
    long long curDelay = 0;
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      if (curDelay == _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay)
        continue;
      curDelay = _groupSearchResult[i]
                     ->_multiLevelTransformSearchResult
                     ->_transformSearchResult[levelSize - 1]
                     ->_result->delay;
      printf("%lld ", _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay);
    }
  }

  void outputTopResult(std::ofstream &ofile, std::ofstream &ofile2,
                       Target &target, int num = 5) {
    int levelSize =
        _groupSearchResult[0]
            ->_multiLevelTransformSearchResult->_transformSearchResult.size();
    sortResult(target);
    std::cout << _groupSearchResult.size() << std::endl;
    num = std::min(num, int(_groupSearchResult.size()));
    ofile << "{\n";
    long long curDelay = 0;
    int count = 0;
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      if (curDelay == _groupSearchResult[i]
                          ->_multiLevelTransformSearchResult
                          ->_transformSearchResult[levelSize - 1]
                          ->_result->delay)
        continue;
      if (i != 0)
        ofile << ",\n";
      curDelay = _groupSearchResult[i]
                     ->_multiLevelTransformSearchResult
                     ->_transformSearchResult[levelSize - 1]
                     ->_result->delay;
      _groupSearchResult[i]->outputLog(ofile, i, _LVec.size());
      count++;
      if (count == num)
        break;
    }
    ofile << "}";
    ofile2 << "{\n";
    for (int i = 0; i < _groupSearchResult.size(); i++) {
      if (i != 0)
        ofile2 << ",\n";
      _groupSearchResult[i]->outputLog(ofile2, i, _LVec.size());
    }
    ofile2 << "}";
  }
  void outputTopResult(int taskIndex, int accIndex, Target &target,
                       int num = 5) {
    std::string name = std::string("./result_task_") +
                       std::to_string(taskIndex) + std::string("_acc_") +
                       std::to_string(accIndex) + ".json";
    std::ofstream ofile;
    ofile.open(name, std::ios::out);
    std::ofstream ofile2;
    ofile2.open("groupresult.json", std::ios::out);
    outputTopResult(ofile, ofile2, target, num);
    ofile.close();
    ofile2.close();
  }
  void outputTopResult(std::ofstream &ofile, std::ofstream &ofile2,
                       int num = 5) {
    int levelNum = _LVec.size();
    Target target(levelNum);
    target.addTarget(levelNum - 1, 9, 1);
    outputTopResult(ofile, ofile2, target, 5);
  }
  double getTopScore() { return _groupSearchResult[0]->score; }
};
} // namespace DSE