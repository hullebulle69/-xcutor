using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using Microsoft.Win32;

namespace Executor;

public partial class MainWindow : Window
{
    // ── Win32 P/Invoke ────────────────────────────────────────────────────
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint OpenProcess(uint access, bool inherit, uint pid);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint VirtualAllocEx(nint hProc, nint addr, nuint size,
                                      uint type, uint protect);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool WriteProcessMemory(nint hProc, nint baseAddr,
                                          byte[] buf, nuint size, out nuint written);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint CreateRemoteThread(nint hProc, nint attrs, nuint stackSize,
                                          nint startAddr, nint param,
                                          uint flags, out uint threadId);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern uint WaitForSingleObject(nint handle, uint ms);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetExitCodeThread(nint handle, out uint exitCode);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool VirtualFreeEx(nint hProc, nint addr, nuint size, uint type);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(nint handle);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern nint GetModuleHandle(string name);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
    static extern nint GetProcAddress(nint hMod, string proc);

    // Process enumeration
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern nint CreateToolhelp32Snapshot(uint flags, uint pid);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct PROCESSENTRY32W
    {
        public uint    dwSize;
        public uint    cntUsage;
        public uint    th32ProcessID;
        public nuint   th32DefaultHeapID;
        public uint    th32ModuleID;
        public uint    cntThreads;
        public uint    th32ParentProcessID;
        public int     pcPriClassBase;
        public uint    dwFlags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string  szExeFile;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool Process32FirstW(nint snap, ref PROCESSENTRY32W entry);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool Process32NextW(nint snap, ref PROCESSENTRY32W entry);

    // Remote module enumeration (for Execute)
    [DllImport("psapi.dll", SetLastError = true)]
    static extern bool EnumProcessModulesEx(nint hProc, [Out] nint[]? mods,
                                            uint cb, out uint needed, uint filter);

    [DllImport("psapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern uint GetModuleFileNameExW(nint hProc, nint hMod,
                                            StringBuilder name, uint size);

    // Win32 constants
    const uint PROCESS_ALL_ACCESS  = 0x1F0FFF;
    const uint MEM_COMMIT_RESERVE  = 0x3000;
    const uint MEM_RELEASE         = 0x8000;
    const uint PAGE_READWRITE      = 0x04;
    const uint INFINITE            = 0xFFFFFFFF;
    const uint TH32CS_SNAPPROCESS  = 0x02;
    const uint LIST_MODULES_64     = 0x01;

    // ── Encoding — must match kXorKey in dllmain.cpp ─────────────────────
    private static readonly byte[] XorKey =
        { 0x5A, 0x9C, 0x3F, 0xA1, 0x77, 0xE4, 0x2D, 0xB8 };

    private static byte[] XorApply(byte[] data)
    {
        var result = new byte[data.Length];
        for (int i = 0; i < data.Length; i++)
            result[i] = (byte)(data[i] ^ XorKey[i % XorKey.Length]);
        return result;
    }

    // ── Application state ─────────────────────────────────────────────────
    private uint    _injectedPid  = 0;
    private string  _injectedDll  = string.Empty;
    private bool    _isInjected   = false;

    // ── Constructor ───────────────────────────────────────────────────────
    public MainWindow()
    {
        InitializeComponent();
    }

    // ── Window chrome ─────────────────────────────────────────────────────
    private void TitleBar_MouseDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }

    private void CloseBtn_Click(object sender, RoutedEventArgs e)    => Close();
    private void MinimizeBtn_Click(object sender, RoutedEventArgs e) =>
        WindowState = WindowState.Minimized;
    private void MaximizeBtn_Click(object sender, RoutedEventArgs e) =>
        WindowState = WindowState == WindowState.Maximized
                    ? WindowState.Normal
                    : WindowState.Maximized;

    // Keep the maximize/restore MDL2 glyph in sync with actual window state.
    private void Window_StateChanged(object sender, EventArgs e) =>
        // E922 = ChromeMaximize, E923 = ChromeRestore
        MaxIcon.Text = WindowState == WindowState.Maximized ? "" : "";

    // ── Browse ────────────────────────────────────────────────────────────
    private void BrowseBtn_Click(object sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog
        {
            Filter = "DLL files (*.dll)|*.dll|All files (*.*)|*.*",
            Title  = "Select DLL to inject"
        };
        if (dlg.ShowDialog() == true)
            DllPathBox.Text = dlg.FileName;
    }

    // ── Inject ────────────────────────────────────────────────────────────
    private void InjectBtn_Click(object sender, RoutedEventArgs e)
    {
        string proc = ProcessBox.Text.Trim();
        string dll  = DllPathBox.Text.Trim();

        if (string.IsNullOrEmpty(proc))  { SetStatus("Enter a process name.",      error: true); return; }
        if (!File.Exists(dll))           { SetStatus($"DLL not found: {dll}",      error: true); return; }

        uint pid = FindPid(proc);
        if (pid == 0) { SetStatus($"Process '{proc}' is not running.", error: true); return; }

        SetStatus($"Injecting into {proc} (PID {pid})…");

        bool ok = Inject(pid, dll);
        if (ok)
        {
            _injectedPid = pid;
            _injectedDll = dll;
            _isInjected  = true;

            InjectedBadge.Visibility = Visibility.Visible;
            InjectedLabel.Text       = $"Attached · {proc}";
            SetStatus($"Injected into {proc} (PID {pid})", error: false, success: true);
        }
        else
        {
            SetStatus($"Injection failed — {Win32Error()}", error: true);
        }
    }

    // ── Execute ───────────────────────────────────────────────────────────
    private void ExecuteBtn_Click(object sender, RoutedEventArgs e)
    {
        if (!_isInjected) { SetStatus("Inject first.", error: true); return; }

        string code = EditorBox.Text;
        if (string.IsNullOrWhiteSpace(code)) return;

        bool ok = Execute(_injectedPid, _injectedDll, code);
        SetStatus(ok ? "Script executed." : $"Execute failed — {Win32Error()}", !ok);
    }

    // ── Clear ─────────────────────────────────────────────────────────────
    private void ClearBtn_Click(object sender, RoutedEventArgs e)
    {
        EditorBox.Clear();
        SetStatus("Editor cleared.");
    }

    // ── Editor: line numbers ──────────────────────────────────────────────
    private void EditorBox_Loaded(object sender, RoutedEventArgs e)
    {
        // Subscribe to the ScrollChanged routed event that bubbles up from
        // the internal ScrollViewer inside the TextBox's control template.
        EditorBox.AddHandler(ScrollViewer.ScrollChangedEvent,
            new ScrollChangedEventHandler((_, se) =>
                LineScroll.ScrollToVerticalOffset(se.VerticalOffset)));
    }

    private void EditorBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        int n = Math.Max(1, EditorBox.LineCount);
        LineNums.Text = string.Join("\n", Enumerable.Range(1, n));
    }

