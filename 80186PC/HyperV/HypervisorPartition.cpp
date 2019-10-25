#include <HyperV/HypervisorPartition.h>

#include <comdef.h>

HypervisorPartition::HypervisorPartition() {
	auto hr = WHvCreatePartition(&m_partition);
	if (FAILED(hr))
		_com_raise_error(hr);
}

HypervisorPartition::~HypervisorPartition() {
	auto hr = WHvDeletePartition(&m_partition);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::getProperty(WHV_PARTITION_PROPERTY_CODE Code, WHV_PARTITION_PROPERTY& Property) {
	auto hr = WHvGetPartitionProperty(m_partition, Code, &Property, sizeof(Property), nullptr);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::setProperty(WHV_PARTITION_PROPERTY_CODE Code, const WHV_PARTITION_PROPERTY& Property) {
	auto hr = WHvSetPartitionProperty(m_partition, Code, &Property, sizeof(Property));
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::setup() {
	auto hr = WHvSetupPartition(m_partition);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::createVirtualProcessor(unsigned int index) {
	auto hr = WHvCreateVirtualProcessor(m_partition, index, 0);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::deleteVirtualProcessor(unsigned int index) {
	auto hr = WHvDeleteVirtualProcessor(m_partition, index);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::runVirtualProcessor(unsigned int index, WHV_RUN_VP_EXIT_CONTEXT& exit) {
	auto hr = WHvRunVirtualProcessor(m_partition, index, &exit, sizeof(exit));
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::cancelRunVirtualProcessor(unsigned int index) {
	auto hr = WHvCancelRunVirtualProcessor(m_partition, index, 0);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::getRegisters(unsigned int processorIndex, const WHV_REGISTER_NAME* names, unsigned int count, WHV_REGISTER_VALUE* values) {
	auto hr = WHvGetVirtualProcessorRegisters(m_partition, processorIndex, names, count, values);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::setRegisters(unsigned int processorIndex, const WHV_REGISTER_NAME* names, unsigned int count, const WHV_REGISTER_VALUE* values) {
	auto hr = WHvSetVirtualProcessorRegisters(m_partition, processorIndex, names, count, values);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::mapGpaRange(void* sourceAddress, WHV_GUEST_PHYSICAL_ADDRESS guestAddress, UINT64 sizeInBytes, WHV_MAP_GPA_RANGE_FLAGS flags) {
	auto hr = WHvMapGpaRange(m_partition, sourceAddress, guestAddress, sizeInBytes, flags);
	if (FAILED(hr))
		_com_raise_error(hr);
}

void HypervisorPartition::unmapGpaRange(WHV_GUEST_PHYSICAL_ADDRESS guestAddress, UINT64 sizeInBytes) {
	auto hr = WHvUnmapGpaRange(m_partition, guestAddress, sizeInBytes);
	if (FAILED(hr))
		_com_raise_error(hr);
}