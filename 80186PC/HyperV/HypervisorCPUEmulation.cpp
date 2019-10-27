#include <HyperV/HypervisorCPUEmulation.h>
#include <Infrastructure/IAddressRangeHandler.h>
#include <Infrastructure/InterruptController.h>

HypervisorCPUEmulation::HypervisorCPUEmulation() : m_cpu0Emulator(&m_partition, 0, this) {
	WHV_PARTITION_PROPERTY prop;
	prop.ProcessorCount = 1;
	m_partition.setProperty(WHvPartitionPropertyCodeProcessorCount, prop);

	m_partition.setup();

	m_partition.createVirtualProcessor(0);

	WHV_REGISTER_NAME name = WHvX64RegisterCs;
	WHV_REGISTER_VALUE cs;
	cs.Segment.Base = 0x000F0000;
	cs.Segment.Limit = 0xFFFF;
	cs.Segment.Selector = 0xF000;
	cs.Segment.Attributes = 0x9B;

	m_partition.setRegisters(0, &name, 1, &cs);
}

HypervisorCPUEmulation::~HypervisorCPUEmulation() {
	m_partition.cancelRunVirtualProcessor(0);
	m_cpu0Thread.join();
}

void HypervisorCPUEmulation::start() {
	m_cpu0Thread = std::thread(&HypervisorCPUEmulation::cpu0Thread, this);
}

HRESULT HypervisorCPUEmulation::ioPortCallback(WHV_EMULATOR_IO_ACCESS_INFO& ioAccess) {
	auto handler = ioDispatcher();
	if (ioAccess.Direction) {
		handler->write(ioAccess.Port, ioAccess.AccessSize, ioAccess.Data);
	}
	else {
		ioAccess.Data = static_cast<uint32_t>(handler->read(ioAccess.Port, ioAccess.AccessSize));
	}
	return S_OK;
}

static inline uint64_t fetchU64(const uint8_t* bytes) {
	return
		static_cast<uint64_t>(bytes[0]) |
		(static_cast<uint64_t>(bytes[1]) << 8) |
		(static_cast<uint64_t>(bytes[2]) << 16) |
		(static_cast<uint64_t>(bytes[3]) << 24) |
		(static_cast<uint64_t>(bytes[4]) << 32) |
		(static_cast<uint64_t>(bytes[5]) << 40) |
		(static_cast<uint64_t>(bytes[6]) << 48) |
		(static_cast<uint64_t>(bytes[7]) << 56);
}

static inline void storeU64(uint64_t value, uint8_t* bytes) {
	bytes[0] = static_cast<uint8_t>(value);
	bytes[1] = static_cast<uint8_t>(value >> 8);
	bytes[2] = static_cast<uint8_t>(value >> 16);
	bytes[3] = static_cast<uint8_t>(value >> 24);
	bytes[4] = static_cast<uint8_t>(value >> 32);
	bytes[5] = static_cast<uint8_t>(value >> 40);
	bytes[6] = static_cast<uint8_t>(value >> 48);
	bytes[7] = static_cast<uint8_t>(value >> 56);
}

HRESULT HypervisorCPUEmulation::memoryCallback(WHV_EMULATOR_MEMORY_ACCESS_INFO& memoryAccess) {
	uint64_t resolvedAddress = memoryAccess.GpaAddress;
	auto handler = mmioDispatcher();

	if (memoryAccess.Direction) {
		handler->write(resolvedAddress, memoryAccess.AccessSize, fetchU64(memoryAccess.Data));
	}
	else {
		storeU64(handler->read(resolvedAddress, memoryAccess.AccessSize), memoryAccess.Data);
	}

	return S_OK;
}

void HypervisorCPUEmulation::mapMemory(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) {
	unsigned int flags = 0;

	if (permissions & IAddressRangeHandler::AccessRead)
		flags |= WHvMapGpaRangeFlagRead;

	if (permissions & IAddressRangeHandler::AccessWrite)
		flags |= WHvMapGpaRangeFlagWrite;

	if (permissions & IAddressRangeHandler::AccessExecute)
		flags |= WHvMapGpaRangeFlagExecute;

	m_partition.mapGpaRange(hostMemory, base, limit - base, static_cast<WHV_MAP_GPA_RANGE_FLAGS>(flags));
}

void HypervisorCPUEmulation::unmapMemory(uint64_t base, uint64_t limit) {
	m_partition.unmapGpaRange(base, limit - base);
}

void HypervisorCPUEmulation::cpu0Thread() {
	while (true) {
		WHV_RUN_VP_EXIT_CONTEXT exit;
		m_partition.runVirtualProcessor(0, exit);
		if (!m_cpu0Emulator.processExit(exit)) {
			switch (exit.ExitReason) {
			case WHvRunVpExitReasonX64InterruptWindow:
			{
				WHV_REGISTER_NAME reg = WHvRegisterPendingInterruption;
				WHV_REGISTER_VALUE val;
				memset(&val, 0, sizeof(val));
				val.PendingInterruption.InterruptionPending = 1;
				val.PendingInterruption.InterruptionType = WHvX64PendingInterrupt;
				val.PendingInterruption.InterruptionVector = interruptController()->processInterruptAcknowledge();
				if(val.PendingInterruption.InterruptionVector != 8)
					printf("Vectoring CPU at %02X\n", val.PendingInterruption.InterruptionVector);
				m_partition.setRegisters(0, &reg, 1, &val);
				break;
			}
			default:
				__debugbreak();
				break;
			}

		}
	}
}

void HypervisorCPUEmulation::setInterruptAsserted(bool interrupt) {
#if defined(TRACE_INTERRUPTS)
	printf("HypervisorCPUEmulation::setInterruptAsserted(%d)\n", interrupt);
#endif

	WHV_REGISTER_NAME reg = WHvX64RegisterDeliverabilityNotifications;
	WHV_REGISTER_VALUE val;
	memset(&val, 0, sizeof(val));
	val.DeliverabilityNotifications.InterruptNotification = interrupt;
	m_partition.setRegisters(0, &reg, 1, &val);
}
