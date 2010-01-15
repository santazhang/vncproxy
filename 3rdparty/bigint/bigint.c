#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "bigint.h"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif


// maximum digits in a int
#define BIGINT_INT_MAX_DIGITS 10

// the initial memory size of a bigint
#define BIGINT_INIT_MEM_SIZE 4

// the maximum precision digits of double value
#define BIGINT_DOUBLE_PRECISION 16

// the maximum length of digits that would be handled by
// traditional multiplication
#define BIGINT_MUL_THRESHOLD 100

// the radix used by big int
#define BIGINT_RADIX 1000000000

// the log10() result of BIGINT_RADIX, also is text length of a segment
#define BIGINT_RADIX_LOG10 9

#define BIGINT_ALLOC(size) bigint_alloc(size)

#define BIGINT_FREE(ptr) bigint_free(ptr)

static int bigint_alloc_counter = 0;


static void* bigint_alloc(int size) {
  bigint_alloc_counter++;
  return malloc(size);
}


static void bigint_free(void* ptr) {
  bigint_alloc_counter--;

  assert(bigint_alloc_counter >= 0);

  free(ptr);
}


// not published, use 'extern' to get the value
int bigint_alloc_count() {
  return bigint_alloc_counter;
}


// used by bigint_assure_memory and bigint_pack_memory
// it is made sure that new_mem_size >= p_bigint->data_len
static void bigint_set_memory_size(bigint* p_bigint, int new_mem_size) {

  int* old_ptr = p_bigint->p_data;
  int* new_ptr = BIGINT_ALLOC(sizeof(int) * new_mem_size);

  assert(new_mem_size >= p_bigint->data_len);

  memcpy(new_ptr, old_ptr, sizeof(int) * p_bigint->data_len);

  p_bigint->mem_size = new_mem_size;
  p_bigint->p_data = new_ptr;

  BIGINT_FREE(old_ptr);

}


// assure that the big int have enough memory length
static void bigint_assure_memory(bigint* p_bigint, int min_size) {

  assert(min_size > 0);

  // required size
  if (min_size > p_bigint->mem_size) {

    bigint_set_memory_size(p_bigint, min_size * 2);

  }

}


// packs the value, remove the leading 0's (except the special case of 0)
static void bigint_pack_memory(bigint* p_bigint) {

  // 'remove' the leading 0's
  int index = p_bigint->data_len - 1;
  while (index > 0 && p_bigint->p_data[index] == 0) {
    p_bigint->data_len--;
    index--;
  }


  if (p_bigint->data_len * 4 < p_bigint->mem_size) {

    bigint_set_memory_size(p_bigint, p_bigint->data_len * 2);

  }
}


// check whether we splited a number correctly
static int bigint_split_number_check(bigint* p_bigint, bigint* p_high,
                                      bigint* p_low, int low_len) {
  
  bigint bi;
  int ret;
//   char str[10000];
// 
//   bigint_to_string(p_bigint, str);
//   printf("CHECK SPLIT: %s is\n", str);
// 
//   bigint_to_string(p_high, str);
//   printf("%s ", str);
// 
//   bigint_to_string(p_low, str);
//   printf("%s\n", str);

  bigint_init(&bi);
  bigint_copy(&bi, p_high);
  bigint_mul_by_pow_10(&bi, low_len * BIGINT_RADIX_LOG10);
  bigint_add_by(&bi, p_low);
  ret = bigint_equal(&bi, p_bigint);

  bigint_release(&bi);
  return ret;
}


// split a bigint into 2 parts, the lower part has length <= low_len
// the sign is copied into high and low parts
// (with the special case of high part = 0)
// and it is made sure that
// biging = high * 10^(low_len * BIGINT_RADIX_LOG10) + low
static void bigint_split_number(bigint* p_bigint, bigint* p_high,
                                bigint* p_low, int low_len) {

  if (low_len >= p_bigint->data_len) {
    // special case
    bigint_set_zero(p_high);
    bigint_copy(p_low, p_bigint);
  } else {

    int index;

    p_high->sign = p_bigint->sign;
    bigint_assure_memory(p_high, p_bigint->data_len - low_len);
    p_high->data_len = p_bigint->data_len - low_len;
    for (index = 0; index < p_high->data_len; index++) {
      p_high->p_data[index] = p_bigint->p_data[low_len + index];
    }
    bigint_pack_memory(p_high);

    p_low->sign = p_bigint->sign;
    bigint_assure_memory(p_low, low_len);
    p_low->data_len = low_len;
    for (index = 0; index < p_low->data_len; index++) {
      p_low->p_data[index] = p_bigint->p_data[index];
    }
    bigint_pack_memory(p_low);
  }

  assert(bigint_split_number_check(p_bigint, p_high, p_low, low_len));

}


//////////////////////////////////////////////////////////////////////////


void bigint_init(bigint* p_bigint) {

  p_bigint->mem_size = BIGINT_INIT_MEM_SIZE;
  p_bigint->p_data = BIGINT_ALLOC(sizeof(int) * p_bigint->mem_size);

  p_bigint->data_len = 1;
  p_bigint->sign = 0;
  p_bigint->p_data[0] = 0;

}



void bigint_release(bigint* p_bigint) {

  BIGINT_FREE(p_bigint->p_data);

  // set p_data to NULL, prevents dangling pointers
  p_bigint->p_data = NULL;

}



