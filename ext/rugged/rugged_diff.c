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

/**
 * The caller has to free the returned git_diff_options pathspec strings array.
 */
void rugged_parse_diff_options(git_diff_options *opts, VALUE rb_options)
{
	if (!NIL_P(rb_options)) {
		VALUE rb_value;
		Check_Type(rb_options, T_HASH);

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("max_size"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts->max_size = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("context_lines"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts->context_lines = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("interhunk_lines"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts->interhunk_lines = FIX2INT(rb_value);
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("reverse")))) {
			opts->flags |= GIT_DIFF_REVERSE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("force_text")))) {
			opts->flags |= GIT_DIFF_FORCE_TEXT;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_whitespace")))) {
			opts->flags |= GIT_DIFF_IGNORE_WHITESPACE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_whitespace_change")))) {
			opts->flags |= GIT_DIFF_IGNORE_WHITESPACE_CHANGE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_whitespace_eol")))) {
			opts->flags |= GIT_DIFF_IGNORE_WHITESPACE_EOL;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_submodules")))) {
			opts->flags |= GIT_DIFF_IGNORE_SUBMODULES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("patience")))) {
			opts->flags |= GIT_DIFF_PATIENCE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_ignored")))) {
			opts->flags |= GIT_DIFF_INCLUDE_IGNORED;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_untracked")))) {
			opts->flags |= GIT_DIFF_INCLUDE_UNTRACKED;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_unmodified")))) {
			opts->flags |= GIT_DIFF_INCLUDE_UNMODIFIED;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("recurse_untracked_dirs")))) {
			opts->flags |= GIT_DIFF_RECURSE_UNTRACKED_DIRS;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("disable_pathspec_match")))) {
			opts->flags |= GIT_DIFF_DISABLE_PATHSPEC_MATCH;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_untracked_content")))) {
			opts->flags |= GIT_DIFF_INCLUDE_UNTRACKED_CONTENT ;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("skip_binary_check")))) {
			opts->flags |= GIT_DIFF_SKIP_BINARY_CHECK;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_typechange")))) {
			opts->flags |= GIT_DIFF_INCLUDE_TYPECHANGE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_typechange_trees")))) {
			opts->flags |= GIT_DIFF_INCLUDE_TYPECHANGE_TREES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_filemode")))) {
			opts->flags |= GIT_DIFF_IGNORE_FILEMODE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("recurse_ignored_dirs")))) {
			opts->flags |= GIT_DIFF_RECURSE_IGNORED_DIRS;
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("paths"));
		if (!NIL_P(rb_value)) {
			int i;
			Check_Type(rb_value, T_ARRAY);

			for (i = 0; i < RARRAY_LEN(rb_value); ++i)
				Check_Type(rb_ary_entry(rb_value, i), T_STRING);

			opts->pathspec.count = RARRAY_LEN(rb_value);
			opts->pathspec.strings = xmalloc(opts->pathspec.count * sizeof(char *));

			for (i = 0; i < RARRAY_LEN(rb_value); ++i) {
				VALUE rb_path = rb_ary_entry(rb_value, i);
				opts->pathspec.strings[i] = StringValueCStr(rb_path);
			}
		}
	}
}

static int diff_print_cb(
	const git_diff_delta *delta,
	const git_diff_range *range,
	char line_origin,
	const char *content,
	size_t content_len,
	void *payload)
{
	VALUE rb_str = (VALUE)payload;

	rb_str_cat(rb_str, content, content_len);

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
	VALUE rb_str = rb_str_new(NULL, 0);
	VALUE rb_opts;

	rb_scan_args(argc, argv, "00:", &rb_opts);

	Data_Get_Struct(self, git_diff_list, diff);

	if (!NIL_P(rb_opts)) {
		if (rb_hash_aref(rb_opts, CSTR2SYM("compact")) == Qtrue)
			git_diff_print_compact(diff, diff_print_cb, (void*)rb_str);
		else
			git_diff_print_patch(diff, diff_print_cb, (void*)rb_str);
	} else {
		git_diff_print_patch(diff, diff_print_cb, (void*)rb_str);
	}

	return rb_str;
}

