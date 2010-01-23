#ifndef XMEM_DEBUG
#define XMEM_DEBUG 0
#endif  // XMEM_DEBUG

#include <malloc.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif  // HAVE_LIBPTHREAD

#include "xdef.h"
#include "xmemory.h"

static int xmalloc_counter = 0;
#ifdef HAVE_LIBPTHREAD
pthread_mutex_t xmem_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif  // HAVE_LIBPTHREAD

#if XMEM_DEBUG == 1
// memory alloction logging utility
// adapted from xhash

typedef xbool (*xmem_log_table_visitor)(void* ptr, const char* file, int line, void* args);

typedef struct xmem_log_table_entry {
  void* ptr;
  const char* file;
  int line;
  struct xmem_log_table_entry* next;
} xmem_log_table_entry;

typedef struct xmem_log_table {
  xmem_log_table_entry** slot;
  int entry_count;
  int extend_ptr;
  int extend_level;
  int base_size;
} xmem_log_table;

xmem_log_table* xmem_log = NULL;

#define XMEM_LOG_TABLE_INIT_SLOT_COUNT 16
#define XMEM_LOG_TABLE_THRESHOLD 3

xmem_log_table* xmem_log_table_new() {
  int i;
  xmem_log_table* xl = (xmem_log_table *) malloc(sizeof(xmem_log_table));

  xl->extend_ptr = 0;
  xl->extend_level = 0;
  xl->base_size = XMEM_LOG_TABLE_INIT_SLOT_COUNT;
  xl->entry_count = 0;

  xl->slot = (xmem_log_table_entry **) malloc(xl->base_size * sizeof(xmem_log_table_entry *));

  for (i = 0; i < xl->base_size; i++) {
    xl->slot[i] = NULL;
  }

  return xl;
}

void xmem_log_table_delete(xmem_log_table* xl) {
  size_t i;
  size_t actual_size = (xl->base_size << xl->extend_level) + xl->extend_ptr;
  xmem_log_table_entry* p;
  xmem_log_table_entry* q;
  
  for (i = 0; i < actual_size; i++) {
    p = xl->slot[i];
    while (p != NULL) {
      q = p->next;
      // do nothing to the log entry's key & value (they are either const value, or they are being managed somewhere else)
      free(p);
      p = q;
    }
  }
  free(xl->slot);
  free(xl);
}

// calculates the actuall slot id of a key
static int xmem_log_table_slot_id(xmem_log_table* xl, void* key) {
  long hcode = (long) key;
  if (hcode % (xl->base_size << xl->extend_level) < xl->extend_ptr) {
    // already extended part
    return hcode % (xl->base_size << (1 + xl->extend_level));
  } else {
    // no the yet-not-extended part
    return hcode % (xl->base_size << xl->extend_level);
  }
}


// extend the hash table if necessary
static void xmem_log_table_try_extend(xmem_log_table* xl) {
  if (xl->entry_count > XMEM_LOG_TABLE_THRESHOLD * (xl->base_size << xl->extend_level)) {
    xmem_log_table_entry* p;
    xmem_log_table_entry* q;

    size_t actual_size = (xl->base_size << xl->extend_level) + xl->extend_ptr;

    xl->slot = (xmem_log_table_entry **) realloc(xl->slot, (actual_size + 1) * sizeof(xmem_log_table_entry *));
    p = xl->slot[xl->extend_ptr];
    xl->slot[xl->extend_ptr] = NULL;
    xl->slot[actual_size] = NULL;
    while (p != NULL) {
      long hcode = (long) p->ptr;
      long slot_id = hcode % (xl->base_size << (1 + xl->extend_level));
      q = p->next;
      p->next = xl->slot[slot_id];
      xl->slot[slot_id] = p;
      p = q;
    }

    xl->extend_ptr++;
    if (xl->extend_ptr == (xl->base_size << xl->extend_level)) {
      xl->extend_ptr = 0;
      xl->extend_level++;
    }
  }

}

void xmem_log_table_put(xmem_log_table* xl, void* ptr, const char* file, int line) {
  size_t slot_id;
  xmem_log_table_entry* entry = (xmem_log_table_entry *) malloc(sizeof(xmem_log_table_entry));

  // first of all, test if need to expand
  xmem_log_table_try_extend(xl);

  slot_id = xmem_log_table_slot_id(xl, ptr);

  entry->next = xl->slot[slot_id];
  entry->ptr = ptr;
  entry->file = file;
  entry->line = line;
  xl->slot[slot_id] = entry;

  xl->entry_count++;
}

const char* xmem_log_table_get_file(xmem_log_table* xl, void* ptr) {
  size_t slot_id = xmem_log_table_slot_id(xl, ptr);

  xmem_log_table_entry* p;
  xmem_log_table_entry* q;

  p = xl->slot[slot_id];
  while (p != NULL) {
    q = p->next;
    if (ptr == p->ptr)
      return p->file;
    p = q;
  }
  return "(unknown)";
}

int xmem_log_table_get_line(xmem_log_table* xl, void* ptr) {
  size_t slot_id = xmem_log_table_slot_id(xl, ptr);

  xmem_log_table_entry* p;
  xmem_log_table_entry* q;

  p = xl->slot[slot_id];
  while (p != NULL) {
    q = p->next;
    if (ptr == p->ptr)
      return p->line;
    p = q;
  }
  return -1;
}

