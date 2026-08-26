#include "loader.h"
#include <errno.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

static void *file_buffer    = NULL;
static void *mapped_segment = NULL;
static size_t mapped_size   = 0;

void loader_cleanup(void) {
    if (mapped_segment != NULL && mapped_segment != MAP_FAILED) {
        munmap(mapped_segment, mapped_size);
    }

    if (file_buffer != NULL) {
        free(file_buffer);
    }

    if (fd >= 0) {
        close(fd);
    }

    mapped_segment = NULL;
    file_buffer    = NULL;
    fd             = -1;
}

void load_and_run_elf(char **exe) {
    const char *path = exe[0];

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: unable to open file \"%s\": %s\n", path, strerror(errno));
        exit(1);
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        fprintf(stderr, "Error: unable to determine size of \"%s\"\n", path);
        close(fd);
        exit(1);
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        fprintf(stderr, "Error: lseek failed on \"%s\": %s\n", path, strerror(errno));
        close(fd);
        exit(1);
    }

    file_buffer = malloc(file_size);
    if (file_buffer == NULL) {
        fprintf(stderr, "Error: malloc failed while reading \"%s\"\n", path);
        close(fd);
        exit(1);
    }

    ssize_t bytes_read = read(fd, file_buffer, file_size);
    if (bytes_read < 0 || bytes_read != file_size) {
        fprintf(stderr, "Error: could not read the whole ELF file \"%s\": %s\n", path, strerror(errno));
        exit(1);
    }

    ehdr = (Elf32_Ehdr *) file_buffer;

    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Error: \"%s\" is not an ELF file (bad magic number)\n", path);
        exit(1);
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Error: \"%s\" is not a 32-bit ELF file\n", path);
        exit(1);
    }
    if (ehdr->e_type != ET_EXEC) {
        fprintf(stderr, "Error: \"%s\" is not a statically linked executable (ET_EXEC)\n", path);
        exit(1);
    }
    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        fprintf(stderr, "Error: \"%s\" has no program header table\n", path);
        exit(1);
    }

    phdr = NULL;
    Elf32_Phdr *phdr_table = (Elf32_Phdr *) ((char *) file_buffer + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr *candidate = &phdr_table[i];

        if (candidate->p_type == PT_LOAD && ehdr->e_entry >= candidate->p_vaddr && ehdr->e_entry < candidate->p_vaddr + candidate->p_memsz) {
            phdr = candidate;
            break;
        }
    }

    if (phdr == NULL) {
        fprintf(stderr, "Error: could not find a PT_LOAD segment covering the entry point\n");
        exit(1);
    }

    mapped_size = phdr->p_memsz;
    mapped_segment = mmap(NULL, mapped_size, PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mapped_segment == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed: %s\n", strerror(errno));
        exit(1);
    }

    memcpy(mapped_segment, (char *) file_buffer + phdr->p_offset, phdr->p_filesz);

    void *entry_point = (char *) mapped_segment + (ehdr->e_entry - phdr->p_vaddr);

    int (*_start)(void) = (int (*)(void)) entry_point;
    int result = _start();

    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv + 1);
    loader_cleanup();

    return 0;
}