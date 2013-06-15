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
extern VALUE rb_cRuggedDiffDelta;
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
	git_diff_patch *patch;
	const git_diff_range *range;
	const char *header;
	size_t header_len, lines_in_hunk;
	int error = 0, hooks_count, h;

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 1, CSTR2SYM("each_hunk"), self);
	}

	Data_Get_Struct(self, git_diff_patch, patch);

	hooks_count = git_diff_patch_num_hunks(patch);
	for (h = 0; h < hooks_count; ++h) {
		error = git_diff_patch_get_hunk(&range, &header, &header_len, &lines_in_hunk, patch, h);
		if (error) break;

		rb_yield(rugged_diff_hunk_new(self, h, range, header, header_len, lines_in_hunk));
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
	git_diff_patch *patch;
	Data_Get_Struct(self, git_diff_patch, patch);

	return INT2FIX(git_diff_patch_num_hunks(patch));
}

/*
 *  call-seq:
 *    patch.delta -> delta
 *
 *  Returns the delta object associated with the patch.
 */
static VALUE rb_git_diff_patch_delta(VALUE self)
{
	git_diff_patch *patch;
	Data_Get_Struct(self, git_diff_patch, patch);

	return rugged_diff_delta_new(rugged_owner(self), git_diff_patch_delta(patch));
}

/*
 *  call-seq:
 *    patch.additions -> int
 *
 *  Returns the number of addition lines in the patch.
 */
static VALUE rb_git_diff_patch_additions(VALUE self)
{
	git_diff_patch *patch;
	size_t additions;
	Data_Get_Struct(self, git_diff_patch, patch);

	git_diff_patch_line_stats(NULL, &additions, NULL, patch);

	return INT2FIX(additions);
}

/*
 *  call-seq:
 *    patch.deletions -> int
 *
 *  Returns the number of deletion lines in the patch.
 */
static VALUE rb_git_diff_patch_deletions(VALUE self)
{
	git_diff_patch *patch;
	size_t deletions;
	Data_Get_Struct(self, git_diff_patch, patch);

	git_diff_patch_line_stats(NULL, NULL, &deletions, patch);

	return INT2FIX(deletions);
}

/*
 *  call-seq:
 *    patch.context -> int
 *
 *  Returns the number of context lines in the patch.
 */
static VALUE rb_git_diff_patch_context(VALUE self)
{
	git_diff_patch *patch;
	size_t context;
	Data_Get_Struct(self, git_diff_patch, patch);

	git_diff_patch_line_stats(&context, NULL, NULL, patch);

	return INT2FIX(context);
}

void Init_rugged_diff_patch(void)
{
	rb_cRuggedDiffPatch = rb_define_class_under(rb_cRuggedDiff, "Patch", rb_cObject);

	rb_define_method(rb_cRuggedDiffPatch, "context", rb_git_diff_patch_context, 0);
	rb_define_method(rb_cRuggedDiffPatch, "additions", rb_git_diff_patch_additions, 0);
	rb_define_method(rb_cRuggedDiffPatch, "deletions", rb_git_diff_patch_deletions, 0);

	rb_define_method(rb_cRuggedDiffPatch, "delta", rb_git_diff_patch_delta, 0);

	rb_define_method(rb_cRuggedDiffPatch, "each_hunk", rb_git_diff_patch_each_hunk, 0);
	rb_define_method(rb_cRuggedDiffPatch, "hunk_count", rb_git_diff_patch_hunk_count, 0);
}
