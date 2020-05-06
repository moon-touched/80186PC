#ifndef X86EMU_CPU_EMULATION_H
#define X86EMU_CPU_EMULATION_H

#include <thread>
#include <atomic>
#include <mutex>

#include <Hardware/CPUEmulation.h>

#include <x86emu.h>

class X86EmuCPUEmulation final : public CPUEmulation {
public:
	X86EmuCPUEmulation();
	~X86EmuCPUEmulation() override;

	void start() override;
	void stop() override;

	void mapMemory(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions) override;
	void unmapMemory(uint64_t base, uint64_t limit) override;

	void setInterruptAsserted(bool interrupt) override;

private:
	void cpu0Thread();

	void mapMemoryInternal(uint64_t base, uint64_t limit, void* hostMemory, unsigned int permissions);
	void unmapMemoryInternal(uint64_t base, uint64_t limit);
	
	static void flushLog(x86emu_t*, char* buf, unsigned size);
	static unsigned int memioHandler(x86emu_t* emu, u32 addr, u32* val, unsigned int type);
	static int codeHandler(x86emu_t* emu);

	bool shouldDeliverInterrupt() const;

	static unsigned int translateSize(unsigned int length);

	struct X86EmuDeleter {
		inline void operator()(x86emu_t* emu) {
			x86emu_done(emu);
		}
	};
	
	std::recursive_mutex m_emulatorMutex;
	std::unique_ptr<x86emu_t, X86EmuDeleter> m_emulator;
	x86emu_memio_handler_t m_nativeMemioHandler;
	std::atomic<bool> m_interruptPending;
	std::atomic<bool> m_run;
	std::thread m_cpu0Thread;
};

#endif
