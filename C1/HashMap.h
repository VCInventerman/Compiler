#ifndef COMPILER_HASHTABLE_H
#define COMPILER_HASHTABLE_H

#include <memory> // unique_ptr
#include <string_view>
#include <string_view>
#include <functional> // hash

template <typename ValT>
class HashMap {
public:
	struct ListEntry {
		std::string_view id;
		ValT val;
		ListEntry* next = nullptr;
	};

private:
	constexpr static int DEFAULT_BUCKET_COUNT = 10;

	int bucketCount = 0;
	std::unique_ptr<ListEntry[]> buckets;

	int hash(std::string_view id) {
		std::size_t hash = std::hash<std::string_view>{}(id);
		return hash % bucketCount;
	}

public:
	HashMap(int bucketCount_ = DEFAULT_BUCKET_COUNT) {
		resize(bucketCount_);
	}

	void resize(int newBucketCount) {
		auto oldPtr = std::move(buckets);
		buckets = std::make_unique<ListEntry[]>(newBucketCount);

		//todo: rehash every element

		bucketCount = newBucketCount;
	}

	// Inserts a new element, returning whether it replaced an existing one
	bool update(std::string_view id, ValT entry) {
		int hash = this->hash(id);
		ListEntry* ptr = buckets[hash];

		if (ptr == nullptr) {
			auto newEntry = new ListEntry{ id, entry };
			buckets[hash] = newEntry;
			return false;
		}
		else {
			while (ptr->id != id && ptr->next) {
				ptr = ptr->next;
			}

			if (ptr->id == id) {
				ptr->val = entry;
				return true;
			}
			else {
				ptr->next = new ListEntry{ id, entry };
				return false;
			}
		}
	}

	ValT* get(std::string_view id) {
		ListEntry* ptr = buckets[hash(id)];

		while (ptr != nullptr && ptr->id != id) {
			ptr = ptr->next;
		}

		return ptr;
	}
};

#endif