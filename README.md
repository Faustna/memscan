# memscan

A simple Linux memory scanner written in C for locating and modifying a specific integer value inside another running process.

This project demonstrates how to inspect process memory using `ptrace`, `/proc/<pid>/maps`, and `/proc/<pid>/mem`. It is intended for educational purposes and for use only on processes you own or are explicitly authorized to inspect.

> Use this tool only on processes you own or on systems where you have explicit permission to analyze memory.

## Features

- Attaches to a target process with `ptrace(PTRACE_ATTACH)`
- Reads memory mappings from `/proc/<pid>/maps`
- Scans readable memory regions for a 32-bit target value
- Re-checks candidate addresses after the target value changes
- Optionally writes a new value back into the discovered memory location

## Project files

- `memscan.c` — the main scanner logic
- `test_target.c` — a sample process used to demonstrate memory scanning and modification

## Requirements

- Linux
- GCC
- permission to attach to the target process (usually via `sudo` or root)

## Build

```bash
gcc memscan.c -o memscan
gcc test_target.c -o test_target
```

## How to use

### 1. Start the target process

```bash
./test_target
```

The program prints:

- the process ID;
- the address of a local variable;
- the current value of that variable.

Example:

```bash
PID: 12345, Address: 0x7ff...
Current value: 21
```

### 2. Run the scanner

In another terminal:

```bash
sudo ./memscan <PID> <VALUE>
```

Example:

```bash
sudo ./memscan 12345 21
```

The scanner will:

1. attach to the target process;
2. read the process memory map;
3. scan readable pages for the target value;
4. display candidate addresses;
5. allow you to re-check after changing the value in the target process;
6. optionally write a new value to the final matching address.

## Example workflow

```bash
./test_target
```

Then, in another terminal:

```bash
sudo ./memscan 12345 21
```

From there, you can:

- change the variable in the target process;
- enter the new value into `memscan` to narrow down candidates;
- if only one survivor remains, write a new value to memory.

## Notes and limitations

- This is not a production-grade memory editing tool.
- Behavior may vary depending on permissions, ASLR, kernel protections, and process layout.
- The code is primarily intended to demonstrate Linux process memory introspection techniques.
- Do not use this against processes you do not own or are not explicitly authorized to inspect.

## License

No explicit license is included in this repository. Before reusing or redistributing the code, confirm the licensing status with the repository owner.
