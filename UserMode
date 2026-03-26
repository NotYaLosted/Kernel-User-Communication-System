#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

namespace driver {

	namespace codes {
		const DWORD attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x775, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const DWORD read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x776, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const DWORD write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;
		SIZE_T size;
		SIZE_T return_size;
	};

	class driver_manager {
	private:
		HANDLE handle;

	public:
		driver_manager() {
			handle = CreateFile(L"\\\\.\\AssholeDriver",
				GENERIC_READ | GENERIC_WRITE,
				0,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);

			if (handle == INVALID_HANDLE_VALUE) {
				std::cout << "[-] CreateFile failed. Error: " << GetLastError() << std::endl;
			}
			else {
				std::cout << "[+] Driver handle opened successfully" << std::endl;
			}
		}

		bool is_valid() {
			return handle != INVALID_HANDLE_VALUE;
		}

		bool attach(DWORD pid) {
			std::cout << "[*] Attempting to attach to PID: " << pid << std::endl;

			Request req{};
			req.process_id = (HANDLE)pid;

			BOOL result = DeviceIoControl(handle, codes::attach,
				&req, sizeof(req),
				&req, sizeof(req),
				nullptr, nullptr);

			if (!result) {
				std::cout << "[-] Attach failed. Error: " << GetLastError() << std::endl;
				return false;
			}

			std::cout << "[+] Attach successful" << std::endl;
			return true;
		}

		template<typename T>
		T rpm(uintptr_t address) {
			T buffer{};

			std::cout << "[*] Reading " << sizeof(T) << " bytes from address: 0x" << std::hex << address << std::dec << std::endl;

			Request req{};
			req.target = (PVOID)address;
			req.buffer = &buffer;
			req.size = sizeof(T);

			BOOL result = DeviceIoControl(handle, codes::read,
				&req, sizeof(req),
				&req, sizeof(req),
				nullptr, nullptr);

			if (!result) {
				std::cout << "[-] Read failed. Error: " << GetLastError() << std::endl;
			}
			else {
				std::cout << "[+] Read successful. Return size: " << req.return_size << std::endl;
			}

			return buffer;
		}

		template<typename T>
		bool wpm(uintptr_t address, const T& value) {
			std::cout << "[*] Writing " << sizeof(T) << " bytes to address: 0x" << std::hex << address << std::dec << std::endl;

			Request req{};
			req.target = (PVOID)address;
			req.buffer = (PVOID)&value;
			req.size = sizeof(T);

			BOOL result = DeviceIoControl(handle, codes::write,
				&req, sizeof(req),
				&req, sizeof(req),
				nullptr, nullptr);

			if (!result) {
				std::cout << "[-] Write failed. Error: " << GetLastError() << std::endl;
				return false;
			}

			std::cout << "[+] Write successful. Return size: " << req.return_size << std::endl;
			return true;
		}

		~driver_manager() {
			if (handle && handle != INVALID_HANDLE_VALUE) {
				std::cout << "[*] Closing driver handle" << std::endl;
				CloseHandle(handle);
				std::cout << "[+] Driver handle closed" << std::endl;
			}
		}
	};
}


DWORD get_process_id(const wchar_t* name) {
	std::cout << "[*] Searching for process: ";
	std::wcout << name << std::endl;

	DWORD pid = 0;

	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) {
		std::cout << "[-] Failed to create process snapshot. Error: " << GetLastError() << std::endl;
		return 0;
	}

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);

	if (Process32FirstW(snap, &entry)) {
		do {
			if (!_wcsicmp(entry.szExeFile, name)) {
				pid = entry.th32ProcessID;
				std::cout << "[+] Process found! PID: " << pid << std::endl;
				break;
			}
		} while (Process32NextW(snap, &entry));
	}

	if (pid == 0) {
		std::cout << "[-] Process not found" << std::endl;
	}

	CloseHandle(snap);
	return pid;
}

int main() {
	
	std::cout << "[*] Client<<" << std::endl;
	std::cout << "[*] Looking for notepad.exe..." << std::endl;
	DWORD pid = get_process_id(L"notepad.exe");
	if (!pid) {
		std::cout << "[-] Process not found. Please run notepad.exe first." << std::endl;
		std::cout << "[*] Press any key to exit..." << std::endl;
		system("pause");
		return 1;
	}

	std::cout << std::endl << "[*] Creating driver manager instance..." << std::endl;
	driver::driver_manager drv;

	if (!drv.is_valid()) {
		std::cout << "[-] Driver open failed. Make sure the driver is loaded." << std::endl;
		std::cout << "[*] Press any key to exit..." << std::endl;
		system("pause");
		return 1;
	}

	std::cout << std::endl << "[*] Attempting to attach to process..." << std::endl;
	if (!drv.attach(pid)) {
		std::cout << "[-] Attach failed. Check driver permissions." << std::endl;
		std::cout << "[*] Press any key to exit..." << std::endl;
		system("pause");
		return 1;
	}

	std::cout << std::endl << "[+] Successfully attached to process!" << std::endl;
	

	system("pause");
	return 0;
}
