#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

struct lock filesys_lock;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	lock_init (&filesys_lock);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}
bool
is_valid (struct intr_frame *f UNUSED) {
	struct thread *curr = thread_current ();
	switch (f->R.rax) {
		case SYS_EXEC :
		case SYS_FORK :
		case SYS_CREATE :
		case SYS_OPEN : {
			const char *fn_copy = (const char*)f->R.rdi;
			while (true) {
				if (is_user_vaddr (fn_copy) && pml4_get_page (curr->pml4, fn_copy)) {
					if (*fn_copy == '\0')
						break;
					fn_copy++;
				}
				else
					return false;
			}
			break;
		}
		case SYS_READ :
		case SYS_WRITE :
		{
			if (f->R.rsi == NULL || is_kernel_vaddr (f->R.rsi))
				return false;
			if ((uint64_t)KERN_BASE - f->R.rsi < f->R.rdx)
				return false;
			uint64_t page_addr = pg_round_down (f->R.rsi);
			while (page_addr <= pg_round_down (f->R.rsi + f->R.rdx - 1)) {
				if (!pml4_get_page (curr->pml4, page_addr))
					return false;
				if (f->R.rax == SYS_READ) {
					if (!is_writable (pml4e_walk (curr->pml4, page_addr, 0)))
						return false;
				}
				page_addr += PGSIZE;
			}
		}
		default :
			break;
	}
	return true;
}
/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	struct thread *curr = thread_current ();
	switch (f->R.rax) {
		case SYS_HALT :
			power_off();
			break;
		case SYS_EXIT :
			curr->ss->exit_status = f->R.rdi;		
			thread_exit ();
			NOT_REACHED ();
			break;
		case SYS_FORK :
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}
			f->R.rax = process_fork (f->R.rdi, f);
			break;
		case SYS_EXEC :
		{
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}
			char *file_name = palloc_get_page (PAL_ZERO);
			if (file_name == NULL) {
				curr->ss->exit_status = -1;
				thread_exit ();
			}
			strlcpy (file_name, f->R.rdi, PGSIZE);
			process_exec (file_name);
			curr->ss->exit_status = -1;
			thread_exit ();
			break;
		}
		case SYS_WAIT :
			f->R.rax = process_wait ((tid_t)f->R.rdi);
			break;
		case SYS_CREATE :
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}
			f->R.rax = filesys_create (f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE :
			break;
		case SYS_OPEN :
		{
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}
			f->R.rax = -1;
			struct file *file = filesys_open (f->R.rdi);
			if (file == NULL) {
				break;
			}
			struct fd_entry *fde = malloc (sizeof (struct fd_entry));
			if (fde == NULL) {
				file_close (file);
				break;
			}
			fde->file = file;

			if (list_empty (&curr->fd_table))
				fde->fd = 2;
			else
				fde->fd = list_entry (list_back (&curr->fd_table), struct fd_entry, elem)->fd + 1;
			list_push_back (&curr->fd_table, &fde->elem);
			f->R.rax = fde->fd;
			break;
		}
		case SYS_FILESIZE :
			f->R.rax = -1;
			if (f->R.rdi == 0 || f->R.rdi == 1)
				break;
			for (struct list_elem *e = list_begin (&curr->fd_table); e != list_end (&curr->fd_table); e = list_next (e)){
				if (f->R.rdi == list_entry (e, struct fd_entry, elem)->fd) {
					f->R.rax = file_length (list_entry (e, struct fd_entry, elem)->file);
					break;
				}
			}			
			break;
		case SYS_READ :
		{
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}

			if (f->R.rdi == STDIN_FILENO) {
				char *buffer = (char *)f->R.rsi;
				for (size_t i = 0; i < f->R.rdx; i++) 
					buffer[i] = input_getc ();
				f->R.rax = f->R.rdx;
				break;
			}
			f->R.rax = -1;
			for (struct list_elem *e = list_begin (&curr->fd_table); e != list_end (&curr->fd_table); e = list_next (e)){
				if (f->R.rdi == list_entry (e, struct fd_entry, elem)->fd) {
					f->R.rax = file_read (list_entry (e, struct fd_entry, elem)->file, (void *)f->R.rsi, f->R.rdx);
					break;
				}
			}
			break;
		}
		case SYS_WRITE :
			if (!is_valid (f)) {
				curr->ss->exit_status = -1;
				thread_exit ();
				NOT_REACHED ();
			}
			if (f->R.rdi == STDOUT_FILENO) {
				putbuf ((char *)f->R.rsi, f->R.rdx);
				f->R.rax = f->R.rdx;
				break;
			}
			f->R.rax = -1;
			for (struct list_elem *e = list_begin (&curr->fd_table); e != list_end (&curr->fd_table); e = e->next) {
				struct fd_entry *fde = list_entry (e, struct fd_entry, elem);
				if (fde->fd == f->R.rdi) {
					f->R.rax = file_write (fde->file, f->R.rsi, f->R.rdx);
					break;
				}
			}
			break;
		
		case SYS_SEEK :
		case SYS_TELL :
		case SYS_CLOSE :
			for (struct list_elem *e = list_begin (&curr->fd_table); e != list_end (&curr->fd_table); e = e->next) {
				struct fd_entry *fde = list_entry (e, struct fd_entry, elem);
				if (fde->fd == f->R.rdi) {
					file_close (fde->file);
					list_remove (&fde->elem);
					free (fde);
					break;
				}
			}
			break;
		case SYS_DUP2 :
		case SYS_MMAP :
		case SYS_MUNMAP :
		case SYS_CHDIR :
		case SYS_MKDIR :
		case SYS_READDIR :
		case SYS_ISDIR :
		case SYS_INUMBER :
		case SYS_SYMLINK :
		case SYS_MOUNT :
		case SYS_UMOUNT :
			break;

		default :
			break;
	}
	return;
}

