#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h> 
#include "threads/interrupt.h"
#include "threads/thread.h" 
#include "threads/vaddr.h" 
#include "userprog/pagedir.h" 
#include "threads/init.h" 
#include "devices/shutdown.h" 
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "devices/input.h" 
#include "threads/malloc.h"
#include "lib/kernel/list.h"


static void syscall_handler(struct intr_frame *);
void is_valid_ptr(void* ptr);
void halt(void);
void exit(int status);
pid_t exec(const char *executable);
tid_t wait(tid_t pid);
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

void syscall_init(void)
{
  lock_init(&filesys_lock);
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED){
  is_valid_ptr(f->esp);
  int *syscall_number = (int*) f->esp;
  switch (* syscall_number){
    case SYS_HALT:{
      halt();
      break;
    }
    case SYS_EXIT:{
      int exit_status = *((int*)f->esp + 1);
      if(!is_user_vaddr(exit_status)) {
          f->eax = -1;
          exit(-1);
      }
      else{
        f->eax = exit_status;
        exit(exit_status);
      }
      break;
    }
    case SYS_EXEC:{
      char * exec_file_name = (char *) (*((int *) f->esp + 1));
      // printf("file name %s\n",(char *) (*((int *) f->esp + 1)));
      f->eax = exec(exec_file_name);
      // printf("\nf->eax %d\n",f->eax);
      break;
    }
    case SYS_WAIT:{
      is_valid_ptr((void*)f->esp +1);
      // int tid = get_int((int **)(&f->esp));
      // printf("\nfrom get int %d\n",*((int*)f->esp + 1));
      f->eax = wait(*((int*)f->esp + 1));
      break;
    }
    case SYS_CREATE:{
      int* ptr = (int*) f->esp;
      char * created_file_name = (char*) *(ptr + 1);
      is_valid_ptr(created_file_name);
      int initial_size = (unsigned) *(ptr + 2);
      f->eax = create(created_file_name,initial_size);
      break;
    }
    case SYS_REMOVE:{
      int* ptr = (int*) f->esp;
      char *  removed_file_name = (char *) (*(ptr + 1));
      is_valid_ptr(removed_file_name);
      f->eax = remove(removed_file_name);
      break;
    }
    case SYS_OPEN:{
      int* ptr = (int*) f->esp;
      char * opened_file_name = (char *) *(ptr + 1);
      is_valid_ptr(opened_file_name);
      f->eax = open(opened_file_name);
      break;
    }
    case SYS_FILESIZE:{
      int* ptr = (int *) f->esp;
      int fd = (int) (*(ptr + 1));
      f->eax = filesize(fd);
      break;
    }
    case SYS_READ:{
      int* ptr  = (int *) f->esp;
      int fd = (int) (*(ptr + 1));
      char * buffer = (char *) (*(ptr + 2));
      is_valid_ptr(buffer);
      if(fd == 1){
          // negative area
          // printf("negative area, can't read from STDOUT");
      }
      unsigned size = *((unsigned *) f->esp + 3);
      f->eax = read(fd,buffer,size);
      break;
    }
    case SYS_WRITE:{
      int* ptr = (int *) f->esp;
      int fd = *(ptr + 1);
      char *buffer = (char *) *(ptr + 2);
      is_valid_ptr(buffer);
      if(fd ==0){
          // negative area
          // printf("negative area, can't write to STDIN");
      }
      unsigned size = (unsigned)(*(ptr + 3));
      f->eax = write(fd, buffer, size);
      break;
    }
    case SYS_SEEK:{
      int* ptr = (int *) f->esp;
      int fd = (int) *(ptr + 1);
      unsigned pos = (unsigned) (*(ptr + 2));
      f->eax = seek(fd,pos);
      break;
    }
    case SYS_TELL:{
      int* ptr = (int *) f->esp;
      int fd = (int) *(ptr + 1);
      f->eax = tell(fd);
      break;
    }
    case SYS_CLOSE:{
      int* ptr = (int *) f->esp;
      int fd = (int) *(ptr + 1);
      if(fd<2){
        exit(-1);
      }
      f->eax = close(fd);
      break;
    }
    default:{
      // printf("invalid system call");
      break;
    }
  }
}

void is_valid_ptr(void* ptr){
  if(! (ptr != NULL && is_user_vaddr(ptr) && pagedir_get_page(thread_current()->pagedir, ptr) != NULL))
    exit(-1);
}

void halt(void)
{
  shutdown_power_off();
}

void exit(int status)
{
  thread_current()->exit_status = status; /* Saves exit status to thread and prints it. */
  printf("%s: exit(%d)\n", thread_current()->name, thread_current()->exit_status);
  thread_exit();
}

pid_t exec(const char *executable)
{
  if (!executable)
    return -1; /* Return if the file is null. */
  lock_acquire(&filesys_lock);
  // printf("from exec sysy\n\n%s\n\n",executable);
  int child_pid_t = process_execute(executable); /* If a file is not null, fork and execute it. */
  lock_release(&filesys_lock);
  // printf("\n%d =  child\n",child_pid_t);
  return child_pid_t;
}

