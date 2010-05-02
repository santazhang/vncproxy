#ifndef XVEC_H_
#define XVEC_H_

/**
  @author
    Santa Zhang

  @file
    xvec.h

  @brief
    Simple vector implementation.
*/


/**
  @brief
    Hidden implementation of xvec.
*/
struct xvec_impl;


/**
  @brief
    Exposed xvec interface.
*/
typedef struct xvec_impl* xvec;

/**
  @brief
    Function that frees memory when removing xvec elements.
*/
typedef void (*xvec_free)(void* ptr);

/**
  @brief
    Create a new xvec.

  @param xvfree
    The function that takes care of memory allocation when removing elements.

  @return
    A new xvec object, initialized, and empty.
*/
xvec xvec_new(xvec_free xvfree);

/**
  @brief
    Destroy an xvec object.

  @param xv
    The xvec object to be destroyed.
*/
void xvec_delete(xvec xv);

/**
  @brief
    Get an element from xvec object.

  @param xv
    The xvec containing data.
  @param index
    The index of data which will be extracted.

  @return
    NULL if index is out of range (index \< 0 or index \>= xvec size).

  @warning
    You'd better check if index is in range before extracting the obejct.
*/
void* xvec_get(xvec xv, int index);

/**
  @brief
    Put a new data into xvec object.

  @param xv
    The xvec object where new data will be put into.
  @param index
    The exact location where new data will be put into.
    This index is in range [0, xvec size]. If "xvec size" is given, it pushes new data at then end of xvec.
  @param data
    Pointer to data which will be put into xvec object.

  @warning
    Be sure that index is not out of range!
    And you should know that old data will be overwritten by new data!
    What's more, keep in mind that the data is not deeply copied into xvec object,
    only the data pointer's value will be put into xvec's internal data structure!
    Also, note that the old data will be free'd!

  @return
    New data's index on success.
    -1 on failure.
*/
int xvec_put(xvec xv, int index, void* data);

/**
  @brief
    Insert a new element into the xvec.

  @param xv
    The xvec object.
  @param index
    The index where new object will be inserted into.
    This index is in range [0, xvec size]. If "xvec size" is given, it pushes new data at then end of xvec.
  @param data
    The pointer to new data.

  @return
    The location of newly inserted data.

  @warning
    Make sure that index is in proper range.
*/
int xvec_insert(xvec xv, int index, void* data);

/**
  @brief
    Remove an element from xvec.

  @param xv
    The xvec which contains data.
  @param index
    The location of element which will be removed.
    Out of bound value of index will result in failure.

  @warning
    Make sure index is in range!
    Also, the removed object will be free'd.

  @return
    The value of index if success.
    -1 on failure.
*/
int xvec_remove(xvec xv, int index);

/**
  @brief
    Add data to the end of xvec.

  @param xv
    The xvec object.
  @param data
    The pointer to new data.

  @return
    New data's index on success.
    -1 on failure.
*/
int xvec_push_back(xvec xv, void* data);

/**
  @brief
    Get size of xvec object.

  @param xv
    The xvec whose size we care about.
  @return
    The size of xvec object.
*/
int xvec_size(xvec xv);

#endif  // XVEC_H_

