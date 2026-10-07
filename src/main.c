/**
 * ============================================================================
 * LINUX MEMORY-MAPPED FILE MANAGEMENT SYSTEM
 * ============================================================================
 * 
 * Course      : Operating Systems (B.Tech CSE)
 * Subject     : Linux System Programming & Virtual Memory Management
 * Language    : C (POSIX / C11 Standard)
 * Compiler    : GCC
 * Platform    : Linux / Ubuntu / WSL
 * 
 * CORE OS CONCEPTS DEMONSTRATED:
 * ----------------------------------------------------------------------------
 * 1. File Descriptors & File Management (open, close, unlink)
 * 2. File Metadata Retrieval (fstat)
 * 3. File Sizing & Truncation (ftruncate)
 * 4. Virtual Memory & Address Space Mapping (mmap)
 * 5. Page-based In-Memory Data Access & Direct Pointers
 * 6. Memory-File Cache Synchronization (msync with MS_SYNC)
 * 7. Memory Unmapping & Address Space Teardown (munmap)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

/* Global Application State Structure */
typedef struct {
    int fd;                     /* Active file descriptor (-1 if closed) */
    char filename[256];         /* Path/Name of currently open file */
    void *mapped_memory;        /* Pointer to starting virtual address of mapping */
    size_t mapped_size;         /* Size of memory region mapped (in bytes) */
    off_t file_size;            /* Actual size of physical file on disk */
    int is_file_open;           /* Flag: 1 if open, 0 otherwise */
    int is_mapped;              /* Flag: 1 if mmap active, 0 otherwise */
} AppState;

static AppState state = {
    .fd = -1,
    .filename = "",
    .mapped_memory = NULL,
    .mapped_size = 0,
    .file_size = 0,
    .is_file_open = 0,
    .is_mapped = 0
};

/* Forward Declarations of Core Functions */
void print_menu(void);
void create_file(void);
void open_file(void);
void display_file_info(void);
void resize_file(void);
void map_file(void);
void display_mapped_data(void);
void modify_mapped_data(void);
void sync_mapped_file(void);
void unmap_file(void);
void close_file(void);
void delete_file(void);
void demonstrate_mmap(void);
void cleanup(void);

/* Utility Functions */
static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static void get_safe_string(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        } else {
            clear_input_buffer();
        }
    }
}

static long get_safe_long(const char *prompt) {
    char input[64];
    long val;
    char *endptr;

    while (1) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return -1;
        }
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        } else {
            clear_input_buffer();
        }

        errno = 0;
        val = strtol(input, &endptr, 10);
        if (endptr == input || *endptr != '\0' || errno != 0) {
            printf("[!] Invalid numeric input. Please try again.\n");
            continue;
        }
        return val;
    }
}

/* Helper to update local file_size stat */
static int update_file_size_stat(void) {
    if (!state.is_file_open || state.fd < 0) return -1;
    struct stat st;
    if (fstat(state.fd, &st) == -1) {
        perror("[!] fstat error");
        return -1;
    }
    state.file_size = st.st_size;
    return 0;
}

/* Main Entry Point */
int main(void) {
    int choice = -1;

    printf("\n====================================================\n");
    printf("  WELCOME TO LINUX MEMORY-MAPPED FILE SYSTEM MANAGER\n");
    printf("====================================================\n");

    while (1) {
        print_menu();
        choice = (int)get_safe_long("Enter your choice (0-12): ");
        printf("\n----------------------------------------------------\n");

        switch (choice) {
            case 1:  create_file(); break;
            case 2:  open_file(); break;
            case 3:  display_file_info(); break;
            case 4:  resize_file(); break;
            case 5:  map_file(); break;
            case 6:  display_mapped_data(); break;
            case 7:  modify_mapped_data(); break;
            case 8:  sync_mapped_file(); break;
            case 9:  unmap_file(); break;
            case 10: close_file(); break;
            case 11: delete_file(); break;
            case 12: demonstrate_mmap(); break;
            case 0:
                printf("[*] Exiting program. Cleaning up allocated resources...\n");
                cleanup();
                printf("[+] Cleanup complete. Goodbye!\n");
                return 0;
            default:
                printf("[!] Invalid choice option: %d. Please select between 0 and 12.\n", choice);
                break;
        }
        printf("----------------------------------------------------\n");
    }

    return 0;
}

