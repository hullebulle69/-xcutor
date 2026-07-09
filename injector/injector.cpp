#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

// ---------------------------------------------------------------------------
// RAII handle wrapper
// ---------------------------------------------------------------------------
struct Handle {
    HANDLE h;
    explicit Handle(HANDLE h) : h(h) {}
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    Handle(const Handle&)            = delete;
    Handle& operator=(const Handle&) = delete;
    operator HANDLE() const { return h; }
    bool ok() const { return h && h != INVALID_HANDLE_VALUE; }
};

// ---------------------------------------------------------------------------
// Win32 error → human-readable string
// ---------------------------------------------------------------------------
static std::wstring win_error(DWORD code = GetLastError()) {
    wchar_t* buf = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buf), 0, nullptr);

    std::wstring msg = buf ? buf : L"Unknown error";
    if (buf) LocalFree(buf);
    while (!msg.empty() && (msg.back() == L'\n' || msg.back() == L'\r'))
        msg.pop_back();
    return msg;
}

// ---------------------------------------------------------------------------
// Locate a process by executable name, return its PID (0 = not found).
// ---------------------------------------------------------------------------
static DWORD find_pid(std::wstring_view name) {
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snap.ok()) {
        fwprintf(stderr, L"[!] Snapshot failed: %s\n", win_error().c_str());
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (!Process32FirstW(snap, &entry)) return 0;

    do {
        if (name == entry.szExeFile) return entry.th32ProcessID;
    } while (Process32NextW(snap, &entry));

    return 0;
}

// ---------------------------------------------------------------------------
// Classic LoadLibraryW injection.
//
// Steps:
//   1. OpenProcess with the rights we need.
//   2. VirtualAllocEx  – allocate a page in the target for the DLL path.
//   3. WriteProcessMemory – copy the path string into that page.
//   4. CreateRemoteThread at LoadLibraryW – the OS loader does the rest.
//   5. Wait for the loader thread, check the HMODULE return value.
//   6. VirtualFreeEx  – clean up the remote string allocation.
// ---------------------------------------------------------------------------
static bool inject(DWORD pid, const wchar_t* dll_path) {
    // ── 1. Open the target process ────────────────────────────────────────
    Handle proc(OpenProcess(
        PROCESS_CREATE_THREAD  |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION   |
        PROCESS_VM_WRITE       |
        PROCESS_VM_READ,
        FALSE, pid));

    if (!proc.ok()) {
        fwprintf(stderr, L"[!] OpenProcess(%lu) failed: %s\n",
                 pid, win_error().c_str());
        return false;
    }

    // ── 2. Allocate remote memory for the DLL path (wide string) ─────────
    const std::size_t path_bytes = (wcslen(dll_path) + 1) * sizeof(wchar_t);

    LPVOID remote_path = VirtualAllocEx(proc, nullptr, path_bytes,
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_READWRITE);
    if (!remote_path) {
        fwprintf(stderr, L"[!] VirtualAllocEx failed: %s\n", win_error().c_str());
        return false;
    }

    // Cleanup lambda – called on every early-return path.
    auto free_remote = [&] {
        VirtualFreeEx(proc, remote_path, 0, MEM_RELEASE);
    };

    // ── 3. Write the DLL path into remote memory ──────────────────────────
    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, remote_path, dll_path, path_bytes, &written)
        || written != path_bytes)
    {
        fwprintf(stderr, L"[!] WriteProcessMemory failed: %s\n", win_error().c_str());
        free_remote();
        return false;
    }

    // ── 4. Resolve LoadLibraryW ───────────────────────────────────────────
    // kernel32.dll is always mapped at the same VA in every process within a
    // session (ASLR randomises it once at boot, not per-process).
    const HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    if (!k32) {
        fwprintf(stderr, L"[!] GetModuleHandle(kernel32) failed: %s\n",
                 win_error().c_str());
        free_remote();
        return false;
    }

    const auto load_lib_w = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(k32, "LoadLibraryW"));
    if (!load_lib_w) {
        fwprintf(stderr, L"[!] GetProcAddress(LoadLibraryW) failed: %s\n",
                 win_error().c_str());
        free_remote();
        return false;
    }

    // ── 5. Spawn a remote thread that calls LoadLibraryW(path) ───────────
    Handle thread(CreateRemoteThread(proc, nullptr, 0,
                                     load_lib_w, remote_path,
                                     0, nullptr));
    if (!thread.ok()) {
        fwprintf(stderr, L"[!] CreateRemoteThread failed: %s\n", win_error().c_str());
        free_remote();
        return false;
    }

    WaitForSingleObject(thread, INFINITE);

    // LoadLibraryW returns the HMODULE (non-zero on success) in the thread
    // exit code.  If it's zero, the DLL failed to load.
    DWORD exit_code = 0;
    GetExitCodeThread(thread, &exit_code);
    if (exit_code == 0) {
        fwprintf(stderr,
                 L"[!] LoadLibraryW returned NULL in the target process.\n"
                 L"    Check: DLL exists at that path inside the target's view,\n"
                 L"    DLL init did not throw, and the bitness matches.\n");
        free_remote();
        return false;
    }

    // ── 6. Free the remote path string ───────────────────────────────────
    free_remote();

    fwprintf(stdout, L"[+] Successfully injected '%s' into PID %lu.\n",
             dll_path, pid);
    return true;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        fwprintf(stderr,
            L"Usage: xcutor-inject <process.exe> <C:\\path\\to\\xcutor.dll>\n"
            L"\n"
            L"  Example:\n"
            L"    xcutor-inject RobloxPlayerBeta.exe C:\\tools\\xcutor.dll\n");
        return EXIT_FAILURE;
    }

    const wchar_t* proc_name = argv[1];
    const wchar_t* dll_path  = argv[2];

    // Verify the DLL is reachable before touching the target process.
    if (GetFileAttributesW(dll_path) == INVALID_FILE_ATTRIBUTES) {
        fwprintf(stderr, L"[!] DLL file not found: %s\n", dll_path);
        return EXIT_FAILURE;
    }

    fwprintf(stdout, L"[*] Searching for process '%s'...\n", proc_name);

    const DWORD pid = find_pid(proc_name);
    if (pid == 0) {
        fwprintf(stderr, L"[!] Process '%s' is not running.\n", proc_name);
        return EXIT_FAILURE;
    }

    fwprintf(stdout, L"[*] Found PID: %lu\n", pid);

    return inject(pid, dll_path) ? EXIT_SUCCESS : EXIT_FAILURE;
}
