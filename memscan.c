#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <pid> <value>\n", program_name);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    char *endptr = NULL;
    errno = 0;
    pid_t pid = (pid_t)strtol(argv[1], &endptr, 10);
    if (errno != 0 || *argv[1] == '\0' || *endptr != '\0') {
        fprintf(stderr, "Invalid PID: %s\n", argv[1]);
        return 1;
    }

    errno = 0;
    uint32_t target_value = (uint32_t)strtoul(argv[2], &endptr, 10);
    if (errno != 0 || *argv[2] == '\0' || *endptr != '\0') {
        fprintf(stderr, "Invalid value: %s\n", argv[2]);
        return 1;
    }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        fprintf(stderr, "ptrace attach failed: %s\n", strerror(errno));
        return 1;
    }

    if (waitpid(pid, NULL, 0) == -1) {
        fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    printf("Attached to PID %d\n", pid);

    char maps_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);

    FILE *maps_file = fopen(maps_path, "r");
    if (maps_file == NULL) {
        fprintf(stderr, "Failed to open %s: %s\n", maps_path, strerror(errno));
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);

    int memfd = open(mem_path, O_RDWR);
    if (memfd == -1) {
        fprintf(stderr, "Failed to open %s: %s\n", mem_path, strerror(errno));
        fclose(maps_file);
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return 1;
    }

    unsigned long start_addr = 0;
    unsigned long end_addr = 0;
    char permissions[8];
    char line[256];
    unsigned char buffer[4096];
    unsigned long candidates[10000];
    int found_count = 0;
    
    printf("Scanning memory for %u...\n", target_value);

    while (fgets(line, sizeof(line), maps_file) != NULL) {
        if (sscanf(line, "%lx-%lx %7s", &start_addr, &end_addr, permissions) != 3) {
            continue;
        }

        if (permissions[0] != 'r') {
            continue;
        }

        for (unsigned long addr = start_addr; addr < end_addr; addr += sizeof(buffer)) {
            ssize_t bytes_read = pread(memfd, buffer, sizeof(buffer), (off_t)addr);

            if (bytes_read <= 0) {
                continue;
            }

            for (ssize_t i = 0; i + sizeof(target_value) <= bytes_read; ++i) {
                if (memcmp(&buffer[i], &target_value, sizeof(target_value)) == 0) {
                    if (found_count < 10000) {
                        candidates[found_count] = addr + (unsigned long)i;
                        found_count++;
                    } 
                }
            }
        }
    }

    printf("Found %d addresses with value %u.\n", found_count, target_value);

    while (1) {
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        printf("\nTarget is UNFROZEN. Change the value, then press Enter here...\n");
        getchar(); 
        getchar(); 

        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
            fprintf(stderr, "ptrace re-attach failed: %s\n", strerror(errno));
            return 1;
        }
        waitpid(pid, NULL, 0);

        printf("Target FROZEN. Enter NEW value: ");
        uint32_t new_value;
        scanf("%u", &new_value);

        int survivors = 0;
        for (int j = 0; j < found_count; j++) {
            uint32_t current_val = 0;
            pread(memfd, &current_val, 4, (off_t)candidates[j]);

            if (current_val == new_value) {
                candidates[survivors] = candidates[j];
                survivors++;
            }
        }

        found_count = survivors;
        printf("Survivors: %d\n", found_count);
        
        for (int j = 0; j < found_count; j++) {
            printf("SURVIVOR at 0x%lx\n", candidates[j]);
        }

        if (found_count == 1) {
            break;
        } else if (found_count == 0) {
            printf("No survivors. Try again from scratch.\n");
            break;
        }

        printf("Too many survivors. Do another Next Scan? (1 - yes, 0 - no): ");
        int choice;
        scanf("%d", &choice);
        if (choice != 1) break;
        
        
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }

    if (found_count == 1) {
        printf("\nDo you want to overwrite this value? (1 - yes, 0 - no): ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            uint32_t write_val;
            printf("Enter value to WRITE: ");
            scanf("%u", &write_val);

            ssize_t written = pwrite(memfd, &write_val, sizeof(write_val), (off_t)candidates[0]);
            if (written == sizeof(write_val)) {
                printf("Successfully wrote %u to 0x%lx!\n", write_val, candidates[0]);
            } else {
                fprintf(stderr, "Failed to write memory: %s\n", strerror(errno));
            }
        }
    }

    close(memfd);
    fclose(maps_file);

    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1) {
        fprintf(stderr, "ptrace detach failed: %s\n", strerror(errno));
        return 1;
    }

    printf("Detached from PID %d\n", pid);
    return 0;
}