/* Display User Interface Menu */
void print_menu(void) {
    printf("\n====================================================\n");
    printf("       LINUX MEMORY-MAPPED FILE MANAGEMENT SYSTEM   \n");
    printf("====================================================\n");
    printf(" Current State: ");
    if (state.is_file_open) {
        printf("File Open [%s | FD: %d | Size: %ld bytes]", state.filename, state.fd, (long)state.file_size);
        if (state.is_mapped) {
            printf(" -> Mapped [%p | %zu bytes]", state.mapped_memory, state.mapped_size);
        } else {
            printf(" -> Not Mapped");
        }
    } else {
        printf("No File Open");
    }
    printf("\n----------------------------------------------------\n");
    printf(" 1. Create File\n");
    printf(" 2. Open File\n");
    printf(" 3. Display File Information\n");
    printf(" 4. Resize File\n");
    printf(" 5. Map File into Memory (mmap)\n");
    printf(" 6. Display Mapped Data\n");
    printf(" 7. Modify Mapped Data\n");
    printf(" 8. Synchronize Changes (msync)\n");
    printf(" 9. Unmap File (munmap)\n");
    printf("10. Close File\n");
    printf("11. Delete File (unlink)\n");
    printf("12. Demonstrate mmap Workflow\n");
    printf(" 0. Exit\n");
    printf("====================================================\n");
}

/**
 * 1. CREATE FILE
 * System Call: open() with O_RDWR | O_CREAT | O_TRUNC
 * Permission: 0644 (Read/Write for owner, Read for group/others)
 */
void create_file(void) {
    char name[256];
    char initial_text[512];

    if (state.is_file_open) {
        printf("[!] A file (%s) is already open. Please close or unmap it first.\n", state.filename);
        return;
    }

    get_safe_string("Enter filename to create (e.g., sample_files/test.txt): ", name, sizeof(name));
    if (strlen(name) == 0) {
        printf("[!] Filename cannot be empty.\n");
        return;
    }

    /* Open/Create with R/W permissions */
    int fd = open(name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("[!] Failed to create file (open failed)");
        return;
    }

    state.fd = fd;
    strncpy(state.filename, name, sizeof(state.filename) - 1);
    state.filename[sizeof(state.filename) - 1] = '\0';
    state.is_file_open = 1;
    state.is_mapped = 0;
    state.mapped_memory = NULL;
    state.mapped_size = 0;
    update_file_size_stat();

    printf("[+] File created successfully!\n");
    printf("    File Descriptor : %d\n", state.fd);
    printf("    Filename        : %s\n", state.filename);

    /* Optionally add initial content */
    get_safe_string("Enter initial content for the file (or press Enter for empty file): ", initial_text, sizeof(initial_text));
    if (strlen(initial_text) > 0) {
        ssize_t written = write(state.fd, initial_text, strlen(initial_text));
        if (written == -1) {
            perror("[!] Failed to write initial content");
        } else {
            printf("[+] Wrote %zd bytes of initial text using write().\n", written);
        }
        update_file_size_stat();
    }
}

/**
 * 2. OPEN FILE
 * System Call: open() with O_RDWR
 */
void open_file(void) {
    char name[256];

    if (state.is_file_open) {
        printf("[!] A file (%s) is already open. Please close or unmap it first.\n", state.filename);
        return;
    }

    get_safe_string("Enter filename to open: ", name, sizeof(name));
    if (strlen(name) == 0) {
        printf("[!] Filename cannot be empty.\n");
        return;
    }

    int fd = open(name, O_RDWR);
    if (fd == -1) {
        perror("[!] Failed to open file (open failed)");
        return;
    }

    state.fd = fd;
    strncpy(state.filename, name, sizeof(state.filename) - 1);
    state.filename[sizeof(state.filename) - 1] = '\0';
    state.is_file_open = 1;
    state.is_mapped = 0;
    state.mapped_memory = NULL;
    state.mapped_size = 0;
    update_file_size_stat();

    printf("[+] File opened successfully!\n");
    printf("    File Descriptor : %d\n", state.fd);
    printf("    Filename        : %s\n", state.filename);
    printf("    Current Size    : %ld bytes\n", (long)state.file_size);
}

