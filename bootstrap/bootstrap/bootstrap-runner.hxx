#pragma once

#include <string>
#include <filesystem>
#include <chrono>

#include <bootstrap/bootstrap.hxx>

namespace bootstrap
{
  // Manages the lifetime of a bootstrapped process.
  //
  class bootstrap_runner
  {
  public:
    struct options
    {
      bool wait_for_debugger;
      bool wait_for_exit;
      std::chrono::milliseconds debugger_timeout;

      options ()
          : wait_for_debugger (false),
            wait_for_exit (true),
            debugger_timeout (30000)
      {
      }
    };

    bootstrap_runner (const std::filesystem::path& exe,
                      const std::filesystem::path& dll,
                      const std::filesystem::path& cwd = std::filesystem::path (),
                      const options& opts = options{});

    unsigned long
    process_id () const noexcept { return process_id_; }

    bool
    wait_for_debugger ();

    unsigned long
    wait ();

    bool
    running () const;

    bool
    terminate (unsigned int exit_code = 1);

  private:
    win32_handle process_;
    unsigned long process_id_;
    options options_;
  };

  // Standalone runner.
  //
  // Returns exit code of the target process or throws on error.
  //
  int
  run_bootstrap (int argc, char* argv[]);
}
