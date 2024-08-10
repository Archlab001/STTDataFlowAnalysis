
#include "include/datastruct/mapping.h"
#include "include/util/eigenUtil.h"

void printMatrix(
    Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic> matrix) {
  std::cout << matrix << '\n' << std::endl;
}

int GCD(int a, int b) {
  int temp = 0;
  while (b != 0) {
    temp = a;
    a = b;
    b = temp % b;
  }
  return a;
}

void compATinv(MAPPING::Transform &T, MAPPING::Access &A,
               std::vector<std::vector<int>> &matrix) {
  int TDimNum = T.getColNum();
  int AColNum = A.getColNum();
  int ARowNum = A.getMatrix()->size() / A.getColNum();
  auto Tmatrix = T.getMatrix();
  auto Amatrix = A.getMatrix();
  std::vector<double> tmpT(TDimNum * TDimNum, 0);
  std::transform(Tmatrix->begin(), Tmatrix->end(), tmpT.begin(),
                 [](int n) { return n; });
  std::vector<double> tmpA(AColNum * ARowNum, 0);
  std::transform(Amatrix->begin(), Amatrix->end(), tmpA.begin(),
                 [](int n) { return n; });
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      TMatrix =
          Eigen::Map<Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic>>(
              tmpT.data(), TDimNum, TDimNum)
              .transpose();
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      AMatrix =
          Eigen::Map<Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic>>(
              tmpA.data(), AColNum, ARowNum)
              .transpose();
  Eigen::Matrix<valueType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      eigenmatrix = AMatrix * TMatrix.inverse();
  std::vector<int> tmp;
  int value;
  for (int i = 0; i < eigenmatrix.rows(); i++) {
    tmp.clear();
    for (int j = 0; j < eigenmatrix.cols(); j++) {
      value = std::floor(eigenmatrix(i, j));
      DEBUG::check((value - eigenmatrix(i, j)) < 1e-5,
                   DEBUG::REUSEVECSOLVEERROR, "compATinv");
      tmp.push_back(value);
    }
    matrix.push_back(tmp);
  }
}

std::pair<int, int>
findFirstNoZeroRow(std::vector<std::vector<Fraction>> &matrix, int startRow) {
  int rowNum = matrix.size();
  int colNum = matrix[0].size();
  int curIndex;
  int minIndex = colNum;
  std::pair<int, int> ret = std::make_pair(-1, -1);
  for (int i = startRow; i < rowNum; i++) {
    curIndex = colNum;
    for (int j = 0; j < colNum; j++) {
      if (!matrix[i][j].isZero()) {
        curIndex = j;
        break;
      }
    }
    if (curIndex < minIndex) {
      minIndex = curIndex;
      ret = std::make_pair(i, curIndex);
    }
  }
  return ret;
}
void rowSubRow(std::vector<std::vector<Fraction>> &matrix, int row1, int row2,
               Fraction rate) {
  int colNum = matrix[0].size();
  for (int i = 0; i < colNum; i++) {
    matrix[row2][i] -= matrix[row1][i] * rate;
  }
}
void rowDivNum(std::vector<std::vector<Fraction>> &matrix, int row1,
               Fraction rate) {
  int colNum = matrix[0].size();
  for (int i = 0; i < colNum; i++) {
    matrix[row1][i] = matrix[row1][i] / rate;
  }
}
void convertTrapezoidalMatrix(std::vector<std::vector<Fraction>> &matrix,
                              std::vector<std::pair<int, int>> &majorVec) {
  int rowNum = matrix.size();
  int colNum = matrix[0].size();
  std::pair<int, int> NZIndex;
  for (int i = 0; i < rowNum; i++) {
    NZIndex = findFirstNoZeroRow(matrix, i);
    DEBUG::check(NZIndex.second != colNum - 1, DEBUG::REUSEVECSOLVEERROR,
                 "convertTrapezoidalMatrix");
    if (i == 0) {
      DEBUG::check(NZIndex.first != -1, DEBUG::REUSEVECSOLVEERROR,
                   "convertTrapezoidalMatrix");
    }
    if (NZIndex.first == -1)
      break;
    majorVec.push_back(NZIndex);
    std::swap(matrix[NZIndex.first], matrix[i]);
    for (int j = i + 1; j < rowNum; j++) {
      rowSubRow(matrix, i, j,
                matrix[j][NZIndex.second] / matrix[i][NZIndex.second]);
    }
  }
  int majorLen = majorVec.size();
  for (int i = 1; i < majorLen; i++) {
    NZIndex = majorVec[i];
    for (int j = 0; j < i; j++) {
      rowSubRow(matrix, i, j,
                matrix[j][NZIndex.second] / matrix[i][NZIndex.second]);
    }
  }
  for (int i = 0; i < majorLen; i++) {
    NZIndex = majorVec[i];
    rowDivNum(matrix, i, matrix[i][NZIndex.second]);
  }
}
void swapCol(std::vector<std::vector<Fraction>> &matrix, int col1, int col2) {
  int rowNum = matrix.size();

  int colNum = matrix[0].size();
  for (int i = 0; i < rowNum; i++) {
    std::swap(matrix[i][col1], matrix[i][col2]);
  }
}
void aggregateMajorVar(std::vector<std::pair<int, int>> &majorVec,
                       std::vector<int> &varSymFlag,
                       std::vector<std::vector<Fraction>> &matrix) {
  int majorLen = majorVec.size();
  std::pair<int, int> NZIndex;
  for (int i = 0; i < majorLen; i++) {
    NZIndex = majorVec[i];
    if (NZIndex.second != i) {
      std::swap(varSymFlag[i], varSymFlag[NZIndex.second]);
      swapCol(matrix, i, NZIndex.second);
    }
  }
}

