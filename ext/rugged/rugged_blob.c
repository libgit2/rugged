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

extern VALUE rb_cRugged;
extern VALUE rb_cRuggedObject;
VALUE rb_cRuggedBlob;

static VALUE rb_git_blob_allocate(VALUE klass)
{
	git_blob *blob = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, blob);
}

static VALUE rb_git_blob_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_BLOB, argc, argv, self);
}

static VALUE rb_git_blob_content_SET(VALUE self, VALUE rb_contents)
{
	git_blob *blob;
	Data_Get_Struct(self, git_blob, blob);

	Check_Type(rb_contents, T_STRING);
	git_blob_set_rawcontent(blob, RSTRING_PTR(rb_contents), RSTRING_LEN(rb_contents));

	return Qnil;
}

static VALUE rb_git_blob_content_GET(VALUE self)
{
	git_blob *blob;
	int size;

	Data_Get_Struct(self, git_blob, blob);
	
	size = git_blob_rawsize(blob);
	if (size == 0)
		return rb_str_new2("");

	return rb_str_new(git_blob_rawcontent(blob), size);
}

static VALUE rb_git_blob_rawsize(VALUE self)
{
	git_blob *blob;
	Data_Get_Struct(self, git_blob, blob);

	return INT2FIX(git_blob_rawsize(blob));
}

void Init_rugged_blob()
{
	rb_cRuggedBlob = rb_define_class_under(rb_cRugged, "Blob", rb_cRuggedObject);
	rb_define_alloc_func(rb_cRuggedBlob, rb_git_blob_allocate);
	rb_define_method(rb_cRuggedBlob, "initialize", rb_git_blob_init, -1);

	rb_define_method(rb_cRuggedBlob, "size", rb_git_blob_rawsize, 0);

	rb_define_method(rb_cRuggedBlob, "content", rb_git_blob_content_GET, 0);
	rb_define_method(rb_cRuggedBlob, "content=", rb_git_blob_content_SET, 1);
}
