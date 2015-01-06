#include "IBMModel.h"

int main() {
	Dictionary englishDictionary;
	Dictionary frenchDictionary;
	TranslationTable translationTable;
	translationTable.load(englishDictionary, frenchDictionary);
	std::ifstream input("translate.txt");
	std::ofstream output("translation.txt");
	while (!input.eof()) {
		Sentence englishSentence(input);
		std::vector<std::string> frenchSentence;
		while (englishSentence.hasNext()) {
			std::string englishWord = englishSentence.getNextWord();
			if (englishWord.empty()) {
				continue;
			}
			size_t frenchWordId = translationTable.getTranslation(englishDictionary.getId(englishWord));
			frenchSentence.push_back(frenchDictionary.getWord(frenchWordId));
		}
		for (auto i = frenchSentence.begin(); i != frenchSentence.end(); ++i) {
			output << *i << ' ';
		}
		output << std::endl;
	}
	return 0;
}