#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xmemory.h"
#include "xutils.h"
#include "xcrypto.h"
#include "xsys.h"

// adapted from www.jb.man.ac.uk/~slowe/cpp/itoa.html
char* xitoa(int value, char* buf, int base) {
  char* ptr = buf;
  char* ptr1 = buf;
  char* digits = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
  char ch;
  int v;

  if (base < 2 || base > 36) {
    *buf = '\0';
    return buf;
  }

  do {
    v = value;
    value /= base;
    *ptr++ = digits[35 + (v - value * base)];
  } while (value);

  if (v < 0)
    *ptr++ = '-';
  
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    ch = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = ch;
  }

  return buf;
}

// adapted from www.jb.man.ac.uk/~slowe/cpp/itoa.html
char* xltoa(long long value, char* buf, int base) {
  char* ptr = buf;
  char* ptr1 = buf;
  char* digits = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
  char ch;
  long v;

  if (base < 2 || base > 36) {
    *buf = '\0';
    return buf;
  }

  do {
    v = value;
    value /= base;
    *ptr++ = digits[35 + (v - value * base)];
  } while (value);

  if (v < 0)
    *ptr++ = '-';
  
  *ptr-- = '\0';
  while (ptr1 < ptr) {
    ch = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = ch;
  }

  return buf;
}


xbool xcstr_startwith_cstr(const char* str, const char* head) {
  int i;
  for (i = 0; str[i] != '\0' && head[i] != '\0'; i++) {
    if (str[i] != head[i])
      return XFALSE;
  }
  return head[i] == '\0';
}

char* xcstr_strip(char* str) {
  int begin = 0;
  int end = strlen(str) - 1;
  int i;

  while (str[begin] == ' ' || str[begin] == '\t' || str[begin] == '\r' || str[begin] == '\n') {
    begin++;
  }

  while (end >= 0 && (str[end] == ' ' || str[end] == '\t' || str[end] == '\r' || str[end] == '\n')) {
    end--;
  }

  if (end <= begin) {
    // whole string stripped
    str[0] = '\0';
    return str;
  }

  for (i = 0; str[begin + i] != '\0' && begin + i <= end; i++) {
    str[i] = str[begin + i];
  }
  str[i] = '\0';
  
  return str;
}

xsuccess xinet_ip2str(int ip, char* str) {
  unsigned char* p = (unsigned char *) &ip;
  int i;
  char seg_str[4];
  str[0] = '\0';

  // TODO big endian? small endian?
  for (i = 3; i >= 0; i--) {
    int seg = p[i];
    xitoa(seg, seg_str, 10);
    strcat(str, seg_str);
    if (i != 0) {
      strcat(str, ".");
    }
  }
  return XSUCCESS;
}

xsuccess xinet_get_sockaddr(const char* host, int port, struct sockaddr_in* addr) {
  in_addr_t a;
  bzero(addr, sizeof(*addr));
  addr->sin_family = AF_INET;
  a = inet_addr(host);
  if (a != INADDR_NONE) {
    addr->sin_addr.s_addr = a;
  } else {
    struct hostent* hp = gethostbyname(host);
    if (hp == 0 || hp->h_length != 4) {
      return XFAILURE;
    }
  }
  addr->sin_port = htons(port);
  return XSUCCESS;
}

void xjoin_path_cstr(xstr fullpath, const char* current_dir, const char* append_dir) {
  const char sep = xsys_fs_sep_char; // filesystem path seperator
  const char* sep_str = xsys_fs_sep_cstr;
  if (append_dir[0] == sep) {
    xstr_set_cstr(fullpath, append_dir);
  } else {
    xstr_set_cstr(fullpath, "");
    xstr_printf(fullpath, "%s%c%s", current_dir, sep, append_dir);
  }
  if (xstr_startwith_cstr(fullpath, sep_str)) {
    xstr fullpath_copy = xstr_copy(fullpath);
    xfilesystem_normalize_abs_path(xstr_get_cstr(fullpath_copy), fullpath);
    xstr_delete(fullpath_copy);
  }
}

