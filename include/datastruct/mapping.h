#pragma once
#include "include/datastruct/workload.h"
#include "include/util/debug.h"
#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
namespace MAPPING {
typedef int mappingValueType;
class Matrix2D {
protected:
  std::shared_ptr<std::vector<mappingValueType>> _value;
  int _colNum;

public:
  Matrix2D(int colNum, std::shared_ptr<std::vector<mappingValueType>> value)
      : _value(value), _colNum(colNum) {}
  Matrix2D(int colNum)
      : _value(std::make_shared<std::vector<mappingValueType>>(
            std::vector<mappingValueType>(colNum * colNum, 0))),
        _colNum(colNum) {}
  bool check() {
    if (_value->size() % _colNum != 0)
      return 0;
    else
      return 1;
  }
  mappingValueType operator()(int i, int j) {
    return (*_value)[i * _colNum + j];
  }

  int getColNum() { return _colNum; }
  int getRowNum() { return _value->size() / _colNum; }
  std::shared_ptr<std::vector<mappingValueType>> getMatrix() { return _value; }
  void setValue(int i, int j, int num) { (*_value)[i * _colNum + j] = num; }
  void print() {
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        std::cout << (*this)(i, j) << ' ';
      }
      std::cout << std::endl;
    }
  }
  std::string to_string() {
    std::string ret;
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        ret += std::to_string((*this)(i, j)) + ' ';
      }
      ret += "    \n";
    }
    return ret;
  }
  void outputT(std::ofstream &logFile) {
    logFile << "{";
    for (int i = 0; i < _colNum; i++) {
      if (i != 0)
        logFile << ',';
      logFile << "\"" << std::to_string(i) << "\":\"";
      for (int j = 0; j < _colNum; j++) {
        logFile << std::to_string((*this)(i, j)) + ' ';
      }
      logFile << "\"\n";
    }
    logFile << "}";
  }
};
class Transform : public Matrix2D {
private:
public:
  Transform(int dimNum,
            std::shared_ptr<std::vector<mappingValueType>> transformMatrix)
      : Matrix2D(dimNum, transformMatrix) {}
  Transform(int dimNum) : Matrix2D(dimNum) {}

  Transform &deepCopy(Transform &other) {
    _colNum = other._colNum;
    _value = std::make_shared<std::vector<mappingValueType>>(
        std::vector<mappingValueType>(_colNum * _colNum, 0));
    *_value = *other._value;
    return *this;
  }
  void addExtraSpatial() {
    _colNum++;
    int addElementNum = _colNum * _colNum - (_colNum - 1) * (_colNum - 1);
    for (int i = 0; i < addElementNum; i++) {
      _value->push_back(0);
    }
    for (int i = _colNum - 2; i >= 0; i--) {
      for (int j = _colNum - 2; j >= 0; j--) {

        (*_value)[(i + 1) * _colNum + 1 + j] = (*_value)[i * (_colNum - 1) + j];
      }
    }
    (*_value)[0] = 1;
    for (int i = 1; i < _colNum; i++) {
      (*_value)[i] = 0;
      (*_value)[i * _colNum] = 0;
    }
  }

  void addExtraTemporal() {
    _colNum++;
    int addElementNum = _colNum * _colNum - (_colNum - 1) * (_colNum - 1);
    for (int i = 0; i < addElementNum; i++) {
      _value->push_back(0);
    }

    for (int i = _colNum - 2; i >= 0; i--) {
      for (int j = _colNum - 2; j >= 0; j--) {

        (*_value)[i * _colNum + j] = (*_value)[i * (_colNum - 1) + j];
      }
    }
    (*_value)[_colNum * _colNum - 1] = 1;
    for (int i = 0; i < _colNum - 1; i++) {
      (*_value)[i * _colNum + _colNum - 1] = 0;
    }
  }

  bool check() {
    if (_value->size() != _colNum * _colNum)
      return 0;
    std::vector<std::set<int>> coupledVec(_colNum);
    for (int i = 0; i < _colNum; i++) {
      for (int j = 0; j < _colNum; j++) {
        if (operator()(i, j) != 1 && operator()(i, j) != 0)
          return false;
        if (operator()(i, j) == 1)
          coupledVec[i].insert(j);
      }
    }
    // check PE dim num
    if (coupledVec[0].size() != 1 || coupledVec[1].size() != 1)
      return false;
    int PEXindex = *coupledVec[0].begin();
    int PEYindex = *coupledVec[1].begin();
    if (PEXindex == PEYindex)
      return false;
    // check Time dim
    int count = 0;
    int index = -1;
    for (int i = 2; i < _colNum; i++) {
      if (coupledVec[i].size() == 0)
        return false;
      if (coupledVec[i].size() > 1) {
        count++;
        index = i;
      }
    }
    if (count > 1)
      return false;
    if (index != -1) {
      count = coupledVec[index].size();
      if (operator()(index, PEXindex) == 1)
        count--;
      if (operator()(index, PEYindex) == 1)
        count--;
      if (count != 1)
        return false;
    }
    // check column
    std::set<int> exist;
    for (int i = 2; i < _colNum; i++) {
      for (auto dim : coupledVec[i]) {
        if (exist.count(dim))
          return false;
        else
          exist.insert(dim);
      }
    }
    return true;
  }

}; // end of Transform
class Access : public Matrix2D {
private:
public:
  Access(int colNum,
         std::shared_ptr<std::vector<mappingValueType>> accessMatrix)
      : Matrix2D(colNum, accessMatrix) {}
  Access() : Matrix2D(0) {}
  bool isScalar() { return _value->size() == 0; }
}; // end of Access
MAPPING::Access constructAccessMatrix(
    WORKLOAD::Tensor &tensor,
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec);

} // namespace MAPPING