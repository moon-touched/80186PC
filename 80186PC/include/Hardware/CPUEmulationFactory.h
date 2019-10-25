#ifndef CPU_EMULATION_FACTORY_H
#define CPU_EMULATION_FACTORY_H

#include <memory>

class CPUEmulation;

class CPUEmulationFactory {
public:
	CPUEmulationFactory();
	~CPUEmulationFactory();

	CPUEmulationFactory(const CPUEmulationFactory& other) = delete;
	CPUEmulationFactory& operator =(const CPUEmulationFactory& other) = delete;

	std::unique_ptr<CPUEmulation> createCPUEmulation() const;
};

#endif