tid_t wait(tid_t pid)
{
  int x;
    // printf("from wait in sys id cal %d\n\n",(pid));

  // printf("from wait in sys cal %d\n\n",x=process_wait(pid));
  return process_wait(pid);
}

bool create(const char *executable, unsigned initial_size)
{
  bool success = 0;
  lock_acquire(&filesys_lock);
  success = filesys_create(executable, initial_size);
  lock_release(&filesys_lock);
  return success;
}

int remove(const char *executable)
{
  int success = 0;
  lock_acquire(&filesys_lock);
  success = filesys_remove(executable);
  lock_release(&filesys_lock);
  return success;
}

int open(const char *executable)
{
  lock_acquire(&filesys_lock);
  struct file *executable_file = filesys_open(executable);
  lock_release(&filesys_lock);

  if (executable_file != NULL)
  {
    struct file_block *open_file = malloc(sizeof(struct file_block));
    open_file->executable = executable_file;
    lock_acquire(&filesys_lock);
    open_file->fd = thread_current()->next_fd++;
    lock_release(&filesys_lock);
    list_push_back(&thread_current()->fdt, &open_file->file_elem);
    return open_file->fd;
  }
  // Default case is no file was open. Return -1 if so.
  return -1;
}

int close(int fd)
{

  struct file_block * fb = returnFile(fd);

  if (fb == NULL)
      return -1;

  lock_acquire(&filesys_lock);
  file_close(fb->executable);
  lock_release(&filesys_lock);
  list_remove(&fb->file_elem);
  return 1;

}

int filesize(int fd)
{

  struct file_block * fb = returnFile(fd);
  if (fb == NULL)
    return -1;

  lock_acquire(&filesys_lock);
  int size = file_length(fb->executable);
  lock_release(&filesys_lock);
  return size;
}

int read(int fd, void *buffer, unsigned size)
{
  int temp = size;
  /*
    We have 3 cases.
    Case 1: reading from fd = 0 --> STDIN. We would then require a keyboard input.
    Case 2: reading from fd = 1 --> STDOUT. We cannot read that.
    Case 3: reading from fd > 2 --> FILE. We would then read a file.
  */

  // Case 1.
  if (fd == 0)
  {
    // lock_release(&filesys_lock);
    // return input_getc();
    while (size--)
    {
      lock_acquire(&filesys_lock);
      char ch = input_getc();
      lock_release(&filesys_lock);
      buffer += ch;
    }
    return temp;
    
  }

  // Case 2.
  if (fd == 1)
    goto done;


  struct file_block *fb = returnFile(fd);

  // Case 3.
  if (fb == NULL)
    return -1;

  lock_acquire(&filesys_lock);
  size = file_read(fb->executable, buffer, size);
  lock_release(&filesys_lock);
  return size;  

done:
  return 0;
}

int write(int fd, void *buffer, unsigned size)
{
  /*
    Similar to read, we have 3 cases.
    Case 1: reading from fd = 0 --> STDIN. We cannot right to an input.
    Case 2: reading from fd = 1 --> STDOUT. We write the output to console.
    Case 3: reading from fd > 2 --> FILE. We would then write to the open file.
  */

  // Case 1.
  if (fd == 0)
    goto done;

  // Case 2.
  if (fd == 1)
  {
    lock_acquire(&filesys_lock);
    putbuf(buffer, size);
    lock_release(&filesys_lock);
    return size;
  }

  struct file_block *fb = returnFile(fd);
  
  // Case 3.
  if (fb == NULL)
    return -1;

  lock_acquire(&filesys_lock); 
  size = file_write(fb->executable, buffer, size);
  lock_release(&filesys_lock);
  return size;
 

done:
  return 0;
}

int seek(int fd, unsigned position)
{

  struct file_block *fb = returnFile(fd);

  if(fb == NULL) 
    return -1;
  
  lock_acquire(&filesys_lock);
  file_seek(fb->executable, position);
  lock_release(&filesys_lock);
  return position;
  
}

unsigned tell(int fd)
{

  struct file_block *fb = returnFile(fd);

  if(fb ==  NULL)
    return -1;

  lock_acquire(&filesys_lock);
  unsigned position = file_tell(fb->executable);
  lock_release(&filesys_lock);
  return position;
}

struct file_block * returnFile(int fd) {

  if (list_empty(&thread_current()->fdt))
    return NULL;

  struct list_elem *file_elem = list_back(&thread_current()->fdt);
  struct file_block *fb;

  if (fb == NULL)
      return NULL;

  while (file_elem != NULL)
  {
    fb = list_entry(file_elem, struct file_block, file_elem);
    if (fb->fd == fd)
      return fb;
    file_elem = file_elem->next;
  }

}