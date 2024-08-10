#include "include/util/debug.h"
namespace DEBUG {

void printError(ErrorType errortype, std::string msg) {
  switch (errortype) {
  case TMATRIXERROR: {
    std::cout << "Error!Invalid T Matrix\n\t" << msg << std::endl;
    break;
  }
  case NETWORKOPERROR: {
    std::cout << "Error!Execute a error network operation:" << msg << std::endl;
    break;
  }
  case REUSEVECSOLVEERROR: {
    std::cout << "Error!ReuseVec solve error:" << msg << std::endl;
    break;
  }
  case ITERATOREDGEERROR: {
    std::cout << "Error!The definition of edge edge is forbidden:" << msg
              << std::endl;
    break;
  }
  case REUSEVECNOTFITNETWORKARCHERROR: {
    std::cout << "Error!The reuseVec doesn't fit for arch:" << msg << std::endl;
    break;
  }
  case EDGEPEDIMERROR: {
    std::cout << "Error!Spatial dim has edge state:" << msg << std::endl;
    break;
  }
  case PEDIMOUTOFRANGE: {
    std::cout << "Error!PEDim out of Range:" << msg << std::endl;
    break;
  }
  case NETWORK_FEATURE_ERROR: {
    std::cout << "Error!Invalid featureVec:" << msg << std::endl;
    break;
  }
  case TASK_INCOMPLETE: {
    std::cout << "Error!Incomplete task:" << msg << std::endl;
    break;
  }
  case ERROR_LEVEL: {
    std::cout << "Error!Incomplete level:" << msg << std::endl;
    break;
  }
  case EMPTY_ACCELERATOR: {
    std::cout << "Error!Empty accelerator:" << msg << std::endl;
    break;
  }
  case EMPTY_TARGET: {
    std::cout << "Error!Empty target:" << msg << std::endl;
    break;
  }
  case EMPTY_TASK: {
    std::cout << "Error!Empty task:" << msg << std::endl;
    break;
  }
  case ERROR_CANDIDATE: {
    std::cout << "Error!Error tile candidate:" << msg << std::endl;
    break;
  }
  case ERROR_ACCELERATOR_SET: {
    std::cout << "Error!Error accelerator set:" << msg << std::endl;
    break;
  }
  case EMPTY_ACCELERATOR_SET: {
    std::cout << "Error!Empty accelerator set:" << msg << std::endl;
    break;
  }
  }
}

void check(bool flag, ErrorType errortype, std::string msg) {
  if (!flag) {
    printError(errortype, msg);
    exit(-1);
  }
}
} // namespace DEBUG
