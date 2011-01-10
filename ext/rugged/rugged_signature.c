/*
 * The MIT License
 *
 * Copyright (c) 2010 Scott Chacon
 * Copyright (c) 2010 Vicent Marti
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
VALUE rb_cRuggedSignature;

void rb_git_signature__free(git_signature *sig)
{
	git_signature_free(sig);
}

VALUE rugged_signature_new(const git_signature *sig)
{
	git_signature *sig_copy;

	if (sig == NULL)
		return Qnil;

	sig_copy = git_signature_dup(sig);
	if (sig_copy == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	return Data_Wrap_Struct(rb_cRuggedSignature, NULL, rb_git_signature__free, sig_copy);
}

git_signature *rugged_signature_get(VALUE rb_sig)
{
	git_signature *sig;

	if (!rb_obj_is_kind_of(rb_sig, rb_cRuggedSignature))
		rb_raise(rb_eTypeError, "expected Rugged::Signature object");

	Data_Get_Struct(rb_sig, git_signature, sig);
	return sig;
}

static VALUE rb_git_signature_allocate(VALUE klass)
{
	git_signature *sig = malloc(sizeof(git_signature));
	memset(sig, 0x0, sizeof(git_signature));
	return Data_Wrap_Struct(klass, NULL, rb_git_signature__free, sig);
}

static VALUE rb_git_signature_name_GET(VALUE self)
{
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);
	return rugged_str_new2(sig->name, NULL);
}

static VALUE rb_git_signature_email_GET(VALUE self)
{
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);
	return rugged_str_new2(sig->email, NULL);
}

static VALUE rb_git_signature_email_SET(VALUE self, VALUE rb_email)
{
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);

	Check_Type(rb_email, T_STRING);

	free(sig->email);
	sig->email = strdup(RSTRING_PTR(rb_email));

	return Qnil;
}

static VALUE rb_git_signature_name_SET(VALUE self, VALUE rb_name)
{
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);

	Check_Type(rb_name, T_STRING);

	free(sig->name);
	sig->name = strdup(RSTRING_PTR(rb_name));

	return Qnil;
}

static VALUE rb_git_signature_time_GET(VALUE self)
{
	VALUE rb_time;
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);

	rb_time = rb_time_new(sig->when.time, 0);
	rb_funcall(rb_time, rb_intern("utc"), 0);
	return rb_time;
}

static VALUE rb_git_signature_time_SET(VALUE self, VALUE rb_time)
{
	VALUE rb_unix_t, rb_offset;
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);

	if (!rb_obj_is_kind_of(rb_time, rb_cTime))
		rb_raise(rb_eTypeError, "expected Time object");

	rb_unix_t = rb_funcall(rb_time, rb_intern("tv_sec"), 0);
	rb_offset = rb_funcall(rb_time, rb_intern("utc_offset"), 0);

	sig->when.time = NUM2LONG(rb_unix_t);
	sig->when.offset = FIX2INT(rb_offset) / 60;

	return Qnil;
}

static VALUE rb_git_signature_time_offset_GET(VALUE self)
{
	git_signature *sig;
	Data_Get_Struct(self, git_signature, sig);

	return INT2FIX(sig->when.offset * 60);
}

static VALUE rb_git_signature_init(VALUE self, VALUE name, VALUE email, VALUE time)
{
	rb_git_signature_name_SET(self, name);
	rb_git_signature_email_SET(self, email);
	rb_git_signature_time_SET(self, time);

	return Qnil;
}


void Init_rugged_signature()
{
	/* signature */
	rb_cRuggedSignature = rb_define_class_under(rb_mRugged, "Signature", rb_cObject);
	rb_define_alloc_func(rb_cRuggedSignature, rb_git_signature_allocate);
	rb_define_method(rb_cRuggedSignature, "initialize", rb_git_signature_init, 3);

	rb_define_method(rb_cRuggedSignature, "name", rb_git_signature_name_GET, 0);
	rb_define_method(rb_cRuggedSignature, "name=", rb_git_signature_name_SET, 1);

	rb_define_method(rb_cRuggedSignature, "email", rb_git_signature_email_GET, 0);
	rb_define_method(rb_cRuggedSignature, "email=", rb_git_signature_email_SET, 1);

	rb_define_method(rb_cRuggedSignature, "time", rb_git_signature_time_GET, 0);
	rb_define_method(rb_cRuggedSignature, "time=", rb_git_signature_time_SET, 1);

	rb_define_method(rb_cRuggedSignature, "time_offset", rb_git_signature_time_offset_GET, 0);
}
