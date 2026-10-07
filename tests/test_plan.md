# Comprehensive Test Suite & Test Plan

## Linux Memory-Mapped File Management System

---

## 1. Test Overview

This document defines the formal test suite for verifying functional correctness, error safety, state transitions, and memory synchronization behavior of the Linux Memory-Mapped File Management System.

All tests have been executed on Ubuntu / WSL using GCC compiled binaries.

---

## 2. Test Cases Table

| Test ID | Scenario / Objective | Test Inputs | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Create a new file | Menu: `1`<br>File: `sample_files/test1.txt`<br>Text: `Hello World` | File created with FD >= 3, initial data written via `write()`. | File created, FD=3, 11 bytes written. | **PASS** |
| **TC-02** | Open existing file | Menu: `2`<br>File: `sample_files/test1.txt` | File opened with `O_RDWR`, state updated to open. | File opened, FD=3, size=11 bytes. | **PASS** |
| **TC-03** | Display file info | Menu: `3` (File Open) | `fstat()` displays inode size, permissions, owner, link count, mmap status. | Complete metadata displayed accurately. | **PASS** |
| **TC-04** | Resize file | Menu: `4`<br>Size: `100` | `ftruncate()` resizes file to 100 bytes. File size stat updated. | File resized to 100 bytes. | **PASS** |
| **TC-05** | Map file into memory | Menu: `5` (File Size > 0) | `mmap()` succeeds, returns non-NULL virtual memory pointer. | Mapping established at virtual address `0x7f...`. | **PASS** |
| **TC-06** | Display mapped contents | Menu: `6` (Mapped State) | Displays memory pointer address, mapped size, and exact file text. | Content displayed directly from memory pointer. | **PASS** |
| **TC-07** | Modify mapped contents | Menu: `7`<br>Offset: `0`<br>Text: `Modified Text!` | `memcpy()` modifies memory pages directly at pointer address. | Memory updated directly without `write()`. | **PASS** |
| **TC-08** | Synchronize memory | Menu: `8` (Mapped State) | `msync(MS_SYNC)` flushes dirty pages to physical disk storage. | `msync()` returned success code 0. | **PASS** |
| **TC-09** | Unmap memory | Menu: `9` (Mapped State) | `munmap()` deallocates virtual address range. `is_mapped` set to 0. | Memory unmapped successfully, pointer set to NULL. | **PASS** |
| **TC-10** | Close file | Menu: `10` (File Open) | `close()` closes file descriptor, resets application state. | FD closed, state reset to No File Open. | **PASS** |
| **TC-11** | Map zero-length file | Create 0-byte file<br>Menu: `5` | Guard detects 0-byte file, displays error, prevents `mmap()` failure. | Error message displayed; `mmap()` blocked safely. | **PASS** |
| **TC-12** | Operation without open file | Menu: `5` or `3` (No File Open) | Error message: `Error: No file is currently open.` | Action blocked; error message displayed. | **PASS** |
| **TC-13** | Mapped operation without mmap | Menu: `6` or `7` (File Open, Not Mapped) | Error message: `Error: No file is currently mapped into memory.` | Action blocked; error message displayed. | **PASS** |
| **TC-14** | Invalid menu input | Input: `99` or `abc` | Validation catches invalid input, re-prompts menu cleanly. | Error message displayed; program remains stable. | **PASS** |
| **TC-15** | Invalid file size input | Menu: `4`<br>Size: `-50` | Negative size rejected; `ftruncate()` not invoked. | Error message displayed; size unchanged. | **PASS** |

---

## 3. Automated Demonstration Verification

Executing Option `12` runs an automated end-to-end verification sequence:
1. `open(O_RDWR | O_CREAT | O_TRUNC)` creates `sample_files/demo_mmap.txt`.
2. Seed data written via `write()`.
3. `fstat()` verifies size.
4. `mmap()` establishes virtual address mapping.
5. Direct pointer read displays initial string.
6. Direct pointer write (`memcpy`) mutates memory pages.
7. `msync(MS_SYNC)` flushes changes to physical file.
8. `munmap()` unmaps memory.
9. `close()` closes descriptor.
10. Re-opening file independently via `open(O_RDONLY)` and reading via `read()` confirms that modifications written to mapped memory persisted to physical disk.

**Result:** 100% Pass Rate across all test cases.
