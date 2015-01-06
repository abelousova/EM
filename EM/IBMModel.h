#include <iostream>
#include <fstream>
#include <future>
#include <random>

#include "Dictionary.h"
#include "FourDimensionalArray.h"
#include "AlignmentTable.h"
#include "OneDimensionalArray.h"
#include "ThreeDimensionalArray.h"
#include "Sentence.h"
#include "LockFreeHashTable.h"
#include "FrenchEnglishDictionary.h"
#include "TranslationTable.h"

const float EPSILON = 0.001;

class IBMModel {
	typedef float probabilityType;
private:
	std::string inputName_;
	std::string outputName_;

	Dictionary englishDictionary_;
	Dictionary frenchDictionary_;

	LockFreeHashTable probability_;
	FourDimensionalArray<probabilityType> qJILM_;

	AlignmentTable alignmentEnFr_;
	AlignmentTable alignmentFrEn_;
	AlignmentTable alignment_;


	LockFreeHashTable counterEF_;
	OneDimensionalArray<probabilityType> counterE_;
	FourDimensionalArray<probabilityType> counterJILM_;
	ThreeDimensionalArray<probabilityType> counterILM_;

	TranslationTable translationEnFr_;

	FrenchEnglishDictionary frenchEnglishDictionary_;
	AlignmentTable sureStandardAlignment_;
	AlignmentTable possibleStandardAlignment_;

	struct WordPairData {
		size_t sentenceNumber_;
		Sentence& englishSentence_;
		Sentence& frenchSentence_;
		size_t englishWordId_;
		size_t frenchWordId_;
		size_t englishWordPosition_;
		size_t frenchWordPosition_;

		WordPairData(size_t sentenceNumber, Sentence& englishSentence, Sentence& frenchSentence, size_t englishWordId,
			size_t frenchWordId, size_t englishWordPosition, size_t frenchWordPosition) :
			sentenceNumber_(sentenceNumber),
			englishSentence_(englishSentence),
			frenchSentence_(frenchSentence),
			englishWordId_(englishWordId),
			frenchWordId_(frenchWordId),
			englishWordPosition_(englishWordPosition),
			frenchWordPosition_(frenchWordPosition)
		{ }
	};

	template<class T>
	void applyForEachSentencePair(T function) {
		std::ifstream input(inputName_);
		size_t countLines = 0;
		std::vector<std::future<void>> futureVector;


		while (!input.eof()) {
			Sentence englishSentence(input);
			Sentence frenchSentence(input);

			futureVector.push_back(std::async([this, countLines, englishSentence, frenchSentence, &function]() mutable {
				WordPairData wordPairData(countLines, englishSentence, frenchSentence, 0, 0, 0, 0);
				function(wordPairData);
			}));
			Sentence alignment(input); // skip alignment
			++countLines;
			loadBar(countLines, 33335, 20, 20);
		}
		std::for_each(futureVector.begin(), futureVector.end(), std::mem_fn(&std::future<void>::wait));
	}

	template<class T>
	void doForAllFrenchWords(WordPairData& wordPairData, T function) {
		size_t frenchWordPosition = 0;
		while (wordPairData.frenchSentence_.hasNext()) {
			std::string frenchWord = wordPairData.frenchSentence_.getNextWord();
			size_t frenchWordId = frenchDictionary_.getId(frenchWord);

			wordPairData.frenchWordId_ = frenchWordId;
			wordPairData.frenchWordPosition_ = frenchWordPosition;
			function(wordPairData);

			++frenchWordPosition;
		}
		wordPairData.frenchSentence_.resetToBegin();
	}

	template<class T>
	void doForAllEnglishWords(WordPairData& wordPairData, T function) {
		size_t englishWordPosition = 0;
		while (wordPairData.englishSentence_.hasNext()) {
			std::string englishWord = wordPairData.englishSentence_.getNextWord();
			size_t englishWordId = englishDictionary_.getId(englishWord);

			wordPairData.englishWordId_ = englishWordId;
			wordPairData.englishWordPosition_ = englishWordPosition;
			function(wordPairData);

			++englishWordPosition;
		}
		wordPairData.englishSentence_.resetToBegin();
	}

