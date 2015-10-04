/*
 * The MIT License
 *
 * Copyright (c) 2015 GitHub, Inc
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

extern VALUE rb_cRuggedRefdbBackend;
VALUE rb_cRuggedRefdbBackendFileSystem;

void rb_git_refdb_backend__free(git_refdb_backend *backend)
{
	if (backend) backend->free(backend);
}

static VALUE rb_git_refdb_backend_fs_new(VALUE self, VALUE rb_repo)
{
	git_repository *repo;
	git_refdb_backend *backend;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(git_refdb_backend_fs(&backend, repo));

	return Data_Wrap_Struct(self, NULL, rb_git_refdb_backend__free, backend);
}

void Init_rugged_refdb_backend_fs(void)
{
	rb_cRuggedRefdbBackendFileSystem = rb_define_class_under(rb_cRuggedRefdbBackend, "FileSystem", rb_cRuggedRefdbBackend);

	rb_define_singleton_method(rb_cRuggedRefdbBackendFileSystem, "new", rb_git_refdb_backend_fs_new, 1);
}