    // ── Status bar helpers ────────────────────────────────────────────────
    private void SetStatus(string msg, bool error = false, bool success = false)
    {
        StatusText.Text = msg;

        // MDL2 glyphs: E783 = ErrorBadge (red),  E73E = Accept (green),
        //              E8FB = Sync/processing (blue for neutral info)
        if (error)
        {
            StatusIcon.Text       = "";
            StatusIcon.Foreground = new SolidColorBrush(Color.FromRgb(0xF8, 0x51, 0x49));
        }
        else if (success)
        {
            StatusIcon.Text       = "";
            StatusIcon.Foreground = new SolidColorBrush(Color.FromRgb(0x3F, 0xB9, 0x50));
        }
        else
        {
            StatusIcon.Text       = "";
            StatusIcon.Foreground = new SolidColorBrush(Color.FromRgb(0x58, 0xA6, 0xFF));
        }
    }

    private static string Win32Error()
    {
        int code = Marshal.GetLastWin32Error();
        return $"0x{code:X8}  {new System.ComponentModel.Win32Exception(code).Message}";
    }

    // ── Win32 helpers ─────────────────────────────────────────────────────

    /// <summary>Find a process by .exe name, returns 0 if not found.</summary>
    private static uint FindPid(string procName)
    {
        nint snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == 0) return 0;
        try
        {
            var e = new PROCESSENTRY32W { dwSize = (uint)Marshal.SizeOf<PROCESSENTRY32W>() };
            if (!Process32FirstW(snap, ref e)) return 0;
            do
            {
                if (string.Equals(e.szExeFile, procName, StringComparison.OrdinalIgnoreCase))
                    return e.th32ProcessID;
            } while (Process32NextW(snap, ref e));
            return 0;
        }
        finally { CloseHandle(snap); }
    }

    /// <summary>LoadLibraryW injection into pid.</summary>
    private static bool Inject(uint pid, string dllPath)
    {
        nint hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if (hProc == 0) return false;
        try
        {
            byte[] pathBytes = Encoding.Unicode.GetBytes(dllPath + "\0");
            nint remote = VirtualAllocEx(hProc, 0, (nuint)pathBytes.Length,
                                         MEM_COMMIT_RESERVE, PAGE_READWRITE);
            if (remote == 0) return false;
            try
            {
                if (!WriteProcessMemory(hProc, remote, pathBytes,
                                        (nuint)pathBytes.Length, out _))
                    return false;

                nint k32     = GetModuleHandle("kernel32.dll");
                nint loadLib = GetProcAddress(k32, "LoadLibraryW");
                if (loadLib == 0) return false;

                nint thread = CreateRemoteThread(hProc, 0, 0, loadLib, remote, 0, out _);
                if (thread == 0) return false;
                try
                {
                    WaitForSingleObject(thread, INFINITE);
                    GetExitCodeThread(thread, out uint code);
                    return code != 0;          // LoadLibraryW returns the HMODULE
                }
                finally { CloseHandle(thread); }
            }
            finally { VirtualFreeEx(hProc, remote, 0, MEM_RELEASE); }
        }
        finally { CloseHandle(hProc); }
    }

    /// <summary>
    /// Invoke xcutor_execute_remote inside the target process via CreateRemoteThread.
    ///
    /// Steps:
    ///   1. Load xcutor.dll locally to compute the export's byte offset from its module base.
    ///   2. Find the same DLL loaded in the remote process to get its remote base.
    ///   3. Compute remote function VA = remote base + local offset.
    ///   4. Allocate a RemoteScriptArgs struct in the remote process and write the source.
    ///   5. Spawn a thread at xcutor_execute_remote passing the struct pointer.
    /// </summary>
    private static bool Execute(uint pid, string dllPath, string source)
    {
        // ── 1. Compute local export offset ────────────────────────────────
        nint localMod = NativeLibrary.Load(dllPath);
        if (localMod == 0) return false;

        bool hasExport = NativeLibrary.TryGetExport(localMod,
                             "xcutor_execute_remote", out nint localFn);
        long offset = localFn.ToInt64() - localMod.ToInt64();
        NativeLibrary.Free(localMod);
        if (!hasExport) return false;

        // ── 2. Find the DLL base inside the remote process ────────────────
        nint hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if (hProc == 0) return false;
        try
        {
            nint remoteBase = FindRemoteModule(hProc, Path.GetFileName(dllPath));
            if (remoteBase == 0) return false;

            nint remoteFn = new nint(remoteBase.ToInt64() + offset);

            // ── 3. Marshal RemoteScriptArgs into remote memory ─────────────
            // struct layout must mirror RemoteScriptArgs in dllmain.cpp:
            //   char source[0x8000]   (32 768 bytes, double-base64-encoded UTF-8, null-terminated)
            //   char chunk_name[64]
            const int SRC_SIZE   = 0x8000;
            const int NAME_SIZE  = 64;
            const int TOTAL      = SRC_SIZE + NAME_SIZE;

            // Encoding pipeline: XOR → base64 → base64
            byte[] srcBytes     = Encoding.UTF8.GetBytes(source);
            byte[] xoredBytes   = XorApply(srcBytes);
            string b64Once      = Convert.ToBase64String(xoredBytes);
            byte[] b64OnceBytes = Encoding.ASCII.GetBytes(b64Once);
            byte[] encodedBytes = Encoding.ASCII.GetBytes(Convert.ToBase64String(b64OnceBytes));
            if (encodedBytes.Length >= SRC_SIZE) return false;   // script too large after encoding

            byte[] argBuf = new byte[TOTAL];
            Buffer.BlockCopy(encodedBytes, 0, argBuf, 0, encodedBytes.Length);
            byte[] nameBytes = Encoding.UTF8.GetBytes("ui\0");
            Buffer.BlockCopy(nameBytes, 0, argBuf, SRC_SIZE, nameBytes.Length);

            nint remote = VirtualAllocEx(hProc, 0, (nuint)TOTAL,
                                         MEM_COMMIT_RESERVE, PAGE_READWRITE);
            if (remote == 0) return false;
            try
            {
                if (!WriteProcessMemory(hProc, remote, argBuf, (nuint)TOTAL, out _))
                    return false;

                nint thread = CreateRemoteThread(hProc, 0, 0, remoteFn, remote,
                                                 0, out _);
                if (thread == 0) return false;
                try   { WaitForSingleObject(thread, 8000); return true; }
                finally { CloseHandle(thread); }
            }
            finally { VirtualFreeEx(hProc, remote, 0, MEM_RELEASE); }
        }
        finally { CloseHandle(hProc); }
    }

    /// <summary>Walk the remote process module list to find a module by file name.</summary>
    private static nint FindRemoteModule(nint hProc, string modName)
    {
        EnumProcessModulesEx(hProc, null, 0, out uint needed, LIST_MODULES_64);
        if (needed == 0) return 0;

        var mods = new nint[needed / (uint)nint.Size];
        if (!EnumProcessModulesEx(hProc, mods, needed, out _, LIST_MODULES_64))
            return 0;

        var sb = new StringBuilder(260);
        foreach (nint mod in mods)
        {
            sb.Clear();
            GetModuleFileNameExW(hProc, mod, sb, 260);
            if (Path.GetFileName(sb.ToString())
                    .Equals(modName, StringComparison.OrdinalIgnoreCase))
                return mod;
        }
        return 0;
    }
}