void xsleep_msec(int msec) {
  struct timespec req = {0};
  time_t sec = msec / 1000;
  long milli = msec - (sec * 1000);
  req.tv_sec = sec;
  req.tv_nsec = milli * 1000000L;
  nanosleep(&req, NULL);
}

xbool xfilesystem_exists(const char* path) {
  struct stat st;
  if (lstat(path, &st) != 0) {
    return XFALSE;
  } else {
    return XTRUE;
  }
}

xbool xfilesystem_is_file(const char* path, xsuccess* optional_succ) {
  struct stat st;
  if (lstat(path, &st) == 0) {
    if (optional_succ != NULL) {
      *optional_succ = XSUCCESS;
    }
    if (S_ISREG(st.st_mode)) {
      return XTRUE;
    } else {
      return XFALSE;
    }
  } else if (optional_succ != NULL){
    *optional_succ = XFAILURE;
  }
  return XFALSE;
}

xbool xfilesystem_is_dir(const char* path, xsuccess* optional_succ) {
  struct stat st;
  if (lstat(path, &st) == 0) {
    if (optional_succ != NULL) {
      *optional_succ = XSUCCESS;
    }
    if (S_ISDIR(st.st_mode)) {
      return XTRUE;
    } else {
      return XFALSE;
    }
  } else if (optional_succ != NULL){
    *optional_succ = XFAILURE;
  }
  return XFALSE;
}

xsuccess xfilesystem_rmrf(const char* path) {
  xsuccess ret = XSUCCESS;
  if (xfilesystem_is_dir(path, NULL) == XTRUE) {
    // TODO detect loops incurred by soft links (or just ignore folder link?)    
    xstr subpath = xstr_new();
    DIR* p_dir = opendir(path);

    if (p_dir == NULL) {
      ret = XFAILURE;
    } else {
      struct dirent* p_dirent;
      while ((p_dirent = readdir(p_dir)) != NULL) {
        if (strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0) {
          // skip these 2 cases, or the whole filesystem might get removed
          continue;
        }
        xjoin_path_cstr(subpath, path, p_dirent->d_name);
        if (xfilesystem_rmrf(xstr_get_cstr(subpath)) != XSUCCESS) {
          ret = XFAILURE;
          break;
        }
      }
      closedir(p_dir);
      if (ret != XFAILURE) {
        if (rmdir(path) != 0) {
          ret = XFAILURE;
        }
      }
    }
    xstr_delete(subpath);
  } else if (xfilesystem_is_file(path, NULL)) {
    if (unlink(path) != 0) {
      ret = XFAILURE;
    }
  } else {
    // possibly file not fould
    ret = XFAILURE;
  }
  return ret;
}


// requirement: norm_path is normalized
// result: norm_path is changed, if necessary
// when norm_path is '/', we cannot cdup
// the result will have an '/' at the end
xsuccess xfilesystem_path_cdup(xstr norm_path) {
  // check if not '/'
  if (strcmp(xstr_get_cstr(norm_path), xsys_fs_sep_cstr) != 0) {
    // ok, not '/', could cdup
    int new_len = xstr_len(norm_path);
    int index = new_len - 1;
    char sep = xsys_fs_sep_char; // filesystem separator
    char* new_cstr_path = xmalloc_ty(new_len + 1, char);
    strcpy(new_cstr_path, xstr_get_cstr(norm_path));
    
    // skip the trailing '/'
    if (new_cstr_path[index] == sep) {
      index--;
    }

    while (index > 0 && new_cstr_path[index] != sep) {
      index--;
    }
    new_cstr_path[index] = '\0';
    xstr_set_cstr(norm_path, new_cstr_path);
    xfree(new_cstr_path);
  }
  return XSUCCESS;
}

