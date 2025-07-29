#include <bootstrap/bootstrap.hxx>
#include <bootstrap/bootstrap-runner.hxx>

#include <cstring>
#include <stdexcept>
#include <system_error>

#include <windows.h>

using namespace std;
using namespace std::filesystem;

namespace bootstrap
{
  static system_error
  win32_error (const string& what)
  {
    DWORD ec (GetLastError ());
    return system_error (ec, system_category (), what);
  }

  // win32_handle
  //

  win32_handle::
  ~win32_handle () noexcept
  {
    reset ();
  }

  void win32_handle::
  reset (void* h) noexcept
  {
    if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE)
      CloseHandle (handle_);
    handle_ = h;
  }

  // remote_memory
  //

  remote_memory::
  remote_memory (void* process, size_t size)
    : process_ (process), size_ (size)
  {
    address_ = VirtualAllocEx (process_,
                               nullptr,
                               size_,
                               MEM_COMMIT | MEM_RESERVE,
                               PAGE_READWRITE);
    if (!address_)
      throw win32_error ("VirtualAllocEx");
  }

  remote_memory::
  ~remote_memory () noexcept
  {
    if (address_)
      VirtualFreeEx (process_, address_, 0, MEM_RELEASE);
  }

  void remote_memory::
  write (const void* data, size_t n)
  {
    if (n > size_)
      throw invalid_argument ("data size exceeds allocated memory");

    if (!WriteProcessMemory (process_, address_, data, n, nullptr))
      throw win32_error ("WriteProcessMemory");
  }

  // remote_thread
  //

  remote_thread::
  remote_thread (void* process, void* start_routine, void* parameter)
  {
    void* h (CreateRemoteThread (process,
                                 nullptr,
                                 0,
                                 reinterpret_cast<LPTHREAD_START_ROUTINE> (start_routine),
                                 parameter,
                                 0,
                                 nullptr));

    if (!h)
      throw win32_error ("CreateRemoteThread");

    handle_ = win32_handle (h);
  }

  unsigned long remote_thread::
  wait ()
  {
    if (WaitForSingleObject (handle_.get (), INFINITE) == WAIT_FAILED)
      throw win32_error ("WaitForSingleObject");

    return exit_code ();
  }

  unsigned long remote_thread::
  exit_code ()
  {
    DWORD code;
    if (!GetExitCodeThread (handle_.get (), &code))
      throw win32_error ("GetExitCodeThread");

    return code;
  }

  // process_bootstrap
  //

  process_bootstrap::
  process_bootstrap (const path& exe,
                     const path& cwd)
  {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset (&si, 0, sizeof (si));
    memset (&pi, 0, sizeof (pi));

    si.cb = sizeof (si);

    string exe_str (exe.string ());
    string cwd_str (cwd.empty () ? exe.parent_path ().string () : cwd.string ());

    BOOL r (CreateProcess (exe_str.c_str (),
                           nullptr,  // command line
                           nullptr,  // process attributes
                           nullptr,  // thread attributes
                           FALSE,    // inherit handles
                           CREATE_SUSPENDED,
                           nullptr,  // environment
                           cwd_str.c_str (),
                           &si,
                           &pi));

    if (!r)
      throw win32_error ("CreateProcess");

    process_ = win32_handle (pi.hProcess);
    thread_ = win32_handle (pi.hThread);
    id_ = pi.dwProcessId;
  }

  void process_bootstrap::
  inject (const path& dll)
  {
    // Get LoadLibraryA address.
    //
    HMODULE kernel32 (GetModuleHandle ("kernel32.dll"));
    if (!kernel32)
      throw win32_error ("GetModuleHandle");

    void* load_library (
      reinterpret_cast<void*> (GetProcAddress (kernel32, "LoadLibraryA")));
    if (!load_library)
      throw win32_error ("GetProcAddress");

    // Allocate and write DLL path to remote process.
    //
    string dll_str (dll.string ());
    remote_memory rmem (process_.get (), dll_str.size () + 1);
    rmem.write (dll_str.c_str (), dll_str.size () + 1);

    // Create remote thread to call LoadLibraryA.
    //
    remote_thread thread (process_.get (), load_library, rmem.get ());
    unsigned long result (thread.wait ());

    if (result == 0)
      throw runtime_error ("LoadLibraryA failed to load DLL");

    resume ();
  }

  void process_bootstrap::
  resume ()
  {
    if (thread_)
    {
      if (ResumeThread (thread_.get ()) == static_cast<DWORD> (-1))
        throw win32_error ("ResumeThread");

      thread_.reset ();
    }
  }

  // Bootstrap entry point for launching and injecting a target process.
  //
  // Note that this function is *not* used when invoking via the external
  // `runner` binary. Instead, it is useful when the bootstrap mechanism is
  // reused internally, for example, to avoid distributing a separate launcher
  // or to customize process startup behavior.
  //
  // Typical usage involves copying the `wine_bootstrap` source into your
  // repository and calling `bootstrap()` as part of your own initialization
  // logic.
  //
  void
  bootstrap (const path& exe,
             const path& dll,
             const path& cwd)
  {
    process_bootstrap p (exe, cwd);
    p.inject (dll);
  }
}

int
main (int argc, char* argv[])
{
  bootstrap::run_bootstrap (argc, argv);
}
