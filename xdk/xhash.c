#include <stdlib.h>

#include "xhash.h"
#include "xmemory.h"

#define XHASH_INIT_SLOT_COUNT 16
#define XHASH_THRESHOLD 3

/**
  @brief
    Hash entry type.
*/
typedef struct xhash_entry {
  void* key;  ///< @brief Key of the hash entry.
  void* value;  ///< @brief Value of the hash entry.
  struct xhash_entry* next; ///< @brief Link to next hash entry (linked hashtable entry).
} xhash_entry;

/**
  @brief
    Linear hash table type.

  TODO add detailed info about the linear hash table
*/
struct xhash_impl {
  xhash_entry** slot; ///< @brief Slots used to store hash entry (as link list).

  int entry_count; ///< @brief For calculating load average.
  int extend_ptr;  ///< @brief Index of next element to be expanded.
  int extend_level; ///< @brief How many times the table get expanded.

  /**
    @brief
      The size of the hash table in the begining.

    Current hash table's "appearing" size is base_size * 2 ^ level.
    Actuall size could be calculated by: extend_ptr + base_size * 2 ^ level.
  */
  int base_size;

  xhash_hash hash_func; ///< @brief Hashcode calculator.
  xhash_eql eql_func; ///< @brief Hash entry equality checker.
  xhash_free free_func; ///< @brief Hash entry destructor.
};

#define ALLOC(ty, n) ((ty *) xmalloc(sizeof(ty) * (n)))
#define REALLOC(ty, ptr, n) (ty *) xrealloc(ptr, sizeof(ty) * (n))

xhash xhash_new(xhash_hash arg_hash, xhash_eql arg_eql, xhash_free arg_free) {
  int i;
  xhash xh = xmalloc_ty(1, struct xhash_impl);

  xh->extend_ptr = 0;
  xh->extend_level = 0;
  xh->base_size = XHASH_INIT_SLOT_COUNT;
  xh->entry_count = 0;

  xh->hash_func = arg_hash;
  xh->eql_func = arg_eql;
  xh->free_func = arg_free;

  xh->slot = ALLOC(xhash_entry*, xh->base_size);

  for (i = 0; i < xh->base_size; i++) {
    xh->slot[i] = NULL;
  }

  return xh;
}

void xhash_delete(xhash xh) {
  int i;
  int actual_size = (xh->base_size << xh->extend_level) + xh->extend_ptr;
  xhash_entry* p;
  xhash_entry* q;
  
  for (i = 0; i < actual_size; i++) {
    p = xh->slot[i];
    while (p != NULL) {
      q = p->next;
      xh->free_func(p->key, p->value);
      xfree(p);
      p = q;
    }
  }
  xfree(xh->slot);
  xfree(xh);
}

// calculates the actuall slot id of a key
static int xhash_slot_id(xhash xh, void* key) {
  int hcode = xh->hash_func(key);
  if (hcode < 0) {
    hcode = -hcode;
  }
  if (hcode % (xh->base_size << xh->extend_level) < xh->extend_ptr) {
    // already extended part
    return hcode % (xh->base_size << (1 + xh->extend_level));
  } else {
    // no the yet-not-extended part
    return hcode % (xh->base_size << xh->extend_level);
  }
}


// extend the hash table if necessary
static void xhash_try_extend(xhash xh) {
  if (xh->entry_count > XHASH_THRESHOLD * (xh->base_size << xh->extend_level)) {
    xhash_entry* p;
    xhash_entry* q;

    int actual_size = (xh->base_size << xh->extend_level) + xh->extend_ptr;

    xh->slot = REALLOC(xhash_entry *, xh->slot, actual_size + 1);
    p = xh->slot[xh->extend_ptr];
    xh->slot[xh->extend_ptr] = NULL;
    xh->slot[actual_size] = NULL;
    while (p != NULL) {
      int hcode = xh->hash_func(p->key);
      int slot_id;
      if (hcode < 0) {
        hcode = -hcode;
      }
      slot_id = hcode % (xh->base_size << (1 + xh->extend_level));
      q = p->next;
      p->next = xh->slot[slot_id];
      xh->slot[slot_id] = p;
      p = q;
    }

    xh->extend_ptr++;
    if (xh->extend_ptr == (xh->base_size << xh->extend_level)) {
      xh->extend_ptr = 0;
      xh->extend_level++;
    }
  }

}