/**
 * 3. DISPLAY FILE INFORMATION
 * System Call: fstat()
 */
void display_file_info(void) {
    if (!state.is_file_open) {
        printf("[!] Error: No file is currently open.\n");
        return;
    }

    struct stat st;
    if (fstat(state.fd, &st) == -1) {
        perror("[!] fstat failed");
        return;
    }

    state.file_size = st.st_size;

    printf("---------------- FILE INFORMATION ----------------\n");
    printf("File Name            : %s\n", state.filename);
    printf("File Descriptor      : %d\n", state.fd);
    printf("File Size            : %ld bytes\n", (long)st.st_size);
    printf("Permissions (Octal)  : %o\n", st.st_mode & 0777);
    printf("Hard Link Count      : %ld\n", (long)st.st_nlink);
    printf("Owner User ID        : %d\n", st.st_uid);
    printf("Owner Group ID       : %d\n", st.st_gid);
    printf("I/O Block Size       : %ld bytes\n", (long)st.st_blksize);
    printf("Blocks Allocated     : %ld (512-byte blocks)\n", (long)st.st_blocks);

    char time_buf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("Last Modification    : %s\n", time_buf);

    printf("Mapped Status        : %s\n", state.is_mapped ? "YES" : "NO");
    if (state.is_mapped) {
        printf("Mapped Address       : %p\n", state.mapped_memory);
        printf("Mapped Size          : %zu bytes\n", state.mapped_size);
    }
    printf("--------------------------------------------------\n");
}

/**
 * 4. RESIZE FILE
 * System Call: ftruncate()
 * 
 * OS Note: If a file is truncated to a smaller or larger size while mapped,
 * accessing beyond the new size may raise SIGBUS or return undefined data.
 * Therefore, if mapped, we notify the user and safely unmap before truncation,
 * requiring remapping after resize.
 */
void resize_file(void) {
    if (!state.is_file_open) {
        printf("[!] Error: No file is currently open.\n");
        return;
    }

    long new_size = get_safe_long("Enter new file size in bytes: ");
    if (new_size < 0) {
        printf("[!] File size cannot be negative.\n");
        return;
    }

    if (state.is_mapped) {
        printf("[!] Warning: File is currently mapped into memory (%zu bytes).\n", state.mapped_size);
        printf("[*] To prevent SIGBUS faults or dangling page tables, automatically unmapping memory first...\n");
        unmap_file();
    }

    if (ftruncate(state.fd, (off_t)new_size) == -1) {
        perror("[!] ftruncate failed");
        return;
    }

    state.file_size = (off_t)new_size;
    printf("[+] File resized successfully.\n");
    printf("    New File Size: %ld bytes\n", (long)state.file_size);
}

/**
 * 5. MAP FILE INTO MEMORY
 * System Call: mmap()
 * 
 * OS Explanation:
 * mmap() creates a new mapping in the virtual address space of the calling process.
 * Flags:
 *  - PROT_READ | PROT_WRITE : Memory pages can be read and written.
 *  - MAP_SHARED             : Updates to the mapping are shared across processes
 *                             and visible in the underlying file upon msync/flush.
 */
