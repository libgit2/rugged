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
extern VALUE rb_cRuggedRawObject;

VALUE rb_cRuggedBackend;

int rugged_backend__exists(git_odb_backend *_backend, const git_oid *oid)
{
	ID method;
	VALUE exists;
	rugged_backend *back;
	char oid_out[40];

	back = (rugged_backend *)_backend;
	method = rb_intern("exists");

	if (!rb_respond_to(back->self, method))
		return 0; /* not found! */

	git_oid_fmt(oid_out, oid);
	exists = rb_funcall(back->self, method, 1, rb_str_new(oid_out, 40));

	if (TYPE(exists) == T_TRUE)
		return 1;

	if (TYPE(exists) == T_FALSE)
		return 0;

	rb_raise(rb_eRuntimeError, "'exists' interface must return true/false");
	return 0; /* never reached */
}

int rugged_backend__write(git_oid *id, git_odb_backend *_backend, git_rawobj *obj)
{
	ID method;
	rugged_backend *back;
	VALUE rb_oid;

	back = (rugged_backend *)_backend;
	method = rb_intern("write");

	if (!rb_respond_to(back->self, method))
		return GIT_ERROR;

	rb_oid = rb_funcall(back->self, method, 1, rugged_rawobject_new(obj));

	if (NIL_P(rb_oid))
		return GIT_ERROR;

	Check_Type(rb_oid, T_STRING);

	return git_oid_mkstr(id, RSTRING_PTR(rb_oid));
}

int rugged_backend__generic_read(int header_only, git_rawobj *obj, git_odb_backend *_backend, const git_oid *oid)
{
	ID method;
	VALUE read_obj;
	rugged_backend *back;
	char oid_out[40];

	back = (rugged_backend *)_backend;
	method = rb_intern(header_only ? "read_header" : "header");

	if (!rb_respond_to(back->self, method))
		return GIT_ENOTFOUND;

	git_oid_fmt(oid_out, oid);

	read_obj = rb_funcall(back->self, method, 1, rb_str_new(oid_out, 40));

	if (NIL_P(read_obj))
		return GIT_ENOTFOUND;

	if (!rb_obj_is_instance_of(read_obj, rb_cRuggedRawObject))
		rb_raise(rb_eTypeError, "'read' interface must return a Rugged::RawObject");

	rugged_rawobject_get(obj, read_obj);

	if (header_only && obj->data != NULL)
		git_rawobj_close(obj);

	return GIT_SUCCESS;
}

int rugged_backend__read(git_rawobj *obj, git_odb_backend *_backend, const git_oid *oid)
{
	return rugged_backend__generic_read(0, obj, _backend, oid);
}

int rugged_backend__read_header(git_rawobj *obj, git_odb_backend *_backend, const git_oid *oid)
{
	return rugged_backend__generic_read(1, obj, _backend, oid);
}

void rugged_backend__free(git_odb_backend *backend)
{
	/* do not free! the GC will take care of this */
}

void rugged_backend__gcfree(void *data)
{
	free(data);
}


static VALUE rb_git_backend_init(VALUE self, VALUE priority)
{
	rugged_backend *backend;

	Data_Get_Struct(self, rugged_backend, backend);
	Check_Type(priority, T_FIXNUM);

	backend->parent.priority = FIX2INT(priority);

	return Qnil;
}

static VALUE rb_git_backend_allocate(VALUE klass)
{
	rugged_backend *backend;

	backend = calloc(1, sizeof(rugged_backend));
	if (backend == NULL)
		rb_raise(rb_eNoMemError, "out of memory");

	backend->parent.read = &rugged_backend__read;
	backend->parent.read_header = &rugged_backend__read_header;
	backend->parent.write = &rugged_backend__write;
	backend->parent.exists = &rugged_backend__exists;
	backend->parent.free = &rugged_backend__free;
	backend->parent.priority = 0; /* set later */
	backend->self = Data_Wrap_Struct(klass, NULL, &rugged_backend__gcfree, backend);

	return backend->self;
}

void Init_rugged_backend()
{
	rb_cRuggedBackend = rb_define_class_under(rb_mRugged, "Backend", rb_cObject);
	rb_define_alloc_func(rb_cRuggedBackend, rb_git_backend_allocate);
	rb_define_method(rb_cRuggedBackend, "initialize", rb_git_backend_init, 1);
}
