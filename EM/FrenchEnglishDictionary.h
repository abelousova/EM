#include <unordered_map>
#include <vector>

#include <boost/tokenizer.hpp>

#include <pugixml.hpp>

class FrenchEnglishDictionary {
private:
	std::unordered_map<std::string, std::vector<std::string>> internalMap_;

	void load() {
		pugi::xml_document doc;
		doc.load_file("dict.xdxf");
		pugi::xml_node node = doc.first_child();
		for (pugi::xml_node child : node.children()) {
			bool english = false;
			std::string frenchWord;
			std::string englishWord;
			for (pugi::xml_node arChild : child.children()) {
				if (!english) {
					frenchWord = arChild.child_value();
					english = true;
					continue;
				}
				englishWord = arChild.value();
				if (frenchWord.empty() || englishWord.empty()) {
					continue;
				}
				boost::tokenizer<> tokFr(frenchWord);
				size_t counter = 0;
				for (auto i = tokFr.begin(); i != tokFr.end(); ++i) {
					++counter;
					if (counter > 1) {
						break;
					}
				}
				if (counter > 1) {
					continue;
				}
				if (internalMap_.find(frenchWord) == internalMap_.end()) {
					internalMap_[frenchWord] = std::vector<std::string>();
				}

				std::string cpy(englishWord.cbegin() + 1, englishWord.cend());
				boost::tokenizer<> tok(cpy);
				internalMap_[frenchWord].push_back(*tok.begin());				
			}
		}
	}
public:
	FrenchEnglishDictionary() {
		load();
		std::cout << internalMap_.size() << std::endl;
	}

	bool isTranslation(std::string& frenchWord, std::string& englishWord) {
		if (internalMap_.find(frenchWord) != internalMap_.end()) {
			for (auto i = internalMap_[frenchWord].begin(); i != internalMap_[frenchWord].end(); ++i) {
				if (englishWord == *i) {
					return true;
				}
			}
		}
		return false;
	}
};