xsuccess xfilesystem_normalize_abs_path(const char* abs_path, xstr norm_path) {
  xsuccess ret = XSUCCESS;
  const char sep = xsys_fs_sep_char; // filesystem path seperator
  const char* sep_str = xsys_fs_sep_cstr;  // filesystem path seperator, in c-string
  xstr seg = xstr_new();  // a segment in the path
  int i;

  xstr_set_cstr(norm_path, sep_str);

  // check if input is absolute path
  if (abs_path[0] != sep) {
    ret = XFAILURE;
  } else {
    for (i = 0; abs_path[i] != '\0'; i++) {
      if (abs_path[i] == sep) {
        if (xstr_len(seg) != 0) {
          // got a new segment
          if (strcmp(xstr_get_cstr(seg), ".") == 0) {
            // do nothing
          } else if (strcmp(xstr_get_cstr(seg), "..") == 0) {
            // cd up
            xfilesystem_path_cdup(norm_path);
          } else {
            // append '/' & 'seg'
            if (xstr_last_char(norm_path) != sep) {
              xstr_append_char(norm_path, sep);
            }
            xstr_append_cstr(norm_path, xstr_get_cstr(seg));
          }
          xstr_set_cstr(seg, "");
        }
      } else {
        xstr_append_char(seg, abs_path[i]);
      }
    }
    if (xstr_len(seg) != 0) {
      // got a new segment
      if (strcmp(xstr_get_cstr(seg), ".") == 0) {
        // do nothing
      } else if (strcmp(xstr_get_cstr(seg), "..") == 0) {
        // cd up
        xfilesystem_path_cdup(norm_path);
      } else {
        // append '/' & 'seg'
        if (xstr_last_char(norm_path) != sep) {
          xstr_append_char(norm_path, sep);
        }
        xstr_append_cstr(norm_path, xstr_get_cstr(seg));
      }
      xstr_set_cstr(seg, "");
    }
  }

  xstr_delete(seg);
  return ret;
}

long xfilesystem_parse_filesize(const char* size_cstr) {
  long size = 0;
  char* p_end;
  double d_val = strtod(size_cstr, &p_end);
  int i;
  long unit = 1;
  if (p_end == size_cstr) {
    return -1;
  }
  for (i = 0; p_end[i] != '\0'; i++) {
    if (p_end[i] == ' ') {
      continue;
    } else if (p_end[i] == 'G' || p_end[i] == 'g') {
      unit = 1024L * 1024 * 1024;
    } else if (p_end[i] == 'M' || p_end[i] == 'm') {
      unit = 1024L * 1024;
    } else if (p_end[i] == 'K' || p_end[i] == 'k') {
      unit = 1024L;
    } else if (p_end[i] == 'B' || p_end[i] == 'b') {
      unit = 1;
    }
    break;
  }
  size = (long) (d_val * unit);
  return size;
}


int xhash_hash_cstr(void* key) {
  int hv = 0;
  unsigned char digest[16];
  int i;
  char* cstr = (char *) key;
  xmd5 xm = xmd5_new();
  xmd5_feed(xm, cstr, strlen(cstr));
  xmd5_result(xm, digest);
  for (i = 0; i < 16; i++) {
    hv ^= (((int) digest[i]) << ((4 * i) % 16));
  }
  xmd5_delete(xm);
  if (hv < 0) {
    hv = -hv;
  }
  return hv;
}

int xhash_hash_xstr(void* key) {
  xstr xs = (xstr) key;
  return xhash_hash_cstr((void *) xstr_get_cstr(xs));
}

xbool xhash_eql_xstr(void* key1, void* key2) {
  return xstr_eql((xstr) key1, (xstr) key2);
}

xsuccess xgetline_fp(FILE* fp, xstr line) {
  xsuccess ret = XSUCCESS;
  xstr_set_cstr(line, "");
  if (feof(fp)) {
    ret = XFAILURE;
  } else {
    int code;
    int counter = 0;
    for (;;) {
      counter++;
      code = fgetc(fp);
      if (code == '\r') {
        continue;
      } else if (code == '\n') {
        break;
      } else if (feof(fp)) {
        if (counter == 1) {
          ret = XFAILURE;
        }
        break;
      } else {
        char ch = (char) code;
        xstr_append_char(line, ch);
      }
    }
  }
  return ret;
}

