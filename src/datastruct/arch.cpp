#include "include/analysis/costAnalysis.h"
#include "include/datastruct/arch.h"
extern COST::COSTDADA _Cost;
namespace ARCH {
void Network::constructNetwork(
    int rowNum, int colNum,
    int doubleNetworkFirstFlag) { // Too difficult to unify so enum

  if (_featureVec[0] == 1 && _featureVec[1] == 0) {
    for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(0, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(0, rowIndex), colNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == 0) {
    for (int rowIndex = 0; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colNum - 1, rowIndex),
          std::make_shared<NetworkItem>(
              std::pair<int, int>(colNum - 1, rowIndex), colNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 0 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, 0),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, 0), rowNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 0 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, rowNum - 1),
          std::make_shared<NetworkItem>(
              std::pair<int, int>(colIndex, rowNum - 1), rowNum)));
      if (doubleNetworkFirstFlag)
        break;
    }
  } else if (_featureVec[0] == 1 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, 0),
          std::make_shared<NetworkItem>(std::pair<int, int>(colIndex, 0),
                                        std::min(rowNum, colNum - colIndex))));
    }
    for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(0, rowIndex),
          std::make_shared<NetworkItem>(std::pair<int, int>(0, rowIndex),
                                        std::min(colNum, rowNum - rowIndex))));
    }
  } else if (_featureVec[0] == 1 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, rowNum - 1),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, rowNum - 1),
                             std::min(rowNum, colNum - colIndex))));
    }
    for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(0, rowIndex),
          std::make_shared<NetworkItem>(std::pair<int, int>(0, rowIndex),
                                        std::min(colNum, rowIndex + 1))));
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == 1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(std::make_pair(
          std::pair<int, int>(colIndex, 0),
          std::make_shared<NetworkItem>(std::pair<int, int>(colIndex, 0),
                                        std::min(rowNum, colIndex + 1))));
    }
    for (int rowIndex = 1; rowIndex < rowNum; rowIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colNum - 1, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colNum - 1, rowIndex),
                             std::min(colNum, rowNum - rowIndex))));
    }
  } else if (_featureVec[0] == -1 && _featureVec[1] == -1) {
    for (int colIndex = 0; colIndex < colNum; colIndex++) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colIndex, rowNum - 1),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colIndex, rowNum - 1),
                             std::min(rowNum, colIndex + 1))));
    }
    for (int rowIndex = rowNum - 2; rowIndex >= 0; rowIndex--) {
      _networkItemMap->insert(
          std::make_pair(std::pair<int, int>(colNum - 1, rowIndex),
                         std::make_shared<NetworkItem>(
                             std::pair<int, int>(colNum - 1, rowIndex),
                             std::min(colNum, rowIndex + 1))));
    }
  }
}
int NetworkGroup::getInitOrOutDelay(int base, int dataWidth,
                                    std::pair<int, int> &PEXRange,
                                    std::pair<int, int> &PEYRange) {
  if (_peFlag) {
    return 0;
  } else if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                    dataWidth);
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY || networkType1 == UNICAST) {
      // STATIONARY - SYSTOLIC UNICAST
      if (networkType2 == SYSTOLIC) {
        return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                        dataWidth);
      } else // UNICAST
      {
        return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                        dataWidth);
      }
    } else if (networkType2 == STATIONARY) {
      if (networkType1 == SYSTOLIC) {
        return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                        dataWidth) *
               (*_networkSet)[0]->getMaxCoupleNum(PEXRange, PEYRange);
      } else // MULTICAST
      {
        return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                        dataWidth);
      }
    } else {
      return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                      dataWidth);
    }
  }
}
long long NetworkGroup::getInitOrOutBW(int base, int dataWidth,
                                       std::pair<int, int> &PEXRange,
                                       std::pair<int, int> &PEYRange,
                                       long long stableDelay) {
  if (_peFlag) {
    return 0;
  } else if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return std::ceil(
        (double)((long long)base * getActiveAccessPointNum(PEXRange, PEYRange) *
                 dataWidth) /
        (double)(stableDelay));
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY || networkType1 == UNICAST) {
      // STATIONARY - SYSTOLIC UNICAST
      if (networkType2 == SYSTOLIC) {
        return std::ceil((double)((long long)base *
                                  getActiveAccessPointNum(PEXRange, PEYRange) *
                                  dataWidth) /
                         (double)(stableDelay));
      } else // UNICAST
      {
        return std::ceil((double)((long long)base *
                                  getActiveAccessPointNum(PEXRange, PEYRange) *
                                  dataWidth) /
                         (double)(stableDelay));
      }
    } else if (networkType2 == STATIONARY) {
      if (networkType1 == SYSTOLIC) {
        return std::ceil(
            (double)((long long)base *
                     getActiveAccessPointNum(PEXRange, PEYRange) *
                     (*_networkSet)[1]->getMaxCoupleNum(PEXRange, PEYRange) *
                     dataWidth) /
            (double)(stableDelay));
      } else // MULTICAST
      {
        return std::ceil((double)((long long)base *
                                  getActiveAccessPointNum(PEXRange, PEYRange) *
                                  dataWidth) /
                         (double)(stableDelay));
      }
    } else {
      return std::ceil((double)((long long)base *
                                getActiveAccessPointNum(PEXRange, PEYRange) *
                                dataWidth) /
                       (double)(stableDelay));
    }
  }
}
int NetworkGroup::getStableDelay(int base, int dataWidth,
                                 std::pair<int, int> &PEXRange,
                                 std::pair<int, int> &PEYRange) {
  if (_peFlag) {
    return 0;
  } else if (_networkSet->size() == 1) {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    // MULTICAST UNICAST SYSTOLIC
    return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                    dataWidth);
  } else {
    NETWORKTYPE networkType1 = (*_networkSet)[0]->getNetworkType();
    NETWORKTYPE networkType2 = (*_networkSet)[1]->getNetworkType();
    if (networkType1 == STATIONARY || networkType2 == STATIONARY) {
      return 0;
    } else if (networkType1 == UNICAST) {
      return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                      dataWidth);
    } else {
      return getDelay(base * getActiveAccessPointNum(PEXRange, PEYRange),
                      dataWidth);
    }
  }
}
int Network::getMaxCoupleNum(std::pair<int, int> &PEXRange,
                             std::pair<int, int> &PEYRange) {
  DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
               DEBUG::NETWORKOPERROR, "getMaxCoupleNum");
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];

  if (featureX == 0) {
    if (featureY == 1) {
      return PEYRange.second + 1;
    } else // featureY = -1
    {
      return _rowNum - PEYRange.first;
    }
  } else if (featureY == 0) {
    if (featureX == 1) {
      return PEXRange.second + 1;
    } else // featureX = -1
    {
      return _colNum - PEXRange.first;
    }
  } else {
    int ret = 0;
    int perCoupleNum;
    for (auto item : (*_networkItemMap)) {
      ret = std::max(ret, getSlantCoupleNum(PEXRange, PEYRange, item.first));
    }
    return ret;
  }
}
int Network::getSlantCoupleNum(std::pair<int, int> &PEXRange,
                               std::pair<int, int> &PEYRange,
                               std::pair<int, int> accessPoint) {
  std::pair<int, int> left, top, right, bottom;
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];
  // ax + by + c = 0
  int a, b, c, x, y;
  x = accessPoint.first;
  y = accessPoint.second;
  a = -featureX;
  b = featureY;
  c = -a * x - b * y;

  x = PEXRange.first;
  y = (-c - a * x) / b;
  left.first = x;
  left.second = y;

  x = PEXRange.second;
  y = (-c - a * x) / b;
  right.first = x;
  right.second = y;

  y = PEYRange.first;
  x = (-c - b * y) / a;
  bottom.first = x;
  bottom.second = y;

  y = PEYRange.second;
  x = (-c - b * y) / a;
  top.first = x;
  top.second = y;

  if (featureX * featureY == 1) {
    return std::min(right.first, top.first) -
           std::max(left.first, bottom.first) + 1;
  } else {
    return std::min(right.first, bottom.first) -
           std::max(left.first, top.first) + 1;
  }
}

