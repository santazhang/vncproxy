#ifndef XHASH_H_
#define XHASH_H_

#include "xdef.h"

/**
  @author
    Santa Zhang

  @file
    xhash.h

  @brief
    Simple linear hash table implementation.

  It will adaptively adjust its slot size to meet storage requirements.
*/

/**
  @brief
    Function type for hashcode calculator.

  @param key
    The key whose hash value should be calculated.

  @return
    The hash value of key.
*/
typedef int (*xhash_hash)(void* key);

/**
  @brief
    Function type for hash entry equality checking.

  @param key1
    The key to be compared.
  @param key2
    The key to be compared.

  @return
    1 if the keys are equal, otherwise 0.
*/
typedef xbool (*xhash_eql)(void* key1, void* key2);

/**
  @brief
    Function type for hash entry destructor.

  @param key
    Pointer to the key.
  @param value
    Pointer to the value.
*/
typedef void (*xhash_free)(void* key, void* value);

/**
  @brief
    Visitor function that iterates on hash table elements.

  @param key
    The key of hash table element.
  @param value
    The value of hash table element.
  @param args
    Additional helper arguments.

  @return
    Whether we should go on visiting other elements. If XFALSE is returned, no more element will be visited.
*/
typedef xbool (*xhash_visitor)(void* key, void* value, void* args);

/**
  @brief
    Interface of xhash which is exposed to users.
*/
typedef struct xhash_impl* xhash; 

/**
  @brief
    Initialize hash table.

  @param arg_hash
    The hashcode calculator function.
  @param arg_eql
    The hash entry equality checker.
  @param arg_free
    The hash entry destructor.
*/
xhash xhash_new(xhash_hash arg_hash, xhash_eql arg_eql, xhash_free arg_free);

/**
  @brief
    Destruct a hash table.

  The hash table itself will be destructed by calling xfree(). free_func() will be invoked on each entry.

  @param xh
    The hash table to be destructed.
*/
void xhash_delete(xhash xh);

/**
  @brief
    Put a new entry (key, value) into hash table.

  @param xh
    The hash table in which new entry will be put.
  @param key
    Pointer to the key element.
  @param value
    Pointer to the value element.
*/
void xhash_put(xhash xh, void* key, void* value);

/**
  @brief
    Get an entry from hash table.

  @param xh
    The hash table from which the value will be fetched.
  @param key
    Pointer to the key element.

  @return
    NULL if not found, otherwise corresponding *value will be returned.
*/
void* xhash_get(xhash xh, void* key);

/**
  @brief
    Remove an entry from hash table.

  @param xh
    The hash table from which entry will be removed.
  @param key
    Pointer to the key which will be removed.

  @return
    0 if successful, -1 if failed.
*/
xsuccess xhash_remove(xhash xh, void* key);

/**
  @brief
    Iterate on the hash table elements by a visitor.

  @param xh
    The hash table to be visited.
  @param visitor
    The visitor function.
  @param args
    Additional arguments to the visitor.
*/
void xhash_visit(xhash xh, xhash_visitor visitor, void* args);

/**
  @brief
    Get the number of elements in a xhash table.

  @param xh
    The xhash table object.

  @return
    Number of elements inside the xhash table.
*/
int xhash_size(xhash xh);

#endif  // XHASH_H_

