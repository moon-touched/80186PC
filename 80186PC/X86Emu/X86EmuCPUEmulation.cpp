#include <X86Emu/X86EmuCPUEmulation.h>
#include <Infrastructure/IAddressRangeHandler.h>
#include <Infrastructure/InterruptController.h>

#include <stdexcept>
#include <string>

X86EmuCPUEmulation::X86EmuCPUEmulation() : m_interruptPending(false) {
	m_emulator.reset(x86emu_new(0, 0));
	//x86emu_set_log(m_emulator.get(), 16384, flushLog);
	m_nativeMemioHandler = x86emu_set_memio_handler(m_emulator.get(), memioHandler);
	x86emu_set_code_handler(m_emulator.get(), codeHandler);

	m_emulator->_private = this;

	//m_emulator->log.trace = X86EMU_TRACE_INTS;
}

X86EmuCPUEmulation::~X86EmuCPUEmulation() {
	m_cpu0Thread.join();
}

void X86EmuCPUEmulation::start() {
	m_cpu0Thread = std::thread(&X86EmuCPUEmulation::cpu0Thread, this);
}

bool X86EmuCPUEmulation::shouldDeliverInterrupt() const {
	auto interruptPending = m_interruptPending.load();
	return interruptPending && (m_emulator->x86.R_FLG & FB_IF) && (m_emulator->x86.intr_type == 0);
}

void X86EmuCPUEmulation::cpu0Thread() {
	while (true) {
		std::unique_lock<std::mutex> locker(m_emulatorMutex);

		if (shouldDeliverInterrupt()) {
			auto vector = interruptController()->processInterruptAcknowledge();

			x86emu_intr_raise(m_emulator.get(), vector, INTR_TYPE_SOFT, 0);
		}

		m_emulator->max_instr = m_emulator->x86.R_TSC + 1;
		auto result = x86emu_run(m_emulator.get(), X86EMU_RUN_NO_EXEC);
		x86emu_clear_log(m_emulator.get(), 1);
		//if (result != X86EMU_RUN_MAX_INSTR) {
//			throw std::runtime_error("unexpected stop: " + std::to_string(result));
//		}
	}
}

void X86EmuCPUEmulation::mapMemory(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) {
	mapMemoryInternal(base & ~0x100000, ((limit - 1) & ~0x100000) + 1, hostMemory, permissions);
	mapMemoryInternal(base |  0x100000, ((limit - 1) |  0x100000) + 1, hostMemory, permissions);
}

void X86EmuCPUEmulation::unmapMemory(uint64_t base, uint64_t limit) {
	unmapMemoryInternal(base & ~0x100000, ((limit - 1) & ~0x100000) + 1);
	unmapMemoryInternal(base |  0x100000, ((limit - 1) |  0x100000) + 1);
}

void X86EmuCPUEmulation::mapMemoryInternal(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) {
	std::unique_lock<std::mutex> locker(m_emulatorMutex);

	auto ptr = reinterpret_cast<uint8_t*>(hostMemory);

	unsigned int perms = X86EMU_PERM_VALID;
	if (permissions & IAddressRangeHandler::AccessRead)
		perms |= X86EMU_PERM_R;

	if (permissions & IAddressRangeHandler::AccessWrite)
		perms |= X86EMU_PERM_W;

	if (permissions & IAddressRangeHandler::AccessExecute)
		perms |= X86EMU_PERM_X;

	for (uint64_t addr = base; addr < limit; addr += X86EMU_PAGE_SIZE) {
		x86emu_set_page(m_emulator.get(), addr, ptr);
		x86emu_set_perm(m_emulator.get(), addr, addr + X86EMU_PAGE_SIZE - 1, perms);
		ptr += X86EMU_PAGE_SIZE;
	}
}

void X86EmuCPUEmulation::unmapMemoryInternal(uint64_t base, uint64_t limit) {
	std::unique_lock<std::mutex> locker(m_emulatorMutex);

	for (uint64_t addr = base; addr < limit; addr += X86EMU_PAGE_SIZE) {
		x86emu_set_page(m_emulator.get(), addr, nullptr);
		x86emu_set_perm(m_emulator.get(), addr, addr + X86EMU_PAGE_SIZE - 1, 0);
	}

}

void X86EmuCPUEmulation::setInterruptAsserted(bool interrupt) {
	m_interruptPending = interrupt;
}

void X86EmuCPUEmulation::flushLog(x86emu_t*, char* buf, unsigned size) {
	fwrite(buf, 1, size, stdout);
	fflush(stdout);
}

unsigned int X86EmuCPUEmulation::memioHandler(x86emu_t* emu, u32 addr, u32* val, unsigned int type) {
	auto this_ = static_cast<X86EmuCPUEmulation*>(emu->_private);

	unsigned int length = type & 0xFF;
	type = type & ~0xFF;

	if (type == X86EMU_MEMIO_R || type == X86EMU_MEMIO_W || type == X86EMU_MEMIO_X) {
		auto nativeResult = this_->m_nativeMemioHandler(emu, addr, val, type | length);
		if (nativeResult == 0)
			return 0;

		if (type == X86EMU_MEMIO_R || type == X86EMU_MEMIO_X) {
			*val = static_cast<uint32_t>(this_->mmioDispatcher()->read(addr, translateSize(length)));
		}
		else {
			this_->mmioDispatcher()->write(
				addr,
				translateSize(length),
				*val
			);
		}

		return 0;
	}
	else if(type == X86EMU_MEMIO_I) {
		// emulate port read

		*val = static_cast<uint32_t>(this_->ioDispatcher()->read(addr, translateSize(length)));

		return 0;
	}
	else if (type == X86EMU_MEMIO_O) {
		// emulate port write;
		
		this_->ioDispatcher()->write(
			addr,
			translateSize(length),
			*val
		);
		
		return 0;
	}
	else {
		throw std::logic_error("unimplemented memio type");
	}
}

unsigned int X86EmuCPUEmulation::translateSize(unsigned int length) {
	switch (length) {
	case X86EMU_MEMIO_8_NOPERM:
	case X86EMU_MEMIO_8:
		return 1;

	case X86EMU_MEMIO_16:
		return 2;

	case X86EMU_MEMIO_32:
		return 4;

	default:
		throw std::logic_error("unimplemented operation length");
	}
}

int X86EmuCPUEmulation::codeHandler(x86emu_t* emu) {
	auto this_ = static_cast<X86EmuCPUEmulation*>(emu->_private);

	if (this_->shouldDeliverInterrupt()) {
		return 1;
	}
	else {
		return 0;
	}
}
