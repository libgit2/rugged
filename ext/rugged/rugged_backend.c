/*
 * The MIT License
 *
 * Copyright (c) 2011 GitHub, Inc
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

	back = (rugged_backend *)_backend;
	method = rb_intern("exists");

	if (!rb_respond_to(back->self, method))
		return 0; /* not found! */

	exists = rb_funcall(back->self, method, 1, rugged_create_oid(oid));

	return rugged_parse_bool(exists);
}

int rugged_backend__write(git_oid *id, git_odb_backend *_backend, const void *data, size_t len, git_otype type)
{
	ID method;
	rugged_backend *back;
	VALUE rb_oid;

	back = (rugged_backend *)_backend;
	method = rb_intern("write");

	if (!rb_respond_to(back->self, method))
		return GIT_ERROR;

	rb_oid = rb_funcall(back->self, method, 2, rugged_str_ascii(data, len), INT2FIX((int)type));

	if (NIL_P(rb_oid))
		return GIT_ERROR;

	Check_Type(rb_oid, T_STRING);

	return git_oid_fromstr(id, StringValueCStr(rb_oid));
}

int rugged_backend__generic_read(int header_only, void **buffer_p, size_t *size_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid)
{
	ID method;
	VALUE read_hash;
	VALUE ruby_type, ruby_size, ruby_data;
	rugged_backend *back;

	back = (rugged_backend *)_backend;
	method = rb_intern(header_only ? "read_header" : "read");

	if (!rb_respond_to(back->self, method))
		return GIT_ENOTFOUND;

	read_hash = rb_funcall(back->self, method, 1, rugged_create_oid(oid));

	if (NIL_P(read_hash))
		return GIT_ENOTFOUND;

	Check_Type(read_hash, T_HASH);

	ruby_type = rb_hash_aref(read_hash, rb_intern("type"));
	ruby_size = rb_hash_aref(read_hash, rb_intern("len"));
	ruby_data = rb_hash_aref(read_hash, rb_intern("data"));

	if (NIL_P(ruby_type))
		rb_raise(rb_eTypeError, "Missing required value ':type'");

	*type_p = rugged_get_otype(ruby_type);

	if (NIL_P(ruby_size))
		rb_raise(rb_eTypeError, "Missing required value ':len'");

	Check_Type(ruby_size, T_FIXNUM);

	*size_p = FIX2INT(ruby_size);

	if (!header_only) {
		void *data;

		assert(buffer_p);

		if (NIL_P(ruby_data))
			rb_raise(rb_eTypeError, "Missing required value ':data'");

		Check_Type(ruby_data, T_STRING);

		data = malloc(RSTRING_LEN(ruby_data));
		memcpy(data, RSTRING_PTR(ruby_data), RSTRING_LEN(ruby_data));

		*buffer_p = data;
	}

	return GIT_SUCCESS;
}

int rugged_backend__read(void **buffer_p, size_t *size_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid)
{
	return rugged_backend__generic_read(0, buffer_p, size_p, type_p, _backend, oid);
}

int rugged_backend__read_header(size_t *size_p, git_otype *type_p, git_odb_backend *_backend, const git_oid *oid)
{
	return rugged_backend__generic_read(1, NULL, size_p, type_p, _backend, oid);
}

void rugged_backend__free(git_odb_backend *backend)
{
	/* do not free! the GC will take care of this */
}

void rugged_backend__gcfree(void *data)
{
	free(data);
}


static VALUE rb_git_backend_init(VALUE self)
{
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
	backend->parent.writestream = NULL;
	backend->parent.readstream = NULL;
	backend->parent.exists = &rugged_backend__exists;

	backend->parent.free = &rugged_backend__free;
	backend->self = Data_Wrap_Struct(klass, NULL, &rugged_backend__gcfree, backend);

	return backend->self;
}

void Init_rugged_backend()
{
	rb_cRuggedBackend = rb_define_class_under(rb_mRugged, "Backend", rb_cObject);
	rb_define_alloc_func(rb_cRuggedBackend, rb_git_backend_allocate);
	rb_define_method(rb_cRuggedBackend, "initialize", rb_git_backend_init, 0);
}
