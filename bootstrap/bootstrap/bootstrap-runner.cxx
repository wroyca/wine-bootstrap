#include <bootstrap/bootstrap-runner.hxx>

#include <iostream>
#include <stdexcept>
#include <system_error>
#include <thread>

#include <windows.h>
#include <psapi.h>

using namespace std;
using namespace std::chrono;
using namespace std::filesystem;

namespace bootstrap
{
  // Check if debugger is attached to a process.
  //
  static bool
  is_debugger_attached (void* process)
  {
    BOOL debugger_present (FALSE);
    if (!CheckRemoteDebuggerPresent (process, &debugger_present))
      return false;

    return debugger_present == TRUE;
  }

  bootstrap_runner::
  bootstrap_runner (const path& exe,
                    const path& dll,
                    const path& cwd,
                    const options& opts)
      : options_ (opts)
  {
    // Create suspended process.
    //
    process_bootstrap proc (exe, cwd);
    process_id_ = proc.id ();

    // Duplicate handle so we own it after proc destructor.
    //
    HANDLE current_process (GetCurrentProcess ());
    HANDLE duplicated;
    if (!DuplicateHandle (current_process,
                          proc.handle (),
                          current_process,
                          &duplicated,
                          0,
                          FALSE,
                          DUPLICATE_SAME_ACCESS))
    {
      throw system_error (GetLastError (), system_category (),
                          "DuplicateHandle");
    }

    process_ = win32_handle (duplicated);

    proc.inject (dll);

    if (options_.wait_for_debugger)
    {
      cout << "process " << process_id_ << " started.\n"
           << "waiting for debugger..." << endl;

      wait_for_debugger ();
    }
  }

  bool bootstrap_runner::
  wait_for_debugger ()
  {
    const auto start (steady_clock::now ());

    while (!is_debugger_attached (process_.get ()))
    {
      if (steady_clock::now () - start > options_.debugger_timeout)
      {
        cerr << "Timeout waiting for debugger" << endl;
        return false;
      }

      // Check if process is still running.
      //
      DWORD exit_code;
      if (!GetExitCodeProcess (process_.get (), &exit_code))
        return false;

      if (exit_code != STILL_ACTIVE)
      {
        cerr << "process exited before debugger attached" << endl;
        return false;
      }

      this_thread::sleep_for (milliseconds (100));
    }

    cout << "debugger attached to process " << process_id_ << endl;
    return true;
  }

  unsigned long bootstrap_runner::
  wait ()
  {
    if (WaitForSingleObject (process_.get (), INFINITE) == WAIT_FAILED)
      throw system_error (GetLastError (), system_category (),
                          "WaitForSingleObject");

    DWORD exit_code;
    if (!GetExitCodeProcess (process_.get (), &exit_code))
      throw system_error (GetLastError (), system_category (),
                          "GetExitCodeProcess");

    return exit_code;
  }

  bool bootstrap_runner::
  running () const
  {
    DWORD exit_code;
    if (!GetExitCodeProcess (process_.get (), &exit_code))
      return false;

    return exit_code == STILL_ACTIVE;
  }

  bool bootstrap_runner::
  terminate (unsigned int exit_code)
  {
    return TerminateProcess (process_.get (), exit_code) != 0;
  }

  int
  run_bootstrap (int argc, char* argv[])
  {
    if (argc < 3)
    {
      auto& o (cout);
      auto& a (argv[0]);

      o << "usage: " << a << " <target.exe> <inject.dll> [options]" << "\n"
        << "options:"                                               << "\n"
        << "  --wait-debugger    Wait for debugger to attach"       << "\n"
        << "  --no-wait          Don't wait for process exit"       << "\n"
        << "  --cwd <path>       Set working directory"             << "\n";

      return 1;
    }

    try
    {
      path exe (canonical (argv[1]));
      path dll (canonical (argv[2]));
      path cwd;

      bootstrap_runner::options opts;

      // Parse options.
      //
      for (int i = 3; i < argc; ++i)
      {
        string arg (argv[i]);

        if (arg == "--wait-debugger")
          opts.wait_for_debugger = true;
        else if (arg == "--no-wait")
          opts.wait_for_exit = false;
        else if (arg == "--cwd" && i + 1 < argc)
          cwd = canonical (argv[++i]);
        else
        {
          cerr << "unknown option: " << arg << endl;
          return 1;
        }
      }

      if (!exists (exe))
      {
        cerr << "error: executable not found: " << exe << endl;
        return 1;
      }

      if (!exists (dll))
      {
        cerr << "error: dll not found: " << dll << endl;
        return 1;
      }

      bootstrap_runner runner (exe, dll, cwd, opts);

      cout << "Bootstrapped process " << runner.process_id ()
           << " with " << dll.filename () << endl;

      if (opts.wait_for_exit)
      {
        unsigned long exit_code = runner.wait ();
        cout << "process exited with code " << exit_code << endl;
        return static_cast<int> (exit_code);
      }

      return 0;
    }
    catch (const exception& e)
    {
      cerr << "error: " << e.what () << endl;
      return 1;
    }
  }
}
