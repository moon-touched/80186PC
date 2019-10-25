#ifndef ACCESS_SIZE_UTILS_H
#define ACCESS_SIZE_UTILS_H

#include <stdint.h>

template<typename Word, typename Class>
void splitWriteAccess(uint64_t address, unsigned int accessSize, uint64_t data, void (Class::* writeWord)(uint64_t address, Word mask, Word word), Class* instance) {
	auto alignedAddress = address & ~7;
	auto misalignment = address & 7;
	auto mask = (1ULL << (8 * accessSize)) - 1;
	auto shiftedMask = mask << (8 * misalignment);
	data <<= (8 * misalignment);

	auto wordBits = sizeof(Word) * 8;
	auto subitemMask = (1ULL << wordBits) - 1;
	for (unsigned int subitem = 0; subitem < sizeof(uint64_t) / sizeof(Word); subitem++) {
		if (shiftedMask & subitemMask) {
			(instance->*writeWord)(alignedAddress + subitem * sizeof(Word), static_cast<Word>(shiftedMask), static_cast<Word>(data));
		}

		shiftedMask >>= wordBits;
		data >>= wordBits;

		if (shiftedMask == 0)
			break;
	}
}

template<typename Word, typename Class>
uint64_t splitReadAccess(uint64_t address, unsigned int accessSize, Word(Class::* readWord)(uint64_t address, Word mask), Class* instance) {
	auto alignedAddress = address & ~7;
	auto misalignment = address & 7;
	auto mask = (1ULL << (8 * accessSize)) - 1;
	auto shiftedMask = mask << (8 * misalignment);

	uint64_t data = 0;

	auto wordBits = sizeof(Word) * 8;
	auto subitemMask = (1ULL << wordBits) - 1;
	for (unsigned int subitem = 0; subitem < sizeof(uint64_t) / sizeof(Word); subitem++) {
		if (shiftedMask & subitemMask) {
			auto val = (instance->*readWord)(alignedAddress + subitem * sizeof(Word), static_cast<Word>(shiftedMask));
			val &= static_cast<Word>(shiftedMask);
			data |= static_cast<uint64_t>(val) << (subitem * wordBits);
		}

		shiftedMask >>= wordBits;

		if (shiftedMask == 0)
			break;
	}

	data >>= (8 * misalignment);

	return data;
}

#endif