void xhash_put(xhash xh, void* key, void* value) {
  int slot_id;
  xhash_entry* entry = ALLOC(xhash_entry, 1);

  // first of all, test if need to expand
  xhash_try_extend(xh);

  slot_id = xhash_slot_id(xh, key);

  entry->next = xh->slot[slot_id];
  entry->key = key;
  entry->value = value;
  xh->slot[slot_id] = entry;

  xh->entry_count++;
}

void* xhash_get(xhash xh, void* key) {
  int slot_id = xhash_slot_id(xh, key);

  xhash_entry* p;
  xhash_entry* q;

  p = xh->slot[slot_id];
  while (p != NULL) {
    q = p->next;
    if (xh->eql_func(key, p->key))
      return p->value;
    p = q;
  }
  return NULL;
}

// shrink the hash table if necessary
// table size is shrinked to 1/2
static void xhash_try_shrink(xhash xh) {
  if (xh->extend_level == 0)
    return;

  if (xh->entry_count * XHASH_THRESHOLD < (xh->base_size << xh->extend_level)) {
    int i;
    int actual_size = (xh->base_size << xh->extend_level) + xh->extend_ptr;
    int new_size = xh->base_size << (xh->extend_level - 1);

    for (i = new_size; i < actual_size; i++) {
      int slot_id = i % new_size;
      //merge(&lh->slot[i % new_size], lh->slot[i]);
      if (xh->slot[slot_id] == NULL) {
        xh->slot[slot_id] = xh->slot[i];
      } else {
        xhash_entry* p = xh->slot[slot_id];
        while (p->next != NULL)
          p = p->next;
        p->next = xh->slot[i];
      }
    }

    xh->extend_level--;
    xh->extend_ptr = 0;

    xh->slot = REALLOC(xhash_entry *, xh->slot, xh->base_size << xh->extend_level);
  }
}

int xhash_remove(xhash xh, void* key) {
  int slot_id;
  xhash_entry* p;
  xhash_entry* q;

  xhash_try_shrink(xh);
  slot_id = xhash_slot_id(xh, key);
  p = xh->slot[slot_id];
  if (p == NULL) {
    // head is null, so no element found
    return XFAILURE;
  } else if (xh->eql_func(key, p->key)) {
    // head is target
    xh->slot[slot_id] = xh->slot[slot_id]->next;
    xh->free_func(p->key, p->value);
    xfree(p);
    xh->entry_count--;
    return XSUCCESS;
  } else {
    // head is not target
    q = p->next;
    for (;;) {
      // q is the item to be checked
      q = p->next;
      if (q == NULL)
        return XFAILURE;
      
      if (xh->eql_func(key, q->key)) {
        // q is target
        p->next = q->next;
        xh->free_func(q->key, q->value);
        xfree(q);
        xh->entry_count--;
        return XSUCCESS;
      }
      p = q;
    }
  }
  return XFAILURE;
}

int xhash_size(xhash xh) {
  return xh->entry_count;
}

void xhash_visit(xhash xh, xhash_visitor visitor, void* args) {
  int i;
  int actual_size = (xh->base_size << xh->extend_level) + xh->extend_ptr;
  xbool should_continue = XTRUE;
  xhash_entry* p;
  
  for (i = 0; i < actual_size && should_continue == XTRUE; i++) {
    p = xh->slot[i];
    while (p != NULL && should_continue == XTRUE) {
      should_continue = visitor(p->key, p->value, args);
      p = p->next;
    }
  }
}

