#include <Utils/WindowsResources.h>

#include <Windows.h>
#include <comdef.h>

extern "C" extern IMAGE_DOS_HEADER __ImageBase;

void getRCDATA(unsigned int index, const void** data, size_t* size) {
	auto mod = reinterpret_cast<HMODULE>(&__ImageBase);

	auto resource = FindResource(mod, MAKEINTRESOURCE(index), RT_RCDATA);
	if (!resource)
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

	*size = SizeofResource(mod, resource);

	auto glob = LoadResource(mod, resource);
	if(!glob)
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

	*data = LockResource(glob);
}