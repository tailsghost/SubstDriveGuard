#include <string>
#include <Windows.h>

#pragma once


class SubstDriveGuard {
public:
	SubstDriveGuard() = default;
	~SubstDriveGuard() { Unmount(); }

	bool Mount(const std::wstring& target) {
		if (target.empty()) return false;
		auto norm = target;
		if (norm.size() > 1 && (norm.back() == L'\\' || norm.back() == L'/')) norm.pop_back();
		m_target = norm;

		for (const auto L = L'Z'; L >= L'D'; --L) {
			if (IsDriveFree(L)) {
				m_drive = L;
				break;
			}
			if (L == L'D') break;
		}
		if (m_drive == 0) return false;

		if (CreateViaDefineDosDevice(m_drive, m_target)) {
			m_created = true;
			m_usedDefine = true;
			return true;
		}


		if (CreateViaSubstCmd(m_drive, m_target)) {
			m_created = true;
			m_usedDefine = false;
			return true;
		}

		m_drive = 0;
		return false;
	}

	void Unmount() {
		if (!m_created) return;
		if (m_drive == 0) return;
		if (m_usedDefine) {
			RemoveViaDefineDosDevice(m_drive, m_target);
		}
		else {
			RemoveViaSubstCmd(m_drive);
		}
		m_created = false;
		m_drive = 0;
	}

	bool IsMounted() const { return m_created; }
	wchar_t DriveLetter() const { return m_drive; }
	std::wstring MappedRoot() const {
		if (!m_created) return L"";
		return std::wstring(1, m_drive) + L":\\";
	}

private:
	wchar_t m_drive = 0;
	std::wstring m_target;
	bool m_created = false;
	bool m_usedDefine = false;

	std::wstring Quote(const std::wstring& s)
	{
		if (s.empty()) return L"\"\"";
		if (s.find(L' ') != std::wstring::npos || s.find(L'"') != std::wstring::npos)
			return L"\"" + s + L"\"";
		return s;
	}

	bool IsDriveFree(const wchar_t letter) {
		if (letter < L'A' || letter > L'Z') return false;
		wchar_t root[4] = { letter, L':', L'\\', 0 };
		UINT type = GetDriveTypeW(root);
		return (type == DRIVE_NO_ROOT_DIR) || (type == DRIVE_UNKNOWN);
	}

	bool CreateViaDefineDosDevice(wchar_t drive, const std::wstring& target) {
		auto raw = L"\\??\\" + target;
		auto result = DefineDosDeviceW(DDD_RAW_TARGET_PATH, (std::wstring(drive) + L":").c_str(), raw.c_str());
		return result != FALSE;
	}

	bool RemoveViaDefineDosDevice(wchar_t drive, const std::wstring& target) {
		auto raw = L"\\??\\" + target;
		auto result = DefineDosDeviceW(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, (std::wstring(1, drive) + L":").c_str(), raw.c_str());
		return result != FALSE;
	}

	bool CreateViaSubstCmd(wchar_t drive, const std::wstring& target) {
		auto cmd = L"cmd.exe /C subst " + std::wstring(drive) + L": " + Quote(target);
		STARTUPINFOW si{}; si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		auto ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
		if (!ok) return false;
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD ec = 0;
		GetExitCodeProcess(pi.hProcess, &ec);
		CloseHandle(pi.hThread); 
		CloseHandle(pi.hProcess);
		return ec == 0;
	}

	bool RemoveViaSubstCmd(wchar_t drive) {
		auto cmd = L"cmd.exe /C subst " + std::wstring(1, drive) + L": /D";
		STARTUPINFOW si{}; si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		auto ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
		if (!ok) return false;
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD ec = 0; 
		GetExitCodeProcess(pi.hProcess, &ec);
		CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
		return ec == 0;
	}
};
