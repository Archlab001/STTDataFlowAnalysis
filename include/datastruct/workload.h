#pragma once

#include "include/util/debug.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
namespace WORKLOAD {
class Iterator {
private:
  int _lowBound;
  int _upBound;
  std::string _sym;
  bool _lock;
  bool _hasEdge;
  bool _edgeFlag;
  bool _isEdgeChild;
  int _edgeLowBound;
  int _edgeUpBound;
  int _cur;
  std::shared_ptr<Iterator> _coupledIterator;

public:
  Iterator() = default;
  Iterator(int lowBound, int upBound, std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(false),
        _hasEdge(false), _edgeFlag(false), _edgeLowBound(0), _edgeUpBound(0),
        _cur(lowBound), _isEdgeChild(false) {}
  Iterator(int range, std::string sym) : Iterator(0, range - 1, sym) {}

  Iterator(int lowBound, int upBound, std::shared_ptr<Iterator> coupledIterator,
           std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(false),
        _hasEdge(true), _edgeFlag(false), _isEdgeChild(false),
        _coupledIterator(coupledIterator), _cur(lowBound), _edgeLowBound(0),
        _edgeUpBound(0) {
    DEBUG::check(!_isEdgeChild, DEBUG::ITERATOREDGEERROR, to_string());
    _coupledIterator->setIsEdgeChild();
  }

  Iterator(int lowBound, int upBound, int edgeLowBound, int edgeUpBound,
           std::string sym)
      : _lowBound(lowBound), _upBound(upBound), _sym(sym), _lock(false),
        _hasEdge(false), _edgeFlag(false), _isEdgeChild(false), _cur(lowBound),
        _edgeLowBound(edgeLowBound), _edgeUpBound(edgeUpBound) {}

  void setIsEdgeChild() { _isEdgeChild = true; }
  std::string to_string() {
    std::string ret;
    ret += "var: " + _sym + " low " + std::to_string(_lowBound) + " up " +
           std::to_string(_upBound);
    if (_isEdgeChild) {
      ret += " edgeLow " + std::to_string(_edgeLowBound) + " edgeUp " +
             std::to_string(_edgeUpBound);
    }
    return ret;
  }

  void setBound(int lowBound, int upBound) {
    _lowBound = lowBound;
    _upBound = upBound;
  }
  void setBound(int range) {
    _lowBound = 0;
    _upBound = range - 1;
  }

  std::string &getSym() { return _sym; }
  int getLowBound() {
    return _lock ? 0 : (_edgeFlag ? _upBound + 1 : _lowBound);
  }
  int getUpBound() { return _lock ? 0 : (_edgeFlag ? _upBound + 1 : _upBound); }
  void lock() { _lock = true; }
  void unlock() { _lock = false; }
  bool islock() { return _lock; }
  int getSize() {
    return _lock ? 0 : (_edgeFlag ? 1 : (_upBound - _lowBound + 1));
  }
  void edgeSwap() {
    std::swap(_edgeLowBound, _lowBound);
    std::swap(_edgeUpBound, _upBound);
  }
  void setEdge() {
    if (!_edgeFlag) {
      _edgeFlag = true;
      _coupledIterator->edgeSwap();
    }
  }
  void unsetEdge() {
    if (_edgeFlag) {
      _edgeFlag = false;
      _coupledIterator->edgeSwap();
    }
  }
  bool isEdge() { return _edgeFlag; }
  std::shared_ptr<Iterator> getCoupledIterator() { return _coupledIterator; }
  bool hasEdge() { return _hasEdge; }
  void reset() { _cur = 0; }
  void getNext() {
    if (isTop()) {
      _cur = _lowBound;
      if (_hasEdge) {
        unsetEdge();
      }
    } else {
      _cur++;
      if (_hasEdge && isTop()) {
        setEdge();
      }
    }
  }
  bool isTop() {
    if (_hasEdge)
      return _cur == _upBound + 1;
    else
      return _cur == _upBound;
  }
  int getCur() { return _cur; }

}; // end of Iterator

class Monomial {
private:
  std::shared_ptr<Iterator> _var;
  int _coef;

public:
  Monomial() = default;
  Monomial(std::shared_ptr<Iterator> var) : _var(var), _coef(1) {}
  Monomial(std::shared_ptr<Iterator> var, int coef) : _var(var), _coef(coef) {}
  std::string to_string() {
    std::string ret;
    ret += std::to_string(_coef) + " * " + _var->getSym();
    return ret;
  }
  std::pair<int, int> getRange() {
    std::pair<int, int> ret;
    if (_coef > 0) {
      ret.first = _coef * _var->getLowBound();
      ret.second = _coef * _var->getUpBound();
    } else {
      ret.second = _coef * _var->getLowBound();
      ret.first = _coef * _var->getUpBound();
    }
    return ret;
  }
  std::shared_ptr<Iterator> getVar() { return _var; }

  int getCoef() { return _coef; }
  int getCur() { return _coef * _var->getCur(); }
  void setCoef(int coef) { _coef = coef; }
}; // end of Monomial

std::shared_ptr<Monomial> operator*(std::shared_ptr<Iterator> var, int coef);
std::shared_ptr<Monomial> operator*(int coef, std::shared_ptr<Iterator> var);

class Polynomial {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Monomial>>> _monomialSet;

public:
  Polynomial() {
    _monomialSet = std::make_shared<std::vector<std::shared_ptr<Monomial>>>();
  }
  Polynomial(std::shared_ptr<Monomial> m) : Polynomial() {

    _monomialSet->push_back(m);
  }
  Polynomial(std::shared_ptr<Iterator> i) : Polynomial() {

    _monomialSet->push_back(std::make_shared<Monomial>(i));
  }

  Polynomial &operator=(Polynomial &other) {
    for (auto &m : *(other._monomialSet))
      _monomialSet->push_back(m);
    return *this;
  }
  Polynomial &operator+(std::shared_ptr<Polynomial> other) {
    for (auto item : *(other->_monomialSet)) {
      _monomialSet->push_back(item);
    }
    return *this;
  }
  Polynomial &operator+(std::shared_ptr<Monomial> other) {
    _monomialSet->push_back(other);
    return *this;
  }
  Polynomial &operator+(std::shared_ptr<Iterator> other) {
    _monomialSet->push_back(std::make_shared<Monomial>(other));
    return *this;
  }
  Polynomial &operator*(int var) {
    for (auto m : *_monomialSet) {
      m->setCoef(var * m->getCoef());
    }
    return *this;
  }
  std::string to_string() {
    std::string ret;
    const int len = _monomialSet->size();
    for (int i = 0; i < len; i++) {
      ret += (*_monomialSet)[i]->to_string();
      if (i != len - 1)
        ret += " + ";
    }
    return ret;
  }
  std::pair<int, int> getRange() {
    std::pair<int, int> ret = {0, 0}, tmp;

    for (auto m : *_monomialSet) {
      tmp = m->getRange();
      ret.first += tmp.first;
      ret.second += tmp.second;
    }
    return ret;
  }
  int getSize() {
    std::pair<int, int> tmp = getRange();
    return tmp.second - tmp.first + 1;
  }

  int lookupVar(std::shared_ptr<Iterator> i) {
    for (auto m : *_monomialSet) {
      if (m->getVar() == i)
        return m->getCoef();
    }
    return 0;
  }
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> getVarVecForVolumn() {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> ret;
    for (auto &m : *_monomialSet)
      ret.push_back(m->getVar());
    return ret;
  }

  std::vector<std::shared_ptr<WORKLOAD::Iterator>> getVarVec() {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> ret;
    for (auto m : *_monomialSet) {
      auto var = m->getVar();
      if (!var->islock()) {
        ret.push_back(m->getVar());
      }
    }
    return ret;
  }
  int getCur() {
    int ret = 0;
    for (auto m : *_monomialSet) {
      ret += m->getCur();
    }
    return ret;
  }
  void splitIterator(std::shared_ptr<WORKLOAD::Iterator> oriIterator,
                     std::shared_ptr<WORKLOAD::Iterator> outer,
                     std::shared_ptr<WORKLOAD::Iterator> inner, int tileSize) {
    bool flag = false;
    int coef = 1;
    std::shared_ptr<std::vector<std::shared_ptr<Monomial>>> _newMonomialSet =
        std::make_shared<std::vector<std::shared_ptr<Monomial>>>();
    for (auto &m : (*_monomialSet)) {
      if (m->getVar() != oriIterator)
        _newMonomialSet->push_back(m);
      else {
        flag = true;
        coef = m->getCoef();
      }
    }
    if (flag) {
      std::shared_ptr<Monomial> innerM =
          std::make_shared<Monomial>(inner, coef);
      std::shared_ptr<Monomial> outerM =
          std::make_shared<Monomial>(outer, coef * tileSize);
      _newMonomialSet->push_back(innerM);
      _newMonomialSet->push_back(outerM);
      _monomialSet = _newMonomialSet;
    }
  }
}; // end of Polynomial
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Monomial> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Monomial> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Iterator> var1,
                                      std::shared_ptr<Monomial> var2);

std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Iterator> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Monomial> var2);
std::shared_ptr<Polynomial> operator+(std::shared_ptr<Polynomial> var1,
                                      std::shared_ptr<Polynomial> var2);
std::shared_ptr<Polynomial> operator*(int var1,
                                      std::shared_ptr<Polynomial> var2);
std::shared_ptr<Polynomial> operator*(std::shared_ptr<Polynomial> var1,
                                      int var2);

void generateEdgeState(
    std::vector<std::vector<int>> &state,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &curSubCoupledVarVec);
class Tensor {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Polynomial>>> _dimensionTable;
  std::string _sym;
  std::shared_ptr<std::vector<int>> _coupled;
  std::set<std::shared_ptr<WORKLOAD::Iterator>> _varSet;
  int compOneStateVolumn() {
    int ret = 1;
    for (auto dim : *_dimensionTable) {
      auto range = dim->getRange();
      ret *= (range.second - range.first + 1);
    }
    return ret;
  }

public:
  Tensor() {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  Tensor &operator=(const Tensor &arr) {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
    _sym = arr._sym;
    _coupled = arr._coupled;
    _varSet = arr._varSet;
    for (auto &oriP : *(arr._dimensionTable)) {
      std::shared_ptr<Polynomial> p = std::make_shared<Polynomial>();
      *p = *oriP;
      _dimensionTable->push_back(p);
    }
    return *this;
  }
  Tensor(std::string sym) : _sym(sym) {
    _dimensionTable =
        std::make_shared<std::vector<std::shared_ptr<Polynomial>>>();
  }
  int getDimensionNum() { return _dimensionTable->size(); }
  int lookupVar(std::shared_ptr<Iterator> i, int dim) {
    return (*_dimensionTable)[dim]->lookupVar(i);
  }
  Tensor &operator[](std::shared_ptr<Polynomial> p) {
    _dimensionTable->push_back(p);
    constructVarSet((*_dimensionTable)[_dimensionTable->size() - 1]);
    return *this;
  }
  Tensor &operator[](std::shared_ptr<Iterator> i) {
    _dimensionTable->push_back(std::make_shared<Polynomial>(i));
    constructVarSet((*_dimensionTable)[_dimensionTable->size() - 1]);
    return *this;
  }
  Tensor &operator[](std::shared_ptr<Monomial> m) {
    _dimensionTable->push_back(std::make_shared<Polynomial>(m));
    constructVarSet((*_dimensionTable)[_dimensionTable->size() - 1]);
    return *this;
  }

  void splitIterator(std::shared_ptr<WORKLOAD::Iterator> oriIterator,
                     std::shared_ptr<WORKLOAD::Iterator> outer,
                     std::shared_ptr<WORKLOAD::Iterator> inner, int tileSize) {
    for (auto &dim : *_dimensionTable) {
      dim->splitIterator(oriIterator, outer, inner, tileSize);
    }
  }

  void constructVarSet(std::shared_ptr<Polynomial> dim) {
    auto oneDimVarVec = dim->getVarVec();
    for (auto var : oneDimVarVec) {
      if (!_varSet.count(var)) {
        _varSet.insert(var);
      }
    }
  }

  std::string to_string() {
    std::string ret;
    ret += _sym;
    for (auto item : *_dimensionTable) {
      ret += "[" + item->to_string() + "]";
    }
    return ret;
  }
  std::pair<int, int> getRange(int dimIndex) {
    return (*_dimensionTable)[dimIndex]->getRange();
  }
  void
  bindVar(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec) {
    const int dimNum = getDimensionNum();
    const int varNum = coupledVarVec.size();
    _coupled = std::make_shared<std::vector<int>>(dimNum, 0);
    for (int dimindex = 0; dimindex < dimNum; dimindex++) {
      for (int varIndex = 0; varIndex < varNum; varIndex++) {
        if (lookupVar(coupledVarVec[varIndex], dimindex)) {
          (*_coupled)[dimindex] = 1;
        }
      }
    }
  }
  int getDimNum() { return _dimensionTable->size(); }
  int getCoupledDimNum() {
    int ret = 0;
    int dimNum = _dimensionTable->size();
    for (int i = 0; i < dimNum; i++) {
      if (checkDimCoupled(i)) {
        ret++;
      }
    }
    return ret;
  }
  bool checkDimCoupled(int dimIndex) { return (*_coupled)[dimIndex]; }
  std::vector<int> getCur() {
    std::vector<int> ret;
    int dimNum = _dimensionTable->size();
    for (int i = 0; i < dimNum; i++) {
      if (checkDimCoupled(i)) {
        ret.push_back((*_dimensionTable)[i]->getCur());
      }
    }
    return ret;
  }

  int getVolumn() {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec;
    for (auto it = _varSet.begin(); it != _varSet.end(); it++) {
      varVec.push_back(*it);
    }
    std::vector<std::vector<int>> state;
    generateEdgeState(state, varVec);
    int stateNum = state.size();
    int varNum = varVec.size();
    int ret = 0;
    if (stateNum == 0) {
      return 1;
    } else {
      for (int i = 0; i < stateNum; i++) {
        for (int j = 0; j < varNum; j++) {
          if (state[i][j]) {
            varVec[j]->setEdge();
          }
        }

        ret += compOneStateVolumn();

        for (int j = 0; j < varNum; j++) {
          if (state[i][j]) {
            varVec[j]->unsetEdge();
          }
        }
      }
    }
    return ret;
  }
  int getOneDimRange(std::shared_ptr<Polynomial> dim) {
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> varVec =
        dim->getVarVecForVolumn();
    std::vector<std::vector<int>> state;
    generateEdgeState(state, varVec);
    int stateNum = state.size();
    int varNum = varVec.size();
    int ret = 0;
    if (stateNum == 0) {
      return 1;
    } else {
      for (int i = 0; i < stateNum; i++) {
        for (int j = 0; j < varNum; j++) {
          if (state[i][j]) {
            varVec[j]->setEdge();
          }
        }
        auto range = dim->getRange();
        ret += (range.second - range.first + 1);
        for (int j = 0; j < varNum; j++) {
          if (state[i][j]) {
            varVec[j]->unsetEdge();
          }
        }
      }
    }
    return ret;
  }

  std::vector<long long> getEveryDimRange() {
    std::vector<long long> ret;
    for (auto dim : *_dimensionTable)
      ret.push_back(getOneDimRange(dim));
    return ret;
  }
  int getCoupledDimIndex(std::shared_ptr<Iterator> curIterator) {
    int dimNum = _dimensionTable->size();
    for (int i = 0; i < dimNum; i++) {
      auto dim = (*_dimensionTable)[i];
      if (dim->lookupVar(curIterator)) {
        return i;
      }
    }
    return -1;
  }

  int getCoupledDimCoef(std::shared_ptr<Iterator> curIterator, int index) {
    if (index == -1)
      return 0;
    return (*_dimensionTable)[index]->lookupVar(curIterator);
  }

}; // end of Tensor

} // namespace WORKLOAD