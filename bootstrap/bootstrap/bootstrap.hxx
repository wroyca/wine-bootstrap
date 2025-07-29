#pragma once

#include <string>
#include <memory>
#include <cstddef>
#include <filesystem>

namespace bootstrap
{
  class win32_handle
  {
  public:
    explicit win32_handle (void* h = nullptr) noexcept
      : handle_ (h) {}

    ~win32_handle () noexcept;

    win32_handle (win32_handle&& h) noexcept
      : handle_ (h.release ()) {}

    win32_handle&
    operator= (win32_handle&& h) noexcept
    {
      reset (h.release ());
      return *this;
    }

    void*
    get () const noexcept { return handle_; }

    void*
    release () noexcept
    {
      void* h = handle_;
      handle_ = nullptr;
      return h;
    }

    void
    reset (void* h = nullptr) noexcept;

    explicit
    operator bool () const noexcept { return handle_ != nullptr; }

  private:
    void* handle_;
  };

  class remote_memory
  {
  public:
    remote_memory (void* process, std::size_t size);
    ~remote_memory () noexcept;

    remote_memory (remote_memory&& m) noexcept
        : process_ (m.process_), address_ (m.address_), size_ (m.size_)
    {
      m.address_ = nullptr;
    }

    void
    write (const void* data, std::size_t n);

    void*
    get () const noexcept { return address_; }

  private:
    void* process_;
    void* address_;
    std::size_t size_;
  };

  class remote_thread
  {
  public:
    remote_thread (void* process, void* start_routine, void* parameter);

    unsigned long
    wait ();

    unsigned long
    exit_code ();

  private:
    win32_handle handle_;
  };

  class process_bootstrap
  {
  public:
    explicit
    process_bootstrap (const std::filesystem::path& exe,
                             const std::filesystem::path& cwd = std::filesystem::path ());

    void
    inject (const std::filesystem::path& dll);

    void
    resume ();

    void*
    handle () const noexcept { return process_.get (); }

    unsigned long
    id () const noexcept { return id_; }

  private:
    win32_handle process_;
    win32_handle thread_;
    unsigned long id_;
  };

  // Bootstrap a process with an injected DLL.
  //
  // Start the specified executable in suspended state, inject the specified
  // DLL, and resume execution. The DLL path must be absolute or relative to
  // the current working directory.
  //
  // Throw std::system_error on failure.
  //
  void
  bootstrap (const std::filesystem::path& exe,
             const std::filesystem::path& dll,
             const std::filesystem::path& cwd = std::filesystem::path ());
}
