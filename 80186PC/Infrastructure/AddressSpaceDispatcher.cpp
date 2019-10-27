#include <Infrastructure/AddressSpaceDispatcher.h>
#include <Infrastructure/AddressRangeRegistration.h>
#include <Infrastructure/IAddressRangeHandler.h>

#include <Hardware/CPUEmulation.h>

#include <assert.h>

#include <stdexcept>
#include <algorithm>

#include <Windows.h>

AddressSpaceDispatcher::AddressSpaceDispatcher(std::string&& name) : m_name(std::move(name)), m_cpuEmulation(nullptr), m_addressSpaceBegin(0), m_addressSpaceLimit(0) {

}

AddressSpaceDispatcher::~AddressSpaceDispatcher() = default;

AddressRangeRegistration AddressSpaceDispatcher::registerAddressRange(uint64_t base, uint64_t limit, IAddressRangeHandler* handler, uint64_t offset) {
	printf("AddressSpaceDispatcher(%s): registering: %08llX - %08llX -> %p\n", m_name.c_str(), base, limit, handler);
	AddressRange range;
	range.beginAddress = base;
	range.endAddress = limit;
	range.handler = handler;
	range.offset = offset;

	auto existing = m_ranges.lower_bound(range);
	if (existing != m_ranges.begin() && (existing == m_ranges.end() || existing->beginAddress != base)) {
		--existing;
	}

	if (existing != m_ranges.end() && existing->beginAddress <= base && existing->endAddress >= limit) {
		if(IsDebuggerPresent())
			__debugbreak();
		throw std::runtime_error("address space conflict");
	}

	auto result = m_ranges.emplace(std::move(range));

	establishAddressRangeMappings(*result.first);
	
	return AddressRangeRegistration(*this, std::move(result.first));
}

void AddressSpaceDispatcher::establishAddressRangeMappings(const AddressRange& registration) {
	if (m_cpuEmulation) {
		auto hostBase = registration.handler->hostMemoryBase();
		if (hostBase) {
			auto hostPermissions = registration.handler->hostMemoryPermissions();
			printf("AddressSpaceDispatcher(%s): establishing mapping: %08llX - %08llX -> %p, permissions %u\n",
				m_name.c_str(), registration.beginAddress, registration.endAddress, hostBase, hostPermissions
			);

			auto length = registration.handler->hostMemorySize();

			for (auto mapBase = registration.beginAddress; mapBase < registration.endAddress; mapBase += length) {
				auto mapLimit = std::min(registration.endAddress, mapBase + length);
				m_cpuEmulation->mapMemory(
					std::min(mapBase + m_addressSpaceBegin, m_addressSpaceLimit),
					std::min(mapLimit + m_addressSpaceBegin, m_addressSpaceLimit),
					hostBase, hostPermissions);
			}
		}

		registration.handler->establishMappings(
			m_cpuEmulation,
			std::min(registration.beginAddress + m_addressSpaceBegin, m_addressSpaceLimit),
			std::min(registration.endAddress + m_addressSpaceBegin, m_addressSpaceLimit)
		);
	}

}

void AddressSpaceDispatcher::removeAddressRangeMappings(const AddressRange& registration) {
	if (m_cpuEmulation) {
		if (registration.handler->hostMemoryBase()) {
			auto hostPermissions = registration.handler->hostMemoryPermissions();
			printf("AddressSpaceDispatcher(%s): removing mapping: %08llX - %08llX\n",
				m_name.c_str(), registration.beginAddress, registration.endAddress
			);

			auto length = registration.handler->hostMemorySize();

			for (auto mapBase = registration.beginAddress; mapBase < registration.endAddress; mapBase += length) {
				auto mapLimit = std::min(registration.endAddress, mapBase + length);
				m_cpuEmulation->unmapMemory(
					std::min(mapBase + m_addressSpaceBegin, m_addressSpaceLimit),
					std::min(mapLimit + m_addressSpaceBegin, m_addressSpaceLimit));
			}
		}

		registration.handler->removeMappings(m_cpuEmulation,
			std::min(registration.beginAddress + m_addressSpaceBegin, m_addressSpaceLimit),
			std::min(registration.endAddress + m_addressSpaceBegin, m_addressSpaceLimit)
		);

	}

}

void AddressSpaceDispatcher::unregisterAddressRange(std::set<AddressRange>::const_iterator it) {
	const auto& registration = *it;

	removeAddressRangeMappings(registration);
	m_ranges.erase(it);
}


void AddressSpaceDispatcher::write(uint64_t address, unsigned int accessSize, uint64_t data) {
	uint64_t resolvedAddress = address;
	auto handler = resolve(resolvedAddress);
	if (!handler) {
		fprintf(stderr, "%s: no handler is defined for write to address %08llX (data %08llX, length %02X)\n", m_name.c_str(), address, data, accessSize);
		//if (IsDebuggerPresent() && (!m_cpuEmulation || address >= 0x08000000))
		//	__debugbreak();
	}
	else {
		handler->write(resolvedAddress, accessSize, data);
	}
}

uint64_t AddressSpaceDispatcher::read(uint64_t address, unsigned int accessSize) {
	uint64_t resolvedAddress = address;
	auto handler = resolve(resolvedAddress);

	if (!handler) {
		fprintf(stderr, "%s: no handler is defined for read from address %08llX (length %02X)\n", m_name.c_str(), address, accessSize);
		//if (IsDebuggerPresent() && (!m_cpuEmulation || address >= 0x08000000))
		//	__debugbreak();

		return 0xFFFFFFFF;
	}
	else {
		return handler->read(resolvedAddress, accessSize);
	}
}

IAddressRangeHandler* AddressSpaceDispatcher::resolve(uint64_t &address) {
	AddressRange request;
	request.beginAddress = address;
	auto handler = m_ranges.lower_bound(request);
	if (handler != m_ranges.begin() && (handler == m_ranges.end() || handler->beginAddress != address)) {
		--handler;
	}

	if (handler != m_ranges.end() && handler->beginAddress <= address && handler->endAddress > address) {
		address = address - handler->beginAddress + handler->offset;
		return handler->handler;
	}
	else {
		return nullptr;
	}
}

IAddressRangeHandler* AddressSpaceDispatcher::findHandlerForRange(uint32_t base) {
	AddressRange request;
	request.beginAddress = base;
	auto it = m_ranges.find(request);
	if (it == m_ranges.end())
		return nullptr;

	return it->handler;
}

void AddressSpaceDispatcher::establishMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) {
	m_cpuEmulation = emulation;
	m_addressSpaceBegin = base;
	m_addressSpaceLimit = limit;

	for (const auto& range : m_ranges) {
		establishAddressRangeMappings(range);
	}
}

void AddressSpaceDispatcher::removeMappings(CPUEmulation* emulation, uint64_t base, uint64_t limit) {
	for (const auto& range : m_ranges) {
		removeAddressRangeMappings(range);
	}

	m_cpuEmulation = nullptr;
	m_addressSpaceBegin = 0;
	m_addressSpaceLimit = 0;
}