static int diff_write_cb(
	const git_diff_delta *delta,
	const git_diff_range *range,
	char line_origin,
	const char *content,
	size_t content_len,
	void *payload)
{
	VALUE rb_io = (VALUE)payload, str = rb_str_new(content, content_len);

	rb_io_write(rb_io, str);

	return GIT_OK;
}

/*
 *  call-seq:
 *    diff.write_patch(io) -> nil
 *    diff.write_patch(io, :compact => true) -> nil
 *
 *  Write a patch directly to an object which responds to "write".
 */
static VALUE rb_git_diff_write_patch(int argc, VALUE *argv, VALUE self)
{
	git_diff_list *diff;
	VALUE rb_io, rb_opts;

	rb_scan_args(argc, argv, "10:", &rb_io, &rb_opts);

	if (!rb_respond_to(rb_io, rb_intern("write")))
		rb_raise(rb_eArgError, "Expected io to respond to \"write\"");

	Data_Get_Struct(self, git_diff_list, diff);

	if (!NIL_P(rb_opts)) {
		if (rb_hash_aref(rb_opts, CSTR2SYM("compact")) == Qtrue)
			git_diff_print_compact(diff, diff_write_cb, (void*)rb_io);
		else
			git_diff_print_patch(diff, diff_write_cb, (void*)rb_io);
	} else {
		git_diff_print_patch(diff, diff_write_cb, (void*)rb_io);
	}

	return Qnil;
}

/*
 *  call-seq:
 *    diff.merge!(other_diff) -> self
 *
 *  Merges all diff information from +other_diff+.
 */
static VALUE rb_git_diff_merge(VALUE self, VALUE rb_other)
{
	git_diff_list *diff;
	git_diff_list *other;
	int error;

	if (!rb_obj_is_kind_of(rb_other, rb_cRuggedDiff))
		rb_raise(rb_eTypeError, "A Rugged::Diff instance is required");

	Data_Get_Struct(self, git_diff_list, diff);
	Data_Get_Struct(rb_other, git_diff_list, other);

	error = git_diff_merge(diff, other);
	rugged_exception_check(error);

	return self;
}

/*
 *  call-seq:
 *    diff.find_similar!([options]) -> self
 *
 *  Detects entries in the diff that look like renames or copies (based on the
 *  given options) and replaces them with actual rename or copy entries.
 *
 *  Additionally, modified files can be broken into add/delete pairs if the
 *  amount of changes are above a specific threshold (see +:break_rewrite_threshold+).
 *
 *  By default, similarity will be measured without leading whitespace. You
 *  you can use the +:dont_ignore_whitespace+ to disable this.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :rename_threshold ::
 *    An integer specifying the similarity to consider a file renamed (default 50).
 *
 *  :rename_from_rewrite_threshold ::
 *    An integer specifying the similarity of modified to be eligible
 *    rename source (default 50).
 *
 *  :copy_threshold ::
 *    An integer specifying the similarity to consider a file a copy (default 50).
 *
 *  :break_rewrite_threshold ::
 *    An integer specifying the similarity to split modify into delete/add pair (default 60).
 *
 *  :rename_limit ::
 *    An integer specifying the maximum amount of similarity sources to examine
 *    (a la diff's +-l+ option or the +diff.renameLimit+ config) (default 200).
 *
 *  :renames ::
 *    If true, looking for renames will be enabled (+--find-renames+).
 *
 *  :renames_from_rewrites ::
 *    If true, the "old side" of modified files will be considered for renames (+--break-rewrites=N+).
 *
 *  :copies ::
 *    If true, looking for copies will be enabled (+--find-copies+).
 *
 *  :copies_from_unmodified ::
 *    If true, unmodified files will be considered as copy sources (+--find-copies-harder+).
 *
 *  :break_rewrites ::
 *    If true, larger rewrites will be split into delete/add pairs (+--break-rewrites=/M+).
 *
 *  :all ::
 *    If true, enables all finding features.
 *
 *  :ignore_whitespace ::
 *    If true, similarity will be measured with all whitespace ignored.
 *
 *  :dont_ignore_whitespace ::
 *    If true, similarity will be measured without ignoring any whitespace.
 *
 */
