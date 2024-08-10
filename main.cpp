
#include "include/analysis/costAnalysis.h"
#include "include/analysis/multiLevelAnalysis.h"
#include "include/analysis/singleLevelAnalysis.h"
#include "include/datastruct/arch.h"
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include "include/searchEngine/groupSearchEngine.h"
#include "include/searchEngine/tileSearchEngine.h"
#include "include/searchEngine/transformSearchEngine.h"
#include "include/util/config.h"
#include "include/util/debug.h"
#include "include/util/eigenUtil.h"
#include "include/util/timeline.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <math.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

long long DSE::TransformSearchEngine::totalCount = 0;
long long DSE::GroupSearchEngine::totalCount = 0;
long long DSE::MultiLevelTransformSearchEngine::_resultCount = 0;
extern COST::COSTDADA _Cost;

void startSearch(TaskSet &taskset, AcceleratorSet &accSet, Target &target) {
  std::vector<DSE::TileSearchEngine> tileSearchEngineVec;
  std::vector<double> accScore(accSet.acceleratorVec.size(), 0);
  int taskIndex = 0;
  for (auto &task : taskset.taskVec) {
    int accIndex = 0;
    for (auto &acc : accSet.acceleratorVec) {
      tileSearchEngineVec.emplace_back(
          task._tensorMap[ARCH::INPUT], task._tensorMap[ARCH::WEIGHT],
          task._tensorMap[ARCH::OUTPUT], task._coupledVarVec);

      auto &tileSearchEngine =
          tileSearchEngineVec[tileSearchEngineVec.size() - 1];
      for (auto &p : task._allIteratorCandidate) {
        for (auto candidate : p.second) {
          tileSearchEngine.addCancidate(p.first, candidate);
        }
      }

      for (auto &p : acc._LVec) {
        tileSearchEngine.addLevel(p);
      }
      tileSearchEngine.oneSearch();
      tileSearchEngine.outputTopResult(taskIndex, accIndex, target, 5);
      accScore[accIndex] += tileSearchEngine.getTopScore() * task._ratio;
      accIndex++;
    }
    taskIndex++;
  }
  std::cout << std::endl;
  std::cout << "accScore:  ";
  for (auto score : accScore) {
    std::cout << score << "\t";
  }
}

int main() {
  TaskSet taskSet;
  AcceleratorSet accSet;
  defineTaskSet(taskSet);
  taskSet.check();
  defineAcceleratorSet(accSet);
  accSet.check();
  Target target(accSet);
  defineTarget(target);
  target.check();
  startSearch(taskSet, accSet, target);
  return 0;
}