void map_file(void) {
    if (!state.is_file_open) {
        printf("[!] Error: No file is currently open. Please open or create a file first.\n");
        return;
    }

    if (state.is_mapped) {
        printf("[!] Error: File is already mapped at address %p (%zu bytes).\n", state.mapped_memory, state.mapped_size);
        printf("[*] Unmap existing memory before creating a new mapping.\n");
        return;
    }

    update_file_size_stat();

    if (state.file_size <= 0) {
        printf("[!] Error: File size is %ld bytes.\n", (long)state.file_size);
        printf("    POSIX mmap() requires a non-zero length mapping. Attempting to map\n");
        printf("    a 0-byte file causes mmap() to fail with EINVAL or raise SIGBUS on access.\n");
        printf("[*] Tip: Use Option 4 (Resize File) or write initial data to set file size > 0.\n");
        return;
    }

    /* Map the entire file size */
    void *addr = mmap(NULL, (size_t)state.file_size, PROT_READ | PROT_WRITE, MAP_SHARED, state.fd, 0);
    if (addr == MAP_FAILED) {
        perror("[!] mmap failed");
        return;
    }

    state.mapped_memory = addr;
    state.mapped_size = (size_t)state.file_size;
    state.is_mapped = 1;

    printf("[+] Memory mapping created successfully!\n");
    printf("    Virtual Memory Address : %p\n", state.mapped_memory);
    printf("    Mapped Size            : %zu bytes\n", state.mapped_size);
    printf("    Protection Flags       : PROT_READ | PROT_WRITE\n");
    printf("    Mapping Flags          : MAP_SHARED\n");
}

/**
 * 6. DISPLAY MAPPED DATA
 * Direct Memory Pointer Dereferencing
 */
