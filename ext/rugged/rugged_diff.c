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
VALUE rb_cRuggedDiff;

static void rb_git_diff__free(git_diff_list *diff)
{
  git_diff_list_free(diff);
}

VALUE rugged_diff_new(VALUE klass, VALUE owner, git_diff_list *diff)
{
  VALUE rb_diff = Data_Wrap_Struct(klass, NULL, rb_git_diff__free, diff);
  rugged_set_owner(rb_diff, owner);
  return rb_diff;
}

static int diff_print_cb(const  git_diff_delta *delta, const git_diff_range *range, char line_origin,
        const char *content, size_t content_len, void *payload)
{
  VALUE *str = payload;

  rb_str_cat(*str, content, content_len);

  return GIT_OK;
}

/*
 *  call-seq:
 *    diff.patch -> patch
 *    diff.patch(:compact => true) -> compact_patch
 *
 *  Return a string containing the diff in patch form.
 */
static VALUE rb_git_diff_patch(int argc, VALUE *argv, VALUE self)
{
  git_diff_list *diff;
  VALUE str;
  VALUE rb_opts;
  VALUE compact;

  rb_scan_args(argc, argv, "01", &rb_opts);

  str = rugged_str_new(NULL, 0, NULL);
  Data_Get_Struct(self, git_diff_list, diff);

  if (!NIL_P(rb_opts)) {
    Check_Type(rb_opts, T_HASH);
    if (rb_hash_aref(rb_opts, CSTR2SYM("compact")) == Qtrue)
      git_diff_print_compact(diff, diff_print_cb, &str);
    else
      git_diff_print_patch(diff, diff_print_cb, &str);
  } else {
    git_diff_print_patch(diff, diff_print_cb, &str);
  }

  return str;
}

static int diff_write_cb(const  git_diff_delta *delta, const git_diff_range *range, char line_origin,
        const char *content, size_t content_len, void *payload)
{
  VALUE *io = payload, str = rugged_str_new(content, content_len, NULL);

  rb_io_write(*io, str);

  return GIT_OK;
}

/*
 *  call-seq:
 *    diff.write_patch(io)
 *    diff.write_patch(io, :compact => true)
 *
 *  Write a patch directly to an object which responds to "write".
 */
static VALUE rb_git_diff_write_patch(int argc, VALUE *argv, VALUE self)
{
  git_diff_list *diff;
  VALUE rb_io;
  VALUE rb_opts;

  rb_scan_args(argc, argv, "11", &rb_io, &rb_opts);

  if (!rb_respond_to(rb_io, rb_intern("write")))
    rb_raise(rb_eArgError, "Expected io to respond to \"write\"");

  Data_Get_Struct(self, git_diff_list, diff);

  if (!NIL_P(rb_opts)) {
    Check_Type(rb_opts, T_HASH);
    if (rb_hash_aref(rb_opts, CSTR2SYM("compact")) == Qtrue)
      git_diff_print_compact(diff, diff_write_cb, &rb_io);
    else
      git_diff_print_patch(diff, diff_write_cb, &rb_io);
  } else {
    git_diff_print_patch(diff, diff_write_cb, &rb_io);
  }

  return Qnil;
}

static VALUE rb_git_diff_merge(VALUE self, VALUE rb_other)
{
  git_diff_list *diff;
  git_diff_list *other;

  Data_Get_Struct(self, git_diff_list, diff);
  Data_Get_Struct(self, git_diff_list, other);

  git_diff_merge(diff, other);

  return self;
}

static VALUE rb_git_diff_each_patch(VALUE self)
{
  git_diff_list *diff;
  git_diff_patch *patch;
  int error = 0, d, delta_count;

  if (!rb_block_given_p()) {
    return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each_patch"), self);
  }

  Data_Get_Struct(self, git_diff_list, diff);

  delta_count = git_diff_num_deltas(diff);
  for (d = 0; d < delta_count; ++d) {
    error = git_diff_get_patch(&patch, NULL, diff, d);
    if (error) break;

    rb_yield(rugged_diff_patch_new(self, patch));
  }

  rugged_exception_check(error);

  return self;
}

void Init_rugged_diff()
{
  rb_cRuggedDiff = rb_define_class_under(rb_mRugged, "Diff", rb_cObject);

  rb_define_method(rb_cRuggedDiff, "patch", rb_git_diff_patch, -1);
  rb_define_method(rb_cRuggedDiff, "write_patch", rb_git_diff_write_patch, -1);
  rb_define_method(rb_cRuggedDiff, "merge!", rb_git_diff_merge, 1);
  rb_define_method(rb_cRuggedDiff, "each_patch", rb_git_diff_each_patch, 0);
}
