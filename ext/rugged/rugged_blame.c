/*
 * The MIT License
 *
 * Copyright (c) 2013 GitHub, Inc
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
VALUE rb_cRuggedBlame;

static VALUE rb_git_blame_hunk_fromC(const git_blame_hunk *hunk)
{
	VALUE rb_hunk;
	if (!hunk)  {
		return Qnil;
	}

	rb_hunk = rb_hash_new();
	rb_hash_aset(rb_hunk, CSTR2SYM("lines_in_hunk"), UINT2NUM(hunk->lines_in_hunk));

	rb_hash_aset(rb_hunk, CSTR2SYM("final_commit_id"), rugged_create_oid(&(hunk->final_commit_id)));
	rb_hash_aset(rb_hunk, CSTR2SYM("final_start_line_number"), UINT2NUM(hunk->final_start_line_number));
	rb_hash_aset(rb_hunk, CSTR2SYM("final_signature"), hunk->final_signature ? rugged_signature_new(hunk->final_signature, NULL) : Qnil);

	rb_hash_aset(rb_hunk, CSTR2SYM("orig_commit_id"), rugged_create_oid(&(hunk->orig_commit_id)));
	rb_hash_aset(rb_hunk, CSTR2SYM("orig_path"), hunk->orig_path ? rb_str_new2(hunk->orig_path) : Qnil);
	rb_hash_aset(rb_hunk, CSTR2SYM("orig_start_line_number"), UINT2NUM(hunk->orig_start_line_number));
	rb_hash_aset(rb_hunk, CSTR2SYM("orig_signature"), hunk->orig_signature ? rugged_signature_new(hunk->orig_signature, NULL) : Qnil);

	rb_hash_aset(rb_hunk, CSTR2SYM("boundary"), hunk->boundary ? Qtrue : Qfalse);

	return rb_hunk;
}

static void rugged_parse_blame_options(git_blame_options *opts, git_repository *repo, VALUE rb_options)
{
	if (!NIL_P(rb_options)) {
		VALUE rb_value;
		Check_Type(rb_options, T_HASH);

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("min_line"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts->min_line = FIX2UINT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("max_line"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts->min_line = FIX2UINT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("newest_commit"));
		if (!NIL_P(rb_value)) {
			int error = rugged_oid_get(&opts->newest_commit, repo, rb_value);
			rugged_exception_check(error);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("oldest_commit"));
		if (!NIL_P(rb_value)) {
			int error = rugged_oid_get(&opts->oldest_commit, repo, rb_value);
			rugged_exception_check(error);
		}
	}
}

/*
 *  call-seq:
 *    Blame.new(repo, path, options = {}) -> blame
 *
 *  Get blame data for the file at +path+ in +repo+.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :newest_commit ::
 *    The ID of the newest commit to consider in the blame. Defaults to +HEAD+.
 *
 *  :oldest_commit ::
 *    The id of the oldest commit to consider. Defaults to the first commit
 *    encountered with a NULL parent.
 *
 *  :min_line ::
 *    The first line in the file to blame. Line numbers start with 1.
 *    Defaults to +1+.
 *
 *  :max_line ::
 *    The last line in the file to blame. Defaults to the last line in
 *    the file.
 */
static VALUE rb_git_blame_new(int argc, VALUE *argv, VALUE klass)
{
	VALUE rb_repo, rb_path, rb_options;
	git_repository *repo;
	git_blame *blame;
	git_blame_options opts = GIT_BLAME_OPTIONS_INIT;

	rb_scan_args(argc, argv, "20:", &rb_repo, &rb_path, &rb_options);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_path, T_STRING);

	rugged_parse_blame_options(&opts, repo, rb_options);

	rugged_exception_check(git_blame_file(
		&blame, repo, StringValueCStr(rb_path), NULL
	));

	return Data_Wrap_Struct(klass, NULL, &git_blame_free, blame);
}

/*
 *  call-seq:
 *    blame.for_line(line_no) -> hunk
 *
 *  Returns the blame hunk data for the given +line_no+ in +blame+.
 *  Line number counting starts with +1+.
 */
static VALUE rb_git_blame_for_line(VALUE self, VALUE rb_line_no)
{
	git_blame *blame;

	Data_Get_Struct(self, git_blame, blame);

	return rb_git_blame_hunk_fromC(
		git_blame_get_hunk_byline(blame, FIX2UINT(rb_line_no))
	);
}

/*
 *  call-seq:
 *    blame[index] -> hunk
 *
 *  Returns the blame hunk data at the given +index+ in +blame+.
 */
static VALUE rb_git_blame_get_by_index(VALUE self, VALUE rb_index)
{
	git_blame *blame;

	Data_Get_Struct(self, git_blame, blame);

	return rb_git_blame_hunk_fromC(
		git_blame_get_hunk_byindex(blame, FIX2UINT(rb_index))
	);
}

/*
 *  call-seq:
 *    blame.count -> count
 *    blame.size -> count
 *
 *  Returns the total +count+ of blame hunks in +blame+.
 */
static VALUE rb_git_blame_count(VALUE self)
{
	git_blame *blame;
	Data_Get_Struct(self, git_blame, blame);
	return UINT2NUM(git_blame_get_hunk_count(blame));
}

/*
 *  call-seq:
 *    blame.each { |hunk| ... } -> blame
 *    blame.each -> enumerator
 *
 *  If given a block, yields each +hunk+ that is part of +blame+.
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_blame_each(VALUE self)
{
	git_blame *blame;
	uint32_t i, blame_count;

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each"), self);
	}

	Data_Get_Struct(self, git_blame, blame);

	blame_count = git_blame_get_hunk_count(blame);
	for (i = 0; i < blame_count; ++i) {
		rb_yield(rb_git_blame_hunk_fromC(
			git_blame_get_hunk_byindex(blame, i)
		));
	}

	return self;
}

void Init_rugged_blame(void)
{
	rb_cRuggedBlame = rb_define_class_under(rb_mRugged, "Blame", rb_cObject);
	rb_include_module(rb_cRuggedBlame, rb_mEnumerable);

	rb_define_singleton_method(rb_cRuggedBlame, "new", rb_git_blame_new, -1);

	rb_define_method(rb_cRuggedBlame, "[]", rb_git_blame_get_by_index, 1);
	rb_define_method(rb_cRuggedBlame, "for_line", rb_git_blame_for_line, 1);

	rb_define_method(rb_cRuggedBlame, "count", rb_git_blame_count, 0);
	rb_define_method(rb_cRuggedBlame, "size", rb_git_blame_count, 0);

	rb_define_method(rb_cRuggedBlame, "each", rb_git_blame_each, 0);
}
