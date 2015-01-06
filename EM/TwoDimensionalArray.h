#pragma once
#include <memory>

template <class T>
class TwoDimensionalArray {
private:
	size_t length_;
	size_t width_;
	std::unique_ptr<T[]> internalArray_;
public:
	TwoDimensionalArray() :
		length_(0),
		width_(0)
	{ }

	TwoDimensionalArray(const size_t length, const size_t width) :
		length_(length),
		width_(width),
		internalArray_(new T[length * width])
	{ }

	T& get(const size_t row, const size_t column) {
		return internalArray_[row * length_ + column];
	}

	void put(const size_t row, const size_t column, const T& value) {
		internalArray_[row * length_ + column] = value;
	}

	void resize(const size_t length, const size_t width) {
		length_ = length;
		width_ = width;
		internalArray_.reset(new T[length * width]);
	}

};
