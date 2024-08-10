#pragma once
#include "include/datastruct/arch.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <vector>
namespace COST {
double linearInterpolation(double x, std::vector<long long> &xData,
                           std::vector<double> &yData);

class MEMDATA {
private:
  friend double linearInterpolation(double x, std::vector<double> &xData,
                                    std::vector<double> &yData);
  std::vector<long long> _capacityVec;
  std::vector<double> _readEnergyVec;   // pj
  std::vector<double> _writeEnergyVec;  // pj
  std::vector<double> _areaVec;         // um2
  std::vector<double> _leakagePowerVec; // mw

public:
  MEMDATA() {}
  void addItem(int capacity, double readEnergy, double writeEnergy, double area,
               double leakagePower) {
    assert(_capacityVec.empty() ||
           _capacityVec[_capacityVec.size() - 1] < capacity);
    _capacityVec.push_back(capacity);
    _readEnergyVec.push_back(readEnergy);
    _writeEnergyVec.push_back(writeEnergy);
    _areaVec.push_back(area);
    _leakagePowerVec.push_back(leakagePower);
  }

  // flag 0 readEnergy
  // flag 1 writeEnergy
  // flag 2 area
  // flag 3 leakagePower
  double lookup(long long capacity, int flag) {
    std::vector<double> *p;
    if (flag == 0) {
      p = &_readEnergyVec;
    } else if (flag == 1) {
      p = &_writeEnergyVec;
    } else if (flag == 2) {
      p = &_areaVec;
    } else {
      p = &_leakagePowerVec;
    }
    return linearInterpolation(capacity, _capacityVec, *p);
  }
};

class MACNETWORKDATA {
private:
  friend double linearInterpolation(double x, std::vector<long long> &xData,
                                    std::vector<double> &yData);
  std::vector<long long> _linkNumVec;
  std::vector<double> _transEnergyVec;  // pj
  std::vector<double> _areaVec;         // um^2
  std::vector<double> _leakagePowerVec; // mw
public:
  MACNETWORKDATA() {}
  void addItem(int linkNum, double transEnergy, double area,
               double leakagePower) {
    assert(_linkNumVec.empty() ||
           _linkNumVec[_linkNumVec.size() - 1] < linkNum);
    _linkNumVec.push_back(linkNum);
    _transEnergyVec.push_back(transEnergy);
    _areaVec.push_back(area);
    _leakagePowerVec.push_back(leakagePower);
  }
  // flag 0 transEnergy
  // flag 1 area
  // flag 2 leakagePower
  double lookup(long long linkNum, int flag) {

    std::vector<double> *p;
    if (flag == 0) {
      p = &_transEnergyVec;
    } else if (flag == 1) {
      p = &_areaVec;
    } else {
      p = &_leakagePowerVec;
    }
    if (_linkNumVec.size() == 1) {
      return (*p)[0];
    } else {
      return linearInterpolation(linkNum, _linkNumVec, *p);
    }
  }
};

class MACNETWORKDATABUNDLE {
private:
  MACNETWORKDATA mac_network_8;
  MACNETWORKDATA mac_network_16;
  MACNETWORKDATA mac_network_32;

public:
  MACNETWORKDATABUNDLE() {}
  void addItem(int datawidth, int linkNum, double transEnergy, double area,
               double leakagePower) {
    if (datawidth == 8) {
      mac_network_8.addItem(linkNum, transEnergy, area, leakagePower);
    } else if (datawidth == 16) {
      mac_network_16.addItem(linkNum, transEnergy, area, leakagePower);
    } else {
      mac_network_32.addItem(linkNum, transEnergy, area, leakagePower);
    }
  }
  double lookup(int dataWidth, int linkNum, int flag) {
    if (dataWidth == 8) {
      return mac_network_8.lookup(linkNum, flag);
    } else if (dataWidth == 16) {
      return mac_network_16.lookup(linkNum, flag);
    } else {
      return mac_network_32.lookup(linkNum, flag);
    }
  }
};

struct COSTDADA {
  MEMDATA _sramData;
  MEMDATA _regData;
  MACNETWORKDATABUNDLE _systolicInput;
  MACNETWORKDATABUNDLE _systolicOutput;
  MACNETWORKDATABUNDLE _multicastOutput;
  MACNETWORKDATABUNDLE _multicastInput;
  MACNETWORKDATABUNDLE _unicastInput;
  MACNETWORKDATABUNDLE _unicastOutput;
  MACNETWORKDATABUNDLE _mac;
  COSTDADA();
};

}; // namespace COST