void display_mapped_data(void) {
    if (!state.is_mapped || state.mapped_memory == NULL) {
        printf("[!] Error: No file is currently mapped into memory.\n");
        return;
    }

    printf("---------------- MAPPED FILE CONTENT ----------------\n");
    printf("Mapped Virtual Address : %p\n", state.mapped_memory);
    printf("Mapped Size            : %zu bytes\n", state.mapped_size);
    printf("-----------------------------------------------------\n");

    const char *ptr = (const char *)state.mapped_memory;
    size_t display_bytes = state.mapped_size;
    int truncated = 0;

    /* Cap preview to 4096 bytes for large mappings */
    if (display_bytes > 4096) {
        display_bytes = 4096;
        truncated = 1;
    }

    /* Print content safely */
    for (size_t i = 0; i < display_bytes; i++) {
        char c = ptr[i];
        if (c == '\0') {
            printf("\\0");
        } else if (isprint((unsigned char)c) || c == '\n' || c == '\t' || c == '\r') {
            putchar(c);
        } else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    if (truncated) {
        printf("\n... [Output truncated at 4096 bytes of %zu total bytes]", state.mapped_size);
    }
    printf("\n-----------------------------------------------------\n");
}

/**
 * 7. MODIFY MAPPED DATA
 * Direct In-Memory Write via Pointer Dereferencing
 * 
 * OS Note: This demonstrates modifying the file contents WITHOUT using read() or write().
 * The process writes directly into its user-space virtual memory pages.
 */
void modify_mapped_data(void) {
    if (!state.is_mapped || state.mapped_memory == NULL) {
        printf("[!] Error: No file is currently mapped into memory.\n");
        return;
    }

    printf("---------------- MODIFY MAPPED MEMORY ----------------\n");
    printf("Mapped Address Range : %p to %p (%zu bytes)\n",
           state.mapped_memory,
           (void *)((char *)state.mapped_memory + state.mapped_size),
           state.mapped_size);

    long offset = get_safe_long("Enter byte offset to start writing (0 to size-1): ");
    if (offset < 0 || (size_t)offset >= state.mapped_size) {
        printf("[!] Error: Offset %ld is out of bounds (valid range: 0 to %zu).\n", offset, state.mapped_size - 1);
        return;
    }

    char new_data[1024];
    get_safe_string("Enter new text content to write into mapped memory: ", new_data, sizeof(new_data));
    size_t text_len = strlen(new_data);

    if (text_len == 0) {
        printf("[!] Warning: Empty string entered. No changes made.\n");
        return;
    }

    if ((size_t)offset + text_len > state.mapped_size) {
        printf("[!] Warning: Input text length (%zu) exceeds mapped memory boundary from offset %ld!\n", text_len, offset);
        printf("[!] Truncating input to fit remaining %zu bytes of mapped space.\n", state.mapped_size - (size_t)offset);
        text_len = state.mapped_size - (size_t)offset;
    }

    /* Direct memory modification via C pointer copy */
    char *dest = (char *)state.mapped_memory + offset;
    memcpy(dest, new_data, text_len);

    printf("[+] Direct memory modification successful!\n");
    printf("    Wrote %zu bytes directly to memory address %p.\n", text_len, (void *)dest);
    printf("    Note: Memory pages marked dirty. Call msync() (Option 8) to flush to disk immediately.\n");
}

/**
 * 8. SYNCHRONIZE CHANGES
 * System Call: msync()
 * 
 * OS Explanation:
 * Flushes dirty pages back to physical storage synchronously using MS_SYNC.
 */
void sync_mapped_file(void) {
    if (!state.is_mapped || state.mapped_memory == NULL) {
        printf("[!] Error: No file is currently mapped into memory.\n");
        return;
    }

    printf("[*] Invoking msync(%p, %zu, MS_SYNC)...\n", state.mapped_memory, state.mapped_size);

    if (msync(state.mapped_memory, state.mapped_size, MS_SYNC) == -1) {
        perror("[!] msync failed");
        return;
    }

    printf("[+] msync() completed successfully! Dirty memory pages flushed to disk.\n");
}

/**
 * 9. UNMAP FILE
 * System Call: munmap()
 * 
 * OS Explanation:
 * munmap() removes the virtual memory mapping for the specified address range.
 */
void unmap_file(void) {
    if (!state.is_mapped || state.mapped_memory == NULL) {
        printf("[!] Error: No active mapping to unmap.\n");
        return;
    }

    printf("[*] Invoking munmap(%p, %zu)...\n", state.mapped_memory, state.mapped_size);

    if (munmap(state.mapped_memory, state.mapped_size) == -1) {
        perror("[!] munmap failed");
        return;
    }

    printf("[+] Memory unmapped successfully.\n");
    state.mapped_memory = NULL;
    state.mapped_size = 0;
    state.is_mapped = 0;
}

/**
 * 10. CLOSE FILE
 * System Call: close()
 */
void close_file(void) {
    if (!state.is_file_open) {
        printf("[!] Error: No file is currently open.\n");
        return;
    }

    /* Unmap memory if still mapped */
    if (state.is_mapped) {
        printf("[*] File is still mapped. Automatically unmapping memory before closing file descriptor...\n");
        unmap_file();
    }

    printf("[*] Closing file descriptor %d (%s)...\n", state.fd, state.filename);
    if (close(state.fd) == -1) {
        perror("[!] close failed");
    } else {
        printf("[+] File closed successfully.\n");
    }

    state.fd = -1;
    state.filename[0] = '\0';
    state.file_size = 0;
    state.is_file_open = 0;
}

/**
 * 11. DELETE FILE
 * System Call: unlink()
 */
void delete_file(void) {
    char target[256];

    if (state.is_file_open) {
        strncpy(target, state.filename, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0';
        printf("[*] Deleting currently open file: %s\n", target);
        close_file();
    } else {
        get_safe_string("Enter filename to delete: ", target, sizeof(target));
        if (strlen(target) == 0) {
            printf("[!] Filename cannot be empty.\n");
            return;
        }
    }

    printf("[*] Invoking unlink(\"%s\")...\n", target);
    if (unlink(target) == -1) {
        perror("[!] unlink failed");
        return;
    }

    printf("[+] File '%s' deleted successfully from filesystem.\n", target);
}

/**
 * 12. DEMONSTRATE MMAP WORKFLOW
 * Automated walkthrough demonstrating the complete lifecycle of memory mapping.
 */
void demonstrate_mmap(void) {
    printf("\n====================================================\n");
    printf("         AUTOMATED mmap() WORKFLOW DEMONSTRATION      \n");
    printf("====================================================\n");

    /* Step 0: Cleanup previous state if open */
    if (state.is_file_open) {
        printf("[*] Pre-demo cleanup: Closing open file descriptor...\n");
        close_file();
    }

    const char *demo_file = "sample_files/demo_mmap.txt";
    const char *initial_data = "Linux Memory Mapping Demonstration File Initial Text.";

    printf("\n[STEP 1] Creating demo file '%s' with open(O_RDWR | O_CREAT | O_TRUNC)...\n", demo_file);
    int fd = open(demo_file, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror(" [!] Demo Step 1 Failed (open)");
        return;
    }
    printf(" [✓] File created with FD = %d\n", fd);

    state.fd = fd;
    strncpy(state.filename, demo_file, sizeof(state.filename) - 1);
    state.filename[sizeof(state.filename) - 1] = '\0';
    state.is_file_open = 1;

    printf("\n[STEP 2] Seeding initial data via write()...\n");
    ssize_t w = write(fd, initial_data, strlen(initial_data));
    if (w == -1) {
        perror(" [!] Demo Step 2 Failed (write)");
        close_file();
        return;
    }
    printf(" [✓] Written %zd bytes to file.\n", w);
    update_file_size_stat();

    printf("\n[STEP 3] Inspecting file info with fstat()...\n");
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror(" [!] Demo Step 3 Failed (fstat)");
        close_file();
        return;
    }
    printf(" [✓] File size verified: %ld bytes\n", (long)st.st_size);

    printf("\n[STEP 4] Executing mmap() system call...\n");
    printf("     mmap(NULL, %ld, PROT_READ | PROT_WRITE, MAP_SHARED, %d, 0);\n", (long)st.st_size, fd);
    void *map = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror(" [!] Demo Step 4 Failed (mmap)");
        close_file();
        return;
    }
    state.mapped_memory = map;
    state.mapped_size = (size_t)st.st_size;
    state.is_mapped = 1;
    printf(" [✓] Mapping established at Virtual Address: %p\n", state.mapped_memory);

    printf("\n[STEP 5] Reading contents directly from Mapped Pointer (No read() call!)\n");
    printf("     Content in Memory: \"%.*s\"\n", (int)state.mapped_size, (char *)state.mapped_memory);

    printf("\n[STEP 6] Modifying memory directly using C pointer dereferencing (memcpy)...\n");
    const char *overwrite_data = "OVERWRITTEN BY MMAP DIRECT POINTER MUTATION!";
    size_t copy_len = strlen(overwrite_data);
    if (copy_len > state.mapped_size) copy_len = state.mapped_size;
    memcpy(state.mapped_memory, overwrite_data, copy_len);
    printf(" [✓] Memory updated. Current Mapped View: \"%.*s\"\n", (int)state.mapped_size, (char *)state.mapped_memory);

    printf("\n[STEP 7] Synchronizing dirty pages to physical storage using msync(MS_SYNC)...\n");
    if (msync(state.mapped_memory, state.mapped_size, MS_SYNC) == -1) {
        perror(" [!] Demo Step 7 Failed (msync)");
    } else {
        printf(" [✓] msync() flushed memory changes to file system.\n");
    }

    printf("\n[STEP 8] Unmapping virtual memory region with munmap()...\n");
    unmap_file();
    printf(" [✓] Memory unmapped.\n");

    printf("\n[STEP 9] Closing File Descriptor with close()...\n");
    close_file();
    printf(" [✓] File descriptor closed.\n");

    printf("\n[STEP 10] Verification: Re-opening file independently and reading via read()...\n");
    int verify_fd = open(demo_file, O_RDONLY);
    if (verify_fd != -1) {
        char verify_buf[256] = {0};
        ssize_t vr = read(verify_fd, verify_buf, sizeof(verify_buf) - 1);
        if (vr > 0) {
            verify_buf[vr] = '\0';
            printf(" [✓] Physical File Content on Disk: \"%s\"\n", verify_buf);
        }
        close(verify_fd);
    }

    printf("\n====================================================\n");
    printf("      DEMONSTRATION COMPLETED SUCCESSFULLY!         \n");
    printf("====================================================\n");
}

/**
 * Safe Cleanup on Exit
 */
void cleanup(void) {
    if (state.is_mapped) {
        unmap_file();
    }
    if (state.is_file_open) {
        close_file();
    }
}
