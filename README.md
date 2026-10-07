# Linux Memory-Mapped File Management System

**Course:** Operating Systems (B.Tech CSE Project)  
**Primary Language:** C (POSIX / C11 Standard)  
**Target Platform:** Linux / Ubuntu / WSL  
**Compiler:** GCC (`gcc -Wall -Wextra -std=c11`)  

---

## 1. Introduction

In traditional Linux file I/O operations using system calls such as `read()` and `write()`, data must be copied across the kernel-user boundary twice:
1. Disk to Kernel Page Cache (Kernel Space)
2. Kernel Page Cache to Process Buffer (User Space)

**Memory mapping (`mmap`)** provides an efficient alternative by mapping a file directly into the process's virtual address space. Operating system page tables map virtual memory pages to kernel page frames backed by the storage file. Once mapped, the process can access and modify file data directly using pointer operations (e.g., `memcpy`, array indexing) without making explicit `read()` or `write()` system calls.

This project implements a command-line **Linux Memory-Mapped File Management System** in C to demonstrate real OS memory-mapping concepts, page faults, virtual memory address management, and synchronization mechanisms.

---

## 2. Objectives

* Demonstrate real Linux memory mapping using POSIX system calls (`mmap`, `munmap`, `msync`).
* Provide a clean, state-aware CLI menu for file creation, opening, inspection, resizing, mapping, editing, synchronizing, unmapping, closing, and deletion.
* Illustrate virtual memory concepts, user-space pointer dereferencing, and dirty page writebacks to storage.
* Implement robust state management and guard against invalid operations (e.g., mapping zero-length files, accessing unmapped memory).
* Provide an automated demonstration mode for academic viva presentations.

---

## 3. Technology Stack

* **Programming Language:** C (C11 standard)
* **Operating System:** Linux / Ubuntu / WSL
* **Compiler:** GCC (`gcc`)
* **Standard POSIX Headers:**
  * `<fcntl.h>`: File control options (`open`, `O_RDWR`, `O_CREAT`, `O_TRUNC`)
  * `<unistd.h>`: System calls (`close`, `ftruncate`, `unlink`)
  * `<sys/mman.h>`: Memory management declarations (`mmap`, `munmap`, `msync`, `MAP_SHARED`, `PROT_READ`, `PROT_WRITE`, `MS_SYNC`)
  * `<sys/stat.h>`: File status and statistics (`fstat`, `struct stat`)
  * `<errno.h>`: System error reporting (`perror`, `errno`)

---

## 4. Key Linux System Calls Used

| System Call | Functionality | OS Role |
| :--- | :--- | :--- |
| `open()` | Opens or creates a file descriptor | Requests file table access from kernel |
| `fstat()` | Obtains metadata of an open file descriptor | Reads inode stat structure (file size, permissions) |
| `ftruncate()` | Resizes physical file to specified length | Adjusts inode block allocations |
| `mmap()` | Maps file into process virtual memory | Allocates Page Table Entries (PTE) mapping virtual address to physical file |
| `msync()` | Synchronizes memory pages with disk storage | Flushes dirty page cache frames to storage using `MS_SYNC` |
| `munmap()` | Unmaps virtual address region | Releases PTE entries and invalidates virtual memory range |
| `close()` | Closes file descriptor | Decrements kernel open file reference count |
| `unlink()` | Deletes filename link from directory | Decrements inode link count, deleting file when count reaches 0 |

---

## 5. Architectural Flow

```text
Traditional File I/O:
+------+       read()        +--------------+      Copy Data      +------------+
| Disk | ------------------> | Kernel Cache | ------------------> | User Space |
+------+                     +--------------+                     +------------+

Memory-Mapped I/O (mmap):
+------+    Kernel Page Table    +-------------------------------+
| Disk | <=====================> | Virtual Address Space (Pointer) |
+------+   (Direct Fault / Sync) +-------------------------------+
```

---

## 6. Compilation & Setup

### Prerequisites
Ensure GCC and Make are installed on your Linux / Ubuntu / WSL environment:
```bash
sudo apt update
sudo apt install build-essential
```

### Build Using Make
```bash
make
```
This executes:
```bash
gcc -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -g main.c -o memory_manager
```

### Build Manually
```bash
gcc -Wall -Wextra -std=c11 main.c -o memory_manager
```

---

## 7. Execution

Run the compiled executable:
```bash
make run
```
or directly:
```bash
./memory_manager
```

---

## 8. Menu Options & Features

```text
====================================================
       LINUX MEMORY-MAPPED FILE MANAGEMENT SYSTEM   
====================================================
 Current State: No File Open
----------------------------------------------------
 1. Create File             - Create new file & seed initial content
 2. Open File               - Open existing file with O_RDWR
 3. Display File Info       - Show inode stat (FD, size, permissions, modification time)
 4. Resize File             - Truncate/extend file using ftruncate()
 5. Map File into Memory    - Map file to process virtual address using mmap()
 6. Display Mapped Data     - Read content directly from virtual memory pointer
 7. Modify Mapped Data      - Modify memory directly using C pointer dereferencing
 8. Synchronize Changes     - Flush dirty pages back to disk via msync(MS_SYNC)
 9. Unmap File              - Release memory range via munmap()
10. Close File              - Safely unmap (if active) and close file descriptor
11. Delete File             - Unlink file from filesystem using unlink()
12. Demonstrate Workflow    - Run automated step-by-step mmap lifecycle
 0. Exit                    - Cleanup resources and terminate safely
====================================================
```

---

## 9. Operating System Concepts Deep Dive

### 9.1. Virtual Memory & Page Tables
When `mmap()` is executed, the Linux kernel creates a Memory Area (`struct vm_area_struct` in kernel space) in the process's Virtual Memory Address space. The kernel does not immediately load the full file content into physical RAM (lazy loading / demand paging).

### 9.2. Page Fault Handling
When the application dereferences the mapped virtual memory pointer:
1. The CPU Memory Management Unit (MMU) checks the Page Table Entry (PTE).
2. If the page is not present in RAM, a **Major/Minor Page Fault** is triggered.
3. The OS Page Fault Handler intercepts the exception, allocates a physical RAM page frame, reads the corresponding 4 KB page block from disk into RAM, and updates the PTE.
4. Execution resumes transparently.

### 9.3. Dirty Pages & Synchronisation (`msync`)
When data is modified via memory pointers (`memcpy`), the CPU marks the memory page as **dirty**. The OS kernel eventually writes dirty pages back to storage during background flushing. Calling `msync()` with `MS_SYNC` forces the kernel to immediately write dirty pages to storage and blocks until I/O is complete.

---

## 10. Advantages & Disadvantages

### Advantages
* **Higher Performance for Large Files:** Eliminates user-kernel buffer copying overhead.
* **Direct Pointer Access:** Simplifies code by treating files as array structures in memory.
* **Shared Memory IPC:** Multiple processes mapping the same file with `MAP_SHARED` can communicate concurrently.

### Disadvantages
* **Overhead for Small Files:** Page granularity (typically 4 KB) means mapping small files wastes address space or page table entries.
* **Page Fault Overhead:** Random access across huge files can trigger frequent page fault interrupts.
* **Careful Bounds Checking:** Out-of-bounds pointer writes cause `SIGSEGV` or `SIGBUS` signals.

---

## 11. Cleanup & Resource Management

The project guarantees resource cleanup:
* Attempts to close or exit automatically invoke `munmap()` if a mapping is active.
* Closing a file descriptor safely resets application state.
* Clean exit handler prevents memory leaks or dangling file descriptors.
