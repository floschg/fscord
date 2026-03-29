#include <os/os.h>
#include <basic/basic.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>


typedef struct {
    u64 mmap_size;
    void *mmap_mem;
    u64 padding[6]; // total size 64 to ensure avx-512 simd-alignment
} OSMemoryHeader;


void *
os_memory_allocate(size_t size)
{
    if (size == 0) {
        return 0;
    }

    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t required_size = size + sizeof(OSMemoryHeader);
    size_t actual_size = ((required_size + page_size - 1) / page_size) * page_size;

    u8 *mem = mmap(0, actual_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap failed");
        return 0;
    }

    OSMemoryHeader *header = (OSMemoryHeader*)mem;
    header->mmap_size = actual_size;
    header->mmap_mem = mem;

    return (void*)(header+1);
}

void
os_memory_free(void *mem)
{
    if (mem == 0) {
        return;
    }

    OSMemoryHeader* header = (OSMemoryHeader*)mem - 1;
    if (munmap(header->mmap_mem, header->mmap_size) == -1) {
        perror("munmap failed");
    }
}

