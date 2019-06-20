#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <math.h>

static size_t page_size;

// align_down - rounds a value down to an alignment
// @x: the value
// @a: the alignment (must be power of 2)
//
// Returns an aligned value.
#define align_down(x, a) ((x) & ~((typeof(x))(a) - 1))

#define AS_LIMIT	(1 << 25) // Maximum limit on virtual memory bytes
#define MAX_SQRTS	(1 << 27) // Maximum limit on sqrt table entries
static double *sqrts;

// Use this helper function as an oracle for square root values.
static void
calculate_sqrts(double *sqrt_pos, int start, int nr)
{
  int i;

  for (i = 0; i < nr; i++)
    sqrt_pos[i] = sqrt((double)(start + i));
}

static void
handle_sigsegv(int sig, siginfo_t *si, void *ctx)
{
  // Your code here.
  static double *last_mapped_addr = NULL;
  size_t size_to_alloc = align_down(1<<12, 1<<12);
  // size_t size_to_alloc = 8;
  // printf("last_mapped_addr:%p\n", last_mapped_addr);
  // replace these three lines with your implementation
  if (last_mapped_addr != 0) {
    if (munmap(last_mapped_addr, size_to_alloc) == -1) {
      fprintf(stderr, "Couldn't munmap() region for sqrt table; %s\n",
              strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  uintptr_t fault_addr = (uintptr_t)si->si_addr;
  // struct rlimit lim = {};
  // getrlimit(RLIMIT_AS, &lim);
  // printf("lim.rlim_cur:%u, rlim_max:%u, AS_LIMIT:%u\n", lim.rlim_cur, lim.rlim_max, AS_LIMIT);
  // printf("fault_addr:%x, aligned:%x\n", fault_addr, align_down(fault_addr, 1<<12));
  last_mapped_addr = (double*)fault_addr;
  last_mapped_addr = mmap((void*)fault_addr, size_to_alloc, PROT_WRITE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (last_mapped_addr == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() region for sqrt table; %s\n",
	    strerror(errno));
    exit(EXIT_FAILURE);
  }
  double *end = last_mapped_addr + size_to_alloc/sizeof(double);
  for (double *curr = last_mapped_addr; curr != end; curr++) {

    calculate_sqrts(curr, (int)(curr - sqrts), 1);
    // printf("curr:%x, *curr:%f, pos:%d, last_mapped_addr:%x, sqrts:%x, end:%x\n", curr, *curr, (int)(curr - sqrts), last_mapped_addr, sqrts, end);
  }
}

static void
setup_sqrt_region(void)
{
  struct rlimit lim = {AS_LIMIT, AS_LIMIT};
  struct sigaction act;

  // Only mapping to find a safe location for the table.
  sqrts = mmap(NULL, MAX_SQRTS * sizeof(double) + AS_LIMIT, PROT_NONE,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (sqrts == MAP_FAILED) {
    fprintf(stderr, "Couldn't mmap() region for sqrt table; %s\n",
	    strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Now release the virtual memory to remain under the rlimit.
  if (munmap(sqrts, MAX_SQRTS * sizeof(double) + AS_LIMIT) == -1) {
    fprintf(stderr, "Couldn't munmap() region for sqrt table; %s\n",
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Set a soft rlimit on virtual address-space bytes.
  if (setrlimit(RLIMIT_AS, &lim) == -1) {
    fprintf(stderr, "Couldn't set rlimit on RLIMIT_AS; %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Register a signal handler to capture SIGSEGV.
  act.sa_sigaction = handle_sigsegv;
  act.sa_flags = SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  if (sigaction(SIGSEGV, &act, NULL) == -1) {
    fprintf(stderr, "Couldn't set up SIGSEGV handler;, %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void
test_sqrt_region(void)
{
  int i, pos = rand() % (MAX_SQRTS - 1);
  double correct_sqrt;

  printf("Validating square root table contents...\n");
  srand(0xDEADBEEF);
  for (i = 0; i < 500000; i++) {
    if (i % 2 == 0)
      pos = rand() % (MAX_SQRTS - 1);
    else
      pos += 1;
    calculate_sqrts(&correct_sqrt, pos, 1);
    // printf("expected sqrts[%d]:%f\n", pos, correct_sqrt);
    if (sqrts[pos] != correct_sqrt) {
      fprintf(stderr, "Square root is incorrect. Expected %f, got %f.\n",
              correct_sqrt, sqrts[pos]);
      exit(EXIT_FAILURE);
    }
  }

  printf("All tests passed!\n");
}

int
main(int argc, char *argv[])
{
  page_size = sysconf(_SC_PAGESIZE);
  printf("page_size is %ld\n", page_size);
  setup_sqrt_region();
  test_sqrt_region();
  return 0;
}
