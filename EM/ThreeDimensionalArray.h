#include <vector>
#include <atomic>

#include "TwoDimensionalArray.h"

template <class TValue>
class ThreeDimensionalArray {
private:
	TwoDimensionalArray<std::unique_ptr<std::atomic<TValue>[]>> internalArray_;

	size_t size1_;
	size_t size2_;
	size_t size3_;

public:
	ThreeDimensionalArray(const size_t x, const size_t y, const size_t z) :
		internalArray_(x, y),
		size1_(x),
		size2_(y),
		size3_(z)
	{
		for (size_t i = 0; i < x; ++i) {
			for (size_t j = 0; j < y; ++j) {
				internalArray_.get(i, j).reset(new std::atomic<TValue>[z]);
			}
		}
	}

	TValue get(const size_t x, const size_t y, const size_t z) {
		return internalArray_.get(x, y)[z];
	}

	void put(const size_t x, const size_t y, const size_t z, const TValue value) {
		internalArray_.get(x, y)[z].store(value);
	}

	void increase(const size_t x, const size_t y, const size_t z, const TValue delta) {
		while (true) {
			TValue oldValue = internalArray_.get(x, y)[z];
			TValue newValue = oldValue + delta;
			if (internalArray_.get(x, y)[z].compare_exchange_strong(oldValue, newValue)) {
				break;
			}
		}
	}

	void setAll(const TValue value) {
		for (size_t i = 0; i < size1_; ++i) {
			for (size_t j = 0; j < size2_; ++j) {
				for (size_t k = 0; k < size3_; ++k) {
					internalArray_.get(i, j)[k].store(value);
				}
			}
		}
	}
};