set(ata_sources 
	include/ATA/ATADemux.h
	include/ATA/ATADevice.h
	include/ATA/ATAHardDisk.h
	include/ATA/ATATypes.h
	include/ATA/IATADevice.h
	ATA/ATADemux.cpp
	ATA/ATADevice.cpp
	ATA/ATAHardDisk.cpp
	ATA/IATADevice.cpp
)

set(hardware_sources
	include/Hardware/CPUEmulation.h
	include/Hardware/CPUEmulationFactory.h
	include/Hardware/HerculesVideo.h
	include/Hardware/Machine.h
	include/Hardware/NMIControl.h
    include/Hardware/PIC.h
    include/Hardware/PIT.h
	include/Hardware/PPI.h
	include/Hardware/PPIConsumer.h
	include/Hardware/RTC.h
	include/Hardware/XTIDE.h
	Hardware/CPUEmulation.cpp
	Hardware/CPUEmulationFactory.cpp
	Hardware/HerculesVideo.cpp
	Hardware/Machine.cpp
	Hardware/NMIControl.cpp
    Hardware/PIC.cpp
    Hardware/PIT.cpp
	Hardware/PPI.cpp
	Hardware/PPIConsumer.cpp
	Hardware/RTC.cpp
	Hardware/XTIDE.cpp

)

set(hyperv_sources
	include/HyperV/HypervisorCPUEmulation.h
	include/HyperV/HypervisorEmulator.h
	include/HyperV/HypervisorPartition.h
	include/HyperV/IHypervisorEmulatorCallbacks.h
	HyperV/HypervisorCPUEmulation.cpp
	HyperV/HypervisorEmulator.cpp
	HyperV/HypervisorPartition.cpp
	HyperV/IHypervisorEmulatorCallbacks.cpp
)

set(infrastructure_sources
	include/Infrastructure/AddressRange.h
	include/Infrastructure/AddressRangeRegistration.h
	include/Infrastructure/AddressSpaceDispatcher.h
	include/Infrastructure/DummyAddressRangeHandler.h
	include/Infrastructure/IAddressRangeHandler.h
	include/Infrastructure/InterruptController.h
	include/Infrastructure/InterruptLine.h
	include/Infrastructure/MappedAddressRange.h
	Infrastructure/AddressRangeRegistration.cpp
	Infrastructure/AddressSpaceDispatcher.cpp
	Infrastructure/DummyAddressRangeHandler.cpp
	Infrastructure/IAddressRangeHandler.cpp
	Infrastructure/InterruptController.cpp
	Infrastructure/InterruptLine.cpp
	Infrastructure/MappedAddressRange.cpp
)

set(ui_sources 
	include/UI/SDLUI.h
	include/UI/VideoAdapter.h
	UI/SDLUI.cpp
	UI/VideoAdapter.cpp
)

set(utils_sources
	include/Utils/AccessSizeUtils.h
	include/Utils/WindowsObjectTypes.h
	Utils/WindowsObjectTypes.cpp
)

add_executable(80186PC
	${ata_sources}
	${hardware_sources}
	${hyperv_sources}
	${infrastructure_sources}
	${ui_sources}
	${utils_sources}

	main.cpp
)

source_group(ATA FILES ${ata_sources})
source_group(Hardware FILES ${hardware_sources})
source_group(HyperV FILES ${hyperv_sources})
source_group(Infrastructure FILES ${infrastructure_sources})
source_group(UI FILES ${ui_sources})
source_group(Utils FILES ${utils_sources})

target_compile_definitions(80186PC PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_VC_EXTRALEAN -DNOMINMAX)
target_include_directories(80186PC PRIVATE include)
target_link_libraries(80186PC PRIVATE WinHvPlatform WinHvEmulation virtdisk)
set_target_properties(80186PC PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)

target_include_directories(80186PC PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(80186PC PRIVATE ${SDL2_LIBRARIES})