// shrink the hash table if necessary
// table size is shrinked to 1/2
static void xmem_log_table_try_shrink(xmem_log_table* xl) {
  if (xl->extend_level == 0)
    return;

  if (xl->entry_count * XMEM_LOG_TABLE_THRESHOLD < (xl->base_size << xl->extend_level)) {
    int i;
    int actual_size = (xl->base_size << xl->extend_level) + xl->extend_ptr;
    int new_size = xl->base_size << (xl->extend_level - 1);

    for (i = new_size; i < actual_size; i++) {
      int slot_id = i % new_size;
      if (xl->slot[slot_id] == NULL) {
        xl->slot[slot_id] = xl->slot[i];
      } else {
        xmem_log_table_entry* p = xl->slot[slot_id];
        while (p->next != NULL)
          p = p->next;
        p->next = xl->slot[i];
      }
    }

    xl->extend_level--;
    xl->extend_ptr = 0;

    xl->slot = (xmem_log_table_entry **) realloc(xl->slot, (xl->base_size << xl->extend_level) * sizeof(xmem_log_table_entry *));
  }
}

int xmem_log_table_remove(xmem_log_table* xl, void* ptr) {
  int slot_id;
  xmem_log_table_entry* p;
  xmem_log_table_entry* q;

  xmem_log_table_try_shrink(xl);
  slot_id = xmem_log_table_slot_id(xl, ptr);
  p = xl->slot[slot_id];
  if (p == NULL) {
    // head is null, so no element found
    return XFAILURE;
  } else if (ptr == p->ptr) {
    // head is target
    xl->slot[slot_id] = xl->slot[slot_id]->next;
    free(p);
    xl->entry_count--;
    return XSUCCESS;
  } else {
    // head is not target
    q = p->next;
    for (;;) {
      // q is the item to be checked
      q = p->next;
      if (q == NULL)
        return XFAILURE;
      
      if (ptr == q->ptr) {
        // q is target
        p->next = q->next;
        free(q);
        xl->entry_count--;
        return XSUCCESS;
      }
      p = q;
    }
  }
  return XFAILURE;
}

int xmem_log_table_size(xmem_log_table* xl) {
  return xl->entry_count;
}

void xmem_log_table_visit(xmem_log_table* xl, xmem_log_table_visitor visitor, void* args) {
  size_t i;
  size_t actual_size = (xl->base_size << xl->extend_level) + xl->extend_ptr;
  xbool should_continue = XTRUE;
  xmem_log_table_entry* p;
  
  for (i = 0; i < actual_size && should_continue == XTRUE; i++) {
    p = xl->slot[i];
    while (p != NULL && should_continue == XTRUE) {
      should_continue = visitor(p->ptr, p->file, p->line, args);
      p = p->next;
    }
  }
}

static xbool xmem_log_table_print_visitor(void* ptr, const char* file, int line, void* args) {
  FILE* fp = (FILE *) args;
  fprintf(fp, "[xdk-xmemory] memory usage: ptr %p, file '%s', line %d\n", ptr, file, line);
  return XTRUE;
}

#endif  // XMEM_DEBUG == 1

void* xmalloc(int size, ...) {
  void* ret = NULL;
  if (size > 0) {
    void* ptr;
#ifdef HAVE_LIBPTHREAD
    pthread_mutex_lock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD

    ptr = malloc(size);
    xmalloc_counter++;

#if XMEM_DEBUG == 1
    {
      va_list argp;
      char* file;
      int line;
      va_start(argp, size);

      file = va_arg(argp, char *);
      line = va_arg(argp, int);

      if (xmem_log == NULL) {
        xmem_log = xmem_log_table_new();
      }
      xmem_log_table_put(xmem_log, ptr, file, line);

      va_end(argp);
    }
#endif  // XMEM_DEBUG == 1

#ifdef HAVE_LIBPTHREAD
    pthread_mutex_unlock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD
    ret = ptr;
  }
  return ret;
}

void xfree(void* ptr) {
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD

  xmalloc_counter--;
  free(ptr);

#if XMEM_DEBUG == 1
  if (xmem_log != NULL) {
    xmem_log_table_remove(xmem_log, ptr);
  }
#endif  // XMEM_DEBUG == 1

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD
}

void* xrealloc(void* ptr, int new_size) {
  void* new_ptr;
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD

  new_ptr = realloc(ptr, new_size);

#if XMEM_DEBUG == 1
  if (xmem_log != NULL) {
    const char* file = xmem_log_table_get_file(xmem_log, ptr);
    int line = xmem_log_table_get_line(xmem_log, ptr);
    xmem_log_table_remove(xmem_log, ptr);
    xmem_log_table_put(xmem_log, new_ptr, file, line);
  }
#endif  // XMEM_DEBUG == 1

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD
  return new_ptr;
}

int xmem_usage(FILE* fp) {
  int ret;
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD

  ret = xmalloc_counter;
  if (fp != NULL) {
    fprintf(fp, "[xdk-xmemory] memory usage: allocation count = %d\n", xmalloc_counter);
#if XMEM_DEBUG == 1
    xmem_log_table_visit(xmem_log, xmem_log_table_print_visitor, fp);
#endif  // XMEM_DEBUG == 1
  }

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD
  return ret;
}

void xmem_reset_counter() {
#ifdef HAVE_LIBPTHREAD
  pthread_mutex_lock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD

  xmalloc_counter = 0;

#if XMEM_DEBUG == 1
  if (xmem_log != NULL) {
    xmem_log_table_delete(xmem_log);
    xmem_log = NULL;
  }
#endif  // XMEM_DEBUG == 1

#ifdef HAVE_LIBPTHREAD
  pthread_mutex_unlock(&xmem_mutex);
#endif  // HAVE_LIBPTHREAD
}

