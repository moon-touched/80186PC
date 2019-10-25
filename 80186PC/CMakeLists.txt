set(hardware_sources
	include/Hardware/CPUEmulation.h
	include/Hardware/CPUEmulationFactory.h
	include/Hardware/Machine.h
    include/Hardware/PIC.h
    include/Hardware/PIT.h
	include/Hardware/RTC.h
	Hardware/CPUEmulation.cpp
	Hardware/CPUEmulationFactory.cpp
	Hardware/Machine.cpp
    Hardware/PIC.cpp
    Hardware/PIT.cpp
	Hardware/RTC.cpp

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

set(utils_sources
	include/Utils/AccessSizeUtils.h
	include/Utils/WindowsObjectTypes.h
	Utils/WindowsObjectTypes.cpp
)

add_executable(80186PC
	${hardware_sources}
	${hyperv_sources}
	${infrastructure_sources}
	${utils_sources}

	main.cpp
)

source_group(Hardware FILES ${hardware_sources})
source_group(HyperV FILES ${hyperv_sources})
source_group(Infrastructure FILES ${infrastructure_sources})
source_group(Utils FILES ${utils_sources})

target_compile_definitions(80186PC PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_VC_EXTRALEAN -DNOMINMAX)
target_include_directories(80186PC PRIVATE include)
target_link_libraries(80186PC PRIVATE WinHvPlatform WinHvEmulation)
set_target_properties(80186PC PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)