void buildFractionReuseVec(std::vector<std::pair<int, int>> &majorVec,
                           std::vector<int> &varSymFlag,
                           std::vector<std::vector<Fraction>> &matrix,
                           std::vector<std::vector<Fraction>> &reuseVec) {
  int rowNum = matrix.size();
  int colNum = matrix[0].size() - 1;
  int majorLen = majorVec.size();
  for (int i = 0; i < rowNum; i++) {
    for (int j = 0; j < colNum - majorLen; j++) {
      reuseVec[j][i] = -matrix[i][majorLen + j];
    }
  }
  for (int j = majorLen; j < colNum; j++) {
    for (int i = 0; i < colNum - majorLen; i++) {
      reuseVec[i][j] = i == j - majorLen;
    }
  }
  for (int j = 0; j < rowNum; j++) {
    reuseVec[colNum - majorLen][j] = matrix[j][colNum];
  }
  for (int j = rowNum; j < colNum; j++) {
    reuseVec[colNum - majorLen][j] = 0;
  }

  for (int i = 0; i < colNum; i++) {
    if (varSymFlag[i] == i)
      continue;
    for (int j = i + 1; j < colNum; j++) {
      if (varSymFlag[j] == i) {
        std::swap(varSymFlag[i], varSymFlag[j]);
        swapCol(reuseVec, i, j);
        break;
      }
    }
  }
}
void buildIntReuseVec(std::vector<std::pair<int, int>> &majorVec,
                      std::vector<std::vector<Fraction>> &matrix,
                      std::vector<std::vector<Fraction>> &reuseVec,
                      std::shared_ptr<std::vector<std::vector<int>>> ret) {
  int rowNum = matrix.size();
  int colNum = matrix[0].size() - 1;
  int majorLen = majorVec.size();
  int lcm;
  for (int i = 0; i < colNum + 1 - majorLen; i++) {
    lcm = 1;
    for (int j = 0; j < colNum; j++) {
      lcm = reuseVec[i][j].denominator * lcm *
            GCD(reuseVec[i][j].denominator, lcm);
    }
    for (int j = 0; j < colNum; j++) {
      (*ret)[i][j] =
          reuseVec[i][j].numerator * lcm / reuseVec[i][j].denominator;
    }
  }
}

