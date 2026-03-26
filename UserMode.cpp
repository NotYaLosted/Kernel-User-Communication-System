#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

namespace drv {

	namespace ioctl {
		const DWORD attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x775, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const DWORD read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x776, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const DWORD write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct request {
		HANDLE pid;
		PVOID  target;
		PVOID  buffer;
		SIZE_T size;
		SIZE_T return_size;
	};

	class manager {
	private:
		HANDLE h;

	public:
		manager() {
			h = CreateFile(L"\\\\.\\AssholeDriver",
				GENERIC_READ,
				0,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);
		}

		bool valid() {
			return h != INVALID_HANDLE_VALUE;
		}

		bool attach(DWORD pid) {
			request r{};
			r.pid = (HANDLE)pid;

			return DeviceIoControl(h, ioctl::attach,
				&r, sizeof(r),
				&r, sizeof(r),
				nullptr, nullptr);
		}

		template<typename T>
		T read(uintptr_t addr) {
			T val{};

			request r{};
			r.target = (PVOID)addr;
			r.buffer = &val;
			r.size = sizeof(T);

			DeviceIoControl(h, ioctl::read,
				&r, sizeof(r),
				&r, sizeof(r),
				nullptr, nullptr);

			return val;
		}

		template<typename T>
		void write(uintptr_t addr, const T& val) {
			request r{};
			r.target = (PVOID)addr;
			r.buffer = (PVOID)&val;
			r.size = sizeof(T);

			DeviceIoControl(h, ioctl::write,
				&r, sizeof(r),
				&r, sizeof(r),
				nullptr, nullptr);
		}

		~manager() {
			if (h && h != INVALID_HANDLE_VALUE)
				CloseHandle(h);
		}
	};
}


DWORD get_pid(const wchar_t* name) {
	DWORD pid = 0;

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32W e{};
	e.dwSize = sizeof(e);

	if (Process32FirstW(snap, &e)) {
		do {
			if (!_wcsicmp(e.szExeFile, name)) {
				pid = e.th32ProcessID;
				break;
			}
		} while (Process32NextW(snap, &e));
	}

	CloseHandle(snap);
	return pid;
}


uintptr_t get_module(DWORD pid, const wchar_t* name) {
	uintptr_t base = 0;

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE)
		return 0;

	MODULEENTRY32W m{};
	m.dwSize = sizeof(m);

	if (Module32FirstW(snap, &m)) {
		do {
			if (!_wcsicmp(m.szModule, name)) {
				base = (uintptr_t)m.modBaseAddr;
				break;
			}
		} while (Module32NextW(snap, &m));
	}

	CloseHandle(snap);
	return base;
}

int main() {

	std::cout << "[*] client >> \n";

	DWORD pid = get_pid(L"notepad.exe");
	if (!pid) {
		std::cout << "[-] no process\n";
		std::cin.get();
		return 1;
	}
	std::cout << "[+] pid: " << pid << std::endl;

	drv::manager driver;
	if (!driver.valid()) {
		std::cout << "[-] cant connect to the driver. is it loaded?\n";
		std::cin.get();
		return 1;
	}
	std::cout << "[+] connected to the driver\n";

	if (!driver.attach(pid)) {
		std::cout << "[-] attach fail\n";
		std::cin.get();
		return 1;
	}
	std::cout << "[+] attached!\n";

	uintptr_t base = get_module(pid, L"notepad.exe");
	if (!base) {
		std::cout << "[-] module base not found\n";
		std::cin.get();
		return 1;
	}
	std::cout << "[+] base: 0x" << std::hex << base << std::dec << std::endl;

	std::cin.get();
	return 0;
}
