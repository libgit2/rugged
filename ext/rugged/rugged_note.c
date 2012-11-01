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
 *	Returns a new <tt>Rugged::Note</tt> object.
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

void Init_rugged_note()
{
	rb_cRuggedNote = rb_define_class_under(rb_mRugged, "Note", rb_cObject);
	rb_define_singleton_method(rb_cRuggedNote, "lookup", rb_git_note_lookup, -1);
}
