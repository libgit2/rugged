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
extern VALUE rb_cRuggedDiff;
VALUE rb_cRuggedDiffHunk;

VALUE rugged_diff_hunk_new(VALUE owner, const char *header, size_t header_len, git_diff_range *range)
{
  VALUE rb_hunk;
  VALUE rb_range;

  rb_hunk = rb_class_new_instance(0, NULL, rb_cRuggedDiffHunk);
  rugged_set_owner(rb_hunk, owner);

  rb_range = rb_hash_new();
  rb_hash_aset(rb_range, CSTR2SYM("old_start"), INT2FIX(range->old_start));
  rb_hash_aset(rb_range, CSTR2SYM("old_lines"), INT2FIX(range->old_lines));
  rb_hash_aset(rb_range, CSTR2SYM("new_start"), INT2FIX(range->new_start));
  rb_hash_aset(rb_range, CSTR2SYM("new_lines"), INT2FIX(range->new_lines));
  rb_iv_set(rb_hunk, "@range", rb_range);

  rb_iv_set(rb_hunk, "@header", rugged_str_new(header, header_len, NULL));

  return rb_hunk;
}

static VALUE rb_git_diff_hunk_each_line(VALUE self)
{
  VALUE rb_diff;
  VALUE rb_hunk;
  VALUE rb_line;
  rugged_diff *diff;
  char line_origin;
  const char *content;
  size_t content_len = 0;
  int err = 0;

  rb_hunk = rugged_owner(self);
  rb_diff = rugged_owner(rb_hunk);
  Data_Get_Struct(rb_diff, rugged_diff, diff);

  while (err != GIT_ITEROVER) {
    err = git_diff_iterator_next_line(&line_origin, &content, &content_len, diff->iter);
    if (err == GIT_ITEROVER)
      break;
    else
      rugged_exception_check(err);

    rb_line = rugged_diff_line_new(self, line_origin, content, content_len);
    rb_yield(rb_line);
  }

  return Qnil;
}

void Init_rugged_diff_hunk()
{
  rb_cRuggedDiffHunk = rb_define_class_under(rb_cRuggedDiff, "Hunk", rb_cObject);

  rb_define_method(rb_cRuggedDiffHunk, "each_line", rb_git_diff_hunk_each_line, 0);
}
