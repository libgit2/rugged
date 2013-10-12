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

void rugged_parse_blame_options(git_blame_options *opts, VALUE rb_options)
{
	if (!NIL_P(rb_options)) {
		Check_Type(rb_options, T_HASH);
	}
}

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

	rugged_parse_blame_options(&opts, rb_options);

	rugged_exception_check(git_blame_file(
		&blame, repo, StringValueCStr(rb_path), &opts
	));

	return Data_Wrap_Struct(klass, NULL, &git_blame_free, blame);
}

static VALUE rb_git_blame_count(VALUE self)
{
	git_blame *blame;
	Data_Get_Struct(self, git_blame, blame);
	return UINT2NUM(git_blame_get_hunk_count(blame));
}

void Init_rugged_blame(void)
{
	rb_cRuggedBlame = rb_define_class_under(rb_mRugged, "Blame", rb_cObject);
	rb_define_singleton_method(rb_cRuggedBlame, "new", rb_git_blame_new, -1);

	rb_define_method(rb_cRuggedBlame, "count", rb_git_blame_count, 0);
	rb_define_method(rb_cRuggedBlame, "size", rb_git_blame_count, 0);
}
