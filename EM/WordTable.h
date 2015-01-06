#include <vector>
#include <memory>

#include "WordList.h"

template<class TValue>
class WordTable {
private:
	std::vector<std::vector<std::shared_ptr<WordList<TValue>>>> internalArray_;
	std::mutex mutex_;
	const size_t columnCount_ = 1000;


public:
	WordTable() :
		internalArray_(30000)
	{
		for (size_t i = 0; i < 30000; ++i) {
			internalArray_[i].resize(columnCount_, std::make_shared<WordList<TValue>>());
		}
	}

	void addWordPair(const size_t englishWordId, const size_t frenchWordId, const TValue value) {	
		/*
		if (internalArray_.size() <= englishWordId) {
			std::lock_guard<std::mutex> guard(mutex_);
			while (internalArray_.size() <= englishWordId) {
				internalArray_.resize(internalArray_.size() * 2 + 1, std::vector<std::shared_ptr<WordList<TValue>>>(columnCount_, std::make_shared<WordList<TValue>>()));
			}
		}*/
		
		if (!internalArray_[englishWordId][frenchWordId % columnCount_]) {
			throw std::logic_error("AAAAAAAAa");
		}
		internalArray_[englishWordId][frenchWordId % columnCount_]->addIfNotExist(frenchWordId, value);
	}

	double getValue(const size_t englishWordId, const size_t frenchWordId) {
		return internalArray_[englishWordId][frenchWordId % columnCount_]->getValue(frenchWordId);
	}

	void setValue(const size_t englishWordId, const size_t frenchWordId, const float value) {
		internalArray_[englishWordId][frenchWordId % columnCount_]->setValue(frenchWordId, value);
	}

	void increaseValue(const size_t englishWordId, const size_t frenchWordId, const float deltaValue) {
		internalArray_[englishWordId][frenchWordId % columnCount_]->increaseValue(frenchWordId, deltaValue);
	}

	void setAllUniform() {
		for (size_t i = 0; i < internalArray_.size(); ++i) {
			size_t size = 0;
			for (size_t j = 0; j < columnCount_; ++j) {
				size += internalArray_[i][j]->getSize();
			}
			if (size != 0) {
				for (size_t j = 0; j < columnCount_; ++j) {
					internalArray_[i][j]->setAll(1.0 / size);
				}
			}
		}
	}

	void setAll(TValue value) {
		for (size_t i = 0; i < internalArray_.size(); ++i) {
			for (size_t j = 0; j < columnCount_; ++j) {
				internalArray_[i][j]->setAll(value);
			}
		}
	}
};