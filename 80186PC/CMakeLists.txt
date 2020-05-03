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
	include/Hardware/BusMouse.h
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
	include/Hardware/XTKeyboard.h
	Hardware/BusMouse.cpp
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
	Hardware/XTKeyboard.cpp
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

set(libx86emu_sources
	libx86emu/include/decode.h
	libx86emu/include/mem.h
	libx86emu/include/ops.h
	libx86emu/include/prim_ops.h
	libx86emu/include/x86emu.h
	libx86emu/include/x86emu_int.h
	libx86emu/api.c
	libx86emu/decode.c
	libx86emu/mem.c
	libx86emu/ops.c
	libx86emu/ops2.c
	libx86emu/prim_ops.c
)

set(ui_sources 
	include/UI/Keyboard.h
	include/UI/Mouse.h
	include/UI/SDLUI.h
	include/UI/VideoAdapter.h
	UI/Keyboard.cpp
	UI/Mouse.cpp
	UI/SDLUI.cpp
	UI/VideoAdapter.cpp
)

set(utils_sources
	include/Utils/AccessSizeUtils.h
	include/Utils/WindowsObjectTypes.h
	Utils/WindowsObjectTypes.cpp
)

set(x86emu_sources
	include/X86Emu/X86EmuCPUEmulation.h
	X86Emu/X86EmuCPUEmulation.cpp
)

add_executable(80186PC
	${ata_sources}
	${hardware_sources}
	${infrastructure_sources}
	${libx86emu_sources}
	${ui_sources}
	${utils_sources}
	${x86emu_sources}

	main.cpp
)

source_group(ATA FILES ${ata_sources})
source_group(Hardware FILES ${hardware_sources})
source_group(Infrastructure FILES ${infrastructure_sources})
source_group(libx86emu FILES ${libx86emu_sources})
source_group(UI FILES ${ui_sources})
source_group(Utils FILES ${utils_sources})
source_group(X86Emu FILES ${x86emu_sources})

target_compile_definitions(80186PC PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_VC_EXTRALEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
target_include_directories(80186PC PRIVATE include libx86emu/include)
target_link_libraries(80186PC PRIVATE virtdisk)
set_target_properties(80186PC PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)

target_include_directories(80186PC PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(80186PC PRIVATE ${SDL2_LIBRARIES})