	void initializeProbability() {
		std::atomic<size_t> sentenceCount = 0;
		applyForEachSentencePair([this, &sentenceCount](WordPairData& wordPairData) {
			doForAllEnglishWords(wordPairData, [this](WordPairData& wordPairData) {
				doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t frenchSentenceSize = wordPairData.frenchSentence_.getSize();

					probability_.addWordPair(englishWordId, frenchWordId, 0.0);
					counterEF_.addWordPair(englishWordId, frenchWordId, 0.0);
				});
			});
			++sentenceCount;
		});

		probability_.setAllUniform(frenchDictionary_.getSize() + 1);

		counterE_.resize(englishDictionary_.getSize() + 1);
		alignmentEnFr_.resize(sentenceCount + 1);
		alignmentFrEn_.resize(sentenceCount + 1);
		alignment_.resize(sentenceCount + 1);
		sureStandardAlignment_.resize(sentenceCount + 1);
		possibleStandardAlignment_.resize(sentenceCount + 1);


		std::cout << englishDictionary_.getSize() << std::endl;
		std::cout << frenchDictionary_.getSize() << std::endl;
		std::cout << std::endl << "Initialization finished" << std::endl;
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

	void setCountersToZero() {
		counterEF_.setAll(0.0);
		counterE_.setAll(0.0);
		counterILM_.setAll(0.0);
		counterJILM_.setAll(0.0);
	}

	void calculateCounters() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
				float deltaDenominator = 0.0f;
				doForAllEnglishWords(wordPairData, [this, &deltaDenominator](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					deltaDenominator += probability_.getValue(englishWordId, frenchWordId);
				});
				//std::cout << deltaDenominator << std::endl;
				doForAllEnglishWords(wordPairData, [this, &deltaDenominator](WordPairData& wordPairData) {
					auto &wpd = wordPairData;
					/*
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();
					*/

					float delta = probability_.getValue(wpd.englishWordId_, wpd.frenchWordId_) / deltaDenominator;
					//std::cout << delta;
					counterEF_.increaseValue(wpd.englishWordId_, wpd.frenchWordId_, delta);
					counterE_.increase(wpd.englishWordId_, delta);
					counterJILM_.increase(wpd.englishSentence_.getSize(), wpd.frenchSentence_.getSize(), wpd.englishWordPosition_, wpd.frenchWordPosition_, delta);
					counterILM_.increase(wpd.englishSentence_.getSize(), wpd.frenchSentence_.getSize(), wpd.frenchWordPosition_, delta);
				});
			});
		});
		std::cout << std::endl << "Counters calculated" << std::endl;
	}

	void calculateCountersIBM2() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
				float deltaDenominator = 0.0f;
				doForAllEnglishWords(wordPairData, [this, &deltaDenominator](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();

					deltaDenominator += qJILM_.get(englishSize, frenchSize, englishWordPosition, frenchWordPosition) * probability_.getValue(englishWordId, frenchWordId);
				});
				doForAllEnglishWords(wordPairData, [this, &deltaDenominator](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();

					float delta = qJILM_.get(englishSize, frenchSize, englishWordPosition, frenchWordPosition) *probability_.getValue(englishWordId, frenchWordId) / deltaDenominator;
					counterEF_.increaseValue(englishWordId, frenchWordId, delta);
					counterE_.increase(englishWordId, delta);
					counterJILM_.increase(englishSize, frenchSize, englishWordPosition, frenchWordPosition, delta);
					counterILM_.increase(englishSize, frenchSize, frenchWordPosition, delta);
				});
			});
		});
		std::cout << std::endl << "Counters calculated" << std::endl;
	}

	void recalculateProbabilities() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
				doForAllEnglishWords(wordPairData, [this](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();

					float newProbability = counterEF_.getValue(englishWordId, frenchWordId) / counterE_.get(englishWordId);
					probability_.setValue(englishWordId, frenchWordId, newProbability);

					float cJILM = counterJILM_.get(englishSize, frenchSize, englishWordPosition, frenchWordPosition);
					float cILM = counterILM_.get(englishSize, frenchSize, frenchWordPosition);
					float newQ = cJILM / cILM;
					qJILM_.put(englishSize, frenchSize, englishWordPosition, frenchWordPosition, newQ);
				});
			});
		});
		std::cout << std::endl << "Probability recalculated" << std::endl;
	}

	void getAlignmentFrEn() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
				alignmentFrEn_.resize(wordPairData.sentenceNumber_, wordPairData.frenchSentence_.getSize());

				std::multimap<float, int> probabilities;
				float maxProbability = 0.0f;
				size_t maxProbabilityPosition = 0;
				doForAllEnglishWords(wordPairData, [this, &maxProbability, &maxProbabilityPosition, &probabilities](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();

					if (std::abs((int)frenchWordPosition - (int)englishWordPosition) > 6) {
						englishWordPosition++;
						return;
					}

					float probability = probability_.getValue(englishWordId, frenchWordId) * qJILM_.get(englishSize, frenchSize, englishWordPosition, frenchWordPosition);

					if (probability > EPSILON) {
						probabilities.emplace(probability, englishWordPosition);
					}
					if (probability > maxProbability) {
						maxProbability = maxProbability;
						maxProbabilityPosition = englishWordPosition;
					}
				});

				size_t counter = 0;
				if (probabilities.empty()) {
					alignmentFrEn_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, maxProbabilityPosition);
				}
				for (auto i = probabilities.rbegin(); i != probabilities.rend(); ++i) {
					alignmentFrEn_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, i->second);
					++counter;
					if (counter > 2) {
						break;
					}
				}
			});
		});
		std::cout << std::endl << "AlignmentFrEn counted" << std::endl;
	}

	void getAlignmentEnFr() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllEnglishWords(wordPairData, [this](WordPairData& wordPairData) {
				alignmentEnFr_.resize(wordPairData.sentenceNumber_, wordPairData.englishSentence_.getSize() + 1);

				std::multimap<float, int> probabilities;
				float maxProbability = 0.0f;
				size_t maxProbabilityPosition = 0;
				doForAllFrenchWords(wordPairData, [this, &maxProbability, &maxProbabilityPosition, &probabilities](WordPairData& wordPairData) {
					size_t englishWordId = wordPairData.englishWordId_;
					size_t frenchWordId = wordPairData.frenchWordId_;
					size_t englishWordPosition = wordPairData.englishWordPosition_;
					size_t frenchWordPosition = wordPairData.frenchWordPosition_;
					size_t englishSize = wordPairData.englishSentence_.getSize();
					size_t frenchSize = wordPairData.frenchSentence_.getSize();

					if (std::abs((int)frenchWordPosition - (int)englishWordPosition) > 6) {
						englishWordPosition++;
						return;
					}

					float probability = probability_.getValue(englishWordId, frenchWordId) * qJILM_.get(englishSize, frenchSize, englishWordPosition, frenchWordPosition);
					if (probability > EPSILON) {
						probabilities.emplace(probability, frenchWordPosition);
					}
					if (probability > maxProbability) {
						maxProbability = maxProbability;
						maxProbabilityPosition = frenchWordPosition;
					}
				});
				size_t counter = 0;
				if (probabilities.empty()) {
					alignmentEnFr_.put(wordPairData.sentenceNumber_, wordPairData.englishWordPosition_, maxProbabilityPosition);
				}
				for (auto i = probabilities.rbegin(); i != probabilities.rend(); ++i) {
					alignmentEnFr_.put(wordPairData.sentenceNumber_, wordPairData.englishWordPosition_, i->second);
					++counter;
					if (counter > 2) {
						break;
					}
				}
			});
		});
		std::cout << std::endl << "AlignmentEnFr counted" << std::endl;
	}

	void printAlignment() {
		std::ofstream out(outputName_);
		alignment_.print(out);
	}

	void findAlignmentIntersection() {
		findIntersectionDifferentLanguages(alignmentFrEn_, alignmentEnFr_, alignment_);
	}

	void refindEmptyAlignmentBuckets() {
		refindEmptyBuckets(alignmentFrEn_, alignmentEnFr_, qJILM_, alignment_);
	}

	void findTranslationTable() {
		translationEnFr_.resize(englishDictionary_.getSize());
		for (size_t i = 1; i < englishDictionary_.getSize(); ++i) {
			size_t translation = probability_.findMax(i);
			translationEnFr_.addTranslation(i, translation);
		}
	}

	void saveTranslationTable() {
		translationEnFr_.save(englishDictionary_, frenchDictionary_);
	}

	void findStandardAlignment() {
		applyForEachSentencePair([this](WordPairData& wordPairData) {
			doForAllFrenchWords(wordPairData, [this](WordPairData& wordPairData) {
				sureStandardAlignment_.resize(wordPairData.sentenceNumber_, wordPairData.frenchSentence_.getSize());
				possibleStandardAlignment_.resize(wordPairData.sentenceNumber_, wordPairData.frenchSentence_.getSize());

				std::string frenchWord = frenchDictionary_.getWord(wordPairData.frenchWordId_);
				bool translated = false;
				doForAllEnglishWords(wordPairData, [this, &frenchWord, &translated](WordPairData& wordPairData) {
					if (std::abs((int)wordPairData.frenchWordPosition_ - (int)wordPairData.englishWordPosition_) > 4) {
						return;
					}
					std::string englishWord = englishDictionary_.getWord(wordPairData.englishWordId_);
					if (frenchEnglishDictionary_.isTranslation(frenchWord, englishWord)) {
						sureStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, wordPairData.englishWordPosition_);
						translated = true;
					}
				});
				if (!translated) {
					std::default_random_engine generator;
					int leftBorder = std::max((int)wordPairData.frenchWordPosition_ - 3, 0);
					int rightBorder = std::min((int)wordPairData.frenchWordPosition_ + 3, (int)wordPairData.englishSentence_.getSize() - 1);
					std::uniform_int_distribution<int> distribution(leftBorder, rightBorder);
					int translation1 = distribution(generator);
					int translation2 = distribution(generator);
					int translation3 = distribution(generator);
					int translation4 = distribution(generator);
					int translation5 = distribution(generator);
					possibleStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, translation1);
					possibleStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, translation2);
					possibleStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, translation3);
					possibleStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, translation4);
					possibleStandardAlignment_.put(wordPairData.sentenceNumber_, wordPairData.frenchWordPosition_, translation5);
				}
			});
		});
	}

	float calculateAlignmentErrorRate() {
		findStandardAlignment();
		float aIntersectS = 1.0f * findIntersectionSize(alignment_, sureStandardAlignment_);
		//std::cout << aIntersectS << std::endl;
		float aIntersectP = 1.0f * findIntersectionSize(alignment_, possibleStandardAlignment_);
		//std::cout << aIntersectP << std::endl;
		//std::cout << sureStandardAlignment_.getSize() << std::endl;
		//std::cout << alignment_.getSize() << std::endl;
		float aer = 1.0f - ((aIntersectP + aIntersectS) / (sureStandardAlignment_.getSize() + alignment_.getSize()));
		return aer;
	}

public:
	IBMModel(std::string inputName, std::string outputName) :
		inputName_(inputName),
		outputName_(outputName),
		probability_(1000, 1000),
		qJILM_(50, 50, 50, 50),
		counterEF_(1000, 1000),
		counterJILM_(50, 50, 50, 50),
		counterILM_(50, 50, 50)
	{ }

	void execute() {
		try {
			initializeProbability();
			for (size_t i = 0; i < 3; ++i) {
				setCountersToZero();
				calculateCounters();
				recalculateProbabilities();
			}
			qJILM_.setAllUniform();
			for (size_t i = 0; i < 3; ++i) {
				setCountersToZero();
				calculateCountersIBM2();
				recalculateProbabilities();
			}

			getAlignmentEnFr();
			getAlignmentFrEn();
			findAlignmentIntersection();
			refindEmptyAlignmentBuckets();

			printAlignment();

			float aer = calculateAlignmentErrorRate();
			std::cout << "Alignment Error Rate: " << aer << std::endl;

			findTranslationTable();
			saveTranslationTable();
		}
		catch (std::logic_error& e) {
			std::cout << e.what() << std::endl;
		}
	}
};