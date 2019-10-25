#ifndef HYPERVISOR_CPU_EMULATION_H
#define HYPERVISOR_CPU_EMULATION_H

#include <thread>

#include <Hardware/CPUEmulation.h>
#include <HyperV/IHypervisorEmulatorCallbacks.h>
#include <HyperV/HypervisorPartition.h>
#include <HyperV/HypervisorEmulator.h>

class HypervisorCPUEmulation final : public CPUEmulation, private IHypervisorEmulatorCallbacks {
public:
	HypervisorCPUEmulation();
	~HypervisorCPUEmulation() override;

	void start() override;

	void mapMemory(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) override;
	void unmapMemory(uint64_t base, uint64_t limit) override;

	void setInterruptAsserted(bool interrupt) override;

private:
	HRESULT ioPortCallback(WHV_EMULATOR_IO_ACCESS_INFO& ioAccess) override;
	HRESULT memoryCallback(WHV_EMULATOR_MEMORY_ACCESS_INFO& memoryAccess) override;

	void cpu0Thread();

	HypervisorPartition m_partition;
	HypervisorEmulator m_cpu0Emulator;
	std::thread m_cpu0Thread;
};

#endif
