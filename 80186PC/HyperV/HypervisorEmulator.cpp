#include <HyperV/HypervisorEmulator.h>
#include <HyperV/IHypervisorEmulatorCallbacks.h>
#include <HyperV/HypervisorPartition.h>

#include <comdef.h>

HypervisorEmulator::HypervisorEmulator(HypervisorPartition* partition, unsigned int processorId, IHypervisorEmulatorCallbacks* callbacks) : m_partition(partition), m_processorId(0), m_callbacks(callbacks) {
	auto hr = WHvEmulatorCreateEmulator(&m_callbackTable, &m_emulator);
	if (FAILED(hr))
		_com_raise_error(hr);
}

HypervisorEmulator::~HypervisorEmulator() {
	auto hr = WHvEmulatorDestroyEmulator(m_emulator);
	if (FAILED(hr))
		_com_raise_error(hr);
}


HRESULT HypervisorEmulator::ioPortCallback(_In_ VOID* Context, _Inout_ WHV_EMULATOR_IO_ACCESS_INFO* IoAccess) {
	auto this_ = static_cast<HypervisorEmulator*>(Context);
	return this_->m_callbacks->ioPortCallback(*IoAccess);
}

HRESULT HypervisorEmulator::memoryCallback(_In_ VOID* Context, _Inout_ WHV_EMULATOR_MEMORY_ACCESS_INFO* MemoryAccess) {
	auto this_ = static_cast<HypervisorEmulator*>(Context);
	return this_->m_callbacks->memoryCallback(*MemoryAccess);
}

HRESULT CALLBACK HypervisorEmulator::getVirtualProcessorRegistersCallback(
	_In_ VOID* Context,
	_In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
	_In_ UINT32 RegisterCount,
	_Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
) {
	auto this_ = static_cast<HypervisorEmulator*>(Context);
	return WHvGetVirtualProcessorRegisters(this_->m_partition->m_partition, this_->m_processorId, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT CALLBACK HypervisorEmulator::setVirtualProcessorRegistersCallback(
	_In_ VOID* Context,
	_In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
	_In_ UINT32 RegisterCount,
	_In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
) {
	auto this_ = static_cast<HypervisorEmulator*>(Context);
	return WHvSetVirtualProcessorRegisters(this_->m_partition->m_partition, this_->m_processorId, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT CALLBACK HypervisorEmulator::translateGvaPageCallback(
	_In_ VOID* Context,
	_In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
	_In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
	_Out_ WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult,
	_Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
) {
	auto this_ = static_cast<HypervisorEmulator*>(Context);
	WHV_TRANSLATE_GVA_RESULT res;
	auto hr = WHvTranslateGva(this_->m_partition->m_partition, this_->m_processorId, Gva, TranslateFlags, &res, Gpa);
	*TranslationResult = res.ResultCode;
	return hr;
}

bool HypervisorEmulator::processExit(WHV_RUN_VP_EXIT_CONTEXT& Context) {
	HRESULT hr;
	WHV_EMULATOR_STATUS status;

	bool retry = false;

	switch (Context.ExitReason) {
	case WHvRunVpExitReasonMemoryAccess:
		do {
			hr = WHvEmulatorTryMmioEmulation(m_emulator, this, &Context.VpContext, &Context.MemoryAccess, &status);
			if (FAILED(hr))
				_com_raise_error(hr);

			if (!status.EmulationSuccessful) {
				fprintf(stderr, "MMIO emulation failure, RIP %08llX accessing GVA %08llX / GPA %08llX\n", Context.VpContext.Rip, Context.MemoryAccess.Gva, Context.MemoryAccess.Gpa);

				if (retry) {
					if (IsDebuggerPresent())
						__debugbreak();
					return false;
				}
				else {
					if(Context.VpContext.InstructionLength == 2 &&
						Context.MemoryAccess.InstructionByteCount >= 7 &&
						Context.MemoryAccess.InstructionBytes[0] == 0x83 &&
						Context.MemoryAccess.InstructionBytes[1] == 0x25 &&
						Context.MemoryAccess.InstructionBytes[6] == 0x00) {
						fprintf(stderr, "Applying MMIO hack: and dword [memory], #0\n");

						WHV_EMULATOR_MEMORY_ACCESS_INFO fakeAccess;
						fakeAccess.AccessSize = 4;
						memset(fakeAccess.Data, 0, sizeof(fakeAccess.Data));
						fakeAccess.Direction = 1;
						fakeAccess.GpaAddress = Context.MemoryAccess.Gpa;
						m_callbacks->memoryCallback(fakeAccess);

						WHV_REGISTER_NAME reg = WHvX64RegisterRip;
						WHV_REGISTER_VALUE regval;
						m_partition->getRegisters(0, &reg, 1, &regval);
						regval.Reg64 += 7;
						m_partition->setRegisters(0, &reg, 1, &regval);
						return true;
					} else if (Context.VpContext.InstructionLength == 2 &&
						Context.MemoryAccess.InstructionByteCount >= 7 &&
						Context.MemoryAccess.InstructionBytes[0] == 0x83 &&
						Context.MemoryAccess.InstructionBytes[1] == 0x0d &&
						Context.MemoryAccess.InstructionBytes[6] == 0xff) {
						fprintf(stderr, "Applying MMIO hack: or dword [memory], #0xFFFFFFFF\n");

						WHV_EMULATOR_MEMORY_ACCESS_INFO fakeAccess;
						fakeAccess.AccessSize = 4;
						memset(fakeAccess.Data, 0xFF, sizeof(fakeAccess.Data));
						fakeAccess.Direction = 1;
						fakeAccess.GpaAddress = Context.MemoryAccess.Gpa;
						m_callbacks->memoryCallback(fakeAccess);

						WHV_REGISTER_NAME reg = WHvX64RegisterRip;
						WHV_REGISTER_VALUE regval;
						m_partition->getRegisters(0, &reg, 1, &regval);
						regval.Reg64 += 7;
						m_partition->setRegisters(0, &reg, 1, &regval);
						return true;
					}
					else {
						WHV_REGISTER_NAME regs[2]{ WHvX64RegisterRsp, WHvX64RegisterSs };
						WHV_REGISTER_VALUE regvals[2];
						m_partition->getRegisters(0, regs, 2, regvals);

						if(IsDebuggerPresent())
							__debugbreak();
						return false;
					}
				}
			}
			else {
				retry = false;
			}
		} while (retry);

		break;

	case WHvRunVpExitReasonX64IoPortAccess:
		hr = WHvEmulatorTryIoEmulation(m_emulator, this, &Context.VpContext, &Context.IoPortAccess, &status);
		if (FAILED(hr))
			_com_raise_error(hr);

		if (!status.EmulationSuccessful) {
			fprintf(stderr, "Port IO emulation failure\n");
			__debugbreak();
		}

		break;

	default:
		return false;
	}
	
	return true;
}

const WHV_EMULATOR_CALLBACKS HypervisorEmulator::m_callbackTable = {
	sizeof(m_callbackTable),
	0,
	ioPortCallback,
	memoryCallback,
	getVirtualProcessorRegistersCallback,
	setVirtualProcessorRegistersCallback,
	translateGvaPageCallback
};

