#ifndef XDK_XMEMORY_H_
#define XDK_XMEMORY_H_

#ifndef XMEM_DEBUG
/**
  @brief
    An switch indication whether we need to check every detail of memory allocation.
    If it is set to 0, we don't do detail checking. If it is set to 1, every detail will be recorded.
    It is advocated to define this macro in Makefile.
*/
#define XMEM_DEBUG 0
#endif  // XMEM_DEBUG

#include <stdio.h>

/**
  @author
    Santa Zhang

  @file
    xmemory.h

  @brief
    Memory management. Some tricks are applied to detect memory leak.
*/

/**
  @brief
    Helper macro for xmalloc().

  @param cnt
    Number of allocation.
  @param ty
    Type of allocated objects.
*/
#define xmalloc_ty(cnt, ty) ((ty *) xmalloc((cnt) * sizeof(ty), __FILE__, __LINE__))

/**
  @brief
    Allocate memory chunk.

  This is merely a wrapper around malloc(). Allocation counts will be recorded.
  This function uses variable number of arguments, when XMEM_DEBUG=1 is defined, it needs 2 more arguments after size:
  a file name, and a line number indicating the position where xmalloc is called. It is advocated that you use
  xmalloc_ty instead of this function.

  @param size
    Size of the memory chunk.

  @return
    Pointer to alloc'ed memory chunk.

  @warning
    xmalloc'ed memory chunk must be freed by xfree().
*/

void* xmalloc(int size, ...);

/**
  @brief
    Free memory chunk.

  This is merely a wrapper around free().
  Allocation counts will be decreased.

  @param ptr
    Pointer to memory chunk.

  @warning
    Do not xfree() an xmalloc'ed memory chunk more than once!
*/
void xfree(void* ptr);

/**
  @brief
    Resize an xmalloc'ed chunk size

  This is merely a wrapper around realloc().
  Allocation counts will not be changed.

  @param ptr
    Pointer to current memory chunk.
  @param new_size
    New memory chunk size.
  
  @return
    Pointer to new memory chunk.
*/
void* xrealloc(void* ptr, int new_size);

/**
  @brief
    Report number of allocated memory chunks.

  At the end of execution, this function should return 0, indicating that
  all xmalloc'ed memory are released. This means there is no memory leak.

  @param fp
    File pointer to which the output message will be printed.
    stderr is advocated. If NULL is given, nothing will be printed.

  @return
    Number of allocated memory chunks.
*/
int xmem_usage(FILE* fp);

/**
  @brief
    Reset memory counters, as if xmalloc was never called.

  @warning
    Do not call this function unless you know what is going to happen!
*/
void xmem_reset_counter();

#endif  // XDK_XMEMORY_H_

