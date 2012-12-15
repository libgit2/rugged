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

extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedObject;

static VALUE rugged_git_note_message(const git_note *note)
{
	const char *message;
	message = git_note_message(note);

	/*
	 * the note object is just a blob, normally it should be human readable
	 * and in unicode like annotated tag's message,
	 * since git allows attaching any blob as a note message
	 * we're just making sure this works for everyone and it doesn't
	 * reencode things it shouldn't.
	 *
	 * since we don't really ever know the encoding of a blob
	 * lets default to the binary encoding (ascii-8bit)
	 * If there is a way to tell, we should just pass 0/null here instead
	 *
	 * we're skipping the use of STR_NEW because we don't want our string to
	 * eventually end up converted to Encoding.default_internal because this
	 * string could very well be binary data
	 */
	return rugged_str_ascii(message, strlen(message));
}

static VALUE rugged_git_note_oid(const git_note* note)
{
	const git_oid *oid;
	oid = git_note_oid(note);

	return rugged_create_oid(oid);
}

/*
 *	call-seq:
 *		obj.notes(notes_ref = 'refs/notes/commits') -> hash
 *
 *	Lookup a note for +obj+ from +notes_ref+:
 *	- +notes_ref+: (optional): cannonical name of the reference to use, defaults to "refs/notes/commits"
 *
 *	Returns a new Hash object.
 *
 *		obj.notes #=> {:message=>"note text\n", :oid=>"94eca2de348d5f672faf56b0decafa5937e3235e"}
 */
static VALUE rb_git_note_lookup(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	const char *notes_ref = NULL;
	VALUE rb_notes_ref;
	VALUE rb_note_hash;
	VALUE owner;
	git_note *note;
	git_object *object;
	int error;

	rb_scan_args(argc, argv, "01", &rb_notes_ref);

	if (!NIL_P(rb_notes_ref)) {
		Check_Type(rb_notes_ref, T_STRING);
		notes_ref = StringValueCStr(rb_notes_ref);
	}

	Data_Get_Struct(self, git_object, object);

	owner = rugged_owner(self);
	Data_Get_Struct(owner, git_repository, repo);

	error = git_note_read(&note, repo, notes_ref, git_object_id(object));

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	rb_note_hash = rb_hash_new();
	rb_hash_aset(rb_note_hash, CSTR2SYM("message"), rugged_git_note_message(note));
	rb_hash_aset(rb_note_hash, CSTR2SYM("oid"), rugged_git_note_oid(note));

	git_note_free(note);

	return rb_note_hash;
}

static int cb_note__each(const git_oid *blob_id, const git_oid *annotated_object_id, void *payload)
{
	VALUE rb_repo = (VALUE)payload;

	git_object *annotated_object;
	git_object *note_blob;

	git_repository *repo;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(
		git_object_lookup(&annotated_object, repo, annotated_object_id, GIT_OBJ_ANY)
	);

	rugged_exception_check(
		git_object_lookup(&note_blob, repo, blob_id, GIT_OBJ_BLOB)
	);

	rb_yield_values(2,
			rugged_object_new(rb_repo, note_blob),
			rugged_object_new(rb_repo, annotated_object)
	);
	return GIT_OK;
}

/*
 *	call-seq:
 *		repo.each_note(notes_ref = "refs/notes/commits") { |note_blob, annotated_object| block }
 *		repo.each_note(notes_ref = "refs/notes/commits") -> an_enumerator
 *
 *	Call the given block once for each note_blob/annotated_object pair in +repository+
 *	- +notes_ref+: (optional): cannonical name of the reference to use defaults to "refs/notes/commits"
 *
 *	If no block is given, an +Enumerator+ is returned.
 *
 *		@repo.each_note do |note_blob, annotated_object|
 *			puts "#{note_blob.oid} => #{annotated_object.oid}"
 *		end
 */
static VALUE rb_git_note_each(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	const char *notes_ref = NULL;
	int error;

	VALUE rb_notes_ref;

	rb_scan_args(argc, argv, "01", &rb_notes_ref);

	if (!rb_block_given_p()) {
		return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each_note"), self, rb_notes_ref);
	}

	if (!NIL_P(rb_notes_ref)) {
		Check_Type(rb_notes_ref, T_STRING);
		notes_ref = StringValueCStr(rb_notes_ref);
	}

	Data_Get_Struct(self, git_repository, repo);

	error = git_note_foreach(repo, notes_ref, &cb_note__each, (void *)self);
	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_notes()
{
	rb_define_method(rb_cRuggedObject, "notes", rb_git_note_lookup, -1);
	rb_define_method(rb_cRuggedRepo, "each_note", rb_git_note_each, -1);
}
