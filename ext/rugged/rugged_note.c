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
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedReference;
VALUE rb_cRuggedNote;

void rb_git_note__free(git_note *note)
{
	git_note_free(note);
}

VALUE rugged_note_new(VALUE klass, VALUE owner, git_note *note)
{
	VALUE rb_note = Data_Wrap_Struct(klass, NULL, &rb_git_note__free, note);
	rugged_set_owner(rb_note, owner);
	return rb_note;
}

/*
 *	call-seq:
 *		Note.lookup(repository, oid, notes_ref = 'refs/notes/commits') -> new_note
 *
 *	Lookup a note for +oid+ from +notes_ref+ in +repository+:
 *	- +oid+: the OID of the git object to read the note from
 *	- +notes_ref+: (optional): cannonical name of the reference to use, defaults to "refs/notes/commits"
 *
 *	Returns a new Rugged::Note object.
 */
static VALUE rb_git_note_lookup(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	git_oid  oid;
	const char *notes_ref = NULL;
	git_note *note;
	int error;

	VALUE rb_repo, rb_oid, rb_notes_ref;

	rb_scan_args(argc, argv, "21", &rb_repo, &rb_oid, &rb_notes_ref);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_oid, T_STRING);
	rugged_exception_check(
		git_oid_fromstr(&oid, StringValueCStr(rb_oid))
	);

	if (!NIL_P(rb_notes_ref)){
		Check_Type(rb_notes_ref, T_STRING);
		notes_ref = StringValueCStr(rb_notes_ref);
	}

	error = git_note_read(&note, repo, notes_ref, &oid);

	rugged_exception_check(error);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	return rugged_note_new(klass, rb_repo, note);
}

/*
 *	call-seq:
 *		note.message -> msg
 *
 *	Return the message of this +note+. This includes the full body of the
 *	message.
 *
 *		note.message #=> "Build status: success"
 */
static VALUE rb_git_note_message_GET(VALUE self)
{
	git_note *note;
	const char *message;

	Data_Get_Struct(self, git_note, note);
	message = git_note_message(note);

	/*
	 * the note object is just a blob, normally it should be human readable
	 * and in unicode like annotated tag's message,
	 * but since git allows attaching any blob as a note message
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

/*
 *	call-seq:
 *		Note.default_ref(repository) -> reference
 *
 *	Get the default notes reference for a +repository+:
 *
 *	Returns a new Rugged::Reference object.
 *
 *		Rugged::Note.default_ref(repo).name #=> "refs/notes/commits"
 */
static VALUE rb_git_note_default_ref_GET(VALUE klass, VALUE rb_repo )
{
	git_repository *repo = NULL;
	git_reference *notes_ref;
	const char * ref_name;
	int error;

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(
		git_note_default_ref(&ref_name, repo)
	);

	error = git_reference_lookup(&notes_ref, repo, ref_name);
	if (error == GIT_ENOTFOUND)
		return Qnil;
	else
		rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rb_repo, notes_ref);
}

/*
 *	call-seq:
 *		note.oid -> oid
 *
 *	Return the oid of the note object, as a <tt>String</tt>
 *	instance.
 *
 *		note.oid #=> "94eca2de348d5f672faf56b0decafa5937e3235e"
 */
static VALUE rb_git_note_oid_GET(VALUE self)
{
	git_note *note;
	const git_oid *oid;

	Data_Get_Struct(self, git_note, note);

	oid = git_note_oid(note);

	return rugged_create_oid(oid);
}

/*
 *	call-seq:
 *		Note.create(repository, data = {}) -> oid
 *
 *	Write a new +Note+ to +repository+, with the given +data+
 *	arguments, passed as a +Hash+:
 *
 *	- +:message+: the content of the note to add to the object
 *	- +:oid+: the OID if the git object to decorate
 *	- +:committer+: a hash with the signature for the committer
 *	- +:author+: a hash with the signature for the author
 *	- +:ref+: (optional): cannonical name of the reference to use, defaults to "refs/notes/commits"
 *
 *	When the note is successfully written to disk, its +oid+ will be
 *	returned as a hex +String+.
 *
 *		author = {:email=>"tanoku@gmail.com", :time=>Time.now, :name=>"Vicent Mart\303\255"}
 *
 *		Rugged::Note.create(repo,
 *			:oid       => 'd13da328e2807e098295bb9bead4d23560c27320',
 *			:author    => author,
 *			:committer => author,
 *			:message   => "Hello world\n\n",
 *			:ref       => 'refs/notes/builds'
 *			)
 */
static VALUE rb_git_note_create(VALUE klass, VALUE rb_repo, VALUE rb_data)
{
	VALUE rb_ref, rb_oid, rb_message;
	git_repository *repo = NULL;
	const char *notes_ref = NULL;

	git_signature *author, *committer;
	git_oid note_oid;

	git_object *target = NULL;
	git_oid oid;
	int error = 0;

	Check_Type(rb_data, T_HASH);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	rb_ref = rb_hash_aref(rb_data, CSTR2SYM("ref"));

	if (!NIL_P(rb_ref)) {
		Check_Type(rb_ref, T_STRING);
		notes_ref = StringValueCStr(rb_ref);
	}

	rb_message = rb_hash_aref(rb_data, CSTR2SYM("message"));
	Check_Type(rb_message, T_STRING);

	committer = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("committer"))
	);

	author = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("author"))
	);

	rb_oid = rb_hash_aref(rb_data, CSTR2SYM("oid"));

	error = git_oid_fromstr(&oid, StringValueCStr(rb_oid));
	if (error < GIT_OK)
		goto cleanup;

	error = git_object_lookup(&target, repo, &oid, GIT_OBJ_ANY);
	if (error < GIT_OK)
		goto cleanup;

	error = git_note_create(
			&note_oid,
			repo,
			author,
			committer,
			notes_ref,
			&oid,
			StringValueCStr(rb_message));


