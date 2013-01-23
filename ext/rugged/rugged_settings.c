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

#if !defined(NUM2SIZET)
#  if SIZEOF_SIZE_T == SIZEOF_LONG
#    define NUM2SIZET(n) ((size_t)NUM2ULONG(n))
#  else
#    define NUM2SIZET(n) ((size_t)NUM2ULL(n))
#  endif
#endif /* ! defined(NUM2SIZET) */


extern VALUE rb_mRugged;
VALUE rb_cRuggedSettings;

/*
 *	call-seq:
 *		Rugged.Settings[option] = value
 *
 *	Sets a libgit2 library option
 *
 */
static VALUE rb_git_set_option(VALUE self, VALUE option, VALUE value)
{
	char *opt;

	Check_Type(option, T_STRING);
	opt = RSTRING_PTR(option);

	if (strcmp(opt, "mwindow_size") == 0) {
		size_t val;
		Check_Type(value, T_FIXNUM);
		val = NUM2SIZET(value);

		git_libgit2_opts(GIT_OPT_MWINDOW_SIZE, val);
	} else if (strcmp(opt, "mwindow_mapped_limit") == 0) {
		size_t val;
		Check_Type(value, T_FIXNUM);
		val = NUM2SIZET(value);

		git_libgit2_opts(GIT_OPT_MWINDOW_MAPPED_LIMIT, val);
	} else {
		rb_raise(rb_eArgError, "Unknown option specified");
	}

	return Qnil;
}


void Init_rugged_settings()
{
	rb_cRuggedSettings = rb_define_class_under(rb_mRugged, "Settings", rb_cObject);
	rb_define_module_function(rb_cRuggedSettings, "[]=", rb_git_set_option, 2);
}
