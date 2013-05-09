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
VALUE rb_cRuggedDiffDelta;

VALUE rugged_diff_delta_new(VALUE owner, const git_diff_delta *delta)
{
	// TODO: Is it okay to cast away the const-ness of the delta here?
	VALUE rb_delta = Data_Wrap_Struct(rb_cRuggedDiffDelta, NULL, NULL, (git_diff_delta *)delta);
	rugged_set_owner(rb_delta, owner);
	return rb_delta;
}

static VALUE rb_git_delta_file_fromC(const git_diff_file *file)
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

/*
 *  call-seq: delta.old_file -> Hash
 *
 *  Returns a Hash describing the left side of a diff delta.
 */
static VALUE rb_git_diff_delta_old_file(VALUE self)
{
	const git_diff_delta *delta;

	Data_Get_Struct(self, git_diff_delta, delta);

	return rb_git_delta_file_fromC(&delta->old_file);
}

/*
 *  call-seq: delta.new_file -> Hash
 *
 *  Returns a Hash describing the right side of a diff delta.
 */
static VALUE rb_git_diff_delta_new_file(VALUE self)
{
	const git_diff_delta *delta;

	Data_Get_Struct(self, git_diff_delta, delta);

	return rb_git_delta_file_fromC(&delta->new_file);
}

/*
 *  call-seq: delta.similarity -> Fixnum
 *
 *  A value between 0-100, indicating the similarity between +old_file+ and +new_file+.
 *  Only valid/set for deltas with a status of +:renamed+ or +:copied:.
 */
static VALUE rb_git_diff_delta_similarity(VALUE self)
{
	const git_diff_delta *delta;

	Data_Get_Struct(self, git_diff_delta, delta);

	return INT2FIX(delta->similarity);
}

/*
 *  call-seq: delta.status -> Symbol
 *
 *  Returns the delta status, indicating the type of change.
 */
static VALUE rb_git_diff_delta_status(VALUE self)
{
	const git_diff_delta *delta;

	Data_Get_Struct(self, git_diff_delta, delta);

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

/*
 *  call-seq: delta.binary -> true or false
 *
 *  Returns true if this delta was recognized as binary.
 */
static VALUE rb_git_diff_delta_binary(VALUE self)
{
	const git_diff_delta *delta;

	Data_Get_Struct(self, git_diff_delta, delta);

	return (!(delta->flags & GIT_DIFF_FLAG_NOT_BINARY) && (delta->flags & GIT_DIFF_FLAG_BINARY)) ? Qtrue : Qfalse;
}

void Init_rugged_diff_delta()
{
	rb_cRuggedDiffDelta = rb_define_class_under(rb_cRuggedDiff, "Delta", rb_cObject);

	rb_define_method(rb_cRuggedDiffDelta, "old_file", rb_git_diff_delta_old_file, 0);
	rb_define_method(rb_cRuggedDiffDelta, "new_file", rb_git_diff_delta_new_file, 0);
	rb_define_method(rb_cRuggedDiffDelta, "similarity", rb_git_diff_delta_similarity, 0);
	rb_define_method(rb_cRuggedDiffDelta, "status", rb_git_diff_delta_status, 0);
	rb_define_method(rb_cRuggedDiffDelta, "binary", rb_git_diff_delta_binary, 0);
}
