#include "include/datastruct/workload.h"

namespace WORKLOAD {
std::shared_ptr<Monomial> operator*(std::shared_ptr<Iterator> var, int coef) {
  return std::make_shared<Monomial>(var, coef);
}
std::shared_ptr<Monomial> operator*(int coef, std::shared_ptr<Iterator> var) {
  return std::make_shared<Monomial>(var, coef);
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Monomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Iterator> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Iterator> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Monomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>(var1);
  *ret + var2;
  return ret;
}

std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Iterator> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>();
  *ret + var1;
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Monomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>();
  *ret + var1;
  *ret + var2;
  return ret;
}
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Polynomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>();
  *ret + var1;
  *ret + var2;
  return ret;
}

std::shared_ptr<Polynomial> operator*(int var1,
                                      std::shared_ptr<Polynomial> var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>();
  *ret + var2;
  *ret *var1;
  return ret;
}

std::shared_ptr<Polynomial> operator*(std::shared_ptr<Polynomial> var1,
                                      int var2) {
  std::shared_ptr<Polynomial> ret = std::make_shared<Polynomial>();
  *ret + var1;
  *ret *var2;
  return ret;
}

void generateEdgeState(
    std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &curSubCoupledVarVec) {
  for (auto var : curSubCoupledVarVec) {
    int len = state.size();
    if (var->hasEdge() && !var->islock()) {
      if (len == 0) {
        state.push_back({0});
        state.push_back({1});
      } else {
        for (int i = 0; i < len; i++) {
          state.push_back(state[i]);
        }
        for (int i = 0; i < len; i++) {
          state[i].push_back(1);
        }
        for (int i = len; i < 2 * len; i++) {
          state[i].push_back(0);
        }
      }
    } else {
      if (len == 0) {
        state.push_back({0});
      } else {
        for (int i = 0; i < len; i++) {
          state[i].push_back(0);
        }
      }
    }
  }
}

} // namespace WORKLOAD