cleanup:
	git_signature_free(author);
	git_signature_free(committer);

	rugged_exception_check(error);

	return rugged_create_oid(&note_oid);
}

/*
 *	call-seq:
 *		Note.remove!(repository, data = {}) -> boolean
 *
 *	Removes a +Note+ from +repository+, with the given +data+
 *	arguments, passed as a +Hash+:
 *
 *	- +:oid+: the OID if the git object to remove the note from
 *	- +:committer+: a hash with the signature for the committer
 *	- +:author+: a hash with the signature for the author
 *	- +:ref+: (optional): cannonical name of the reference to use, defaults to "refs/notes/commits"
 *
 *	When the note is successfully removed +true+ will be
 *	returned as a +Boolean+.
 *
 *		author = {:email=>"tanoku@gmail.com", :time=>Time.now, :name=>"Vicent Mart\303\255"}
 *
 *		Rugged::Note.remove!(repo,
 *		        :oid       => '8496071c1b46c854b31185ea97743be6a8774479',
 *			:author    => author,
 *			:committer => author,
 *			:ref       => 'refs/notes/builds'
 *			)
 */
static VALUE rb_git_note_remove(VALUE klass, VALUE rb_repo, VALUE rb_data)
{
	VALUE rb_ref, rb_oid, rb_message;
	git_repository *repo = NULL;
	const char *notes_ref = NULL;

	git_signature *author, *committer;

	git_object *target = NULL;
	git_oid oid;
	int error = 0;

	Check_Type(rb_data, T_HASH);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	rb_ref = rb_hash_aref(rb_data, CSTR2SYM("ref"));

	if (!NIL_P(rb_ref)) {
		Check_Type(rb_ref, T_STRING);
		notes_ref = StringValueCStr(rb_ref);
	}

	committer = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("committer"))
	);

	author = rugged_signature_get(
		rb_hash_aref(rb_data, CSTR2SYM("author"))
	);

	rb_oid = rb_hash_aref(rb_data, CSTR2SYM("oid"));

	error = git_oid_fromstr(&oid, StringValueCStr(rb_oid));
	if (error < GIT_OK)
		goto cleanup;

	error = git_object_lookup(&target, repo, &oid, GIT_OBJ_ANY);
	if (error < GIT_OK)
		goto cleanup;

	error = git_note_remove(
			repo,
			notes_ref,
			author,
			committer,
			&oid);


cleanup:
	git_signature_free(author);
	git_signature_free(committer);

	rugged_exception_check(error);

	return Qtrue;
}

static int cb_note__each(git_note_data *note_data, void *payload)
{
	VALUE rb_repo = (VALUE)payload;

	git_object *annotated_object;
	git_object *note_blob;

	git_repository *repo;

	Data_Get_Struct(rb_repo, git_repository, repo);

	rugged_exception_check(
		git_object_lookup(&annotated_object, repo, &note_data->annotated_object_oid, GIT_OBJ_ANY)
	);

	rugged_exception_check(
		git_object_lookup(&note_blob, repo, &note_data->blob_oid, GIT_OBJ_BLOB)
	);

	rb_yield_values(2,
			rugged_object_new(rb_repo, note_blob),
			rugged_object_new(rb_repo, annotated_object)
	);
	return GIT_OK;
}

/*
 *	call-seq:
 *		Note.each(repository, notes_ref = "refs/notes/commits") { |note_blob, annotated_object| block }
 *		Note.each(repository, notes_ref = "refs/notes/commits") -> an_enumerator
 *
 *	Call the given block once for each note_blob/annotated_object pair in +repository+
 *	- +notes_ref+: (optional): cannonical name of the reference to use defaults to "refs/notes/commits"
 *
 *	If no block is given, an +Enumerator+ is returned.
 *              Rugged::Note.each(@repo) do |note_blob, annotated_object|
 *			puts "#{note_blob.oid} => #{annotated_object.oid}"
 *		end
 */
static VALUE rb_git_note_each(int argc, VALUE *argv, VALUE klass)
{
	git_repository *repo;
	const char *notes_ref = NULL;
	int error;

	VALUE rb_repo, rb_notes_ref;

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_notes_ref);

	if (!rb_block_given_p()) {
		return rb_funcall(klass, rb_intern("to_enum"), 3, CSTR2SYM("each"), rb_repo, rb_notes_ref);
	}

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	if (!NIL_P(rb_notes_ref)){
		Check_Type(rb_notes_ref, T_STRING);
		notes_ref = StringValueCStr(rb_notes_ref);
	}

	error = git_note_foreach(repo, notes_ref, &cb_note__each, (void *)rb_repo);
	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_note()
{
	rb_cRuggedNote = rb_define_class_under(rb_mRugged, "Note", rb_cObject);
	rb_define_singleton_method(rb_cRuggedNote, "create", rb_git_note_create, 2);
	rb_define_singleton_method(rb_cRuggedNote, "remove!", rb_git_note_remove, 2);
	rb_define_singleton_method(rb_cRuggedNote, "lookup", rb_git_note_lookup, -1);
	rb_define_singleton_method(rb_cRuggedNote, "default_ref", rb_git_note_default_ref_GET, 1);
	rb_define_singleton_method(rb_cRuggedNote, "each", rb_git_note_each, -1);

	rb_define_method(rb_cRuggedNote, "message", rb_git_note_message_GET, 0);
	rb_define_method(rb_cRuggedNote, "oid", rb_git_note_oid_GET, 0);
}
