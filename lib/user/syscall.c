#include <stdint.h>
#include <syscall.h>

#include "../syscall-nr.h"
#include "stddef.h"

__attribute__((always_inline)) static __inline int64_t syscall(
    uint64_t num_, uint64_t a1_, uint64_t a2_, uint64_t a3_, uint64_t a4_,
    uint64_t a5_, uint64_t a6_) {
  int64_t ret;
  register uint64_t *num asm("rax") = (uint64_t *)num_;
  register uint64_t *a1 asm("rdi") = (uint64_t *)a1_;
  register uint64_t *a2 asm("rsi") = (uint64_t *)a2_;
  register uint64_t *a3 asm("rdx") = (uint64_t *)a3_;
  register uint64_t *a4 asm("r10") = (uint64_t *)a4_;
  register uint64_t *a5 asm("r8") = (uint64_t *)a5_;
  register uint64_t *a6 asm("r9") = (uint64_t *)a6_;

  __asm __volatile(
      "mov %1, %%rax\n"
      "mov %2, %%rdi\n"
      "mov %3, %%rsi\n"
      "mov %4, %%rdx\n"
      "mov %5, %%r10\n"
      "mov %6, %%r8\n"
      "mov %7, %%r9\n"
      "syscall\n"
      : "=a"(ret)
      : "g"(num), "g"(a1), "g"(a2), "g"(a3), "g"(a4), "g"(a5), "g"(a6)
      : "cc", "memory");
  return ret;
}

/* Invokes syscall NUMBER, passing no arguments, and returns the
   return value as an `int'. */
#define syscall0(NUMBER) (syscall(((uint64_t)NUMBER), 0, 0, 0, 0, 0, 0))

/* Invokes syscall NUMBER, passing argument ARG0, and returns the
   return value as an `int'. */
#define syscall1(NUMBER, ARG0) \
  (syscall(((uint64_t)NUMBER), ((uint64_t)ARG0), 0, 0, 0, 0, 0))
/* Invokes syscall NUMBER, passing arguments ARG0 and ARG1, and
   returns the return value as an `int'. */
#define syscall2(NUMBER, ARG0, ARG1) \
  (syscall(((uint64_t)NUMBER), ((uint64_t)ARG0), ((uint64_t)ARG1), 0, 0, 0, 0))

#define syscall3(NUMBER, ARG0, ARG1, ARG2)                         \
  (syscall(((uint64_t)NUMBER), ((uint64_t)ARG0), ((uint64_t)ARG1), \
           ((uint64_t)ARG2), 0, 0, 0))

#define syscall4(NUMBER, ARG0, ARG1, ARG2, ARG3)                     \
  (syscall(((uint64_t *)NUMBER), ((uint64_t)ARG0), ((uint64_t)ARG1), \
           ((uint64_t)ARG2), ((uint64_t)ARG3), 0, 0))

#define syscall5(NUMBER, ARG0, ARG1, ARG2, ARG3, ARG4)             \
  (syscall(((uint64_t)NUMBER), ((uint64_t)ARG0), ((uint64_t)ARG1), \
           ((uint64_t)ARG2), ((uint64_t)ARG3), ((uint64_t)ARG4), 0))
void halt(void) {
  syscall0(SYS_HALT);
  NOT_REACHED();
}

void exit(int status) {
  syscall1(SYS_EXIT, status);
  NOT_REACHED();
}

pid_t fork(const char *thread_name) {
  return (pid_t)syscall1(SYS_FORK, thread_name);
}

int exec(const char *file) { return (pid_t)syscall1(SYS_EXEC, file); }

int wait(pid_t pid) { return syscall1(SYS_WAIT, pid); }

bool create(const char *file, unsigned initial_size) {
  return syscall2(SYS_CREATE, file, initial_size);
}

bool remove(const char *file) { return syscall1(SYS_REMOVE, file); }

int open(const char *file) {
  check_address(file);
  struct file *open_file = filesys_open(file);

  if (open_file == NULL) {
    return -1;
  }
  // fd table에 file추가
  int fd = add_file_to_fdt(open_file);

  // fd table 가득 찼을경우
  if (fd == -1) {
    file_close(open_file);
  }
  return fd;
}

int filesize(int fd) {
  struct file *open_file = find_file_by_fd(fd);
  if (open_file == NULL) {
    return -1;
  }
  return file_length(open_file);
}

int read(int fd, void *buffer, unsigned size) {
  check_address(buffer);
  off_t read_byte;
  uint8_t *read_buffer = buffer;
  if (fd == 0) {
    char key;
    for (read_byte = 0; read_byte < size; read_byte++) {
      key = input_getc();
      *read_buffer++ = key;
      if (key == '\0') {
        break;
      }
    }
  } else if (fd == 1) {
    return -1;
  } else {
    struct file *read_file = find_file_by_fd(fd);
    if (read_file == NULL) {
      return -1;
    }
    lock_acquire(&filesys_lock);
    read_byte = file_read(read_file, buffer, size);
    lock_release(&filesys_lock);
  }
  return read_byte;
}

int write(int fd, const void *buffer, unsigned size) {
  return syscall3(SYS_WRITE, fd, buffer, size);
}

void seek(int fd, unsigned position) {
  struct file *seek_file = find_file_by_fd(fd);
  if (seek_file <= 2) {  // 초기값 2로 설정. 0: 표준 입력, 1: 표준 출력
    return;
  }
  seek_file->pos = position;
}

unsigned tell(int fd) {
  struct file *tell_file = find_file_by_fd(fd);
  if (tell_file <= 2) {
    return;
  }
  return file_tell(tell_file);
}

void close(int fd) {
  struct file *fileobj = find_file_by_fd(fd);
  if (fileobj == NULL) {
    return;
  }

  remove_file_from_fdt(fd);
}

int dup2(int oldfd, int newfd) { return syscall2(SYS_DUP2, oldfd, newfd); }

void *mmap(void *addr, size_t length, int writable, int fd, off_t offset) {
  return (void *)syscall5(SYS_MMAP, addr, length, writable, fd, offset);
}

void munmap(void *addr) { syscall1(SYS_MUNMAP, addr); }

bool chdir(const char *dir) { return syscall1(SYS_CHDIR, dir); }

bool mkdir(const char *dir) { return syscall1(SYS_MKDIR, dir); }

bool readdir(int fd, char name[FDCOUNT_LIMIT + 1]) {
  return syscall2(SYS_READDIR, fd, name);
}

bool isdir(int fd) { return syscall1(SYS_ISDIR, fd); }

int inumber(int fd) { return syscall1(SYS_INUMBER, fd); }

int symlink(const char *target, const char *linkpath) {
  return syscall2(SYS_SYMLINK, target, linkpath);
}

int mount(const char *path, int chan_no, int dev_no) {
  return syscall3(SYS_MOUNT, path, chan_no, dev_no);
}

int umount(const char *path) { return syscall1(SYS_UMOUNT, path); }
