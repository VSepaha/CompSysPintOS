#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* These header files were added in */
#include "filesys/filesys.h"
#include "filesys/files.h"
#include "lib/kernel/list.h"

#include "devices/input.h"
/* End of header files added in */

/* These Global variables were added in */
#define MAX_FILE_SIZE 204800
#define MAX_FILE_NAME_LENGTH 64



/* End of Global variables added in */

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

//TO DO: syscalls: halt, exit, exec, wait, remove, open, filesize, read, write, seek, tell, close

//NOTE: Global Variables "MAX_FILE_SIZE" and "MAX_FILE_NAME_LENGTH" must be defined
bool
create (const char *file, unsigned initial_size)
{	
	//Case 1: "initial_size" is lower than the limit
	if (initial_size <= MAX_FILE_SIZE){
		//Must check if file name length is lower than limit too
		if (strlen(file) > MAX_FILE_NAME_LENGTH){
			printf("Cannot create file because file name is too long");
			}
		else if (strlen(file) == 0){
			printf("Cannot create file because there is no file name");
			}
		//file name length and initial size are lower than the limits...
		//...then it's ok to create the file then
		else{
			return filesys_create(file, initial_size);
			//NOTE: filesys_create is a built in function located in "filesys.c"
		}
	}
	//Case 2: "initial_size" is larger than the limit
	else{
		printf("File size is too large!");
	}

return false;
}

bool
remove (const char *file)
{
	return filesys_remove(file);
}

int 
open (const char *file) 
{

	if (strlen(file) == 0) {
//		printf("File open failed because file name can not be empty!\n");
		;
	} else {

		//if the file is required to be removed,
		//it is invisible for opening operation.

		struct list_elem *rfe = list_begin(&removing_list);

//		printf("rfe == NULL ? %d\n", rfe==NULL);
//		printf("removing list size = %d\n", list_size(&removing_list));

		while(rfe != list_end(&removing_list)) {
			struct list_elem *tmprfe = rfe;
			rfe = list_next(rfe);
			struct removing_file *rf = list_entry(tmprfe, struct removing_file, r_elem);
			if (strcmp(rf->file_name, file) == 0) {
				return -1;
			}
		}

		//if the file is not required to be removed,
		//it can be opened.
		//lock_acquire(&fd_lock);
		struct file *f = filesys_open(file);
		if (f != NULL) {
			uint32_t n_fd = global_fd++;

			//fd == 1 screen
			//fd == 0 keyboard
			//fd >= 2 file
			if (n_fd >= 2) {
				struct list *fd_list = thread_add_fd(thread_current(), n_fd, f, file);
//				printf("fd_list size = %d\n", list_size(fd_list));
				if (fd_list != NULL) {
					if (list_size(fd_list) == 1) {
						struct fds_stub *fdss = malloc(sizeof(struct fds_stub));
						fdss->l = fd_list;
						list_push_back(&fds_ll, &fdss->fds_elem);
//						printf("fds_ll_size = %d\n", list_size(&fds_ll));
					}
				}
			}

			//lock_release(&fd_lock);
			return n_fd;
		}
		//lock_release(&fd_lock);
	}

	return -1;
}

int
filesize (int fd)
{
	struct file *f = get_file_p(fd);
	if (f != NULL){
		return file_length(f);
	}
}

/* The following function was added in */

//This function is used to get the pointer of the file structure
struct file* get_file_p(unsigned int fd) {

	if (fd == 1 || fd == 0) {
		//Cannot get the file structure pointer if fd is either 0 or 1...
		return NULL;
	}

	struct list_elem *le = list_begin(&fds_ll);
	while(le != list_end(&fds_ll)) {
		struct list_elem *tmp = le;
		le = list_next(le);
		struct fds_stub *fdss = list_entry(tmp, struct fds_stub, fds_elem);
		if (fdss != NULL) {
			struct list * fd_list = fdss->l;
			if (fd_list != NULL) {
				struct list_elem *fd_le = list_begin(fd_list);
				while(fd_le != list_end(fd_list)) {
					struct list_elem *tmp_fd_le = fd_le;
					fd_le = list_next(fd_le);
					struct fd_elem *fde = list_entry(tmp_fd_le, struct fd_elem, fdl_elem);
					if (fde != NULL) {
						if (fde->fd == fd) {

							return fde->f;
						}
					}
				}
			}
		}
	}
	return NULL;
}

/* Reads size bytes from buffer to open file fd. Returns the 
number of bytes actually read (0 at end of file), or -1 if the file
couldn't be read. Fd 0 reads from the keyboard using input_getc().*/
static int 
read(int fd, void *buffer, unsigned size)
{
	//return value
	int returnVal;
	//declared 'offset' outside of for loop because of error in compliation
	unsigned offset;

	lock_acquire(&file_lock);
	
	//read syscall
	if (fd == STDIN_FILENO){
		//read each input 
		for(offset = 0; offset < size; offset++){
			//use uint8 because we are reading size bytes
			*(uint8_t *)(buffer + offset) = input_getc();		
		}
		//make the return value equal to the size
		returnVal = size;
	} else {
		returnVal = -1;
	}
	
	lock_release(&file_lock);
	return returnVal;
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes actually
written, which may be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the file, but file growth is not implemented
by the basic file system. The expected behavior is to write as many bytes as
possible up to end-of-file and return the actual number written, or 0 if no bytes could
be written at all.
Fd 1 writes to the console. Your code to write to the console should write all of buffer
in one call to putbuf(), at least as long as size is not bigger than a few hundred
bytes. (It is reasonable to break up larger buffers.) Otherwise, lines of text output
by different processes may end up interleaved on the console, confusing both human
readers and our grading scripts. */
static int 
write(int fd, const void *buffer, unsigned size){

	//return value
	int returnVal;

}


/* End of function added in */


