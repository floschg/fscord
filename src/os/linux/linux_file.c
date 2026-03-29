#include <os/os.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


char *
os_file_read_as_string(Arena *arena, char *filepath, size_t *len)
{
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("can't open file %s for reading\n", filepath);
        return 0;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        printf("can't fstat file %s\n", filepath);
        return 0;
    }

    char *dest = arena_push(arena, statbuf.st_size+1);
    ssize_t bytes_read = read(fd, dest, statbuf.st_size);
    if (bytes_read != statbuf.st_size) {
        printf("only read %ld/%ld bytes\n", statbuf.st_size, bytes_read);
        arena->size_used -= statbuf.st_size+1; // Todo: cleanup
        return 0;
    }
    dest[bytes_read] = '\0';
    *len = statbuf.st_size;

    return dest;
}


