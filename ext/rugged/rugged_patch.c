/*
 * The MIT License
 *
 * Copyright (c) 2014 GitHub, Inc
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
extern VALUE rb_cRuggedDiffDelta;
VALUE rb_cRuggedPatch;

/*
 *  call-seq:
 *    Patch.from_strings(old_content = nil, new_content = nil, options = {}) -> patch
 *
 *  Directly generate a Rugged::Patch from the difference between the content of
 *  the two strings `old_content` and `new_content`.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :old_path ::
 *    An optional string to treat +blob+ as if it had this filename.
 *
 *  :new_path ::
 *    An optional string to treat +other+ as if it had this filename.
 *
 *  Additionally, `options` can also contain all other valid diff options
 *  (see Rugged::Tree#diff for a complete list).
 */
VALUE rb_git_patch_from_strings(int argc, VALUE *argv, VALUE self)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_patch *patch;
	char * old_path = NULL, * new_path = NULL;
	VALUE rb_old_buffer, rb_new_buffer, rb_options;

	rb_scan_args(argc, argv, "02:", &rb_old_buffer, &rb_new_buffer, &rb_options);

	if (!NIL_P(rb_options)) {
		VALUE rb_value;

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("old_path"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_STRING);
			old_path = StringValueCStr(rb_value);
		}

		rb_value = rb_hash_aref(rb_options, CSTR2SYM("new_path"));
		if (!NIL_P(rb_value)) {
			Check_Type(rb_value, T_STRING);
			new_path = StringValueCStr(rb_value);
		}

		rugged_parse_diff_options(&opts, rb_options);
	}

	rugged_exception_check(git_patch_from_buffers(&patch,
		NIL_P(rb_old_buffer) ? NULL : StringValuePtr(rb_old_buffer),
		NIL_P(rb_old_buffer) ? 0 : RSTRING_LEN(rb_old_buffer),
		old_path,
		NIL_P(rb_new_buffer) ? NULL : StringValuePtr(rb_new_buffer),
		NIL_P(rb_new_buffer) ? 0 : RSTRING_LEN(rb_new_buffer),
		new_path,
		&opts
	));

	return rugged_patch_new(self, patch);
}

VALUE rugged_patch_new(VALUE owner, git_patch *patch)
{
	VALUE rb_patch = Data_Wrap_Struct(rb_cRuggedPatch, NULL, &git_patch_free, patch);
	rugged_set_owner(rb_patch, owner);
	return rb_patch;
}

/*
 *  call-seq:
 *    patch.each_hunk { |hunk| } -> self
 *    patch.each_hunk -> enumerator
 *
 *  If given a block, yields each hunk that is part of the patch.
 *
 *  If no block is given, an enumerator is returned instead.
 */
static VALUE rb_git_diff_patch_each_hunk(VALUE self)
{
	git_patch *patch;
	const git_diff_hunk *hunk;
	size_t lines_in_hunk;
	int error = 0;
	size_t hunks_count, h;

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_hunk"), self);
	}

	Data_Get_Struct(self, git_patch, patch);

	hunks_count = git_patch_num_hunks(patch);
	for (h = 0; h < hunks_count; ++h) {
		error = git_patch_get_hunk(&hunk, &lines_in_hunk, patch, h);
		if (error) break;

		rb_yield(rugged_diff_hunk_new(self, h, hunk, lines_in_hunk));
	}
	rugged_exception_check(error);

	return self;
}

/*
 *  call-seq:
 *    patch.hunk_count -> int
 *
 *  Returns the number of hunks in the patch.
 */
static VALUE rb_git_diff_patch_hunk_count(VALUE self)
{
	git_patch *patch;
	Data_Get_Struct(self, git_patch, patch);

	return INT2FIX(git_patch_num_hunks(patch));
}

/*
 *  call-seq:
 *    patch.delta -> delta
 *
 *  Returns the delta object associated with the patch.
 */
static VALUE rb_git_diff_patch_delta(VALUE self)
{
	git_patch *patch;
	Data_Get_Struct(self, git_patch, patch);

	return rugged_diff_delta_new(rugged_owner(self), git_patch_get_delta(patch));
}

/*
 *  call-seq:
 *    patch.stat -> int, int
 *
 *  Returns the number of additions and deletions in the patch.
 */
static VALUE rb_git_diff_patch_stat(VALUE self)
{
	git_patch *patch;
	size_t additions, deletions;
	Data_Get_Struct(self, git_patch, patch);

	git_patch_line_stats(NULL, &additions, &deletions, patch);

	return rb_ary_new3(2, INT2FIX(additions), INT2FIX(deletions));
}

