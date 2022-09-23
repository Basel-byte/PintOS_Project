#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include <debug.h>
#include "lib/kernel/list.h"
#include "threads/thread.h"
/* A struct to hold information about each file. */
struct file_block
{
    struct file *executable;    /* File name. */
    int fd;                     /* File descriptor. */
    struct list_elem file_elem; /* List element for file element. */
};

typedef int pid_t;

struct lock filesys_lock; /* Global lock for system calls. */
void syscall_init(void);

/* System calls. */
void halt(void);
void exit(int status);
tid_t exec(const char *executable);
tid_t wait(tid_t pid);

/* File related system calls. */
bool create(const char *executable, unsigned initial_size);
int remove(const char *executable);
int open(const char *executable);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, void *buffer, unsigned size);
int seek(int fd, unsigned position);
unsigned tell(int fd);
int close(int fd);
struct file_block * returnFile(int fd);

#endif /* userprog/syscall.h */