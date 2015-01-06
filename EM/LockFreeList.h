#include <string>
#include <atomic>

#include "Dictionary.h"

class LockFreeList {
private:
	struct Node {
		size_t englishWordId;
		size_t frenchWordId;
		std::atomic<float> value;
		std::atomic<Node*> next;

		Node(const size_t englishId, const size_t frenchId, const float value_) :
			englishWordId(englishId),
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
	LockFreeList() :
		null(new Node(0, 0, 0.0f)),
		head(null)
	{ }

	~LockFreeList() {
		deleteElementsFrom(head);
		delete null;
	}

	float getValue(const size_t englishWordId, const size_t frenchWordId) const {
		Node* currentNode = head;
		while (true) {
			if (currentNode->englishWordId == englishWordId && currentNode->frenchWordId == frenchWordId) {
				return currentNode->value;
			}
			currentNode = currentNode->next;
			if (currentNode == null) {
				throw std::logic_error("error in List.gefloat");
			}
		}
	}

	void setAll(float value) {
		Node* currentNode = head;
		while (currentNode != null) {
			currentNode->value.store(value);
			currentNode = currentNode->next;
		}
	}

	void setValue(const size_t englishWordId, const size_t frenchWordId, const float value) {
		Node* currentNode = head;
		while (true) {
			if (currentNode->englishWordId == englishWordId && currentNode->frenchWordId == frenchWordId) {
				currentNode->value.store(value);
				return;
			}
			currentNode = currentNode->next;
		}
	}

	void increaseValue(const size_t englishWordId, const size_t frenchWordId, const float deltaValue) {
		Node* currentNode = head;
		while (true) {
			if (currentNode->englishWordId == englishWordId && currentNode->frenchWordId == frenchWordId) {
				while (true) {
					float previousValue = currentNode->value;
					float newValue = previousValue + deltaValue;
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

	void addIfNotExist(const size_t englishWordId, const size_t frenchWordId, const float value) {
		Node* newNode = new Node(englishWordId, frenchWordId, value);
		newNode->next.store(null);

		Node* localNull = null;
		if (head.compare_exchange_strong(localNull, newNode)) {
			return;
		}
		
		localNull = null;
		Node* currentNode = head;
		while (true) {
			if (currentNode->englishWordId == englishWordId && currentNode->frenchWordId == frenchWordId) {
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

	size_t getSize(const size_t frenchWordId) {
		size_t size = 0;
		Node* currentNode = head;
		while (currentNode != null) {
			if (currentNode->frenchWordId == frenchWordId) {
				size++;
			}
			currentNode = currentNode->next;
		}
		return size;
	}

	void setAll(const size_t frenchWordId, const float value) {
		Node* currentNode = head;
		while (currentNode != null) {
			if (currentNode->frenchWordId == frenchWordId) {
				currentNode->value.store(value);
			}
			currentNode = currentNode->next;
		}
	}

	std::pair<float, size_t> findMax(size_t englishWordId) {
		float max = 0.0f;
		size_t frenchWordId = 0;
		Node* currentNode = head;
		while (currentNode != null) {
			if (currentNode->englishWordId == englishWordId) {
				if (currentNode->value > max) {
					max = currentNode->value;
					frenchWordId = currentNode->frenchWordId;
				}
			}
			currentNode = currentNode->next;
		}
		return std::pair<float, size_t>(max, frenchWordId);
	}

	void write(std::ofstream& out, Dictionary& englishDictionary, Dictionary& frenchDictionary) {
		Node* currentNode = head;
		while (currentNode != null) {
			out << englishDictionary.getWord(currentNode->englishWordId) << " ";
			out << frenchDictionary.getWord(currentNode->frenchWordId) << " ";
			out << currentNode->value << " ";
			currentNode = currentNode->next;
		}
	}

	void load(std::stringstream& in, Dictionary& englishDictionary, Dictionary& frenchDictionary) {
		while (!in.eof()) {
			std::string englishWord;
			std::string frenchWord;
			float value;

			in >> englishWord;
			in >> frenchWord;
			in >> value;

			if (englishWord == "") {
				break;
			}

			addIfNotExist(englishDictionary.getId(englishWord), frenchDictionary.getId(frenchWord), value);
		}
	}
};