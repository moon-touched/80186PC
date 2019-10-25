#ifndef WINDOWS_OBJECT_TYPES_H
#define WINDOWS_OBJECT_TYPES_H

#include <Windows.h>
#include <type_traits>
#include <memory>

struct WindowsHandleDeleter {
	void operator()(HANDLE handle) const;
};
using WindowsHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, WindowsHandleDeleter>;

struct WindowsSectionViewDeleter {
	void operator()(void *base) const;
};
using WindowsSectionView = std::unique_ptr<std::remove_pointer<void>::type, WindowsSectionViewDeleter>;

class WindowsMemoryRegion {
public:
	explicit WindowsMemoryRegion(unsigned int length, DWORD protect);
	~WindowsMemoryRegion();

	WindowsMemoryRegion(const WindowsMemoryRegion &other) = delete;
	WindowsMemoryRegion &operator =(const WindowsMemoryRegion &other) = delete;

	inline void *base() const { return m_base; }
	inline unsigned int length() const { return m_length; }

private:
	void *m_base;
	unsigned int m_length;
};

#endif

