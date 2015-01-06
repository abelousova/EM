#include <vector>
#include <fstream>
#include <string>

class AlignmentTable {
private:
	std::vector<std::vector<std::vector<size_t>>> internalTable_;
public:
	void resize(const size_t size) {
		internalTable_.resize(size);
		
	}

	void resize(const size_t sentenceNumber, const size_t size) {
		if (internalTable_.size() <= sentenceNumber) {
			std::cout << ":(" << internalTable_.size() << "  " << sentenceNumber << std::endl;
		}
		internalTable_[sentenceNumber].resize(size);
	}

	void put(const size_t sentenceNumber, const size_t wordNumber, const size_t value) {
		if (sentenceNumber >= internalTable_.size()) {
			throw std::logic_error("sentence num" + std::to_string(sentenceNumber));
		}
		if (wordNumber >= internalTable_[sentenceNumber].size()) {
			throw std::logic_error("word num" + std::to_string(wordNumber) + "  " + std::to_string(internalTable_[sentenceNumber].size()));
		}
		internalTable_[sentenceNumber][wordNumber].push_back(value);
	}

	void print(std::ofstream& out) const {
		for (size_t i = 0; i < internalTable_.size(); ++i) {
			for (size_t j = 0; j < internalTable_[i].size(); ++j) {
				//if (internalTable_[i][j].empty()) {
				//	out << "0-" << j << " ";
				//}
				for (auto iterator = internalTable_[i][j].begin(); iterator != internalTable_[i][j].end(); ++iterator) {
					out << *iterator << "-" << j << " ";
				}
			}
			out << std::endl;
		}
	}

	size_t getSize() const {
		size_t result = 0;
		for (size_t i = 0; i < internalTable_.size(); ++i) {
			for (size_t j = 0; j < internalTable_[i].size(); ++j) {
				result += internalTable_[i][j].size();
			}
		}
		return result;
	}

	friend void findIntersectionDifferentLanguages(const AlignmentTable& firstTable, const AlignmentTable& secondTable, AlignmentTable& resultTable);
	friend size_t findIntersectionSize(const AlignmentTable& firstTable, const AlignmentTable& secondTable);
	friend void findIntersection(const AlignmentTable& firstTable, const AlignmentTable& secondTable, AlignmentTable& resultTable, const size_t sentenceNumber, const size_t wordNumber);
	friend bool bucketHasValue(const AlignmentTable& table, size_t row, size_t column, size_t value);
	friend void refindEmptyBuckets(const AlignmentTable& firstTable, const AlignmentTable& secondTable, FourDimensionalArray<float>& qJILM, AlignmentTable& resultTable);
};

void findIntersectionDifferentLanguages(const AlignmentTable& firstTable, const AlignmentTable& secondTable, AlignmentTable& resultTable) {
	resultTable.resize(firstTable.internalTable_.size());
	for (size_t i = 0; i < firstTable.internalTable_.size(); ++i) {
		resultTable.internalTable_[i].resize(firstTable.internalTable_[i].size());
		for (size_t j = 0; j < firstTable.internalTable_[i].size(); ++j) {
			findIntersection(firstTable, secondTable, resultTable, i, j);
		}
	}
}

void findIntersection(const AlignmentTable& firstTable, const AlignmentTable& secondTable, AlignmentTable& resultTable, const size_t sentenceNumber, const size_t wordNumber){
	for (size_t k = 0; k < firstTable.internalTable_[sentenceNumber][wordNumber].size(); ++k) {
		if (bucketHasValue(secondTable, sentenceNumber, firstTable.internalTable_[sentenceNumber][wordNumber][k], wordNumber)) {
			resultTable.internalTable_[sentenceNumber][wordNumber].push_back(firstTable.internalTable_[sentenceNumber][wordNumber][k]);
		}
	}

	if (wordNumber == 0 && resultTable.internalTable_[sentenceNumber][wordNumber].empty()) {
		resultTable.internalTable_[sentenceNumber][wordNumber].push_back(0);
	}
}

bool bucketHasValue(const AlignmentTable& table, size_t row, size_t column, size_t value) {
	for (size_t i = 0; i < table.internalTable_[row][column].size(); ++i) {
		if (table.internalTable_[row][column][i] == value) {
			return true;
		}
	}
	return false;
}

void refindEmptyBuckets(const AlignmentTable& firstTable, const AlignmentTable& secondTable, FourDimensionalArray<float>& qJILM, AlignmentTable& resultTable) {
	for (size_t i = 0; i < resultTable.internalTable_.size(); ++i) {
		for (size_t j = 0; j < resultTable.internalTable_[i].size(); ++j) {
			if (resultTable.internalTable_[i][j].empty()) {
				if (j != 0 && j != resultTable.internalTable_[i].size() - 1) {
					size_t leftBorder = *std::min_element(resultTable.internalTable_[i][j - 1].begin(), resultTable.internalTable_[i][j - 1].end());
					size_t rightBorder;
					if (resultTable.internalTable_[i][j + 1].empty()) {
						rightBorder = *std::max_element(resultTable.internalTable_[i][j - 1].begin(), resultTable.internalTable_[i][j - 1].end()) + 2;
					}
					else {
						rightBorder = *std::min_element(resultTable.internalTable_[i][j + 1].begin(), resultTable.internalTable_[i][j + 1].end());
					}
					if (leftBorder > rightBorder) {
						rightBorder = *std::max_element(resultTable.internalTable_[i][j + 1].begin(), resultTable.internalTable_[i][j + 1].end());
					}
					if (leftBorder > rightBorder) {
						std::swap(leftBorder, rightBorder);
					}
					if (rightBorder - leftBorder == 2) {
						resultTable.internalTable_[i][j].push_back(leftBorder + 1);
						continue;
					}
					float maxProbability = 0.0f;
					size_t maxProbabilityPosition = 0;
					for (size_t k = leftBorder; k <= rightBorder; ++k) {
						float probability = qJILM.get(secondTable.internalTable_[i].size(), firstTable.internalTable_[i].size(), k, j);
						if (probability > maxProbability) {
							maxProbability = probability;
							maxProbabilityPosition = k;
						}
					}
					resultTable.internalTable_[i][j].push_back(maxProbabilityPosition);
				}
			}
		}
	}
}

size_t findIntersectionSize(const std::vector<size_t>& firstVector, const std::vector<size_t>& secondVector) {
	size_t result = 0;
	for (size_t i = 0; i < firstVector.size(); ++i) {
		for (size_t j = 0; j < secondVector.size(); ++j) {
			if (firstVector[i] == secondVector[j]) {
				++result;
			}
		}
	}
	return result;
}

size_t findIntersectionSize(const AlignmentTable& firstTable, const AlignmentTable& secondTable) {
	size_t result = 0;
	for (size_t i = 0; i < firstTable.internalTable_.size(); ++i) {
		for (size_t j = 0; j < firstTable.internalTable_[i].size(); ++j) {
			result += findIntersectionSize(firstTable.internalTable_[i][j], secondTable.internalTable_[i][j]);
		}
	}
	return result;
}