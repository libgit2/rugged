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
#include <ctype.h>

extern VALUE rb_mRugged;
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;

VALUE rb_cRuggedBlob;

/*
 *	call-seq:
 *		blob.text(max_lines = -1, encoding = Encoding.default_external) -> String
 *
 *	Return up to +max_lines+ of text from a blob as a +String+.
 *
 *	In Ruby 1.9.x, the string is created with the given +encoding+,
 *	defaulting to Encoding.default_external.
 *
 *	In previous versions, the +encoding+ argument is dummy and has no
 *	effect on the returned string.
 *
 *	When limiting the size of the text with +max_lines+, the string is
 *	expected to have an ASCII-compatible encoding, and is checked
 *	for the newline +\n+ character.
 */
static VALUE rb_git_blob_text_GET(int argc, VALUE *argv, VALUE self)
{
#ifdef HAVE_RUBY_ENCODING_H
	rb_encoding *encoding = rb_default_external_encoding();
#endif

	git_blob *blob;
	size_t size;
	const char *content;
	VALUE rb_max_lines, rb_encoding;

	Data_Get_Struct(self, git_blob, blob);
	rb_scan_args(argc, argv, "02", &rb_max_lines, &rb_encoding);

	content = git_blob_rawcontent(blob);
	size = git_blob_rawsize(blob);

	if (!NIL_P(rb_max_lines)) {
		size_t i = 0;
		int lines = 0, maxlines;

		Check_Type(rb_max_lines, T_FIXNUM);
		maxlines = FIX2INT(rb_max_lines);

		if (maxlines >= 0) {
			while (i < size && lines < maxlines) {
				if (content[i++] == '\n')
					lines++;
			}
		}

		size = (size_t)i;
	}

#ifdef HAVE_RUBY_ENCODING_H
	if (!NIL_P(rb_encoding)) {
		encoding = rb_to_encoding(rb_encoding);
	}
#endif

	if (size == 0)
		return rugged_str_new("", 0, encoding);

	return rugged_str_new(content, size, encoding);
}

/*
 *	call-seq:
 *		blob.content(max_bytes=-1) -> String
 *
 *	Return up to +max_bytes+ from the contents of a blob as bytes +String+.
 *	If max_bytes is less than 0, the full string is returned.
 *
 *	In Ruby 1.9.x, this string is tagged with the ASCII-8BIT encoding: the
 *	bytes are returned as-is, since Git is encoding agnostic.
 */
static VALUE rb_git_blob_content_GET(int argc, VALUE *argv, VALUE self)
{
	git_blob *blob;
	size_t size;
	const char *content;
	VALUE rb_max_bytes;

	Data_Get_Struct(self, git_blob, blob);
	rb_scan_args(argc, argv, "01", &rb_max_bytes);

	content = git_blob_rawcontent(blob);
	size = git_blob_rawsize(blob);

	if (!NIL_P(rb_max_bytes)) {
		int maxbytes;

		Check_Type(rb_max_bytes, T_FIXNUM);
		maxbytes = FIX2INT(rb_max_bytes);

		if (maxbytes >= 0 && (size_t)maxbytes < size)
			size = (size_t)maxbytes;
	}

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
	return rugged_str_ascii(content, size);
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

/*
 *	call-seq:
 *		blob.sloc -> Integer
 *
 *	Return the number of non-empty code lines for the blob,
 *	assuming the blob is plaintext (i.e. not binary)
 */
static VALUE rb_git_blob_sloc(VALUE self)
{
	git_blob *blob;
	const char *data, *data_end;
	size_t sloc = 0;

	Data_Get_Struct(self, git_blob, blob);

	data = git_blob_rawcontent(blob);
	data_end = data + git_blob_rawsize(blob);

	if (data == data_end)
		return INT2FIX(0);

	/* go through the whole blob, counting lines
	 * that are not empty */
	while (data < data_end) {
		if (*data++ == '\n') {
			while (data < data_end && isspace(*data))
				data++;

			sloc++;
		}
	}

	/* last line without trailing '\n'? */
	if (data[-1] != '\n')
		sloc++;

	return INT2FIX(sloc);
}

void Init_rugged_blob()
{
	rb_cRuggedBlob = rb_define_class_under(rb_mRugged, "Blob", rb_cRuggedObject);

	rb_define_method(rb_cRuggedBlob, "size", rb_git_blob_rawsize, 0);
	rb_define_method(rb_cRuggedBlob, "content", rb_git_blob_content_GET, -1);
	rb_define_method(rb_cRuggedBlob, "text", rb_git_blob_text_GET, -1);
	rb_define_method(rb_cRuggedBlob, "sloc", rb_git_blob_sloc, 0);

	rb_define_singleton_method(rb_cRuggedBlob, "create", rb_git_blob_create, 2);
	rb_define_singleton_method(rb_cRuggedBlob, "write_file", rb_git_blob_writefile, 2);
}
