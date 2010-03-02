#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xconf.h"

#include "xmemory.h"
#include "xutils.h"

/**
  @brief
    Hidden implementation of xconf.
*/
struct xconf_impl {
  xhash default_sec;  ///< @brief The default section.
  xhash sections; ///< @brief Hash of other sections.
};

static void free_default_sec(void* key, void* value) {
  xstr_delete((xstr) key);
  xstr_delete((xstr) value);
}

static void free_sections(void* key, void* value) {
  xstr_delete((xstr) key);
  xhash_delete((xhash) value);
}

xconf xconf_new() {
  xconf xcf = xmalloc_ty(1, struct xconf_impl);
  xcf->default_sec = xhash_new(xhash_hash_xstr, xhash_eql_xstr, free_default_sec);
  xcf->sections = xhash_new(xhash_hash_xstr, xhash_eql_xstr, free_sections);
  return xcf;
}

xsuccess xconf_load(xconf xcf, const char* fname) {
  xsuccess ret = XSUCCESS;
  FILE* fp = fopen(fname, "r");
  xstr line = xstr_new();
  if (fp != NULL) {
    xhash current_sec = xcf->default_sec;
    while (xgetline_fp(fp, line) == XSUCCESS) {
      xstr_strip(line, " \t");
      if (xstr_startwith_cstr(line, "#") || xstr_len(line) == 0) {
        // continue; ignore comments and empty lines
      } else if (xstr_startwith_cstr(line, "[")) {
        xstr_strip(line, "[]");
        xstr sec_name = xstr_copy(line);
        xhash sec_hash = xhash_new(xhash_hash_xstr, xhash_eql_xstr, free_default_sec);
        xhash_put(xcf->sections, sec_name, sec_hash);
        current_sec = sec_hash;
      } else {
        // item = value
        xstr item = xstr_new();
        xstr value = xstr_new();
        const char* line_cstr = xstr_get_cstr(line);
        int i;
        for (i = 0; line_cstr[i] != '\0' && line_cstr[i] != '='; i++) {
          xstr_append_char(item, line_cstr[i]);
        }
        if (line_cstr[i] == '=') {
          for (i++; line_cstr[i] != '\0'; i++) {
            xstr_append_char(value, line_cstr[i]);
          }
        }
        xstr_strip(item, " \t");
        xstr_strip(value, " \t");
        xhash_put(current_sec, item, value);
      }
    }
    fclose(fp);
  } else {
    ret = XFAILURE;
  }
  xstr_delete(line);
  return ret;
}

static xbool write_item_visitor(void* key, void* value, void* args) {
  FILE* fp = (FILE *) args;
  fprintf(fp, "%s=%s\n", xstr_get_cstr((xstr) key), xstr_get_cstr((xstr) value));
  return XTRUE;
}

static xbool write_section_visitor(void* key, void* value, void* args) {
  FILE* fp = (FILE *) args;
  fprintf(fp, "[%s]\n", xstr_get_cstr((xstr) key));
  xhash_visit((xhash) value, write_item_visitor, fp);
  fprintf(fp, "\n");
  return XTRUE;
}

xsuccess xconf_save(xconf xcf, const char* fname) {
  xsuccess ret = XSUCCESS;
  FILE* fp = fopen(fname, "w");
  if (fp != NULL) {
    char timebuf[32];
    struct tm* tm_struct;
    time_t tm_val = time(NULL);
    tm_struct = localtime(&tm_val);
    strftime(timebuf, sizeof(timebuf), "%Y.%m.%d %H:%M:%S", tm_struct);
    fprintf(fp, "# This file was automatically generated on %s\n", timebuf);
    fprintf(fp, "# Warning: every modification made to this file will be lost on next automatical generation!\n");

    xhash_visit(xcf->default_sec, write_item_visitor, fp);
    xhash_visit(xcf->sections, write_section_visitor, fp);

    fclose(fp);
  } else {
    ret = XFAILURE;
  }
  return ret;
}

xhash xconf_get_section(xconf xcf, const char* section_name) {
  xhash ret = NULL;
  if (section_name != NULL) {
    xstr query_xs = xstr_new();
    xstr_set_cstr(query_xs, section_name);
    ret = (xhash) xhash_get(xcf->sections, query_xs);
    xstr_delete(query_xs);
  } else {
    ret = xcf->default_sec;
  }
  return ret;
}

xstr xconf_get_value(xconf xcf, const char* section_name, const char* item_name) {
  xstr ret = NULL;
  xhash xh = xconf_get_section(xcf, section_name);
  if (xh != NULL) {
    xstr query_xs = xstr_new();
    xstr_set_cstr(query_xs, item_name);
    ret = xhash_get(xh, query_xs);
    xstr_delete(query_xs);
  }
  return ret;
}

xsuccess xconf_set_value(xconf xcf, const char* section_name, const char* item_name, const char* value) {
  xsuccess ret = XFAILURE;
  xhash xh = xconf_get_section(xcf, section_name);
  if (xh != NULL) {
    xstr query_xs = xstr_new();
    xstr_set_cstr(query_xs, item_name);
    if (value == NULL) {
      if (xhash_remove(xh, query_xs) > 0) {
        ret = XSUCCESS;
      }
    } else {
      xstr find_xs = (xstr) xhash_get(xh, query_xs);
      if (find_xs != NULL) {
        xstr_set_cstr(find_xs, value);
        ret = XSUCCESS;
      } else {
        xstr new_value = xstr_new();
        xstr new_item = xstr_copy(query_xs);
        xstr_set_cstr(new_value, value);
        xhash_put(xh, new_item, new_value);
      }
    }
    xstr_delete(query_xs);
  }
  return ret;
}

void xconf_delete(xconf xcf) {
  xhash_delete(xcf->default_sec);
  xhash_delete(xcf->sections);
  xfree(xcf);
}
