#include <atomic>

#include "TwoDimensionalArray.h"

template<class TValue>
class FourDimensionalArray {
private:
	TwoDimensionalArray<TwoDimensionalArray<std::atomic<TValue>>> internalArray_;

	size_t size1_;
	size_t size2_;
	size_t size3_;
	size_t size4_;

public:
	FourDimensionalArray(const size_t x, const size_t y, const size_t z, const size_t w) :
		internalArray_(x, y),
		size1_(x),
		size2_(y),
		size3_(z),
		size4_(w)
	{
		for (size_t i = 0; i < x; ++i) {
			for (size_t j = 0; j < y; ++j) {
				internalArray_.get(i, j).resize(z, w);
			}
		}
	}

	TValue get(const size_t x, const size_t y, const size_t z, const size_t w) {
		return internalArray_.get(x, y).get(z, w);
	}

	void put(const size_t x, const size_t y, const size_t z, const size_t w, const TValue value) {
		internalArray_.get(x, y).get(z, w).store(value);
	}

	void increase(const size_t x, const size_t y, const size_t z, const size_t w, const TValue delta) {
		while (true) {
			TValue oldValue = internalArray_.get(x, y).get(z, w);
			TValue newValue = oldValue + delta;
			if (internalArray_.get(x, y).get(z, w).compare_exchange_strong(oldValue, newValue)) {
				break;
			}
		}
	}

	void setAllUniform() {
		for (size_t i = 0; i < size1_; ++i) {
			for (size_t j = 0; j < size2_; ++j) {
				for (size_t k = 0; k < size3_; ++k) {
					for (size_t l = 0; l < size4_; ++l) {
						internalArray_.get(i, j).get(k, l).store(1.0 / i);
					}
				}
			}
		}
	}

	void setAll(const TValue value) {
		for (size_t i = 0; i < size1_; ++i) {
			for (size_t j = 0; j < size2_; ++j) {
				for (size_t k = 0; k < size3_; ++k) {
					for (size_t l = 0; l < size4_; ++l) {
						internalArray_.get(i, j).get(k, l).store(value);
					}
				}
			}
		}
	}
};