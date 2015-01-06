#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <memory>

class Sentence {
private:
	std::vector<std::string> wordsVector;
	mutable size_t wordsVectorPtr;
public:
	Sentence(std::istream& in) :
		wordsVectorPtr(0)
	{
		std::stringstream wordsStream;
		std::string line;
		std::getline(in, line);
		wordsStream << line;

		while (!wordsStream.eof()) {
			std::string word;
			wordsStream >> word;
			std::transform(word.begin(), word.end(), word.begin(), ::tolower);
			wordsVector.push_back(word);
		}
	}

	std::string getNextWord() const {
		++wordsVectorPtr;
		return wordsVector[wordsVectorPtr - 1];
	}

	bool hasNext() const {
		return (wordsVectorPtr < wordsVector.size());
	}

	size_t getSize() const {
		return wordsVector.size();
	}

	void resetToBegin() const {
		wordsVectorPtr = 0;
	}
};