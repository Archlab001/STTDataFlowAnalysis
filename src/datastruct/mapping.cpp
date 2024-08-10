#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"

namespace MAPPING {
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec) {
  tensor.bindVar(coupledVarVec);
  const int varNum = coupledVarVec.size();
  std::shared_ptr<std::vector<int>> tmp;
  const int dimNum = tensor.getDimensionNum();
  tmp = std::make_shared<std::vector<int>>();
  for (int dimIndex = 0; dimIndex < dimNum; dimIndex++) {
    if (tensor.checkDimCoupled(dimIndex)) {
      for (int varIndex = 0; varIndex < varNum; varIndex++) {
        tmp->push_back(tensor.lookupVar(coupledVarVec[varIndex], dimIndex));
      }
    }
  }
  return Access(varNum, tmp);
}
} // namespace MAPPING
