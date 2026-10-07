# System Design & Architecture Document

## Linux Memory-Mapped File Management System

---

## 1. System Architecture Overview

The system bridges user space application logic with Linux kernel virtual memory subsystems using standard POSIX system calls.

```
+-----------------------------------------------------------------------+
|                         USER SPACE PROCESS                            |
|                                                                       |
|  +-----------------------------------------------------------------+  |
|  |             CLI Menu & State Controller (main.c)                |  |
|  +-----------------------------------------------------------------+  |
|            |                        |                       |         |
|            v                        v                       v         |
|  +------------------+     +------------------+    +-----------------+ |
|  | File Descriptor  |     | Virtual Memory   |    | Memory Pointer  | |
|  |  State (fd)      |     |  Address Pointer |    |  Dereference    | |
|  +------------------+     +------------------+    +-----------------+ |
+-----------------------------------------------------------------------+
             |                        |                       |
====== SYSTEM CALL INTERFACE (POSIX System Call Boundary) ==============
             |                        |                       |
             v                        v                       v
+-----------------------------------------------------------------------+
|                         LINUX KERNEL SPACE                            |
|                                                                       |
|  +--------------------+   +--------------------+  +-----------------+ |
|  | Virtual Memory     |   | Page Table Entry   |  | Inode & File    | |
|  | Subsystem (VMA)    |   | (PTE & MMU Fault)  |  | System Manager  | |
|  +--------------------+   +--------------------+  +-----------------+ |
|            |                        |                       |         |
|            +------------------------+-----------------------+         |
|                                     |                                 |
|                                     v                                 |
|                        +------------------------+                     |
|                        | Page Cache (Dirty Pages)|                    |
|                        +------------------------+                     |
+-----------------------------------------------------------------------+
                                      |
                                      v
+-----------------------------------------------------------------------+
|                         PHYSICAL STORAGE                              |
|                          (Block Device / Disk)                        |
+-----------------------------------------------------------------------+
```

---

## 2. Process Virtual Memory Layout

When `mmap()` is executed with `MAP_SHARED`, Linux allocates space in the process's Virtual Memory Address space between the Heap and Stack (the Memory Mapping Segment):

```text
High Address  +-----------------------------------+
              | Kernel Space (Forbidden to User)  |
              +-----------------------------------+
              | User Stack (Local variables)      |
              |                |                  |
              |                v                  |
              |                                   |
              | Memory Mapping Segment            |
              |  [mmap Address: 0x7f...] --------+--> Mapped File Pages
              |                ^                  |    (PROT_READ | PROT_WRITE)
              |                |                  |
              | Heap (malloc / dynamic memory)    |
              +-----------------------------------+
              | Uninitialized Data (.bss)         |
              +-----------------------------------+
              | Initialized Data (.data)          |
              +-----------------------------------+
              | Text Segment (Executable Code)    |
Low Address   +-----------------------------------+
```

---

## 3. Finite State Machine (FSM) Design

The system enforces strict state transition rules to ensure safety and prevent kernel exceptions or dangling memory references:

```mermaid
stateDiagram-v2
    [*] --> Closed: Program Start
    Closed --> FileOpened: open() / create()
    FileOpened --> Closed: close()
    FileOpened --> MemoryMapped: mmap() [file_size > 0]
    MemoryMapped --> FileOpened: munmap()
    MemoryMapped --> Closed: close() [Auto-unmap trigger]
    FileOpened --> Deleted: unlink()
    Closed --> Deleted: unlink()
    Deleted --> [*]
```

### State Guard Rules:
1. **Zero-Length Mapping Guard:** `mmap()` cannot be called on files where `file_size == 0`. The user must extend file size via `ftruncate()` or write data first.
2. **Resizing Guard:** `ftruncate()` on an active mapping triggers an automatic `munmap()` first to prevent `SIGBUS` faults.
3. **Closing Guard:** Invoking `close()` while `is_mapped == 1` automatically invokes `munmap()` before closing the file descriptor.

---

## 4. Detailed Memory-Mapping Sequence Diagram

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant App as Application (User Space)
    participant Kernel as Linux Kernel / MMU
    participant Cache as Page Cache (RAM)
    participant Disk as Physical Storage

    User->>App: Select Menu Option 5 (Map File)
    App->>Kernel: mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)
    Kernel->>App: Return Virtual Address Pointer (e.g. 0x7f...)
    Note over App: Mapping is lazy. No pages loaded yet.

    User->>App: Select Option 6 or 7 (Access/Modify Pointer)
    App->>Kernel: Access memory address 0x7f...
    Kernel->>Kernel: MMU triggers Page Fault (Page not in RAM)
    Kernel->>Disk: Fetch 4KB page block from file
    Disk->>Cache: Load block into Page Cache Frame
    Kernel->>Kernel: Update Page Table Entry (PTE)
    Kernel->>App: Resume instruction (Write/Read byte)
    Note over App,Cache: Page marked Dirty in Cache

    User->>App: Select Option 8 (msync)
    App->>Kernel: msync(addr, size, MS_SYNC)
    Kernel->>Cache: Flush Dirty Pages
    Cache->>Disk: Synchronous Block Write to Disk
    Kernel->>App: Return Success (0)

    User->>App: Select Option 9 (munmap)
    App->>Kernel: munmap(addr, size)
    Kernel->>Kernel: Invalidate PTE entries & release address space
    Kernel->>App: Return Success (0)
```

---

## 5. Security & Safety Design Principles

1. **Bounds Validation:** All memory modifications compute remaining length `(mapped_size - offset)` to prevent buffer overflows or segmentation faults (`SIGSEGV`).
2. **Safe Input Processing:** `fgets()` and custom numeric parsing (`strtol()`) are used throughout to prevent buffer overflow attacks inherent in `gets()` or `scanf()`.
3. **Resource Leak Prevention:** Global `cleanup()` handler guarantees unmapping and descriptor closure on program termination.