void convertIntAb2Fraction(std::vector<std::vector<int>> &src,
                           std::vector<std::vector<Fraction>> &dst,
                           std::vector<int> &b) {
  int rowNum = src.size();
  int colNum = src[0].size();
  std::vector<Fraction> tmp;
  for (int i = 0; i < rowNum; i++) {
    tmp.clear();
    for (int j = 0; j < colNum; j++) {
      tmp.push_back(Fraction(src[i][j]));
    }
    tmp.push_back(b[i]);
    dst.push_back(tmp);
  }
}

void convertIntAb2Fraction(std::vector<std::vector<int>> &src,
                           std::vector<std::vector<Fraction>> &dst, int b) {
  int rowNum = src.size();
  int colNum = src[0].size();
  std::vector<Fraction> tmp;
  for (int i = 0; i < rowNum; i++) {
    tmp.clear();
    for (int j = 0; j < colNum; j++) {
      tmp.push_back(Fraction(src[i][j]));
    }
    tmp.push_back(b);
    dst.push_back(tmp);
  }
}

std::shared_ptr<std::vector<std::vector<int>>>
doSolvingLinearEquations(std::vector<std::vector<Fraction>> &tmpM) {
  int rowNum = tmpM.size();
  int colNum = tmpM[0].size();
  std::vector<std::pair<int, int>> majorVec;
  convertTrapezoidalMatrix(tmpM, majorVec);
  std::vector<int> varSymFlag(colNum - 1);
  int count = 0;
  std::generate(varSymFlag.begin(), varSymFlag.end(),
                [&count] { return count++; });
  int majorLen = majorVec.size();
  aggregateMajorVar(majorVec, varSymFlag, tmpM);
  std::vector<std::vector<Fraction>> reuseVec(
      colNum - majorLen, std::vector<Fraction>(colNum - 1, 0));
  buildFractionReuseVec(majorVec, varSymFlag, tmpM, reuseVec);
  std::shared_ptr<std::vector<std::vector<int>>> ret =
      std::make_shared<std::vector<std::vector<int>>>(
          std::vector<std::vector<int>>(colNum - majorLen,
                                        std::vector<int>(colNum - 1, 0)));
  buildIntReuseVec(majorVec, tmpM, reuseVec, ret);
  return ret;
}

std::shared_ptr<std::vector<std::vector<int>>>
solvingLinearEquations(std::vector<std::vector<int>> &matrix, int b) {
  std::vector<std::vector<Fraction>> tmpM;
  convertIntAb2Fraction(matrix, tmpM, b);
  return doSolvingLinearEquations(tmpM);
}

std::shared_ptr<std::vector<std::vector<int>>>
solvingLinearEquations(std::vector<std::vector<int>> &matrix,
                       std::vector<int> &b) {
  std::vector<std::vector<Fraction>> tmpM;
  convertIntAb2Fraction(matrix, tmpM, b);
  return doSolvingLinearEquations(tmpM);
}

std::shared_ptr<std::vector<std::vector<int>>>
compReuseVec(MAPPING::Transform &T, MAPPING::Access &A) {
  std::vector<std::vector<int>> ATinv;
  compATinv(T, A, ATinv);
  auto reuseVec = solvingLinearEquations(ATinv, 0);
  reuseVec->pop_back(); // remove particular solution
  return reuseVec;
}
std::shared_ptr<std::vector<std::vector<int>>> scalarReuseVec(int coupledNum) {
  std::shared_ptr<std::vector<std::vector<int>>> reuseVec =
      std::make_shared<std::vector<std::vector<int>>>(
          2, std::vector<int>(coupledNum, 0));
  (*reuseVec)[0][0] = 1;
  (*reuseVec)[1][1] = 1;
  return reuseVec;
}