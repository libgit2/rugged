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
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;

VALUE rb_cRuggedBlob;

/*
 *	call-seq:
 *		blob.content -> cnt
 *
 *	Return the contents of a blob as bytes +String+.
 *	In Ruby 1.9.x, this string has ASCII encoding: the
 *	bytes are returned as-is.
 */
static VALUE rb_git_blob_content_GET(VALUE self)
{
	git_blob *blob;
	size_t size;

	Data_Get_Struct(self, git_blob, blob);
	
	size = git_blob_rawsize(blob);
	if (size == 0)
		return rugged_str_ascii("", 0);

	/*
	 * since we don't really ever know the encoding of a blob
	 * lets default to the binary encoding (ascii-8bit)
	 * If there is a way to tell, we should just pass 0/null here instead
	 *
	 * we're skipping the use of STR_NEW because we don't want our string to
	 * eventually end up converted to Encoding.default_internal because this
	 * string could very well be binary data
	 */
	return rugged_str_ascii(git_blob_rawcontent(blob), size);
}

/*
 *	call-seq:
 *		blob.rawsize -> int
 *
 *	Return the size in bytes of the blob. This is the real,
 *	uncompressed size and the length of +blob.content+, not
 *	the compressed size.
 */
static VALUE rb_git_blob_rawsize(VALUE self)
{
	git_blob *blob;
	Data_Get_Struct(self, git_blob, blob);

	return INT2FIX(git_blob_rawsize(blob));
}

/*
 *	call-seq:
 *		Blob.create(repository, bytes) -> oid
 *
 *	Write a blob to +repository+ with the contents specified
 *	in +buffer+. In Ruby 1.9.x, the encoding of +buffer+ is
 *	ignored and bytes are copied as-is.
 */
static VALUE rb_git_blob_create(VALUE self, VALUE rb_repo, VALUE rb_buffer)
{
	int error;
	git_oid oid;
	git_repository *repo;

	Check_Type(rb_buffer, T_STRING);
	if (!rb_obj_is_instance_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged Repository");

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_blob_create_frombuffer(&oid, repo, RSTRING_PTR(rb_buffer), RSTRING_LEN(rb_buffer));
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

/*
 *	call-seq:
 *		Blob.write_file(repository, file_path) -> oid
 *
 *	Write the file specified in +file_path+ to a blob in +repository+.
 *	+file_path+ must be relative to the repository's working folder.
 *
 *		Blob.write_file(repo, 'src/blob.h') #=> '9d09060c850defbc7711d08b57def0d14e742f4e'
 */
static VALUE rb_git_blob_writefile(VALUE self, VALUE rb_repo, VALUE rb_path)
{
	int error;
	git_oid oid;
	git_repository *repo;

	Check_Type(rb_path, T_STRING);
	if (!rb_obj_is_instance_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged Repository");

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_blob_create_fromfile(&oid, repo, StringValueCStr(rb_path));
	rugged_exception_check(error);

	return rugged_create_oid(&oid);
}

void Init_rugged_blob()
{
	rb_cRuggedBlob = rb_define_class_under(rb_mRugged, "Blob", rb_cRuggedObject);

	rb_define_method(rb_cRuggedBlob, "size", rb_git_blob_rawsize, 0);
	rb_define_method(rb_cRuggedBlob, "content", rb_git_blob_content_GET, 0);

	rb_define_singleton_method(rb_cRuggedBlob, "create", rb_git_blob_create, 2);
	rb_define_singleton_method(rb_cRuggedBlob, "write_file", rb_git_blob_writefile, 2);
}
