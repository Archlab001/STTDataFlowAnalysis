#include "include/util/timeline.h"
namespace TIMELINE {
bool timeLineGreater(std::shared_ptr<TimeLine> &t1,
                     std::shared_ptr<TimeLine> &t2) {
  for (int i = t1->_time.size() - 1; i >= 0; i--) {
    if (t1->_time[i] > t2->_time[i])
      return false;
    else if (t1->_time[i] < t2->_time[i])
      return true;
  }
  if (t1->PEX > t2->PEX)
    return false;
  else if (t1->PEX < t2->PEX)
    return true;
  else {
    if (t1->PEY > t2->PEY)
      return false;
    else
      return true;
  }
}

void getTimeLine(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    MAPPING::Transform &T, WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
    WORKLOAD::Tensor &O) {
  int colNum = T.getColNum();
  std::shared_ptr<WORKLOAD::Iterator> PEX;
  std::shared_ptr<WORKLOAD::Iterator> PEY;
  I.bindVar(coupledVarVec);
  O.bindVar(coupledVarVec);
  W.bindVar(coupledVarVec);

  for (int i = 0; i < colNum; i++) {
    if (T(0, i) == 1)
      PEX = coupledVarVec[i];
    if (T(1, i) == 1)
      PEY = coupledVarVec[i];
  }
  for (auto var : coupledVarVec) {
    var->reset();
  }
  Generator generator(coupledVarVec, PEX, PEY, T, I, W, O);
  while (!generator.isEnd()) {
    generator.generateTimeLine();
    generator.getNext();
  }
  generator.generateTimeLine();
  generator.generatorSort();
}
} // namespace TIMELINE