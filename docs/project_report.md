# ACADEMIC PROJECT REPORT

## Project Title: Linux Memory-Mapped File Management System
**Degree Program:** Bachelor of Technology (B.Tech) in Computer Science & Engineering  
**Course Name:** Operating Systems  
**Implementation Language:** C (POSIX / C11)  
**Target Platform:** Linux / Ubuntu / WSL  

---

## 1. Title Page Information
* **Project Title:** Linux Memory-Mapped File Management System
* **Subject:** Operating Systems
* **Domain:** Systems Programming & Linux Kernel Virtual Memory
* **Developer:** B.Tech CSE Student
* **Development Environment:** VS Code, GCC Compiler, Linux/Ubuntu/WSL

---

## 2. Abstract
Traditional file I/O operations in Unix-like operating systems rely on explicit `read()` and `write()` system calls that copy data back and forth between kernel page caches and user-space memory buffers. This double-copy mechanism introduces CPU cycle overhead, context switching latency, and buffer memory overhead.

This project implements a command-line **Linux Memory-Mapped File Management System** in C using the POSIX `mmap()` system call family. The system allows user-space processes to map file content directly into their virtual address space, enabling high-performance read and write operations via direct pointer manipulation. The application manages file creation, descriptor state transitions, metadata retrieval via `fstat()`, resizing via `ftruncate()`, virtual memory mapping via `mmap()`, memory-to-disk cache synchronization via `msync()`, unmapping via `munmap()`, and deletion via `unlink()`. 

The project incorporates strict state management, robust error handling, and an automated demonstration workflow suitable for Operating Systems laboratory evaluation and viva examinations.

---

## 3. Introduction
In modern operating systems, virtual memory management is a fundamental abstraction provided by the OS kernel in conjunction with hardware Memory Management Units (MMU). Memory-mapped file I/O (`mmap`) bridges storage device files and virtual address spaces by integrating file system cache pages into a process's page table.

Instead of issuing system calls to transfer data blocks sequentially, `mmap()` binds a physical file descriptor to a contiguous range of virtual addresses. When the process dereferences a memory location within this range, the MMU handles address translation and triggers page faults as needed to load file data demand-paged from disk. Modifications made to the virtual address region directly alter the underlying page cache frames, which can then be synchronized to disk using `msync()`.

---

## 4. Problem Statement
Standard file I/O processing (`read`/`write`) exhibits several limitations:
1. **Double Buffering Overhead:** Data is transferred from disk to kernel page cache, then copied to user space memory.
2. **Context Switching Costs:** Every `read()` or `write()` call incurs kernel mode transitions.
3. **Complex Shared Memory IPC:** Shared file access between multiple processes requires manual buffer synchronization.

There is a need for an educational yet production-quality software system that demonstrates how Linux memory-mapped files overcome these challenges through direct pointer-based virtual memory manipulation.

---

## 5. Objectives
* Design and implement a modular C-based Linux terminal utility demonstrating POSIX memory mapping.
* Utilize real Linux kernel system calls (`open`, `fstat`, `ftruncate`, `mmap`, `msync`, `munmap`, `close`, `unlink`).
* Provide clear visualization of process virtual memory addresses, page states, and dirty page writebacks.
* Enforce state safety rules (e.g., zero-length mapping prevention, automatic unmapping before resizing/closing).
* Deliver a complete automated demonstration mode for academic presentation.

---

## 6. Existing System vs. Proposed System

### 6.1 Existing System (Traditional File I/O)
* **Mechanism:** Explicit `read()` and `write()` calls.
* **Memory Flow:** Disk -> Kernel Page Cache -> User Space Buffer -> Modification -> Kernel Page Cache -> Disk.
* **Overhead:** Multiple memory copies and context switches per I/O call.

### 6.2 Proposed System (Memory-Mapped I/O)
* **Mechanism:** Virtual address range mapping via `mmap()`.
* **Memory Flow:** Disk <-> Kernel Page Cache <-> Virtual Address Space Pointer.
* **Advantage:** Zero-copy in user space; direct C pointer assignment (`memcpy`, array access); hardware-accelerated MMU page faults.

---

## 7. System Requirements

### 7.1 Hardware Requirements
* **Processor:** x86_64 or ARM64 Architecture (1.5 GHz or higher)
* **RAM:** Minimum 2 GB (4 GB recommended)
* **Storage:** 50 MB available disk space

### 7.2 Software Requirements
* **Operating System:** Linux / Ubuntu 20.04+ or Windows Subsystem for Linux (WSL2)
* **Compiler:** GCC (GNU Compiler Collection) with standard C11 support
* **Build Tool:** GNU Make
* **Text Editor / IDE:** Visual Studio Code / Terminal

---

## 8. Technologies Used
* **C Programming Language (C11):** Native language for systems programming.
* **Linux Kernel API & POSIX System Calls:** Direct kernel interaction via system headers `<fcntl.h>`, `<unistd.h>`, `<sys/mman.h>`, `<sys/stat.h>`.
* **GNU Compiler Collection (GCC):** Standard compilation with `-Wall -Wextra -std=c11`.