void bigint_from_int(bigint* p_bigint, int value) {

  // set up the sign
  if (value < 0) {
    p_bigint->sign = -1;
  } else if (value > 0) {
    p_bigint->sign = 1;
  } else {
    p_bigint->sign = 0;
  }

  // from now on, only consider the case of value >= 0
  if (value < 0) {
    value = -value;
  }

  
  if (value == 0) {
    // handle special case, value = 0

    p_bigint->data_len = 1;
    p_bigint->p_data[0] = 0;

  } else {
    // value > 0

    int index = 0;

    // reset data length
    p_bigint->data_len = 0;

    while (value > 0) {

      // pick the lower digits from value
      int lower_value = value % BIGINT_RADIX;
      value = value / BIGINT_RADIX;


      // assure we have memory for 1 more segment
      bigint_assure_memory(p_bigint, p_bigint->data_len + 1);

      p_bigint->p_data[index] = lower_value;
      p_bigint->data_len++;
      index++;

    }

  }

  // pack the memory, keep low memory usage
  bigint_pack_memory(p_bigint);
  
}



bigint_errno bigint_from_double(bigint* p_bigint, double value) {

  if (-0.5 < value && value < 0.5) {
    // the special case of 0
    bigint_set_zero(p_bigint);

  } else {

    // TODO check NaN and INF

    double lg_value;

    int index;

    if (value < 0) {
      // handle the case of value < 0
      value = -value;
      p_bigint->sign = -1;

    } else {
      p_bigint->sign = 1;
    }

    lg_value = log10(value);

    bigint_assure_memory(p_bigint, ((int) lg_value) / BIGINT_RADIX_LOG10 + 2);

    // set the lower segments to 0
    index = 0;
    while (index * BIGINT_RADIX_LOG10 < lg_value - BIGINT_DOUBLE_PRECISION) {
      p_bigint->p_data[index] = 0;
      index++;
    }

    value = pow(10.0, lg_value - index * BIGINT_RADIX_LOG10);

    // now write the precise value
    while (value > 0.0) {
      double r = fmod(value, BIGINT_RADIX);
      value = (value - r) / BIGINT_RADIX;
      p_bigint->p_data[index] = (int) (r + 0.5);
      index++;
    }

    p_bigint->data_len = index;
    bigint_pack_memory(p_bigint);

  }

  return -BIGINT_NOERR;
}


