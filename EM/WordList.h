#include <string>
#include <atomic>

template <class TValue>
class WordList {
private:
	struct Node {
		size_t frenchWordId;
		std::atomic<TValue> value;
		std::atomic<Node*> next;

		Node(const size_t frenchId, const TValue value_) :
			frenchWordId(frenchId),
			value(value_)
		{ }
	};

	Node* null;
	std::atomic<Node*> head;

	void deleteElementsFrom(Node* firstElement) {
		if (firstElement == null) {
			return;
		}
		if (firstElement->next == null) {
			delete firstElement;
		}
		else {
			deleteElementsFrom(firstElement->next);
			delete firstElement;
		}
	}

public:
	WordList() :
		null(new Node(0, 0.0f)),
		head(null)
	{ }

	~WordList() {
		deleteElementsFrom(head);
		delete null;
	}

	TValue getValue(const size_t frenchWordId) const {
		Node* currentNode = head;
		while (true) {
			if (currentNode->frenchWordId == frenchWordId) {
				return currentNode->value;
			}
			currentNode = currentNode->next;
			if (currentNode == null) {
				throw std::logic_error("error in List.getValue");
			}
		}
	}

	void setAll(const TValue value) {
		Node* currentNode = head;
		while (currentNode != null) {
			currentNode->value.store(value);
			currentNode = currentNode->next;
		}
	}

	void setValue(const size_t frenchWordId, const TValue value) {
		Node* currentNode = head;
		while (true) {
			if (currentNode->frenchWordId == frenchWordId) {
				currentNode->value.store(value);
				return;
			}
			currentNode = currentNode->next;
		}
	}

	void increaseValue(const size_t frenchWordId, const TValue deltaValue) {
		Node* currentNode = head;
		while (true) {
			if (currentNode->frenchWordId == frenchWordId) {
				while (true) {
					TValue previousValue = currentNode->value;
					TValue newValue = previousValue + deltaValue;
					if (currentNode->value.compare_exchange_strong(previousValue, newValue)) {
						return;
					}
				}
			}
			currentNode = currentNode->next;
			if (currentNode == null) {
				throw std::logic_error("error in List.increaseValue");
			}
		}
	}

	void addIfNotExist(const size_t frenchWordId, const TValue value) {
		Node* newNode = new Node(frenchWordId, value);
		newNode->next.store(null);

		Node* localNull = null;
		if (head.compare_exchange_strong(localNull, newNode)) {
			return;
		}

		localNull = null;
		Node* currentNode = head;
		while (true) {
			if (currentNode->frenchWordId == frenchWordId) {
				delete newNode;
				return;
			}
			if (currentNode->next.compare_exchange_strong(localNull, newNode)) {
				return;
			}
			localNull = null;
			currentNode = currentNode->next;
			if (currentNode == null) {
				throw std::logic_error("error in List.addIfNotExist");
			}
		}
	}

	size_t getSize() const {
		size_t size = 0;
		Node* currentNode = head;
		while (true) {
			if (currentNode == null) {
				return size;
			}
			++size;
			currentNode = currentNode->next;
		}
	}
};