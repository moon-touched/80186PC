#ifndef HYPERVISOR_PARTITION_H
#define HYPERVISOR_PARTITION_H

#include <Windows.h>
#include <WinHvPlatform.h>

class HypervisorEmulator;

class HypervisorPartition {
public:
	HypervisorPartition();
	~HypervisorPartition();

	HypervisorPartition(const HypervisorPartition& other) = delete;
	HypervisorPartition& operator =(const HypervisorPartition& other) = delete;

	void getProperty(WHV_PARTITION_PROPERTY_CODE Code, WHV_PARTITION_PROPERTY& Property);
	void setProperty(WHV_PARTITION_PROPERTY_CODE Code, const WHV_PARTITION_PROPERTY& Property);

	void setup();

	void createVirtualProcessor(unsigned int index);
	void deleteVirtualProcessor(unsigned int index);

	void runVirtualProcessor(unsigned int index, WHV_RUN_VP_EXIT_CONTEXT& exit);
	void cancelRunVirtualProcessor(unsigned int index);

	void getRegisters(unsigned int processorIndex, const WHV_REGISTER_NAME* names, unsigned int count, WHV_REGISTER_VALUE* values);
	void setRegisters(unsigned int processorIndex, const WHV_REGISTER_NAME* names, unsigned int count, const WHV_REGISTER_VALUE* values);

	void mapGpaRange(void* sourceAddress, WHV_GUEST_PHYSICAL_ADDRESS guestAddress, UINT64 sizeInBytes, WHV_MAP_GPA_RANGE_FLAGS flags);
	void unmapGpaRange(WHV_GUEST_PHYSICAL_ADDRESS guestAddress, UINT64 sizeInBytes);

private:
	friend class HypervisorEmulator;

	WHV_PARTITION_HANDLE m_partition;
};


#endif
