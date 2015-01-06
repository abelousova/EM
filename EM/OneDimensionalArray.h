#include <atomic>

#include <memory>

template<class TValue>
class OneDimensionalArray {
private:
	std::unique_ptr<std::atomic<TValue>[]> internalArray_;

	size_t size_;
public:
	OneDimensionalArray() :
		size_(0)
	{ }

	OneDimensionalArray(const size_t size) :
		size_(size),
		internalArray_(new std::atomic<TValue>[size])
	{	}

	TValue get(const size_t index) const {
		return internalArray_[index];
	}

	void put(const size_t index, const TValue value) {
		internalArray_[index].store(value);
	}

	void resize(const size_t size) {
		internalArray_.reset(new std::atomic<TValue>[size]);
	}

	void increase(const size_t index, const TValue delta) {
		while (true) {
			TValue oldValue = internalArray_[index];
			TValue newValue = oldValue + delta;
			if (internalArray_[index].compare_exchange_strong(oldValue, newValue)) {
				break;
			}
		}
	}

	void setAll(const TValue value) {
		for (size_t i = 0; i < size_; ++i) {
			internalArray_[i].store(value);
		}
	}
};