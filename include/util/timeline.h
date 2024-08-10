#pragma once
#include "include/datastruct/mapping.h"
#include "include/datastruct/workload.h"
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace TIMELINE {

struct TimeLine {
  std::vector<int> _time;
  std::vector<int> _varCur;
  std::vector<int> _curInput;
  std::vector<int> _curWeight;
  std::vector<int> _curOutput;
  int PEX;
  int PEY;
};

void getTimeLine(
    std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
    MAPPING::Transform &T, WORKLOAD::Tensor &I, WORKLOAD::Tensor &W,
    WORKLOAD::Tensor &O);
bool timeLineGreater(std::shared_ptr<TimeLine> &t1,
                     std::shared_ptr<TimeLine> &t2);
class Generator {
  std::vector<std::shared_ptr<WORKLOAD::Iterator>> &_coupledVarVec;
  std::vector<std::shared_ptr<TimeLine>> _timeline;
  std::shared_ptr<WORKLOAD::Iterator> _PEX;
  std::shared_ptr<WORKLOAD::Iterator> _PEY;
  MAPPING::Transform &_T;
  WORKLOAD::Tensor &_I;
  WORKLOAD::Tensor &_W;
  WORKLOAD::Tensor &_O;
  int _count;

public:
  Generator(std::vector<std::shared_ptr<WORKLOAD::Iterator>> &coupledVarVec,
            std::shared_ptr<WORKLOAD::Iterator> PEX,
            std::shared_ptr<WORKLOAD::Iterator> PEY, MAPPING::Transform &T,
            WORKLOAD::Tensor &I, WORKLOAD::Tensor &W, WORKLOAD::Tensor &O)
      : _coupledVarVec(coupledVarVec), _PEX(PEX), _PEY(PEY), _T(T), _O(O),
        _W(W), _I(I), _count(0) {}

  void getNext() {
    int coupledVarVecNum = _coupledVarVec.size();
    for (int i = 0; i < coupledVarVecNum; i++) {
      if (_coupledVarVec[i]->isTop()) {
        _coupledVarVec[i]->getNext();
      } else {
        _coupledVarVec[i]->getNext();
        break;
      }
    }
  }
  bool isEnd() {
    for (auto var : _coupledVarVec) {
      if (!var->isTop()) {
        return false;
      }
    }
    return true;
  }
  void getCur() {
    for (auto var : _coupledVarVec) {
      std::cout << var->getCur() << ' ';
    }
    auto vec = _O.getCur();
    for (auto tmp : vec) {
      std::cout << tmp << ' ';
    }
    std::cout << std::endl;
  }
  void generateTimeLine() {
    std::shared_ptr<TimeLine> curTime = std::make_shared<TimeLine>();

    for (auto var : _coupledVarVec) {
      curTime->_varCur.push_back(var->getCur());
    }
    int coupledVarVecNum = _coupledVarVec.size();
    for (int i = 2; i < coupledVarVecNum; i++) {
      curTime->_time.push_back(getTCur(i));
    }
    curTime->_curInput = _I.getCur();
    curTime->_curOutput = _O.getCur();
    curTime->_curWeight = _W.getCur();
    curTime->PEX = _PEX->getCur();
    curTime->PEY = _PEY->getCur();
    _timeline.push_back(curTime);
  }
  bool isTimeEq(std::vector<int> &curTime, std::vector<int> &other) {
    static int timeLen = curTime.size();
    for (int i = 0; i < timeLen; i++) {
      if (curTime[i] != other[i])
        return false;
    }
    return true;
  }

  void displayLine(std::shared_ptr<TimeLine> item, std::ofstream &logFile) {
    logFile << "PEX " << item->PEX << ' ';
    logFile << "PEY " << item->PEY << ' ';
    logFile << "O";
    for (auto o : item->_curOutput) {
      logFile << "[" + std::to_string(o) + "]";
    }
    logFile << " += I";
    for (auto i : item->_curInput) {
      logFile << "[" + std::to_string(i) + "]";
    }
    logFile << " * W";
    for (auto w : item->_curWeight) {
      logFile << "[" + std::to_string(w) + "]";
    }
    logFile << ' ';
  }
  void displayLine(int pex, int pey, int dim1, int dim2, int dim3,
                   std::ofstream &ofile) {

    ofile << "PEX " << pex << ' ';
    ofile << "PEY " << pey << ' ';
    ofile << "O";
    for (int i = 0; i < dim1; i++) {
      ofile << "[  ]";
    }
    ofile << " += I";
    for (int i = 0; i < dim2; i++) {
      ofile << "[  ]";
    }
    ofile << " * W";
    for (int i = 0; i < dim3; i++) {
      ofile << "[  ]";
    }
    ofile << ' ';
  }

  void generatorSort() {
    std::ofstream outfile;
    outfile.open("timeline.txt", std::ios::out);
    int PEXMAX = _PEX->getSize();
    int PEYMAX = _PEY->getSize();
    std::sort(_timeline.begin(), _timeline.end(), timeLineGreater);
    std::string ret;
    std::vector<int> curTime = _timeline[0]->_time;
    curTime[0] = -1;
    int timeLineLen = _timeline.size();

    int weightCoupledDimNum = _W.getCoupledDimNum();
    int outputCoupledDimNum = _O.getCoupledDimNum();
    int inputCoupledDimNum = _I.getCoupledDimNum();

    for (int i = 0; i < timeLineLen;) {
      if (isTimeEq(curTime, _timeline[i]->_time)) {
        for (int j = 0; j < PEXMAX; j++) {
          for (int k = 0; k < PEYMAX; k++) {
            if (_timeline[i]->PEX == j && _timeline[i]->PEY == k) {
              displayLine(_timeline[i], outfile);
              outfile << "    ";
              i++;
            } else {
              displayLine(j, k, outputCoupledDimNum, inputCoupledDimNum,
                          weightCoupledDimNum, outfile);
              outfile << "    ";
            }
          }
          outfile << std::endl;
        }
      } else {
        outfile << std::endl;
        curTime = _timeline[i]->_time;
        outfile << std::to_string(_count) + " time\t";
        _count++;
        for (auto t : curTime) {
          outfile << t << ' ';
        }
        outfile << std::endl;
      }
    }
    outfile.close();
  }
  int getTCur(int row) {
    int colNumT = _T.getColNum();
    std::shared_ptr<WORKLOAD::Polynomial> dim =
        std::make_shared<WORKLOAD::Polynomial>();
    for (int i = 0; i < colNumT; i++) {
      if (_T(row, i) == 1)
        dim = dim + _coupledVarVec[i];
    }
    return dim->getCur();
  }
};
} // namespace TIMELINE