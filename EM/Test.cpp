#include <iostream>
#include <fstream>

#include <ThreadPool.h>

#include "Sentence.h"
#include "LockFreeHashTable.h"
#include "Dictionary.h"

class IBM1 {
	template<class T> using Array3D = std::vector<std::vector<std::vector<T>>>;
private:
	std::string inputName_;
	std::string outputName_;

	Dictionary englishDictionary_;
	Dictionary frenchDictionary_;

	LockFreeHashTable probability_;
	TwoDimensionalArray<TwoDimensionalArray<std::atomic<float>>> qJILM_;

	Array3D<size_t> alignmentEnFr_;
	Array3D<size_t> alignmentFrEn_;
	Array3D<size_t> alignment_;


	LockFreeHashTable counterEF_;
	std::atomic<float>* counterE_;
	TwoDimensionalArray<TwoDimensionalArray<std::atomic<float>>> counterJILM_;
	TwoDimensionalArray<std::atomic<float>*> counterILM_;

	ThreadPool<void> threadPool_;

	void initializeProbability() {
		std::ifstream input(inputName_);
		size_t countLines = 0;
		std::vector<std::future<void>> futureVector;

		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			futureVector.push_back(threadPool_.addTask([this, countLines, englishSentence, frenchSentence]() mutable {
				while (englishSentence.hasNext()) {
					std::string englishWord = englishSentence.getNextWord();
					size_t englishWordId = englishDictionary_.getId(englishWord);
					while (frenchSentence.hasNext()) {
						std::string frenchWord = frenchSentence.getNextWord();
						size_t frenchWordId = frenchDictionary_.getId(frenchWord);
						probability_.addWordPair(englishWordId, frenchWordId, 1.0f / ( frenchSentence.getSize()));
						counterEF_.addWordPair(englishWordId, frenchWordId, 0.0);
					}
					frenchSentence.resetToBegin();
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}		
		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		counterE_ = new std::atomic<float>[englishDictionary_.getSize() + 1];
		std::cout << std::endl <<  "Initialization finished" << std::endl;
	}

	static inline void loadBar(int x, int n, int r, int w)
	{
		if ((x != n) && (x % (n / 100 + 1) != 0)) return;

		float ratio = x / (float)n;
		int   c = ratio * w;

		std::cout << std::setw(3) << (int)(ratio * 100) << "% [";
		for (int x = 0; x<c; x++) std::cout << "=";
		for (int x = c; x<w; x++) std::cout << " ";
		std::cout << "]\r" << std::flush;
	}

	void increaseValue(std::atomic<float>& value, float deltaValue) {
		while (true) {
			float previousValue = value;
			float newValue = previousValue + deltaValue;
			if (value.compare_exchange_strong(previousValue, newValue)) {
				return;
			}
		}
	}

	void setCountersZero() {
		counterEF_.setAll(0.0f);
		for (size_t i = 0; i <= englishDictionary_.getSize(); ++i) {
			counterE_[i].store(0.0f);
		}
		for (size_t i = 0; i < 50; ++i) {
			for (size_t j = 0; j < 50; ++j) {
				for (size_t k = 0; k < 50; ++k) {
					counterILM_.get(i, j)[k].store(0.0f);
				}
			}
		}
		for (size_t i = 0; i < 50; ++i) {
			for (size_t j = 0; j < 50; ++j) {
				for (size_t k = 0; k < 50; ++k) {
					for (size_t l = 0; l < 50; ++l) {
						counterJILM_.get(i, j).get(k, l).store(0.0f);
					}
				}
			}
		}
	}

	void calculateCounters() {
		std::ifstream input(inputName_);
		size_t countLines = 0;

		std::vector<std::future<void>> futureVector;
		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			futureVector.push_back(threadPool_.addTask([this, englishSentence, frenchSentence]() {
				size_t frenchWordPosition = 0;
				while (frenchSentence.hasNext()) {
					size_t frenchWordId = frenchDictionary_.getId(frenchSentence.getNextWord());
					//count delta
					float deltaDenominator = 0.0f;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);
						deltaDenominator += probability_.getValue(englishWordId, frenchWordId);
					}
					englishSentence.resetToBegin();
					
					size_t englishWordPosition = 0;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);	
						//std::cout << "eng word id: " << englishWordId << std::endl;
						float delta = probability_.getValue(englishWordId, frenchWordId) / deltaDenominator;
						
						counterEF_.increaseValue(englishWordId, frenchWordId, delta);
						
						increaseValue(counterE_[englishWordId], delta);
						increaseValue(counterJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition), delta);
						increaseValue(counterILM_.get(englishSentence.getSize(), frenchSentence.getSize())[frenchWordPosition], delta);
						englishWordPosition++;
					}
					englishSentence.resetToBegin();
					frenchWordPosition++;
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}
		alignment_.resize(countLines + 1);
		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		std::cout << std::endl << "Counters calculated" << std::endl;
	}

	void calculateCountersIBM2() {
		std::ifstream input(inputName_);
		size_t countLines = 0;

		std::vector<std::future<void>> futureVector;
		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			futureVector.push_back(threadPool_.addTask([this, englishSentence, frenchSentence]() {
				size_t frenchWordPosition = 0;
				while (frenchSentence.hasNext()) {
					size_t frenchWordId = frenchDictionary_.getId(frenchSentence.getNextWord());
					
					float deltaDenominator = 0.0f; 
					size_t englishWordPosition = 0;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);
						deltaDenominator += qJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition) * probability_.getValue(englishWordId, frenchWordId);

						++englishWordPosition;
					}
					englishSentence.resetToBegin();

					englishWordPosition = 0;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);
						//std::cout << "eng word id: " << englishWordId << std::endl;
						float delta = qJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition) * probability_.getValue(englishWordId, frenchWordId) / deltaDenominator;

						counterEF_.increaseValue(englishWordId, frenchWordId, delta);

						increaseValue(counterE_[englishWordId], delta);
						increaseValue(counterJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition), delta);
						increaseValue(counterILM_.get(englishSentence.getSize(), frenchSentence.getSize())[frenchWordPosition], delta);
						englishWordPosition++;
					}
					englishSentence.resetToBegin();
					frenchWordPosition++;
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}
		alignmentEnFr_.resize(countLines + 1);
		alignmentFrEn_.resize(countLines + 1);
		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		std::cout << std::endl << "Counters calculated" << std::endl;
	}

	void recalculateProbabilities() {
		std::ifstream input(inputName_);
		size_t countLines = 0;

		std::vector<std::future<void>> futureVector;
		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			futureVector.push_back(threadPool_.addTask([this, englishSentence, frenchSentence]() {
				size_t frenchWordPosition = 0;
				while (frenchSentence.hasNext()) {
					size_t frenchWordId = frenchDictionary_.getId(frenchSentence.getNextWord());
					size_t englishWordPosition = 0;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);

						float newProbability = counterEF_.getValue(englishWordId, frenchWordId) / counterE_[englishWordId];
						probability_.setValue(englishWordId, frenchWordId, newProbability);
						
						float cJILM = counterJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition);
						float cILM = counterILM_.get(englishSentence.getSize(), frenchSentence.getSize())[frenchWordPosition];
						float newQ = cJILM / cILM;
						qJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition).store(newQ);

						englishWordPosition++;
					}
					englishSentence.resetToBegin();
					frenchWordPosition++;
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}

		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		std::cout << std::endl << "Probability recalculated" << std::endl;
	}

	void getAlignmentFrEn() {
		std::ifstream input(inputName_);
		size_t countLines = 0;

		std::vector<std::future<void>> futureVector;
		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			alignmentFrEn_[countLines].resize(frenchSentence.getSize());

			futureVector.push_back(threadPool_.addTask([this, countLines, englishSentence, frenchSentence]() {
				int frenchWordPosition = 0;
				while (frenchSentence.hasNext()) {	
					std::string frenchWord = frenchSentence.getNextWord();
					size_t frenchWordId = frenchDictionary_.getId(frenchWord);

					int maxProbabilityPosition = 0;
					float maxProbability = 0.0f;

					std::multimap<float, int> probabilities;
					int englishWordPosition = 0;
					while (englishSentence.hasNext()) {
						std::string englishWord = englishSentence.getNextWord();
						size_t englishWordId = englishDictionary_.getId(englishWord);				
						
						if (frenchWordPosition - englishWordPosition > 6) {
							englishWordPosition++;
							continue;
						}
						if (englishWordPosition - frenchWordPosition > 6) {
							break;
						}

						float probability = qJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition) * probability_.getValue(englishWordId, frenchWordId);
						if (probability > 0.005) {
							probabilities.emplace(probability, englishWordPosition);
						}
						englishWordPosition++;
					}
					size_t counter = 0;
					for (auto i = probabilities.rbegin(); i != probabilities.rend(); ++i) {
						alignmentFrEn_[countLines][frenchWordPosition].push_back(i->second);
						if (counter > 2) {
							break;
						}
					}

					englishSentence.resetToBegin();
					frenchWordPosition++;
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}

		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		std::cout << std::endl << "AlignmentFrEn counted" << std::endl;
	}

	void getAlignmentEnFr() {
		std::ifstream input(inputName_);
		size_t countLines = 0;

		std::vector<std::future<void>> futureVector;
		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);
			alignmentEnFr_[countLines].resize(englishSentence.getSize());

			futureVector.push_back(threadPool_.addTask([this, countLines, englishSentence, frenchSentence]() {
				int englishWordPosition = 0;
				while (englishSentence.hasNext()) {
					std::string englishWord = englishSentence.getNextWord();
					size_t englishWordId = englishDictionary_.getId(englishWord);

					std::multimap<float, int> probabilities;

					int frenchWordPosition = 0;
					while (frenchSentence.hasNext()) {
						std::string frenchWord = frenchSentence.getNextWord();
						size_t frenchWordId = frenchDictionary_.getId(frenchWord);

						if (englishWordPosition - frenchWordPosition > 6) {
							frenchWordPosition++;
							continue;
						}
						if (frenchWordPosition - englishWordPosition > 6) {
							break;
						}

						float probability = qJILM_.get(englishSentence.getSize(), frenchSentence.getSize()).get(englishWordPosition, frenchWordPosition) * probability_.getValue(englishWordId, frenchWordId);
						if (probability > 0.005) {
							probabilities.emplace(probability, frenchWordPosition);
						}
						frenchWordPosition++;
					}

					size_t counter = 0;
					for (auto i = probabilities.rbegin(); i != probabilities.rend(); ++i) {
						alignmentEnFr_[countLines][englishWordPosition].push_back(i->second);
						counter++;
						if (counter > 2) {
							break;
						}
					}

					frenchSentence.resetToBegin();
					englishWordPosition++;
				}
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			//std::cout << countLines << std::endl;
			loadBar(countLines, 33335, 20, 20);
		}
		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
		std::cout << std::endl << "AlignmentEnFr counted" << std::endl;
	}

	void findAlignmentIntersection() {
		for (size_t i = 0; i < alignmentFrEn_.size(); ++i) {
			alignment_[i].resize(alignmentFrEn_[i].size());
			for (size_t j = 0; j < alignmentFrEn_[i].size(); ++j) {				
				findIntersection(i, j);
			}
		}
	}

	template<class T>
	bool bucketHasValue(Array3D<T>& array, size_t row, size_t column, T value) {
		for (size_t i = 0; i < array[row][column].size(); ++i) {
			if (array[row][column][i] == value) {
				return true;
			}
		}
		return false;
	}

	void findIntersection(size_t sentenceNumber, size_t wordNumber){
		for (size_t k = 0; k < alignmentFrEn_[sentenceNumber][wordNumber].size(); ++k) {
			if (bucketHasValue(alignmentEnFr_, sentenceNumber, alignmentFrEn_[sentenceNumber][wordNumber][k], wordNumber)) {
				alignment_[sentenceNumber][wordNumber].push_back(alignmentFrEn_[sentenceNumber][wordNumber][k]);
			}
			if (alignment_[sentenceNumber][wordNumber].empty()) {

			}
		}
	}

	void printAlignment() {
		std::ofstream out(outputName_);
		for (size_t i = 0; i < alignment_.size(); ++i) {
			for (size_t j = 0; j < alignment_[i].size(); ++j) {
				//out << alignment_[i][j] << "-" << j << " ";
				
				for (auto iterator = alignment_[i][j].begin(); iterator != alignment_[i][j].end(); ++iterator) {
					out << *iterator << "-" << j << " ";
				}
			}
			out << std::endl;
		}
	}

	void setQEqual() {
		for (size_t i = 0; i < 50; ++i) {
			for (size_t j = 0; j < 50; ++j) {
				for (size_t k = 0; k < 50; ++k) {
					for (size_t l = 0; l < 50; ++l) {
						qJILM_.get(i, j).get(k, l).store(1.0f / 50);
					}
				}
			}
		}
	}

public:
	IBM1(std::string inputName, std::string outputName) :
		inputName_(inputName),
		outputName_(outputName),
		probability_(1000, 1000),
		qJILM_(50, 50),
		alignment_(),
		counterEF_(1000, 1000),
 		counterE_(),
		counterJILM_(50, 50),
		counterILM_(50, 50),
		threadPool_(4)
	{
		for (size_t i = 0; i < 50; ++i) {
			for (size_t j = 0; j < 50; ++j) {
				qJILM_.get(i, j).resize(50, 50);
				counterJILM_.get(i, j).resize(50, 50);
				counterILM_.get(i, j) = new std::atomic<float>[50];
			}
		}
	}

	void execute() {
		try {
			initializeProbability();
			for (size_t i = 0; i < 10; ++i) {
				setCountersZero();
				calculateCounters();
				recalculateProbabilities();
			}
			/*
			setQEqual();
			for (size_t i = 0; i < 10; ++i) {
				setCountersZero();
				calculateCountersIBM2();
				recalculateProbabilities();
			}*/

			getAlignmentEnFr();
			getAlignmentFrEn();
			findAlignmentIntersection();

			printAlignment();
		}
		catch (std::logic_error& e) {
			std::cout << e.what() << std::endl;
		}
	}
};

int main() {	
	IBM1 model("alignment-en-fr.txt", "output.txt");
	model.execute();
	return 0;
}