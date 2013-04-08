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

extern VALUE rb_cRuggedDiff;
VALUE rb_cRuggedDiffPatch;

static void rb_git_diff_patch__free(git_diff_patch *patch)
{
  git_diff_patch_free(patch);
}

VALUE rugged_diff_patch_new(VALUE owner, git_diff_patch *patch)
{
  VALUE rb_patch = Data_Wrap_Struct(rb_cRuggedDiffPatch, NULL, rb_git_diff_patch__free, patch);
  rugged_set_owner(rb_patch, owner);
  return rb_patch;
}


static VALUE rb_git_patch_file_fromC(const git_diff_file *file)
{
  VALUE rb_file;

  if (!file)
    return Qnil;

  rb_file = rb_hash_new();

  rb_hash_aset(rb_file, CSTR2SYM("oid"), rugged_create_oid(&file->oid));
  rb_hash_aset(rb_file, CSTR2SYM("path"), rugged_str_new2(file->path, NULL));
  rb_hash_aset(rb_file, CSTR2SYM("size"), INT2FIX(file->size));
  rb_hash_aset(rb_file, CSTR2SYM("flags"), UINT2NUM(file->flags));
  rb_hash_aset(rb_file, CSTR2SYM("mode"), UINT2NUM(file->mode));

  return rb_file;
}

static VALUE rb_git_diff_patch_old_file(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_delta *delta;

  Data_Get_Struct(self, git_diff_patch, patch);
  delta = git_diff_patch_delta(patch);

  return rb_git_patch_file_fromC(&delta->old_file);
}

static VALUE rb_git_diff_patch_new_file(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_delta *delta;

  Data_Get_Struct(self, git_diff_patch, patch);
  delta = git_diff_patch_delta(patch);

  return rb_git_patch_file_fromC(&delta->new_file);
}

static VALUE rb_git_diff_patch_similarity(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_delta *delta;

  Data_Get_Struct(self, git_diff_patch, patch);
  delta = git_diff_patch_delta(patch);

  return INT2FIX(delta->similarity);
}

static VALUE rb_git_diff_patch_status(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_delta *delta;
  VALUE rb_status;

  Data_Get_Struct(self, git_diff_patch, patch);
  delta = git_diff_patch_delta(patch);

  switch(delta->status) {
    case GIT_DELTA_UNMODIFIED:
      return CSTR2SYM("unmodified");
    case GIT_DELTA_ADDED:
      return CSTR2SYM("added");
    case GIT_DELTA_DELETED:
      return CSTR2SYM("deleted");
    case GIT_DELTA_MODIFIED:
      return CSTR2SYM("modified");
    case GIT_DELTA_RENAMED:
      return CSTR2SYM("renamed");
    case GIT_DELTA_COPIED:
      return CSTR2SYM("copied");
    case GIT_DELTA_IGNORED:
      return CSTR2SYM("ignored");
    case GIT_DELTA_UNTRACKED:
      return CSTR2SYM("untracked");
    case GIT_DELTA_TYPECHANGE:
      return CSTR2SYM("typechange");
    default:
      /* FIXME: raise here instead? */
      return CSTR2SYM("unknown");
  }
}

static VALUE rb_git_diff_patch_binary(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_delta *delta;

  Data_Get_Struct(self, git_diff_patch, patch);
  delta = git_diff_patch_delta(patch);

  return (delta->flags | GIT_DIFF_FLAG_NOT_BINARY) ? Qtrue : Qfalse;
}

static VALUE rb_git_diff_patch_each_hunk(VALUE self)
{
  git_diff_patch *patch;
  const git_diff_range *range;
  const char *header;
  size_t header_len, lines_in_hunk;
  int error = 0, hooks_count, h;

  if (!rb_block_given_p()) {
    return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each_hunk"), self);
  }

  Data_Get_Struct(self, git_diff_patch, patch);

  hooks_count = git_diff_patch_num_hunks(patch);
  for (h = 0; h < hooks_count; ++h) {
    error = git_diff_patch_get_hunk(&range, &header, &header_len, &lines_in_hunk, patch, h);
    if (error) break;

    rb_yield(rugged_diff_hunk_new(self, h, range, header, header_len, lines_in_hunk));
  }
  rugged_exception_check(error);

  return Qnil;
}

static VALUE rb_git_diff_patch_hunk_count(VALUE self)
{
  git_diff_patch *patch;
  Data_Get_Struct(self, git_diff_patch, patch);

  return INT2FIX(git_diff_patch_num_hunks(patch));
}

static VALUE rb_git_diff_patch_additions(VALUE self)
{
  git_diff_patch *patch;
  size_t additions;
  Data_Get_Struct(self, git_diff_patch, patch);

  git_diff_patch_line_stats(NULL, &additions, NULL, patch);

  return INT2FIX(git_diff_patch_num_hunks(patch));
}

static VALUE rb_git_diff_patch_deletions(VALUE self)
{
  git_diff_patch *patch;
  size_t deletions;
  Data_Get_Struct(self, git_diff_patch, patch);

  git_diff_patch_line_stats(NULL, NULL, &deletions, patch);

  return INT2FIX(git_diff_patch_num_hunks(patch));
}

void Init_rugged_diff_patch()
{
  rb_cRuggedDiffPatch = rb_define_class_under(rb_cRuggedDiff, "Patch", rb_cObject);

  rb_define_method(rb_cRuggedDiffPatch, "old_file", rb_git_diff_patch_old_file, 0);
  rb_define_method(rb_cRuggedDiffPatch, "new_file", rb_git_diff_patch_new_file, 0);
  rb_define_method(rb_cRuggedDiffPatch, "similarity", rb_git_diff_patch_similarity, 0);
  rb_define_method(rb_cRuggedDiffPatch, "status", rb_git_diff_patch_status, 0);
  rb_define_method(rb_cRuggedDiffPatch, "binary", rb_git_diff_patch_binary, 0);

  rb_define_method(rb_cRuggedDiffPatch, "additions", rb_git_diff_patch_additions, 0);
  rb_define_method(rb_cRuggedDiffPatch, "deletions", rb_git_diff_patch_deletions, 0);

  rb_define_method(rb_cRuggedDiffPatch, "each_hunk", rb_git_diff_patch_each_hunk, 0);
  rb_define_method(rb_cRuggedDiffPatch, "hunk_count", rb_git_diff_patch_hunk_count, 0);
}
