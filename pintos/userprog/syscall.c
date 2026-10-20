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
#include <string.h>

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

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

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

bool is_valid (struct intr_frame *f UNUSED) {
	switch (f->R.rax) {

	}
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	struct thread *curr = thread_current();
	switch (f->R.rax) {
		case SYS_HALT :
			break;
		case SYS_EXIT :
			curr->ss->exit_status = f->R.rdi;		
			thread_exit ();
			NOT_REACHED ();
			break;
		case SYS_FORK :	
			f->R.rax = process_fork ((const char *)f->R.rdi, f);
			break;
		case SYS_EXEC :
			const char *cmd_line = (char *)f->R.rdi;
			char *kernel_buf;		
			int len;

			if (!cmd_line || is_kernel_vaddr (cmd_line)) 
				goto error;					
			
			for (int i = 0; i < PGSIZE; i++) {
				char *check = cmd_line + i;

				if (i == 0 || pg_ofs (check) == 0)
					if (is_kernel_vaddr (check) || !pml4_get_page (curr->pml4, check))
						goto error;

				if (*check == '\0') {
					len = i + 1;
					goto copy;
				}
			}
			goto error;
			
			copy:
				kernel_buf = palloc_get_page(0);
				if (!kernel_buf)
					goto error;
				memcpy (kernel_buf, cmd_line, len);

			if (process_exec (kernel_buf) == -1) 
				goto error;

			error:
				curr->ss->exit_status = -1;
				thread_exit();
			break;
		case SYS_WAIT :
			/* 인자는 rdi */
			f->R.rax = process_wait((tid_t)f->R.rdi);
			break;
		case SYS_CREATE :
		case SYS_REMOVE :
		case SYS_OPEN :
		case SYS_FILESIZE :
		case SYS_READ :
			break;
		case SYS_WRITE :
			if (f->R.rdi == STDOUT_FILENO) {
				putbuf ((char *)f->R.rsi, f->R.rdx);
				f->R.rax = f->R.rdx;	
			}
			break;
		case SYS_SEEK :
		case SYS_TELL :
		case SYS_CLOSE :
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

}

