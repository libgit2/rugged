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
VALUE rb_cRuggedDiffLine;

VALUE rugged_diff_line_new(
	VALUE owner,
	const char line_origin,
	const char *content,
	size_t content_len,
	int old_lineno,
	int new_lineno)
{
	VALUE rb_line;
	VALUE rb_line_origin;

	rb_line = rb_class_new_instance(0, NULL, rb_cRuggedDiffLine);
	rugged_set_owner(rb_line, owner);

	switch(line_origin) {
		case GIT_DIFF_LINE_CONTEXT:
			rb_line_origin = CSTR2SYM("context");
			break;
		case GIT_DIFF_LINE_ADDITION:
			rb_line_origin = CSTR2SYM("addition");
			break;
		case GIT_DIFF_LINE_DELETION:
			rb_line_origin = CSTR2SYM("deletion");
			break;
		case GIT_DIFF_LINE_CONTEXT_EOFNL: /* neither file has newline at the end */
			rb_line_origin = CSTR2SYM("eof_no_newline");
			break;
		case GIT_DIFF_LINE_ADD_EOFNL: /* added at end of old file */
			rb_line_origin = CSTR2SYM("eof_newline_added");
			break;
		case GIT_DIFF_LINE_DEL_EOFNL: /* removed at end of old file */
			rb_line_origin = CSTR2SYM("eof_newline_removed");
			break;
		default:
			/* FIXME: raise here instead? */
			rb_line_origin = CSTR2SYM("unknown");
	}

	rb_iv_set(rb_line, "@line_origin", rb_line_origin);
	rb_iv_set(rb_line, "@content", rb_str_new(content, content_len));
	rb_iv_set(rb_line, "@old_lineno", INT2FIX(old_lineno));
	rb_iv_set(rb_line, "@new_lineno", INT2FIX(new_lineno));

	return rb_line;
}

void Init_rugged_diff_line(void)
{
	rb_cRuggedDiffLine = rb_define_class_under(rb_cRuggedDiff, "Line", rb_cObject);
}
