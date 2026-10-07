#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
 
#define PREVIEW_BYTES 256
 
static void print_file_info(const char *path, const struct stat *st)
{
    printf("\n--- File Information (fstat) ---\n");
    printf("Path         : %s\n", path);
    printf("Size         : %lld bytes\n", (long long)st->st_size);
    printf("Permissions  : %o (octal)\n", (unsigned)(st->st_mode & 0777));
    printf("Inode        : %llu\n", (unsigned long long)st->st_ino);
    printf("Regular file : %s\n", S_ISREG(st->st_mode) ? "yes" : "no");
}
 
static void print_preview(const char *map, size_t length)
{
    size_t n = (length < PREVIEW_BYTES) ? length : PREVIEW_BYTES;
    printf("\n--- Mapped Content (first %zu bytes) ---\n", n);
    fwrite(map, 1, n, stdout);
    printf("\n--- End of Preview ---\n");
}
 
int main(int argc, char *argv[])
{
    char path[PATH_MAX];
    char line[512];
    struct stat st;
    int fd;
    char *map;
    size_t length;
    long page_size;
 
    /* Step 1-2: obtain the file name */
    if (argc >= 2) {
        strncpy(path, argv[1], sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    } else {
        printf("Enter file name to map: ");
        if (fgets(path, sizeof(path), stdin) == NULL) {
            fprintf(stderr, "Error: no input received.\n");
            return EXIT_FAILURE;
        }
        path[strcspn(path, "\n")] = '\0';
    }
    if (path[0] == '\0') {
        fprintf(stderr, "Error: file name must not be empty.\n");
        return EXIT_FAILURE;
    }
 
    /* Step 3: open the file for reading and writing */
    fd = open(path, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Error: open(\"%s\") failed: %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }
    printf("File opened successfully (file descriptor = %d).\n", fd);
 
    /* Step 4: obtain file metadata */
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "Error: fstat() failed: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }
    print_file_info(path, &st);
 
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: only regular files can be mapped by this program.\n");
        close(fd);
        return EXIT_FAILURE;
    }
 
    /* Step 5: determine the mapping size */
    if (st.st_size == 0) {
        fprintf(stderr, "Error: file is empty; a zero-length mapping is not allowed.\n");
        close(fd);
        return EXIT_FAILURE;
    }
    length = (size_t)st.st_size;
    page_size = sysconf(_SC_PAGESIZE);
 
    /* Step 6: map the file into the virtual address space */
    map = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "Error: mmap() failed: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }
 
    /* Step 7: display mapping information */
    printf("\n--- Mapping Information ---\n");
    printf("Mapped address : %p\n", (void *)map);
    printf("Mapped length  : %zu bytes\n", length);
    printf("Page size      : %ld bytes\n", page_size);
    printf("Pages spanned  : %ld\n", ((long)length + page_size - 1) / page_size);
    printf("Protection     : PROT_READ | PROT_WRITE\n");
    printf("Mapping type   : MAP_SHARED\n");
    printf("Process ID     : %d\n", (int)getpid());
    printf("Press Enter to continue (inspect /proc/%d/maps now if required)...",
           (int)getpid());
    fflush(stdout);
    (void)fgets(line, sizeof(line), stdin);
 
    /* Step 8: read the file through the mapped memory */
    print_preview(map, length);
 
    /* Step 9: modify the mapped region */
    printf("\nEnter byte offset to modify (0 to %zu): ", length - 1);
    if (fgets(line, sizeof(line), stdin) != NULL) {
        char *end;
        errno = 0;
        long offset = strtol(line, &end, 10);
        if (end == line || errno != 0 || offset < 0 || (size_t)offset >= length) {
            fprintf(stderr, "Invalid offset. No modification performed.\n");
        } else {
            printf("Enter replacement text: ");
            if (fgets(line, sizeof(line), stdin) != NULL) {
                size_t n = strcspn(line, "\n");
                if (n > length - (size_t)offset)
                    n = length - (size_t)offset;   /* never write past the mapping */
                memcpy(map + offset, line, n);
                printf("%zu byte(s) written through the mapping at offset %ld.\n",
                       n, offset);
            }
        }
    }
 
    /* Step 10: synchronize the mapped changes with the file */
    if (msync(map, length, MS_SYNC) == -1)
        fprintf(stderr, "Error: msync() failed: %s\n", strerror(errno));
    else
        printf("msync(MS_SYNC) completed: changes requested to be written to the file.\n");
 
    /* Step 11: remove the mapping */
    if (munmap(map, length) == -1)
        fprintf(stderr, "Error: munmap() failed: %s\n", strerror(errno));
    else
        printf("munmap() completed: mapping removed from the address space.\n");
 
    /* Step 12: release the file descriptor */
    if (close(fd) == -1)
        fprintf(stderr, "Error: close() failed: %s\n", strerror(errno));
    else
        printf("close() completed: file descriptor released.\n");
 
    /* Step 13: final status */
    printf("\nOperation finished successfully.\n");
    return EXIT_SUCCESS;
}
 
