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
	VALUE rb_diff;

	rb_diff = Data_Wrap_Struct(klass, NULL, rb_git_diff__free, diff);
	rugged_set_owner(rb_diff, owner);
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
 *	call-seq:
 *		diff.patch -> patch
 *
 *	Return a string containing the diff in patch form.
 *
 *
 */
static VALUE rb_git_diff_patch_GET(VALUE self)
{
	git_diff_list *diff;
	VALUE str;

	str = rugged_str_new(NULL, 0, NULL);
	Data_Get_Struct(self, git_diff_list, diff);

	git_diff_print_patch(diff, &str, diff_print_cb);

	return str;
}

static VALUE rb_git_diff_file_fromC(const git_diff_file *file)
{
	VALUE rb_file;

	if (!file)
		return Qnil;

	rb_file = rb_hash_new();

	rb_hash_aset(rb_file, CSTR2SYM("oid"), rugged_create_oid(&file->oid));
	rb_hash_aset(rb_file, CSTR2SYM("path"), rugged_str_new2(file->path, NULL));
	rb_hash_aset(rb_file, CSTR2SYM("size"), INT2FIX(file->size));
	/* FIXME: flags and mode are missing */

	return rb_file;
}

static VALUE rb_git_diff_delta_fromC(const git_diff_delta *delta)
{
	VALUE rb_delta;

	if (!delta)
		return Qnil;

	rb_delta = rb_hash_new();
	rb_hash_aset(rb_delta, CSTR2SYM("old_file"), rb_git_diff_file_fromC(&delta->old_file));
	rb_hash_aset(rb_delta, CSTR2SYM("new_file"), rb_git_diff_file_fromC(&delta->new_file));
	rb_hash_aset(rb_delta, CSTR2SYM("similarity"), INT2FIX(delta->similarity));
	/* FIXME: how to represent delta->status? (git_delta_t) */
	rb_hash_aset(rb_delta, CSTR2SYM("binary"), delta->binary ? Qtrue : Qfalse);

	return rb_delta;
}

static int each_file_cb(void *data, git_diff_delta *delta, float progress)
{
	rb_yield(rb_git_diff_delta_fromC(delta));

	return 0;
}

static VALUE rb_git_diff_each_file(VALUE self)
{
	git_diff_list *diff;

	Data_Get_Struct(self, git_diff_list, diff);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	git_diff_foreach(diff, NULL, each_file_cb, NULL, NULL);

	return Qnil;
}

static VALUE rb_git_diff_range_fromC(const git_diff_range *range)
{
	VALUE rb_range;

	if (!range)
		return Qnil;

	rb_range = rb_hash_new();

	rb_hash_aset(rb_range, CSTR2SYM("old_start"), INT2FIX(range->old_start));
	rb_hash_aset(rb_range, CSTR2SYM("old_lines"), INT2FIX(range->old_lines));
	rb_hash_aset(rb_range, CSTR2SYM("new_start"), INT2FIX(range->new_start));
	rb_hash_aset(rb_range, CSTR2SYM("new_lines"), INT2FIX(range->new_lines));

	return rb_range;
}

static int each_hunk_cb(void *data, git_diff_delta *delta, git_diff_range *range,
			const char *header, size_t header_len)
{
	VALUE rb_hunk;

	rb_hunk = rb_hash_new();

	rb_hash_aset(rb_hunk, CSTR2SYM("delta"), rb_git_diff_delta_fromC(delta));
	rb_hash_aset(rb_hunk, CSTR2SYM("range"), rb_git_diff_range_fromC(range));
	rb_hash_aset(rb_hunk, CSTR2SYM("header"), rugged_str_new(header, header_len, NULL));

	rb_yield(rb_hunk);

	return 0;
}


static VALUE rb_git_diff_each_hunk(VALUE self)
{
	git_diff_list *diff;

	Data_Get_Struct(self, git_diff_list, diff);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 0);

	git_diff_foreach(diff, NULL, NULL, each_hunk_cb, NULL);

	return Qnil;
}

void Init_rugged_diff()
{
	rb_cRuggedDiff = rb_define_class_under(rb_mRugged, "Diff", rb_cObject);

	rb_define_method(rb_cRuggedDiff, "patch", rb_git_diff_patch_GET, 0);
	rb_define_method(rb_cRuggedDiff, "each_file", rb_git_diff_each_file, 0);
	rb_define_method(rb_cRuggedDiff, "each_hunk", rb_git_diff_each_hunk, 0);
}
