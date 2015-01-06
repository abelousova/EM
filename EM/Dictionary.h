#pragma once

#include <unordered_map>
#include <boost\thread.hpp>

class Dictionary {
private:
	std::unordered_map<std::string, size_t> wordToIdMap_;
	std::unordered_map<size_t, std::string> IdToWordMap_;

	boost::shared_mutex mutex_;

	size_t IdCounter_;

	size_t getNewId() {
		return IdCounter_++;
	}

public:
	Dictionary() :
		IdCounter_(0)
	{ }

	~Dictionary() {
		std::cout << "Dictionary destroyed" << std::endl;
	}
	size_t getId(std::string& word) {
		boost::shared_lock<boost::shared_mutex> readLock(mutex_);
		auto wordIterator = wordToIdMap_.find(word);
		if (wordIterator != wordToIdMap_.end()) {
			return wordIterator->second;
		}
		else {			
			readLock.unlock();
			boost::unique_lock<boost::shared_mutex> writeLock(mutex_);
			auto wordNewIterator = wordToIdMap_.find(word);
			if (wordNewIterator != wordToIdMap_.end()) {
				return wordIterator->second;
			}
			else {
				size_t id = getNewId();
				wordToIdMap_.emplace(word, id);
				IdToWordMap_.emplace(id, word);
				return id;
			}
		}
	}

	std::string& getWord(size_t id) {
		boost::shared_lock<boost::shared_mutex> readLock(mutex_);
		auto wordIterator = IdToWordMap_.find(id);
		if (wordIterator != IdToWordMap_.end()) {
			return wordIterator->second;
		}
		else {
			throw std::logic_error("Invalid id" + std::to_string(id));
		}
	}

	size_t getSize() {
		return wordToIdMap_.size();
	}
};