/*
 *  call-seq:
 *    patch.lines(options = {}) -> int
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :exclude_context ::
 *    Boolean value specifying that context line counts should be excluded from
 *    the returned total.
 *
 *  :exclude_additions ::
 *    Boolean value specifying that addition line counts should be excluded from
 *    the returned total.
 *
 *  :exclude_deletions ::
 *    Boolean value specifying that deletion line counts should be excluded from
 *    the returned total.
 *
 *  Returns the total number of lines in the patch, depending on the options
 *  specified.
 */
static VALUE rb_git_diff_patch_lines(int argc, VALUE *argv, VALUE self)
{
	git_patch *patch;
	size_t context_lines, additions, deletions;
	size_t total_out;
	VALUE rb_options;
	Data_Get_Struct(self, git_patch, patch);

	context_lines = 0;
	additions = 0;
	deletions = 0;

	git_patch_line_stats(&context_lines, &additions, &deletions, patch);

	total_out = context_lines + additions + deletions;

	rb_scan_args(argc, argv, "0:", &rb_options);
	if (!NIL_P(rb_options)) {
		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("exclude_context")))) {
			total_out -= context_lines;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("exclude_additions")))) {
			total_out -= additions;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("exclude_deletions")))) {
			total_out -= deletions;
		}
	}

	return INT2FIX(total_out);
}
/*
 *  call-seq:
 *    patch.bytesize(options = {}) -> int
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :include_context ::
 *    Boolean value specifying that context lines should be included when
 *    counting the number of bytes in the patch.
 *
 *  :include_hunk_headers ::
 *    Boolean value specifying that hunk headers should be included when
 *    counting the number of bytes in the patch.
 *
 *  :include_file_headers ::
 *    Boolean value specifying that file headers should be included when
 *    counting the number of bytes in the patch.
 *
 *  Returns the number of bytes in the patch, depending on which options are
 *  specified.
 */
static VALUE rb_git_diff_patch_bytesize(int argc, VALUE *argv, VALUE self)
{
	git_patch *patch;
	size_t bytesize;
	VALUE rb_options;
	int options[3];
	Data_Get_Struct(self, git_patch, patch);

	memset(options, 0, sizeof(options));

	rb_scan_args(argc, argv, "0:", &rb_options);
	if (!NIL_P(rb_options)) {
		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_context")))) {
			options[0] = 1;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_hunk_headers")))) {
			options[1] = 1;
		}

		if (RTEST(rb_hash_aref(rb_options, CSTR2SYM("include_file_headers")))) {
			options[2] = 1;
		}
	}

	bytesize = git_patch_size(patch, options[0], options[1], options[2]);

	return INT2FIX(bytesize);
}

static int patch_print_cb(
	const git_diff_delta *delta,
	const git_diff_hunk *hunk,
	const git_diff_line *line,
	void *payload)
{
	VALUE rb_str = (VALUE)payload;

	switch (line->origin) {
		case GIT_DIFF_LINE_CONTEXT:
		case GIT_DIFF_LINE_ADDITION:
		case GIT_DIFF_LINE_DELETION:
			rb_str_cat(rb_str, &line->origin, 1);
	}

	rb_str_cat(rb_str, line->content, line->content_len);

	return GIT_OK;
}

/*
 *  call-seq:
 *    patch.to_s -> str
 *
 *  Returns the contents of the patch as a single diff string.
 */
static VALUE rb_git_diff_patch_to_s(VALUE self)
{
	git_patch *patch;
	VALUE rb_str = rb_str_new(NULL, 0);
	Data_Get_Struct(self, git_patch, patch);

	rugged_exception_check(git_patch_print(patch, patch_print_cb, (void*)rb_str));

	return rb_str;
}

void Init_rugged_patch(void)
{
	rb_cRuggedPatch = rb_define_class_under(rb_mRugged, "Patch", rb_cObject);

	rb_define_singleton_method(rb_cRuggedPatch, "from_strings", rb_git_patch_from_strings, -1);

	rb_define_method(rb_cRuggedPatch, "stat", rb_git_diff_patch_stat, 0);
	rb_define_method(rb_cRuggedPatch, "lines", rb_git_diff_patch_lines, -1);
	rb_define_method(rb_cRuggedPatch, "bytesize", rb_git_diff_patch_bytesize, -1);

	rb_define_method(rb_cRuggedPatch, "delta", rb_git_diff_patch_delta, 0);

	rb_define_method(rb_cRuggedPatch, "to_s", rb_git_diff_patch_to_s, 0);

	rb_define_method(rb_cRuggedPatch, "each_hunk", rb_git_diff_patch_each_hunk, 0);
	rb_define_method(rb_cRuggedPatch, "hunk_count", rb_git_diff_patch_hunk_count, 0);
}