int Network::getActiveAccessPointNum(std::pair<int, int> &PEXRange,
                                     std::pair<int, int> &PEYRange) {
  DEBUG::check(_networkType != UNICAST && _networkType != STATIONARY,
               DEBUG::NETWORKOPERROR, "getActiveAccessPointNum");
  int featureX = _featureVec[0], featureY = _featureVec[1],
      featureT = _featureVec[2];

  if (featureX == 0) {
    return PEXRange.second - PEXRange.first + 1;
  } else if (featureY == 0) {
    return PEYRange.second - PEYRange.first + 1;
  } else {
    int ret = 0;
    int perCoupleNum;
    for (auto item : (*_networkItemMap)) {
      if (getSlantCoupleNum(PEXRange, PEYRange, item.first) > 0)
        ret++;
    }
    return ret;
  }
}
double getNetworkCost(ARCH::DATATYPE dataType, ARCH::NETWORKTYPE networkType,
                      int linkNum, int flag, int dataWidth) {
  if (networkType == ARCH::UNICAST) {
    if (dataType == ARCH::INPUT || ARCH::WEIGHT)
      return _Cost._unicastInput.lookup(dataWidth, linkNum, flag);
    else
      return _Cost._unicastOutput.lookup(dataWidth, linkNum, flag);
  } else if (networkType == ARCH::MULTICAST) {
    if (dataType == ARCH::INPUT || ARCH::WEIGHT)
      return _Cost._multicastInput.lookup(dataWidth, linkNum, flag);
    else
      return _Cost._multicastOutput.lookup(dataWidth, linkNum, flag);
  } else {
    assert(networkType == ARCH::SYSTOLIC);
    if (dataType == ARCH::INPUT || ARCH::WEIGHT)
      return _Cost._systolicInput.lookup(dataWidth, linkNum, flag);
    else
      return _Cost._systolicOutput.lookup(dataWidth, linkNum, flag);
  }
}
double Buffer::getBufferCost(int flag, int bankNum, int dataWidthRatio) {

  double ret;
  if (_bufferType == REG) {
    ret = _Cost._regData.lookup((_capacity + bankNum - 1) / bankNum, flag);
  } else if (_bufferType == SRAM) {
    ret = _Cost._sramData.lookup((_capacity + bankNum - 1) / bankNum, flag);
  } else {
    return 0;
  }
  ret *= dataWidthRatio;
  if (flag == 0 || flag == 1) { // for per energy
    return ret;
  } else { // for area and leakage power
    return bankNum * ret;
  }
}
double Level::getMacCost(int flag, int rowNum, int colNum) {
  return _Cost._mac.lookup(_dataWidth, 1, flag) * rowNum * colNum;
}
} // namespace ARCH
