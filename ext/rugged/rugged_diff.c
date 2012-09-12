/*
 * The MIT License
 *
 * Copyright (c) 2012 GitHub, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "rugged.h"

extern VALUE rb_mRugged;
extern VALUE rb_cRuggedDiffFile;
VALUE rb_cRuggedDiff;
static ID id_write;

static void rb_git_diff__free(rugged_diff *diff)
{
  if (diff->iter)
    git_diff_iterator_free(diff->iter);

  git_diff_list_free(diff->diff);
  xfree(diff);
}

VALUE rugged_diff_new(VALUE klass, VALUE owner, rugged_diff *diff)
{
  VALUE rb_diff;

  rb_diff = Data_Wrap_Struct(klass, NULL, rb_git_diff__free, diff);
  rugged_set_owner(rb_diff, owner);
  diff->iter = NULL;
  return rb_diff;
}

static int diff_print_cb(void *data, git_diff_delta *delta, git_diff_range *range, char usage,
        const char *line, size_t line_len)
{
  VALUE *str = data;

  rb_str_cat(*str, line, line_len);

  return 0;
}

/*
 *  call-seq:
 *    diff.patch -> patch
 *
 *  Return a string containing the diff in patch form.
 *
 *
 */
static VALUE rb_git_diff_patch_GET(VALUE self)
{
  rugged_diff *diff;
  VALUE str;

  str = rugged_str_new(NULL, 0, NULL);
  Data_Get_Struct(self, rugged_diff, diff);

  git_diff_print_patch(diff->diff, &str, diff_print_cb);

  return str;
}

static int diff_write_cb(void *data, git_diff_delta *delta, git_diff_range *range, char usage,
        const char *line, size_t line_len)
{
  VALUE *io = data;
  VALUE str;

  str = rugged_str_new(line, line_len, NULL);
  rb_funcall(*io, id_write, 1, str);

  return 0;
}

/*
 *  call-seq:
 *    diff.write_patch(io)
 *
 *  Write a patch directly to an object which responds to "write".
 */
static VALUE rb_git_diff_write_patch(VALUE self, VALUE io)
{
  rugged_diff *diff;

  if (!rb_respond_to(io, id_write))
    rb_raise(rb_eArgError, "Expected io to respond to \"write\"");

  Data_Get_Struct(self, rugged_diff, diff);

  git_diff_print_patch(diff->diff, &io, diff_write_cb);

  return Qnil;
}

static VALUE rb_git_diff_merge(VALUE self, VALUE rb_other)
{
  rugged_diff *diff;
  rugged_diff *other;

  Data_Get_Struct(self, rugged_diff, diff);
  Data_Get_Struct(self, rugged_diff, other);

  git_diff_merge(diff->diff, other->diff);

  return self;
}

static VALUE rb_git_diff_each_delta(VALUE self)
{
  rugged_diff *diff;
  int err = 0;
  git_diff_delta *delta;

  Data_Get_Struct(self, rugged_diff, diff);

  if (diff->iter != NULL)
    git_diff_iterator_free(diff->iter);

  err = git_diff_iterator_new(&diff->iter, diff->diff);
  rugged_exception_check(err);

  while (err != GIT_ITEROVER) {
    err = git_diff_iterator_next_file(&delta, diff->iter);
    if (err == GIT_ITEROVER)
      break;
    else
      rugged_exception_check(err);

    rb_yield(rugged_diff_delta_new(self, delta));
  }

  return Qnil;
}

void Init_rugged_diff()
{
  id_write = rb_intern("write");

  rb_cRuggedDiff = rb_define_class_under(rb_mRugged, "Diff", rb_cObject);

  rb_define_method(rb_cRuggedDiff, "patch", rb_git_diff_patch_GET, 0);
  rb_define_method(rb_cRuggedDiff, "write_patch", rb_git_diff_write_patch, 1);
  rb_define_method(rb_cRuggedDiff, "merge!", rb_git_diff_merge, 1);
  rb_define_method(rb_cRuggedDiff, "each_delta", rb_git_diff_each_delta, 0);
}