static VALUE rb_git_diff_find_similar(int argc, VALUE *argv, VALUE self)
{
	git_diff_list *diff;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	VALUE rb_options;
	int error;

	Data_Get_Struct(self, git_diff_list, diff);

	rb_scan_args(argc, argv, "00:", &rb_options);

	if (!NIL_P(rb_options)) {
		VALUE rb_value = rb_hash_aref(rb_options, CSTR2SYM("rename_threshold"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts.rename_threshold = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("rename_from_rewrite_threshold"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts.rename_from_rewrite_threshold = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("copy_threshold"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts.copy_threshold = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("break_rewrite_threshold"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts.break_rewrite_threshold = FIX2INT(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("rename_limit"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_FIXNUM);
			opts.rename_limit = FIX2INT(rb_value);
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("renames")))) {
			opts.flags |= GIT_DIFF_FIND_RENAMES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("renames_from_rewrites")))) {
			opts.flags |= GIT_DIFF_FIND_RENAMES_FROM_REWRITES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("copies")))) {
			opts.flags |= GIT_DIFF_FIND_COPIES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("copies_from_unmodified")))) {
			opts.flags |= GIT_DIFF_FIND_COPIES_FROM_UNMODIFIED;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("break_rewrites")))) {
			opts.flags |= GIT_DIFF_FIND_AND_BREAK_REWRITES;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("all")))) {
			opts.flags |= GIT_DIFF_FIND_ALL;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("ignore_whitespace")))) {
			opts.flags |= GIT_DIFF_FIND_IGNORE_WHITESPACE;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("dont_ignore_whitespace")))) {
			opts.flags |= GIT_DIFF_FIND_DONT_IGNORE_WHITESPACE;
		}
	}

	error = git_diff_find_similar(diff, &opts);
	rugged_exception_check(error);

	return self;
}

/*
 *  call-seq:
 *    diff.each_patch { |patch| } -> self
 *    diff.each_patch -> enumerator
 *
 *  If given a block, yields each patch that is part of the diff.
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_diff_each_patch(VALUE self)
{
	git_diff_list *diff;
	git_diff_patch *patch;
	int error = 0, d, delta_count;

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_patch"), self);
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

/*
 *  call-seq:
 *    diff.each_delta { |delta| } -> self
 *    diff.each_delta -> enumerator
 *
 *  If given a block, yields each delta that is part of the diff.
 *  If no block is given, an enumerator will be returned.
 *
 *  This method should be preferred over #each_patch if you're not interested
 *  in the actual line-by-line changes of the diff.
 */
static VALUE rb_git_diff_each_delta(VALUE self)
{
	git_diff_list *diff;
	const git_diff_delta *delta;
	int error = 0, d, delta_count;

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_delta"), self);
	}

	Data_Get_Struct(self, git_diff_list, diff);

	delta_count = git_diff_num_deltas(diff);
	for (d = 0; d < delta_count; ++d) {
		error = git_diff_get_patch(NULL, &delta, diff, d);
		if (error) break;

		rb_yield(rugged_diff_delta_new(self, delta));
	}

	rugged_exception_check(error);

	return self;
}

/*
 *  call-seq: diff.size -> int
 *
 *  Returns the number of deltas/patches in this diff.
 */
static VALUE rb_git_diff_size(VALUE self)
{
	git_diff_list *diff;

	Data_Get_Struct(self, git_diff_list, diff);

	return INT2FIX(git_diff_num_deltas(diff));
}

void Init_rugged_diff(void)
{
	rb_cRuggedDiff = rb_define_class_under(rb_mRugged, "Diff", rb_cObject);

	rb_define_method(rb_cRuggedDiff, "patch", rb_git_diff_patch, -1);
	rb_define_method(rb_cRuggedDiff, "write_patch", rb_git_diff_write_patch, -1);

	rb_define_method(rb_cRuggedDiff, "find_similar!", rb_git_diff_find_similar, -1);
	rb_define_method(rb_cRuggedDiff, "merge!", rb_git_diff_merge, 1);

	rb_define_method(rb_cRuggedDiff, "size", rb_git_diff_size, 0);

	rb_define_method(rb_cRuggedDiff, "each_patch", rb_git_diff_each_patch, 0);
	rb_define_method(rb_cRuggedDiff, "each_delta", rb_git_diff_each_delta, 0);
}
