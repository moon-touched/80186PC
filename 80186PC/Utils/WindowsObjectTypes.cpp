#include <Utils/WindowsObjectTypes.h>

#include <comdef.h>

void WindowsHandleDeleter::operator()(HANDLE handle) const {
	CloseHandle(handle);
}

void WindowsSectionViewDeleter::operator()(void *base) const {
	UnmapViewOfFile(base);
}

WindowsMemoryRegion::WindowsMemoryRegion(unsigned int length, DWORD protect) : m_length(length) {
	m_base = VirtualAlloc(nullptr, m_length, MEM_RESERVE | MEM_COMMIT, protect);
	if (!m_base) {
		auto error = HRESULT_FROM_WIN32(GetLastError());
		_com_raise_error(error);
	}

	memset(m_base, 0, length);
}

WindowsMemoryRegion::~WindowsMemoryRegion() {
	VirtualFree(m_base, m_length, MEM_RELEASE | MEM_DECOMMIT);
}
