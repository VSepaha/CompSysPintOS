#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include <list.h>
#include "threads/malloc.h"
#include "devices/shutdown.h"

#define ARG0 (*(esp + 1))
#define ARG1 (*(esp + 2))
#define ARG2 (*(esp + 3))
#define ARG3 (*(esp + 4))
#define ARG4 (*(esp + 5))
#define ARG5 (*(esp + 6))

//removed and put into syscall.h
// struct lock file_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

struct file *get_file(int fd)
{
  struct list_elem *e;
  struct file_descriptor *file_d;
  struct thread *cur = thread_current();
  for (e = list_tail (&cur->files); e != list_head (&cur->files); e = list_prev (e))
    {
      file_d = list_entry (e, struct file_descriptor, elem);
      if (file_d->fd == fd) 
        return file_d->file;   
    }
  return NULL;
}

bool not_valid(const void *pointer)
{
  return (!is_user_vaddr(pointer) || pointer == NULL || pagedir_get_page (thread_current ()->pagedir, pointer) == NULL);
}

void 
halt (void)
{
  shutdown_power_off();
}

void 
exit (int status)
{	
  thread_current ()->proc->exit = status;
  thread_exit ();
}

pid_t exec (const char *cmd_line)
{
  if (not_valid(cmd_line))
    exit (-1);
  return process_execute(cmd_line); 
}

int 
wait (pid_t pid)
{
  return process_wait (pid);
}

bool
create(const char *file, unsigned initial_size)
{
  if (not_valid(file))
    exit (-1);

  lock_acquire(&file_lock);
  bool result = filesys_create (file, initial_size);
  lock_release(&file_lock);
  return result;
}

bool
remove (const char *file)
{
  if (not_valid(file))
    exit (-1);

  lock_acquire(&file_lock);
  /* In case the file is opened. First check its existence. */
  struct file *f = filesys_open (file);
  bool result;
  if (f == NULL)
    result = false;
  else
    {
      file_close (f);
      result = filesys_remove (file);
    }
  lock_release(&file_lock);
  return result;
}

int 
open (const char *file)
{
  if (not_valid(file))
    exit (-1);

  lock_acquire(&file_lock);
  struct file_descriptor *file_d = malloc(sizeof(struct file_descriptor));
  struct file *f = filesys_open(file);
  struct thread *cur = thread_current();
  if (f == NULL)
    {
      lock_release(&file_lock);
      return -1;
    }
  file_d->file = f;
  file_d->fd = cur->fd;
  cur->fd = cur->fd + 1;
  list_push_back(&thread_current()->files,&file_d->elem);
  lock_release(&file_lock);
  return file_d->fd;
}

int
filesize (int fd)
{
  lock_acquire(&file_lock);
  struct file *file = get_file(fd);
  int result = file ? file_length(file) : -1;
  lock_release(&file_lock);
  return result;
}

int 
read (int fd, void *buffer, unsigned size)
{
  if (not_valid(buffer) || not_valid(buffer+size) || fd == STDOUT_FILENO)
    exit (-1);
  lock_acquire(&file_lock);
  int count = 0, result = 0;
  if (fd == STDIN_FILENO)
    {
      while (count < size)
        {
          *((uint8_t *) (buffer + count)) = input_getc ();
          count++;
        }
      result = size;
    }
  else 
    {
      struct file *file = get_file(fd);
      result = file ? file_read(file, buffer, size) : -1;
    }
  lock_release(&file_lock);
  return result;
}

int 
write (int fd, const void *buffer, unsigned size)
{
  if (not_valid(buffer) || not_valid(buffer+size) || fd == STDIN_FILENO)
    exit (-1);
  lock_acquire(&file_lock);
  int result = 0;
  if (fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      result = size;
    }
  else 
    {
      struct file *file = get_file(fd);
      result = file? file_write(file, buffer, size) : -1;
    }
  lock_release(&file_lock); 
  return result;
}

void 
seek (int fd, unsigned position)
{
  lock_acquire(&file_lock);
  struct file *file = get_file(fd);
  if (file == NULL)
    exit (-1);
  file_seek(file,position);
  lock_release(&file_lock);
}

unsigned 
tell (int fd)
{
  lock_acquire(&file_lock);
  struct file *file = get_file(fd);
  int result = file ? file_tell(file) : 0;  
  lock_release(&file_lock);
  return result;
}

void 
close (int fd)
{
  lock_acquire(&file_lock);
  struct list_elem *e;
  struct file_descriptor *file_d;  
  struct thread *cur;
  cur = thread_current ();
  // would fail if go backward
  // Why would fail if scan backwards???????
  for (e = list_begin (&cur->files); e != list_tail (&cur->files); e = list_next (e))
  {
    file_d = list_entry (e, struct file_descriptor, elem);
    if (file_d->fd == fd)
    {
      file_close (file_d->file);
      list_remove (&file_d->elem);
      free (file_d); 
      break;   
    }
  }
  lock_release(&file_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t *esp = f->esp;
  if (not_valid(esp))
    exit (-1);
  switch (*esp)
    {
      case SYS_HALT:
        halt ();
        break;
      case SYS_EXIT:
        if (not_valid(esp+1)) exit(-1);
        exit ((int) ARG0);
        break;
      case SYS_EXEC:
        f->eax = exec ((const char *) ARG0);
        break;
      case SYS_WAIT:
        f->eax = wait ((pid_t) ARG0);
        break;
      case SYS_CREATE:
        f->eax = create ((const char *) ARG0, (unsigned) ARG1);
        break;
      case SYS_REMOVE:
        f->eax = remove ((const char *) ARG0);
        break;
      case SYS_OPEN:
        f->eax = open ((const char *) ARG0);
        break;
      case SYS_FILESIZE:
        f->eax = filesize ((int) ARG0);
        break;
      case SYS_READ:
        f->eax = read ((int) ARG0, (void *) ARG1, (unsigned) ARG2);
        break;
      case SYS_WRITE:
        f->eax = write ((int) ARG0, (void *) ARG1, (unsigned) ARG2);
        break;
      case SYS_SEEK:
        seek ((int) ARG0, (unsigned) ARG1);
        break;
      case SYS_TELL:
        f->eax = tell ((int) ARG0);
        break;
      case SYS_CLOSE:
        close ((int) ARG0);
        break;
      default:
        printf ("Invalid syscall!\n");
        thread_exit();
    }
}


