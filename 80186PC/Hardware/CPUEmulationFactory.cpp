#include <Hardware/CPUEmulationFactory.h>
#include <HyperV/HypervisorCPUEmulation.h>
#include <X86Emu/X86EmuCPUEmulation.h>

CPUEmulationFactory::CPUEmulationFactory() = default;

CPUEmulationFactory::~CPUEmulationFactory() = default;

std::unique_ptr<CPUEmulation> CPUEmulationFactory::createCPUEmulation() const {
	return std::make_unique<X86EmuCPUEmulation>();
}
