#ifndef HYPERVISOR_EMULATOR_H
#define HYPERVISOR_EMULATOR_H

#include <Windows.h>
#include <WinHvEmulation.h>

class HypervisorPartition;
class IHypervisorEmulatorCallbacks;

class HypervisorEmulator {
public:
	explicit HypervisorEmulator(HypervisorPartition* partition, unsigned int processorId, IHypervisorEmulatorCallbacks* callbacks);
	~HypervisorEmulator();

	HypervisorEmulator(const HypervisorEmulator& other) = delete;
	HypervisorEmulator& operator =(const HypervisorEmulator& other) = delete;

	bool processExit(WHV_RUN_VP_EXIT_CONTEXT& Context);

private:
	static HRESULT CALLBACK ioPortCallback(_In_ VOID* Context, _Inout_ WHV_EMULATOR_IO_ACCESS_INFO* IoAccess);
	static HRESULT CALLBACK memoryCallback(_In_ VOID* Context, _Inout_ WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess);
	static HRESULT CALLBACK getVirtualProcessorRegistersCallback(
		_In_ VOID* Context,
		_In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
		_In_ UINT32 RegisterCount,
		_Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
	);
	static HRESULT CALLBACK setVirtualProcessorRegistersCallback(
		_In_ VOID* Context,
		_In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
		_In_ UINT32 RegisterCount,
		_In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
	);
	static HRESULT CALLBACK translateGvaPageCallback(
		_In_ VOID* Context,
		_In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
		_In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
		_Out_ WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult,
		_Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
	);

	HypervisorPartition* m_partition;
	unsigned int m_processorId;
	IHypervisorEmulatorCallbacks* m_callbacks;
	WHV_EMULATOR_HANDLE m_emulator;
	static const WHV_EMULATOR_CALLBACKS m_callbackTable;
};


#endif
