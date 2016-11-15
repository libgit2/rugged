/*
 * Copyright (C) the Rugged contributors.  All rights reserved.
 *
 * This file is part of Rugged, distributed under the MIT license.
 * For full terms see the included LICENSE file.
 */

#include "rugged.h"
extern VALUE rb_mRugged;

VALUE rb_cRuggedCancellation;

VALUE rugged_cancellation_new(VALUE klass, git_cancellation *cancellation)
{
	return Data_Wrap_Struct(klass, NULL, NULL, cancellation);
}

static VALUE rb_git_cancellation_new(VALUE klass)
{
	git_cancellation *c;

	rugged_exception_check(git_cancellation_new(&c));
	return rugged_cancellation_new(klass, c);
}

static VALUE rb_git_cancellation_request(VALUE self)
{
	git_cancellation *c;

	Data_Get_Struct(self, git_cancellation, c);
	rugged_exception_check(git_cancellation_request(c));

	return Qnil;
}

void Init_rugged_cancellation(void)
{
	rb_cRuggedCancellation = rb_define_class_under(rb_mRugged, "Cancellation", rb_cObject);

	rb_define_singleton_method(rb_cRuggedCancellation, "new", rb_git_cancellation_new, 0);
	rb_define_method(rb_cRuggedCancellation, "request", rb_git_cancellation_request, 0);
}