---

## 9. System Architecture
*(Refer to `docs/system_design.md` for complete architectural diagrams, process memory layout, and sequence flows).*

The application is structured into four primary modules:
1. **User Interface Module:** Terminal menu controller and safe input parser.
2. **File Management Module:** Wrapper functions for `open`, `fstat`, `ftruncate`, `close`, `unlink`.
3. **Memory Mapping Module:** Wrapper functions for `mmap`, `msync`, `munmap`.
4. **Demonstration & Verification Module:** Automated 10-step lifecycle test harness.

---

## 10. Linux System Calls Overview

### `open(const char *pathname, int flags, mode_t mode)`
Allocates a file descriptor entry in the process file table referring to `pathname`.

### `fstat(int fd, struct stat *statbuf)`
Retrieves metadata (file size, permissions, owner, modification time) associated with open descriptor `fd`.

### `ftruncate(int fd, off_t length)`
Truncates or expands the file associated with `fd` to exactly `length` bytes.

### `mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)`
Maps `length` bytes starting at `offset` from file descriptor `fd` into the process virtual address space.
* `PROT_READ | PROT_WRITE`: Grants read and write access to pages.
* `MAP_SHARED`: Modifications update the underlying file and are shared with other mapping processes.

### `msync(void *addr, size_t length, int flags)`
Synchronizes modified dirty pages in virtual address range `[addr, addr+length)` with storage disk using flag `MS_SYNC`.

### `munmap(void *addr, size_t length)`
Deallocates virtual memory range `[addr, addr+length)`, removing page table mappings.

### `close(int fd)`
Deallocates the file descriptor.

### `unlink(const char *pathname)`
Removes directory entry referring to `pathname`.

---

## 11. Core Algorithm

```text
ALGORITHM: Memory-Mapped File Management

1. Initialize Application State (fd = -1, is_mapped = 0, is_open = 0).
2. Display Terminal Menu and Read User Choice.
3. IF Choice == 1 (Create File):
     a. Read filename and open with O_RDWR | O_CREAT | O_TRUNC (0644).
     b. Optionally seed initial text via write().
     c. Fetch file size via fstat().
4. IF Choice == 5 (Map File):
     a. Verify file is open and file_size > 0.
     b. Call mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0).
     c. IF mmap returns MAP_FAILED, output error via perror().
     d. ELSE store returned address pointer and set is_mapped = 1.
5. IF Choice == 7 (Modify Mapped Data):
     a. Read target byte offset and new string.
     b. Verify offset + string length <= mapped_size (truncate if necessary).
     c. Copy string into (mapped_memory + offset) using memcpy().
6. IF Choice == 8 (Synchronize):
     a. Call msync(mapped_memory, mapped_size, MS_SYNC).
7. IF Choice == 9 (Unmap):
     a. Call munmap(mapped_memory, mapped_size).
     b. Reset pointer to NULL and set is_mapped = 0.
8. IF Choice == 10 (Close):
     a. IF is_mapped == 1, execute step 7 first.
     b. Call close(fd) and set is_open = 0.
9. IF Choice == 0 (Exit):
     a. Execute cleanup logic and terminate process.
```

---

## 12. Testing & Experimental Results

The application underwent rigorous verification across 15 test cases (detailed in `tests/test_plan.md`). 

Key findings:
* Modifications performed via `memcpy()` on virtual memory pointers persisted to physical storage after calling `msync(MS_SYNC)`.
* Zero-length files correctly reported safety guard warnings, preventing invalid `mmap()` execution.
* Automatic unmapping before closing file descriptors prevented kernel dangling pointer references.

---

## 13. Advantages & Limitations

### Advantages
1. Zero-copy user-space memory access.
2. Simplified data structural manipulation via direct C pointers.
3. Kernel-managed demand paging and hardware MMU page fault acceleration.

### Limitations
1. Minimum mapping granularity is bounded by CPU page size (4096 bytes).
2. Dynamic expansion requires unmapping, resizing via `ftruncate()`, and remapping.

---

## 14. Viva Preparation Highlights

* **What is `MAP_SHARED`?**  
  `MAP_SHARED` ensures modifications to the mapped memory range are written back to the physical file and visible to other processes mapping the same file.
* **Why does `mmap()` fail on zero-byte files?**  
  POSIX mandates that mapping length must be greater than zero. Mapping zero bytes returns `EINVAL` or raises `SIGBUS` upon memory access.
* **What does `msync(..., MS_SYNC)` do?**  
  It forces the kernel to write dirty page cache frames back to physical disk storage synchronously before returning.

---

## 15. Conclusion & References
The **Linux Memory-Mapped File Management System** successfully demonstrates real Linux memory mapping concepts using standard POSIX system calls. The project provides an intuitive CLI interface, robust error handling, and complete academic documentation.

### References
1. Michael Kerrisk, *The Linux Programming Interface*, No Starch Press.
2. Silberschatz, Galvin, Gagne, *Operating System Concepts*, Wiley.
3. Linux Programmer's Manual (`man 2 mmap`, `man 2 msync`, `man 2 fstat`).
