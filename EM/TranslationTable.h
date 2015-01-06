#include <vector>
#include <fstream>

#include "Dictionary.h"

class TranslationTable {
private:
	std::vector<size_t> internalArray_;

public:
	void resize(const size_t size) {
		internalArray_.resize(size);
	}

	void addTranslation(const size_t englishWordId, const size_t frenchWordId) {
		if (englishWordId >= internalArray_.size()) {
			internalArray_.resize(2 * (internalArray_.size() + 1));
		}
		internalArray_[englishWordId] = frenchWordId;
	}

	size_t getTranslation(const size_t englishWordId) const {
		return internalArray_[englishWordId];
	}

	void save(Dictionary& englishDictionary, Dictionary& frenchDictionary) {
		std::ofstream out("TranslationTable.txt");
		for (size_t i = 1; i < englishDictionary.getSize(); ++i) {
			out << englishDictionary.getWord(i) << " ";
			out << frenchDictionary.getWord(internalArray_[i]);
			out << std::endl;
		}
	}

	void load(Dictionary& englishDictionary, Dictionary& frenchDictionary) {
		std::ifstream in("TranslationTable.txt");
		while (!in.eof()) {
			std::string englishWord;
			std::string frenchWord;

			in >> englishWord;
			in >> frenchWord;

			if (englishWord.empty()) {
				break;
			}

			addTranslation(englishDictionary.getId(englishWord), frenchDictionary.getId(frenchWord));
		}
	}
};