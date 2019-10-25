#ifndef IHYPERVISOR_EMULATOR_CALLBACKS_H
#define IHYPERVISOR_EMULATOR_CALLBACKS_H

#include <Windows.h>
#include <WinHvEmulation.h>

class IHypervisorEmulatorCallbacks {
protected:
	IHypervisorEmulatorCallbacks();
	~IHypervisorEmulatorCallbacks();

public:
	IHypervisorEmulatorCallbacks(const IHypervisorEmulatorCallbacks& other) = delete;
	IHypervisorEmulatorCallbacks& operator =(const IHypervisorEmulatorCallbacks& other) = delete;

	virtual HRESULT ioPortCallback(WHV_EMULATOR_IO_ACCESS_INFO& ioAccess) = 0;
	virtual HRESULT memoryCallback(WHV_EMULATOR_MEMORY_ACCESS_INFO& memoryAccess) = 0;
};


#endif
