#include <vector>
#include <boost/iostreams/device/mapped_file.hpp>


#include "TwoDimensionalArray.h"
#include "LockFreeList.h"

class LockFreeHashTable {
private:
	TwoDimensionalArray<LockFreeList> internalArray_;
	size_t rowCount_;
	size_t columnCount_;

	const size_t polinomialBase_ = 31;
	std::vector<size_t> polinomialTable_;

	size_t countRowHash(const size_t strId) {
		return strId % rowCount_;
	}

	size_t countColumnHash(const size_t strId) {
		return strId % columnCount_;
	}

public:
	LockFreeHashTable(size_t rowCount, size_t columnCount) :
		rowCount_(rowCount),
		columnCount_(columnCount),
		polinomialTable_(31),
		internalArray_(columnCount, rowCount)
	{ }

	~LockFreeHashTable() {
		std::cout << "Lock free hash table" << std::endl;
	}

	void addWordPair(const size_t englishWordId, const size_t frenchWordId, const float value) {
		size_t row = countRowHash(englishWordId);
		size_t column = countColumnHash(frenchWordId);
		internalArray_.get(row, column).addIfNotExist(englishWordId, frenchWordId, value);
	}

	double getValue(const size_t englishWordId, const size_t frenchWordId) {
		size_t row = countRowHash(englishWordId);
		size_t column = countColumnHash(frenchWordId);
		return internalArray_.get(row, column).getValue(englishWordId, frenchWordId);
	}

	void setValue(const size_t englishWordId, const size_t frenchWordId, const float value) {
		size_t row = countRowHash(englishWordId);
		size_t column = countColumnHash(frenchWordId);
		internalArray_.get(row, column).setValue(englishWordId, frenchWordId, value);
	}

	void increaseValue(const size_t englishWordId, const size_t frenchWordId, const float deltaValue) {
		size_t row = countRowHash(englishWordId);
		size_t column = countColumnHash(frenchWordId);
		internalArray_.get(row, column).increaseValue(englishWordId, frenchWordId, deltaValue);
	}

	void setAll(float value) {
		for (size_t i = 0; i < rowCount_; ++i) {
			for (size_t j = 0; j < columnCount_; ++j) {
				internalArray_.get(i, j).setAll(value);
			}
		}
	}

	void setAllUniform(size_t frenchWordsCount) {
		for (size_t i = 0; i < frenchWordsCount; ++i) {
			size_t column = countColumnHash(i);
			size_t size = 0;
			for (size_t j = 0; j < rowCount_; ++j) {
				size += internalArray_.get(j, column).getSize(i);
			}
			if (size != 0) {
				for (size_t j = 0; j < rowCount_; ++j) {
					internalArray_.get(j, column).setAll(i, 1.0f / size);
				}
			}
		}
	}

	size_t findMax(size_t englishWordId) {
		size_t row = countRowHash(englishWordId);
		float max = 0.0f;
		size_t frenchWordId = 0;
		for (size_t i = 0; i < columnCount_; ++i) {
			std::pair<float, size_t> pair = internalArray_.get(row, i).findMax(englishWordId);
			if (pair.first > max) {
				max = pair.first;
				frenchWordId = pair.second;
			}
		}
		return frenchWordId;
	}

};