# wine-bootstrap

A lightweight DLL injector for Wine processes, made for myself to quickly patch or bootstrap code into Windows binaries running under Wine.

Every now and then, Wine processes may exhibit subtle bugs or behavioral quirks, issues that are often simple enough to patch... if you're lucky. In some cases, you can recompile the target process, assuming you have the source. Otherwise, you'll need to patch the faulty behavior at runtime, which requires a way to inject your code.

This injector is something I wrote quickly to solve that specific problem. It's not a general-purpose tool and makes no attempt to support Windows. There are subtle differences between Windows and Wine in how processes behave, and this tool is designed exclusively with Wine in mind. I don't have a Windows machine, and I'm not planning to work around or account for those differences.

## How to Build

The development setup for `wine-bootstrap` uses the standard `bdep`-based workflow from the [build2](https://build2.org) build system.

To build the project:

```sh
git clone https://github.com/wroyca/wine-bootstrap.git
cd wine-bootstrap

bdep init -C @gcc cc config.cxx=g++
bdep update
bdep test
```

You may substitute `@gcc` with another configuration name (e.g., `@clang`) if you prefer a different compiler.

## Usage

```sh
wine-bootstrap <target.exe> <inject.dll> [options]
```

### Options

* `--wait-debugger`
  Pause the target process after injection and wait for a debugger to attach before resuming execution.

* `--no-wait`
  Do not wait for the target process to exit. By default, `wine-bootstrap` will wait for the process lifetime to finish.

* `--cwd <path>`
  Set the working directory for the launched process. If omitted, the current directory is used.