//
// A few frustrating numbers:
// 0000e0 00000e-1   0.0000e1
// 5e-1 (shoule be 1)
// 4e-1 (should be 0)
// 0.395e2 (should be 40) (set BIGINT_RADIX = 10, there is a carriage problem)
bigint_errno bigint_from_string(bigint* p_bigint, char* str) {

  // first of all, check grammar, and get the approximate length
  int approx_len = 0;

  int has_neg_sign = 0;

  int mantissa_value = 0;

  // marks for 3 parts of value: fixed, fraction, mantissa
  // every part is in the range [begin, end), notice the end is not included
  int fixed_begin = -1;
  int fixed_end = -1;
  int fixed_len = 0;
  int fraction_begin = -1;
  int fraction_end = -1;
  int fraction_len = 0;
  int mantissa_begin = -1;
  int mantissa_end = -1;

  // we should construct a FSM to parst the string
  // the FSM has the following states: (end states are marked with *)
  // 
  //  0: start state
  //  1: just read '+' or '-', waiting for 0-9
  // *2: has read some digits
  //  3: just read '.', waiting for 0-9
  // *4: has read some digits
  //  5: just read 'E' or ''e, waiting for 0-9
  //  6: just read '+' or '-', waiting for 0-9
  // *7: has read some digits
  // 
  // the transition actions are:
  // 
  //  state 0 -> state 1 (read '+'/'-')
  //  state 0 -> state 2 (read 0-9)
  // 
  //  state 1 -> state 2 (read 0-9)
  //
  //  state 2 -> state 2 (read 0-9)
  //  state 2 -> state 3 (read '.')
  //  state 2 -> state 5 (read 'E'/'e')
  //
  //  state 3 -> state 4 (read 0-9)
  // 
  //  state 4 -> state 4 (read 0-9)
  //  state 4 -> state 5 (read 'E'/'e')
  //
  //  state 5 -> state 6 (read '+'/'-')
  //  state 5 -> state 7 (read 0-9)
  //
  //  state 6 -> state 7 (read 0-9)
  //
  //  state 7 -> state 7 (read 0-9)

  int state = 0;
  int index = 0;

  while (str[index] != '\0') {
    char ch = str[index];

    assert(state < 8);

    switch (state) {
      case 0:

        if (ch == '+') {
          // jumps to state 1, and do nothing else
          state = 1;
        } else if (ch == '-') {
          // jumps to state 1, and mark the sign
          has_neg_sign = 1;
          state = 1;
        } else if ('0' <= ch && ch <= '9') {
          // jumps to state 2, and mark the fixed_begin
          fixed_begin = index;
          state = 2;
        } else {
          // not acceptable input
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 1:

        if ('0' <= ch && ch <= '9') {
          // mark the beginning of fixed part, and goes to state 2
          fixed_begin = index;
          state = 2;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 2:

        fixed_end = index;
        if ('0' <= ch && ch <= '9') {
          // state not changed, still state 2
        } else if (ch == '.') {
          state = 3;
        } else if (ch == 'E' || ch == 'e') {
          mantissa_begin = index;
          state = 5;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 3:

        if ('0' <= ch && ch <= '9') {
          fraction_begin = index;
          state = 4;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 4:

        fraction_end = index;
        if ('0' <= ch && ch <= '9') {
          // state not changed, still state 4
        } else if (ch == 'E' || ch == 'e') {
          state = 5;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 5:

        mantissa_begin = index;
        if ('0' <= ch && ch <= '9') {
          state = 7;
        } else if (ch == '+' || ch == '-') {
          state = 6;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 6:

        if ('0' <= ch && ch <= '9') {
          mantissa_end = index + 1;
          state = 7;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;

      case 7:

        if ('0' <= ch && ch <= '9') {
          // state is not changed
          mantissa_end = index + 1;
        } else {
          return -BIGINT_ILLEGAL_PARAM;
        }
        break;
    }

    
//    printf("index=%d char=%c state=%d\n", index, ch, state);

    index++;
// 
//     printf("fixed_begin = %d\n", fixed_begin);
//     printf("fixed_end = %d\n", fixed_end);
//     printf("fraction_begin = %d\n", fraction_begin);
//     printf("fraction_end = %d\n", fraction_end);
//     printf("mantissa_begin = %d\n", mantissa_begin);
//     printf("mantissa_end = %d\n", mantissa_end);
  }

  // test if arrived on ending states, and mark the corresponding values
  if (state == 2) {
    fixed_end = index;
  } else if (state == 4) {
    fraction_end = index;
  } else if (state == 7) {
    mantissa_end = index;
  } else {
    return -BIGINT_ILLEGAL_PARAM;
  }

  // we are sure that the end of mantissa part is also the end of string
  // if not, we will have ILLEGAL_FORMAT
  // so we could get the mantissa safely by the following code
  if (mantissa_begin != -1) {
    mantissa_value = atoi(str + mantissa_begin);
  } else {
    mantissa_value = 0;
  }

  fixed_len = fixed_end - fixed_begin;
  fraction_len = fraction_end - fraction_begin;

//  printf("Mantissa = %d\n", mantissa_value);

  approx_len = mantissa_value + fixed_len;
//  printf("Approx len = %d\n", approx_len);

  if (approx_len < 0) {
    // special case, such as '1e-2', '123e-4', which should have 0 as result
    bigint_set_zero(p_bigint);

  } else if (approx_len == 0) {
    // special case, which might have 1 or -1 as result
    // consider '5.5e-1' which should result in 1
    // and '4.5e-1' which should result in 0
    if (str[fixed_begin] >= '5') {
      bigint_set_one(p_bigint);

      if (has_neg_sign) {
        bigint_change_sign(p_bigint);
      }

    } else {
      bigint_set_zero(p_bigint);
    }

  } else {
    // normal case, approx_len > 0

    int i;

    // index in the str. 'index' is used in p_bigint's p_data
    int pos = 0;
    
    // used to set the digit on a certain position
    int weight = 1;

    // whether this number is 'all zero', such as the case '0000000e10' = 0
    int is_zero = 1;

    // used for the carry between different digits
    int carry = 0;

    bigint_assure_memory(p_bigint, approx_len / BIGINT_RADIX_LOG10 + 2);

    // fill the allocated memory with 0's
    for (i = 0; i < p_bigint->mem_size; i++) {
      p_bigint->p_data[i] = 0;
    }

    // adjust mantissa value and start position
    // for a value A.B e X, we change it to AB e Y
    mantissa_value -= fraction_len;

    // move the index in string to last digit in fixed/fraction part
    if (fraction_len > 0) {
      pos = fraction_end - 1;
    } else {
      pos = fixed_end - 1;
    }

    // and now, if the mantissa is negative, we try to make it to 0
    // by removing several digits. notice that the carry should be tested
    if (mantissa_value < 0) {

      // the number of digits to be dropped
      int drop = -mantissa_value;
      
      // dropping
      while (drop > 0) {

        // skip the period
        if (str[pos] == '.')
          pos--;

        // update the carry
        if (str[pos] >= '5') {
          carry = 1;
        } else {
          carry = 0;
        }
        
        // drop digits in the lower part
        pos--;

        drop--;
      }

      mantissa_value = 0;

    }

    // from then on, we don't consider the fraction part
    // instead, we consider the fixed and fraction part as a whole

    // skip the lower segments, leave them to be 0
    index = mantissa_value / BIGINT_RADIX_LOG10;

    // raise the weight for the lowest digit, 'i' is a helper variable
    i = 0;
    while (index * BIGINT_RADIX_LOG10 + i < mantissa_value) {
      weight *= 10;
      i++;
    }

    // write the digits in the integer
    while (pos >= fixed_begin) {

      // skip the period
      if (str[pos] == '.')
        pos--;

      // test all zero numbers
      if (('1' <= str[pos] && str[pos] <= '9') || carry != 0) {
        is_zero = 0;
      }

      p_bigint->p_data[index] += weight * (str[pos] - '0' + carry);

      carry = 0;

      weight *= 10;

      // go to a higher segment
      if (weight >= BIGINT_RADIX) {
        weight = 1;
        if (p_bigint->p_data[index] > BIGINT_RADIX) {
          p_bigint->p_data[index] %= BIGINT_RADIX;
          carry = 1;
        }
        index++;
        p_bigint->p_data[index] = 0;
      }

      pos--;
    }

    if (is_zero) {
      bigint_set_zero(p_bigint);

    } else if (has_neg_sign) {
      p_bigint->sign = -1;
      p_bigint->data_len = index + 1;
      bigint_pack_memory(p_bigint);
    } else {
      p_bigint->sign = 1;
      p_bigint->data_len = index + 1;
      bigint_pack_memory(p_bigint);
    }
    
  }

  return -BIGINT_NOERR;
}



int bigint_digit_count(bigint* p_bigint) {
  int len;

  if (bigint_is_zero(p_bigint)) {
    // special case of 0

    assert(p_bigint->data_len == 1 &&
      p_bigint->sign == 0 &&
      p_bigint->p_data[0] == 0);

    len = 1;

  } else {

    int value;

    len = 0;

    // except the first segment, other segments has length BIGINT_RADIX_LOG10
    len += BIGINT_RADIX_LOG10 * (p_bigint->data_len - 1);

    // handle the first segment
    value = p_bigint->p_data[p_bigint->data_len - 1];

    while (value > 0) {
      len++;
      value /= 10;
    }

  }

  return len;
}



int bigint_string_length(bigint* p_bigint) {

  if (bigint_is_negative(p_bigint)) {
    // negative numbers have a leading '-'
    return 1 + bigint_digit_count(p_bigint);
  } else {
    return bigint_digit_count(p_bigint);
  }
  
}



void bigint_to_string(bigint* p_bigint, char* str) {

  // index in the data segment array
  int index = p_bigint->data_len - 1;
  int value;
  int first_seg_length = 0;
  int i;

  if (p_bigint->sign < 0) {
    // negative numbers
    *str = '-';
    str++;
  }

  if (bigint_is_zero(p_bigint)) {
    // handle the special case of 0

    assert(p_bigint->data_len == 1 &&
      p_bigint->sign == 0 &&
      p_bigint->p_data[0] == 0);

    *str = '0';
    str++;

  } else {

    // handle first segment, calculate its length
    first_seg_length = 0;
    value = p_bigint->p_data[index];
    while (value > 0) {
      first_seg_length++;
      value /= 10;    
    }

    // write the first segment
    value = p_bigint->p_data[index];
    i = 0;
    while (value > 0) {
      int r = value % 10;
      value /= 10;
      str[first_seg_length - i - 1] = (char) (r + '0');
      i++;
    }

    str += first_seg_length;
    index--;

    // write the other segments

    while (index >= 0) {

      // we make sure that each segment gets BIGINT_RADIX_LOG10 digits printed
      // 0's are inserted
      int counter = BIGINT_RADIX_LOG10;
      value = p_bigint->p_data[index];
      i = 0;
      while (counter > 0) {
        int r = value % 10;
        value /= 10;
        str[BIGINT_RADIX_LOG10 - i - 1] = (char) (r + '0');
        i++;
        counter--;
      }

      str += BIGINT_RADIX_LOG10;
      index--;
    }

  }

  // append the last '\0'
  *str = '\0';
}




bigint_errno bigint_to_double(bigint* p_bigint, double* p_double) {

  // TODO check overflow

  int index = p_bigint->data_len - 1;

  // trivial overflow test
  if (BIGINT_RADIX_LOG10 * p_bigint->data_len > 310) {
    return -BIGINT_OVERFLOW;
  }

  *p_double = 0;

  while (index >= 0) {
    *p_double *= BIGINT_RADIX;
    *p_double += p_bigint->p_data[index];
    index--;
  }

  if (p_bigint->sign == -1) {
    *p_double = -*p_double;
  }

  return -BIGINT_NOERR;
}



bigint_errno bigint_to_int(bigint* p_bigint, int* p_int) {

  int index;

  // used to test overflow
  long long overflow_tester = 0;

  if (p_bigint->sign > 0) {
    for (index = p_bigint->data_len - 1; index >= 0; index--) {
      overflow_tester *= (long long) BIGINT_RADIX;
      overflow_tester += (long long) p_bigint->p_data[index];
      if (overflow_tester >= 2147483648LL)
        return -BIGINT_OVERFLOW;
    }
  } else if (p_bigint->sign < 0) {
    for (index = p_bigint->data_len - 1; index >= 0; index--) {
      overflow_tester *= (long long) BIGINT_RADIX;
      overflow_tester += (long long) p_bigint->p_data[index];
      if (overflow_tester > 2147483648LL)
        return -BIGINT_OVERFLOW;
    }
  }

  index = p_bigint->data_len - 1;

  *p_int = 0;

  while (index >= 0) {
    *p_int *= BIGINT_RADIX;
    *p_int += p_bigint->p_data[index];
    index--;
  }

  if (p_bigint->sign == -1) {
    *p_int = -*p_int;
  }

  return -BIGINT_NOERR;
}




void bigint_copy(bigint* p_dst, bigint* p_src) {

  bigint_assure_memory(p_dst, p_src->data_len);

  memcpy(p_dst->p_data, p_src->p_data, sizeof(int) * p_src->data_len);

  p_dst->data_len = p_src->data_len;
  p_dst->sign = p_src->sign;
  
}



void bigint_change_sign(bigint* p_bigint) {
  p_bigint->sign = -p_bigint->sign;
}



int bigint_is_positive(bigint* p_bigint) {
  return p_bigint->sign > 0;
}



int bigint_is_negative(bigint* p_bigint) {
  return p_bigint->sign < 0;
}



int bigint_is_zero(bigint* p_bigint) {
  return p_bigint->sign == 0;
}



void bigint_set_zero(bigint* p_bigint) {

  p_bigint->data_len = 1;
  p_bigint->p_data[0] = 0;
  p_bigint->sign = 0;
  
  bigint_pack_memory(p_bigint);

  assert(bigint_is_zero(p_bigint));
  assert(p_bigint->mem_size > 0);
}



void bigint_set_one(bigint* p_bigint) {

  p_bigint->data_len = 1;
  p_bigint->p_data[0] = 1;
  p_bigint->sign = 1;

  bigint_pack_memory(p_bigint);

  assert(bigint_is_positive(p_bigint));
  assert(p_bigint->mem_size > 0);
}



void bigint_add_by(bigint* p_dst, bigint* p_src) {

  if (bigint_is_zero(p_dst)) {
    // destination is zero, then we only need to copy src to dst

    bigint_copy(p_dst, p_src);


  } else if (bigint_is_zero(p_src)) {
    //source is zero, then we need to do nothing


  } else if (p_src->data_len == 1) {
    // if we got a small number, we could use quicker routine to handle that

    assert(p_src->sign != 0);

    if (p_src->sign < 0) {
      bigint_add_by_int(p_dst, -p_src->p_data[0]);

    } else if (p_src->sign > 0) {
      bigint_add_by_int(p_dst, p_src->p_data[0]);

    }

  } else if (p_dst == p_src) {
    // if the source is the same as destination, we could simply
    // multiply the destination by 2
    bigint_mul_by_int(p_dst, 2);

  } else {

    // normal case

    int index;

    // the max mem size of result
    int result_mem_size_bound = max(p_dst->data_len, p_src->data_len) + 1;

    // assure we have enough memory
    bigint_assure_memory(p_dst, result_mem_size_bound);


    // set the top segments to 0
    for (index = p_dst->data_len; index < result_mem_size_bound; index++) {
      p_dst->p_data[index] = 0;
    }

    // from now on, we can extend the data length, though there might be
    // leading zeros. we will get rid of the leading zeros by using
    // bigint_pack_memory at the end
    p_dst->data_len = result_mem_size_bound;

    // from then on, we put the 'sign' into p_data, and after the addition, 
    // we determine the sign of result, and put it back into 'sign'.
    // also, we make all the p_data in result to be positive numbers (or 0)

    // if p_dst is negative, mark all the numbers inside to be negative
    if (p_dst->sign < 0) {
      for (index = 0; index < p_dst->data_len; index++) {
        p_dst->p_data[index] = -p_dst->p_data[index];
      }
    }

    // add all the numbers in p_src into p_dst, the sign of p_src determines
    // whether we 'add' them, or 'subtract' them
    if (p_src->sign < 0) {
      for (index = 0; index < p_src->data_len; index++) {
        p_dst->p_data[index] -= p_src->p_data[index];
      }
    } else {
      for (index = 0; index < p_src->data_len; index++) {
        p_dst->p_data[index] += p_src->p_data[index];
      }
    }

    // and the sign of the first non-zero element (if there is)
    // determines the sign of result

    // set the sign to 0 by default, for the case of A+(-A)
    p_dst->sign = 0;
    for (index = p_dst->data_len - 1; index >= 0; index--) {

      if (p_dst->p_data[index] < 0) {
        p_dst->sign = -1;
        break;

      } else if (p_dst->p_data[index] > 0) {
        p_dst->sign = 1;
        break;
      }
    }

    // if the result sign is negative, change all the p_data elements' sign
    if (p_dst->sign < 0) {
      for (index = 0; index < p_dst->data_len; index++) {
        p_dst->p_data[index] = -p_dst->p_data[index];
      }
    }

    // now, we need to handle the carry. we handle it from lowest digits
    // toward the highest digits
    for (index = 0; index < p_dst->data_len; index++) {
      if (p_dst->p_data[index] < 0) {

        // 'borrows' 1 from higher digit

        p_dst->p_data[index] += BIGINT_RADIX;
        p_dst->p_data[index + 1]--;

      } else if (p_dst->p_data[index] >= BIGINT_RADIX) {

        // 'carries' 1 to higher digit
        p_dst->p_data[index] -= BIGINT_RADIX;
        p_dst->p_data[index + 1]++;

      }
    }

    // reduce memory usage
    bigint_pack_memory(p_dst);
  }

}



void bigint_add_by_int(bigint* p_dst, int value) {

  if (bigint_is_zero(p_dst)) {
    // special case that p_dst is zero, we only need to store value in it

    bigint_from_int(p_dst, value);

  } else if (value == 0) {
    // if value == 0, we don't do any thing
    // only need to work when value != 0

  } else if (-BIGINT_RADIX < value && value < BIGINT_RADIX) {

    // small integers, could be done quickly

    if (p_dst->data_len == 1) {
      // if p_dst is small, too, then we simply use int addition

      if (p_dst->sign < 0) {
        bigint_from_int(p_dst, -p_dst->p_data[0] + value);

      } else {
        bigint_from_int(p_dst, p_dst->p_data[0] + value);
      }

    } else {
      // if p_dst is large, then adding value will NOT change the sign
      // and the length of p_dst will be changed by at most 1

      int value_sign;

      int index = 0;

      int max_segment_incr = (BIGINT_INT_MAX_DIGITS / BIGINT_RADIX_LOG10) + 1;

      if (value < 0) {
        value_sign = -1;
        value = -value;
      } else {
        value_sign = 1;
      }

      bigint_assure_memory(p_dst, p_dst->data_len + max_segment_incr);

      // add leading 0
      p_dst->p_data[p_dst->data_len] = 0;
      p_dst->data_len++;

      // from here on, value is positive

      if (value_sign == p_dst->sign) {
        // same sign, do addition
        while (value != 0) {
          p_dst->p_data[index] += value;
          if (p_dst->p_data[index] > BIGINT_RADIX) {
            value = 1;
            p_dst->p_data[index] -= BIGINT_RADIX;
          } else {
            value = 0;
          }
          index++;
        }
        
      } else {
        // different sign, do subtraction
        while (value != 0) {
          p_dst->p_data[index] -= value;
          if (p_dst->p_data[index] < 0) {
            value = 1;
            p_dst->p_data[index] += BIGINT_RADIX;
          } else {
            value = 0;
          }
          index++;
        }

      }

      bigint_pack_memory(p_dst);
    }
   
  } else {

    // really big integer, use bigint to handle them
    bigint bi;
    bigint_init(&bi);
    bigint_from_int(&bi, value);
    bigint_add_by(p_dst, &bi);
    bigint_release(&bi);

  }

}



void bigint_sub_by(bigint* p_dst, bigint* p_src) {

  if (p_dst == p_src) {
    // self subtraction, return 0
    bigint_set_zero(p_dst);

  } else {

    // convert 'A-B' into 'A+(-B)'
    bigint_change_sign(p_src);

    bigint_add_by(p_dst, p_src);

    bigint_change_sign(p_src);
  }

}



void bigint_sub_by_int(bigint* p_dst, int value) {
  // convert 'A-B' into 'A+(-B)'
  bigint_add_by_int(p_dst, -value);
}


// traditional multiplication, do it segment by segment
// not published, this function is not very fast on large numbers
void bigint_mul_by_trad(bigint* p_dst, bigint* p_src) {

  if (p_dst == p_src) {
    // self multiply self
    bigint bi;
    bigint_init(&bi);
    bigint_copy(&bi, p_src);
    bigint_mul_by_trad(p_dst, &bi);
    bigint_release(&bi);
  } else {

    bigint bi;
    bigint bi_add;
    int i;
    bigint_init(&bi);
    bigint_init(&bi_add);
    bigint_set_zero(&bi);

    if (p_src->sign < 0) {
      bigint_change_sign(p_dst);
    }

    for (i = 0; i < p_src->data_len; i++) {
      bigint_copy(&bi_add, p_dst);
      bigint_mul_by_int(&bi_add, p_src->p_data[i]);
      bigint_mul_by_pow_10(&bi_add, i * BIGINT_RADIX_LOG10);
      bigint_add_by(&bi, &bi_add);
    }

    bigint_copy(p_dst, &bi);

    bigint_release(&bi);
    bigint_release(&bi_add);
  }
}


void bigint_mul_by(bigint* p_dst, bigint* p_src) {
  if (p_dst == p_src) {
    // self multiply self
    bigint bi;
    bigint_init(&bi);
    bigint_copy(&bi, p_src);
    bigint_mul_by(p_dst, &bi);
    bigint_release(&bi);
  } else {

    if (bigint_is_zero(p_dst) || bigint_is_zero(p_src)) {
      // handle special cases
      bigint_set_zero(p_dst);

    } else if (p_dst->data_len < BIGINT_MUL_THRESHOLD &&
                p_src->data_len < BIGINT_MUL_THRESHOLD) {

      // small operands, use traditional method
      bigint_mul_by_trad(p_dst, p_src);

    } else if (p_dst->data_len <= BIGINT_MUL_THRESHOLD &&
                p_src->data_len > BIGINT_MUL_THRESHOLD) {                  

      // src is long enough
      bigint bi;
      bigint bi_high;
      bigint bi_low;
      int low_len = p_src->data_len / 2;

      bigint_init(&bi);
      bigint_init(&bi_high);
      bigint_init(&bi_low);

      bigint_copy(&bi, p_dst);

      bigint_split_number(p_src, &bi_high, &bi_low, low_len);

      bigint_mul_by(&bi, &bi_high);
      bigint_mul_by_pow_10(&bi, low_len * BIGINT_RADIX_LOG10);

      bigint_mul_by(p_dst, &bi_low);

      bigint_add_by(p_dst, &bi);

      bigint_release(&bi);
      bigint_release(&bi_high);
      bigint_release(&bi_low);

      if (p_src->sign < 0) {
        bigint_change_sign(p_dst);
      }

    } else if (p_dst->data_len > BIGINT_MUL_THRESHOLD &&
                p_src->data_len <= BIGINT_MUL_THRESHOLD) {

      // dst is long enough
      bigint bi_high;
      bigint bi_low;
      int low_len = p_dst->data_len / 2;

      bigint_init(&bi_high);
      bigint_init(&bi_low);

      bigint_split_number(p_dst, &bi_high, &bi_low, low_len);
      bigint_mul_by(&bi_high, p_src);
      bigint_mul_by_pow_10(&bi_high, low_len * BIGINT_RADIX_LOG10);
      bigint_mul_by(&bi_low, p_src);

      bigint_copy(p_dst, &bi_high);
      bigint_add_by(p_dst, &bi_low);

      bigint_release(&bi_high);
      bigint_release(&bi_low);

      if (p_src->sign < 0) {
        bigint_change_sign(p_dst);
      }

    } else {
      // both operands are long enough

      bigint bi_dst_high;
      bigint bi_dst_low;
      bigint bi_src_high;
      bigint bi_src_low;

      bigint bi;

      int low_len = (p_dst->data_len + p_src->data_len) / 4;

      bigint_init(&bi_dst_high);
      bigint_init(&bi_dst_low);
      bigint_init(&bi_src_high);
      bigint_init(&bi_src_low);

      bigint_init(&bi);

      bigint_split_number(p_src, &bi_src_high, &bi_src_low, low_len);
      bigint_split_number(p_dst, &bi_dst_high, &bi_dst_low, low_len);

      bigint_copy(&bi, &bi_dst_high);
      bigint_mul_by(&bi, &bi_src_low);

      bigint_copy(p_dst, &bi);

      bigint_copy(&bi, &bi_src_high);
      bigint_mul_by(&bi, &bi_dst_low);

      bigint_add_by(p_dst, &bi);

      bigint_mul_by_pow_10(p_dst, low_len * BIGINT_RADIX_LOG10);

      bigint_mul_by(&bi_dst_low, &bi_src_low);
      bigint_add_by(p_dst, &bi_dst_low);

      bigint_mul_by(&bi_dst_high, &bi_src_high);
      bigint_mul_by_pow_10(&bi_dst_high, low_len * BIGINT_RADIX_LOG10 * 2);
      bigint_add_by(p_dst, &bi_dst_high);

      bigint_release(&bi_dst_high);
      bigint_release(&bi_dst_low);
      bigint_release(&bi_src_high);
      bigint_release(&bi_src_low);

      bigint_release(&bi);

      if (p_src->sign < 0) {
        bigint_change_sign(p_dst);
      }

    }
  }

}


void bigint_mul_by_int(bigint* p_bigint, int value) {

  if (value == 0) {
    // special case, set p_bigint to 0
    bigint_set_zero(p_bigint);

  } else if (value == 1) {
    // do nothing

  } else if (value == -1) {
    // change sign
    bigint_change_sign(p_bigint);

  } else {
    // general case

    // the maximum number of shfit
    int max_segment_incr = (BIGINT_INT_MAX_DIGITS / BIGINT_RADIX_LOG10) + 1;

    long long prod;

    int index;

    int carry = 0;

    // assure memory for more segments
    bigint_assure_memory(p_bigint, p_bigint->data_len + max_segment_incr);
 
    if (value < 0) {
      bigint_change_sign(p_bigint);
      value = -value;
    }

    // from now on, only consider the case of value > 1

    for (index = 0; index < p_bigint->data_len || carry != 0; index++) {

      // set uninitialized parts to be 0
      if (index >= p_bigint->data_len) {
        p_bigint->p_data[index] = 0;
      }

      prod = (p_bigint->p_data[index] * (long long) value + carry);

      carry = (int) (prod / BIGINT_RADIX);

      p_bigint->p_data[index] = prod % BIGINT_RADIX;

    }

    p_bigint->data_len = index;

  }

}



void bigint_mul_by_pow_10(bigint* p_bigint, int pow) {

  // we dont consider the case of pow = 1, where nothing should be done
  if (pow < 0) {
    bigint_div_by_pow_10(p_bigint, -pow);
    
  } else if (pow > 0) {
    int approx_segments = pow / BIGINT_RADIX_LOG10 + 2 + p_bigint->data_len;
    int* p_new_data = BIGINT_ALLOC(sizeof(int) * (approx_segments));
    int prod;
    int index;

    // helper variable
    int helper_var;

    // number of 0 segments need to insert at the back of segments
    helper_var = pow / BIGINT_RADIX_LOG10;

    for (index = 0; index < helper_var; index++) {
      p_new_data[index] = 0;
    }

    // copy original numbers here
    for (index = 0; index < p_bigint->data_len; index++) {
      p_new_data[index + helper_var] = p_bigint->p_data[index];
    }

    p_bigint->data_len += helper_var;
    BIGINT_FREE(p_bigint->p_data);
    p_bigint->p_data = p_new_data;
    p_bigint->mem_size = approx_segments;


    // get the value to multiply as a param of bigint_mul_by_int
    helper_var = pow % BIGINT_RADIX_LOG10;
    prod = 1;
    while (helper_var > 0) {
      prod *= 10;
      helper_var--;
    }

    bigint_mul_by_int(p_bigint, prod);

  }
}



bigint_errno bigint_pow_by_int(bigint* p_bigint, int pow) {

  if (pow < 0) {
    return -BIGINT_ILLEGAL_PARAM;
  } else if (pow == 0) {
    bigint_set_one(p_bigint);
  } else if (pow > 1) {
    if (pow % 2 == 1) {
      bigint bi;
      bigint_init(&bi);
      bigint_copy(&bi, p_bigint);
      bigint_pow_by_int(p_bigint, pow / 2);
      bigint_mul_by(p_bigint, p_bigint);
      bigint_mul_by(p_bigint, &bi);
      bigint_release(&bi);
    } else {
      bigint_pow_by_int(p_bigint, pow / 2);
      bigint_mul_by(p_bigint, p_bigint);
    }
  }

  return BIGINT_NOERR;
}


bigint_errno bigint_div_by_int(bigint* p_bigint, int div) {

  if (div == 0) {
    return -BIGINT_ILLEGAL_PARAM;
  }

  if (div < 0) {
    div = -div;
    bigint_change_sign(p_bigint);
  }

  if (div != 1) {
    // from now on, only consider the case of value > 1

    // the value that holds a segment's value
    long long val = 0;
    long long r;

    int index;
    for (index = p_bigint->data_len - 1; index >= 0; index--) {
      val += (long long) p_bigint->p_data[index];
      r = val % ((long long) div);
      p_bigint->p_data[index] = (int) (val / (long) (long) div);
      val = r * BIGINT_RADIX;
    }
    
    bigint_pack_memory(p_bigint);
  }

  return BIGINT_NOERR;
}



void bigint_div_by_pow_10(bigint* p_bigint, int pow) {

  // we don't consider the case of pow = 1, where nothing should be done
  if (pow < 0) {
    bigint_mul_by_pow_10(p_bigint, -pow);

  } else if (pow > 0) {

    // the number of segments that should be directly thrown away
    int throw_segments = pow / BIGINT_RADIX_LOG10;

    // the additional number of digits that should be thrown away
    int throw_offset = pow % BIGINT_RADIX_LOG10;

    // approximate segments in result
    int approx_len = p_bigint->data_len - throw_segments + 1;

    if (p_bigint->data_len <= throw_segments) {
      // too much data is thrown away, the result is definitly zero

      assert(pow > bigint_digit_count(p_bigint));
      bigint_set_zero(p_bigint);

    } else {

      int index;

      // we might need to shift the number, insert new digits in the lower
      // part of the number. we will do this by multiplying 'prod'
      int prod = 1;

      // we might need to add a trailing number, by adding 'trailing'
      int trailing = 0;

      int* p_new_data = (int *) BIGINT_ALLOC(sizeof(int) * approx_len);

      if (throw_offset != 0) {

        // if the throw_offset is not zero, then we have to consider
        // the segment between 'thrown' and 'not thrown' segments.
        // we try to get the final trailing zeros, and use the following
        // formula:
        //   value = 'not thrown' * prod + trailing

        int i;

        trailing = p_bigint->p_data[throw_segments];
        prod = BIGINT_RADIX;

        for (i = 0; i < throw_offset; i++) {
          trailing /= 10;
          prod /= 10;
        }

        for (index = 0; index + throw_segments + 1 < p_bigint->data_len; index++) {
          p_new_data[index] = p_bigint->p_data[index + throw_segments + 1];
        }

        BIGINT_FREE(p_bigint->p_data);

        p_bigint->data_len -= throw_segments + 1;
        p_bigint->p_data = p_new_data;
        p_bigint->mem_size = approx_len;

        bigint_mul_by_int(p_bigint, prod);
        bigint_add_by_int(p_bigint, trailing);

      } else {
        // only need to discard a few segments

        BIGINT_FREE(p_bigint->p_data);

        p_bigint->data_len -= throw_segments;
        p_bigint->p_data = p_new_data;
        p_bigint->mem_size = approx_len;

      }
      
    }
  }
}



bigint_errno bigint_mod_int(bigint* p_bigint, int value, int* p_result) {

  long long r = 0;

  int i;

  if (value == 0) {
    return -BIGINT_ILLEGAL_PARAM;
  }

  for (i = p_bigint->data_len - 1; i >= 0; i--) {
    r = (r * BIGINT_RADIX + p_bigint->p_data[i]) % value;
  }

  if (p_bigint->sign < 0) {
    r = -r;
  }

  assert(-2147483648ll <= r && r < 2147483648ull);

  *p_result = (int) r;

  return BIGINT_NOERR;
}



bigint_errno bigint_mod_by_pow_10(bigint* p_bigint, int pow) {

  // the segments to keep
  int keep_segments;

  // the digits to keep with the segments to keep
  int keep_digits;

  if (pow < 0) {
    return -BIGINT_ILLEGAL_PARAM;
  }

  // from now on pow >= 0

  keep_segments = pow / BIGINT_RADIX_LOG10;
  keep_digits = pow % BIGINT_RADIX_LOG10;

  if (keep_segments < p_bigint->data_len) {

    // only consider the case that only some number should be kept
    int r = 1;
    int i;

    p_bigint->data_len = keep_segments + 1;

    for (i = 0; i < keep_digits; i++) {
      r *= 10;
    }

    p_bigint->p_data[p_bigint->data_len - 1] %= r;
    bigint_pack_memory(p_bigint);
  }

  return BIGINT_NOERR;
}



int bigint_compare(bigint* p_bigint1, bigint* p_bigint2) {
  if (p_bigint1->sign < p_bigint2->sign) {
    return -1;
  } else if (p_bigint1->sign > p_bigint2->sign) {
    return 1;
  } else {
    // same sign

    if (p_bigint1->sign == 0) {
      // both zero

      return 0;
    } else if (p_bigint1->sign < 0) {
      // both negative
      // use the positive solution

      int result;

      bigint_change_sign(p_bigint1);
      bigint_change_sign(p_bigint2);

      result = bigint_compare(p_bigint2, p_bigint1);

      bigint_change_sign(p_bigint2);
      bigint_change_sign(p_bigint1);

      return result;

    } else {
      // both positive

      if (p_bigint1->data_len > p_bigint2->data_len) {
        return 1;
      } else if (p_bigint1->data_len < p_bigint2->data_len) {
        return -1;
      } else {
        // both positive, same length
        int index = p_bigint1->data_len - 1;

        // compare segment to segment
        while (index >= 0) {
          if (p_bigint1->p_data[index] > p_bigint2->p_data[index]) {
            return 1;
          } else if (p_bigint1->p_data[index] < p_bigint2->p_data[index]) {
            return -1;
          }
          index--;
        }

        return 0;
      }
      
    }    
  }
}




int bigint_equal(bigint* p_bigint1, bigint* p_bigint2) {
  if (p_bigint1 == p_bigint2)
    return 1;
  else
    return bigint_compare(p_bigint1, p_bigint2) == 0;
}




int bigint_nth_digit(bigint* p_bigint, int nth) {

  if (nth < 0) {
    // handle error
    return -1;

  } else if (bigint_is_zero(p_bigint)) {
    // when the bigint is zero, every nth will return 0
    return 0;

  } else {
    // general case, with bigint > 0

    // which segment is the value located
    int segment = nth / BIGINT_RADIX_LOG10;
    
    // the offset in the segment
    int segment_offset = nth % BIGINT_RADIX_LOG10;

    // test if the segment is with in data range
    if (segment >= p_bigint->data_len) {
      // nth is too big for this number, always return 0
      return 0;

    } else {
      // the value of the segment
      int segment_value = p_bigint->p_data[segment];
      while (segment_offset > 0) {
        segment_value /= 10;
        segment_offset--;
      }
      return segment_value % 10;
    }
  }
}

