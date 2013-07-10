/*
 * The MIT License
 *
 * Copyright (c) 2013 GitHub, Inc
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
VALUE rb_cRuggedReference;

void rb_git_ref__free(git_reference *ref)
{
	git_reference_free(ref);
}

VALUE rugged_ref_new(VALUE klass, VALUE owner, git_reference *ref)
{
	VALUE rb_ref = Data_Wrap_Struct(klass, NULL, &rb_git_ref__free, ref);
	rugged_set_owner(rb_ref, owner);
	return rb_ref;
}

static VALUE rb_git_ref__each(int argc, VALUE *argv, VALUE self, int only_names)
{
	git_repository *repo;
	git_reference_iterator *iter;
	int error, exception = 0;
	VALUE rb_repo, rb_glob;

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_glob);

	if (!rb_block_given_p()) {
		return rb_funcall(self,
			rb_intern("to_enum"), 3,
			only_names ? CSTR2SYM("each_name") : CSTR2SYM("each"),
			rb_repo, rb_glob);
	}

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, git_repository, repo);

	if (!NIL_P(rb_glob)) {
		Check_Type(rb_glob, T_STRING);
		error = git_reference_iterator_glob_new(&iter, repo, StringValueCStr(rb_glob));
	} else {
		error = git_reference_iterator_new(&iter, repo);
	}

	rugged_exception_check(error);

	if (only_names) {
		const char *ref_name;
		while (!exception && (error = git_reference_next_name(&ref_name, iter)) == GIT_OK) {
			rb_protect(rb_yield, rb_str_new_utf8(ref_name), &exception);
		}
	} else {
		git_reference *ref;
		while (!exception && (error = git_reference_next(&ref, iter)) == GIT_OK) {
			rb_protect(rb_yield, rugged_ref_new(rb_cRuggedReference, rb_repo, ref), &exception);
		}
	}

	git_reference_iterator_free(iter);

	if (exception)
		rb_jump_tag(exception);

	if (error != GIT_ITEROVER)
		rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    Reference.each(repository, glob = nil) { |ref| block } -> nil
 *    Reference.each(repository, glob = nil) -> enumerator
 *
 *  Iterate through all the references in +repository+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +glob+, a standard Unix filename glob.
 *
 *  The given block will be called once with a Rugged::Reference
 *  instance for each reference.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_ref_each(int argc, VALUE *argv, VALUE self)
{
	return rb_git_ref__each(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    Reference.each_name(repository, glob = nil) { |ref_name| block } -> nil
 *    Reference.each_name(repository, glob = nil) -> enumerator
 *
 *  Iterate through all the reference names in +repository+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +glob+, a standard Unix filename glob.
 *
 *  The given block will be called once with the name of each reference.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_ref_each_name(int argc, VALUE *argv, VALUE self)
{
	return rb_git_ref__each(argc, argv, self, 1);
}

/*
 *  call-seq:
 *    Reference.lookup(repository, ref_name) -> new_ref
 *
 *  Lookup a reference from the +repository+.
 *  Returns a new Rugged::Reference object.
 */
static VALUE rb_git_ref_lookup(VALUE klass, VALUE rb_repo, VALUE rb_name)
{
	git_repository *repo;
	git_reference *ref;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);
	Check_Type(rb_name, T_STRING);

	error = git_reference_lookup(&ref, repo, StringValueCStr(rb_name));
	if (error == GIT_ENOTFOUND)
		return Qnil;
	else
		rugged_exception_check(error);

	return rugged_ref_new(klass, rb_repo, ref);
}

/*
 *  call-seq:
 *    Reference.valid_name?(ref_name) -> true or false
 *
 *  Check if a reference name is well-formed.
 *
 *  Valid reference names must follow one of two patterns:
 *
 *  1. Top-level names must contain only capital letters and underscores,
 *     and must begin and end with a letter. (e.g. "HEAD", "ORIG_HEAD").
 *  2. Names prefixed with "refs/" can be almost anything.  You must avoid
 *     the characters '~', '^', ':', '\\', '?', '[', and '*', and the
 *     sequences ".." and "@{" which have special meaning to revparse.
 *
 *  Returns true if the reference name is valid, false if not.
 */
static VALUE rb_git_ref_valid_name(VALUE klass, VALUE rb_name)
{
	Check_Type(rb_name, T_STRING);
	return git_reference_is_valid_name(StringValueCStr(rb_name)) == 1 ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    ref.peel -> oid
 *
 *  Peels tag objects to the sha that they point at. Replicates
 *  +git show-ref --dereference+.
 */
static VALUE rb_git_ref_peel(VALUE self)
{
	/* Leave room for \0 */
	git_reference *ref;
	git_object *object;
	char oid[GIT_OID_HEXSZ + 1];
	int error;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reference_peel(&object, ref, GIT_OBJ_ANY);
	if (error == GIT_ENOTFOUND)
		return Qnil;
	else
		rugged_exception_check(error);

	if (git_reference_type(ref) == GIT_REF_OID &&
			!git_oid_cmp(git_object_id(object), git_reference_target(ref))) {
		git_object_free(object);
		return Qnil;
	} else {
		git_oid_tostr(oid, sizeof(oid), git_object_id(object));
		git_object_free(object);
		return rb_str_new_utf8(oid);
	}
}

/*
 *  call-seq:
 *    Reference.exist?(repository, ref_name) -> true or false
 *    Reference.exists?(repository, ref_name) -> true or false
 *
 *  Check if a given reference exists on +repository+.
 */
static VALUE rb_git_ref_exist(VALUE klass, VALUE rb_repo, VALUE rb_name)
{
	git_repository *repo;
	git_reference *ref;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);
	Check_Type(rb_name, T_STRING);

	error = git_reference_lookup(&ref, repo, StringValueCStr(rb_name));
	git_reference_free(ref);

	if (error == GIT_ENOTFOUND)
		return Qfalse;
	else
		rugged_exception_check(error);

	return Qtrue;
}

/*
 *  call-seq:
 *    Reference.create(repository, name, oid, force = false) -> new_ref
 *    Reference.create(repository, name, target, force = false) -> new_ref
 *
 *  Create a symbolic or direct reference on +repository+ with the given +name+.
 *  If the third argument is a valid OID, the reference will be created as direct.
 *  Otherwise, it will be assumed the target is the name of another reference.
 *
 *  If a reference with the given +name+ already exists and +force+ is +true+,
 *  it will be overwritten. Otherwise, an exception will be raised.
 */
static VALUE rb_git_ref_create(int argc, VALUE *argv, VALUE klass)
{
	VALUE rb_repo, rb_name, rb_target, rb_force;
	git_repository *repo;
	git_reference *ref;
	git_oid oid;
	int error, force = 0;

	rb_scan_args(argc, argv, "31", &rb_repo, &rb_name, &rb_target, &rb_force);

	Data_Get_Struct(rb_repo, git_repository, repo);
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	if (git_oid_fromstr(&oid, StringValueCStr(rb_target)) == GIT_OK) {
		error = git_reference_create(
			&ref, repo, StringValueCStr(rb_name), &oid, force);
	} else {
		error = git_reference_symbolic_create(
			&ref, repo, StringValueCStr(rb_name), StringValueCStr(rb_target), force);
	}

	rugged_exception_check(error);
	return rugged_ref_new(klass, rb_repo, ref);
}

/*
 *  call-seq:
 *    reference.target -> oid
 *    reference.target -> ref_name
 *
 *  Return the target of the reference, which is an OID for +:direct+
 *  references, and the name of another reference for +:symbolic+ ones.
 *
 *    r1.type #=> :symbolic
 *    r1.target #=> "refs/heads/master"
 *
 *    r2.type #=> :direct
 *    r2.target #=> "de5ba987198bcf2518885f0fc1350e5172cded78"
 */
static VALUE rb_git_ref_target(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);

	if (git_reference_type(ref) == GIT_REF_OID) {
		return rugged_create_oid(git_reference_target(ref));
	} else {
		return rb_str_new_utf8(git_reference_symbolic_target(ref));
	}
}

/*
 *  call-seq:
 *    reference.set_target(oid) -> new_ref
 *    reference.set_target(ref_name) -> new_ref
 *
 *  Set the target of a reference. If +reference+ is a direct reference,
 *  the new target must be a +String+ representing a SHA1 OID.
 *
 *  If +reference+ is symbolic, the new target must be a +String+ with
 *  the name of another reference.
 *
 *  The original reference is unaltered; a new reference object is
 *  returned with the new target, and the changes are persisted to
 *  disk.
 *
 *    r1.type #=> :symbolic
 *    r1.set_target("refs/heads/master") #=> <Reference>
 *
 *    r2.type #=> :direct
 *    r2.set_target("de5ba987198bcf2518885f0fc1350e5172cded78") #=> <Reference>
 */
static VALUE rb_git_ref_set_target(VALUE self, VALUE rb_target)
{
	git_reference *ref, *out;
	int error;

	Data_Get_Struct(self, git_reference, ref);
	Check_Type(rb_target, T_STRING);

	if (git_reference_type(ref) == GIT_REF_OID) {
		git_oid target;

		error = git_oid_fromstr(&target, StringValueCStr(rb_target));
		rugged_exception_check(error);

		error = git_reference_set_target(&out, ref, &target);
	} else {
		error = git_reference_symbolic_set_target(&out, ref, StringValueCStr(rb_target));
	}

	rugged_exception_check(error);
	return rugged_ref_new(rb_cRuggedReference, rugged_owner(self), out);
}

/*
 *  call-seq:
 *    reference.type -> :symbolic or :direct
 *
 *  Return whether the reference is +:symbolic+ or +:direct+
 */
static VALUE rb_git_ref_type(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);

	switch (git_reference_type(ref)) {
		case GIT_REF_OID:
			return CSTR2SYM("direct");
		case GIT_REF_SYMBOLIC:
			return CSTR2SYM("symbolic");
		default:
			return Qnil;
	}
}

/*
 *  call-seq:
 *    reference.name -> name
 *
 *  Returns the name of the reference
 *
 *    reference.name #=> 'HEAD'
 */
static VALUE rb_git_ref_name(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return rb_str_new_utf8(git_reference_name(ref));
}

/*
 *  call-seq:
 *    reference.resolve -> peeled_ref
 *
 *  Peel a symbolic reference to its target reference.
 *
 *    r1.type #=> :symbolic
 *    r1.name #=> 'HEAD'
 *    r1.target #=> 'refs/heads/master'
 *
 *    r2 = r1.resolve #=> #<Rugged::Reference:0x401b3948>
 *    r2.target #=> '9d09060c850defbc7711d08b57def0d14e742f4e'
 */
static VALUE rb_git_ref_resolve(VALUE self)
{
	git_reference *ref;
	git_reference *resolved;
	int error;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reference_resolve(&resolved, ref);
	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rugged_owner(self), resolved);
}

/*
 *  call-seq:
 *    reference.rename(new_name, force = false)
 *
 *  Change the name of a reference. If +force+ is +true+, any previously
 *  existing references will be overwritten when renaming.
 *
 *  Return a new reference object with the new object
 *
 *    reference.name #=> 'refs/heads/master'
 *    new_ref = reference.rename('refs/heads/development') #=> <Reference>
 *    new_ref.name #=> 'refs/heads/development'
 */
static VALUE rb_git_ref_rename(int argc, VALUE *argv, VALUE self)
{
	git_reference *ref, *out;
	VALUE rb_name, rb_force;
	int error, force = 0;

	Data_Get_Struct(self, git_reference, ref);
	rb_scan_args(argc, argv, "11", &rb_name, &rb_force);

	Check_Type(rb_name, T_STRING);
	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	error = git_reference_rename(&out, ref, StringValueCStr(rb_name), force);
	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rugged_owner(self), out);
}

/*
 *  call-seq:
 *    reference.delete! -> nil
 *
 *  Delete this reference from disk. 
 *
 *    reference.name #=> 'HEAD'
 *    reference.delete!
 *    # Reference no longer exists on disk
 */
static VALUE rb_git_ref_delete(VALUE self)
{
	git_reference *ref;
	int error;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reference_delete(ref);
	rugged_exception_check(error);

	return Qnil;
}

static VALUE reflog_entry_new(const git_reflog_entry *entry)
{
	VALUE rb_entry = rb_hash_new();
	const char *message;

	rb_hash_aset(rb_entry,
		CSTR2SYM("id_old"),
		rugged_create_oid(git_reflog_entry_id_old(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("id_new"),
		rugged_create_oid(git_reflog_entry_id_new(entry))
	);

	rb_hash_aset(rb_entry,
		CSTR2SYM("committer"),
		rugged_signature_new(git_reflog_entry_committer(entry), NULL)
	);

	if ((message = git_reflog_entry_message(entry)) != NULL) {
		rb_hash_aset(rb_entry, CSTR2SYM("message"), rb_str_new_utf8(message));
	}

	return rb_entry;
}

/*
 *  call-seq:
 *    reference.log -> [reflog_entry, ...]
 *
 *  Return an array with the log of all modifications to this reference
 *
 *  Each +reflog_entry+ is a hash with the following keys:
 *
 *  - +:id_old+: previous OID before the change
 *  - +:id_new+: OID after the change
 *  - +:committer+: author of the change
 *  - +:message+: message for the change
 *
 *  Example:
 *
 *    reference.log #=> [
 *    # {
 *    #  :id_old => nil,
 *    #  :id_new => '9d09060c850defbc7711d08b57def0d14e742f4e',
 *    #  :committer => {:name => 'Vicent Marti', :email => {'vicent@github.com'}},
 *    #  :message => 'created reference'
 *    # }, ... ]
 */
static VALUE rb_git_reflog(VALUE self)
{
	git_reflog *reflog;
	git_reference *ref;
	int error;
	VALUE rb_log;
	size_t i, ref_count;

	Data_Get_Struct(self, git_reference, ref);

	error = git_reflog_read(&reflog, ref);
	rugged_exception_check(error);

	ref_count = git_reflog_entrycount(reflog);
	rb_log = rb_ary_new2(ref_count);

	for (i = 0; i < ref_count; ++i) {
		const git_reflog_entry *entry =
			git_reflog_entry_byindex(reflog, ref_count - i - 1);

		rb_ary_push(rb_log, reflog_entry_new(entry));
	}

	git_reflog_free(reflog);
	return rb_log;
}

/*
 *  call-seq:
 *    reference.log? -> true or false
 *
 *  Return +true+ if the reference has a reflog, +false+ otherwise.
 */
static VALUE rb_git_has_reflog(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_has_log(ref) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    reference.log!(committer, message = nil) -> nil
 *
 *  Log a modification for this reference to the reflog.
 */
static VALUE rb_git_reflog_write(int argc, VALUE *argv, VALUE self)
{
	git_reference *ref;
	git_reflog *reflog;
	int error;

	VALUE rb_committer, rb_message;

	git_signature *committer;
	const char *message = NULL;

	Data_Get_Struct(self, git_reference, ref);

	rb_scan_args(argc, argv, "11", &rb_committer, &rb_message);

	if (!NIL_P(rb_message)) {
		Check_Type(rb_message, T_STRING);
		message = StringValueCStr(rb_message);
	}

	error = git_reflog_read(&reflog, ref);
	rugged_exception_check(error);

	committer = rugged_signature_get(rb_committer);

	if (!(error = git_reflog_append(reflog,
					git_reference_target(ref),
					committer,
					message)))
		error = git_reflog_write(reflog);

	git_reflog_free(reflog);
	git_signature_free(committer);

	rugged_exception_check(error);

	return Qnil;
}

/*
 *  call-seq:
 *    reference.branch? -> true or false
 *
 *  Return whether a given reference is a branch
 */
static VALUE rb_git_ref_is_branch(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_is_branch(ref) ? Qtrue : Qfalse;
}

/*
 *  call-seq:
 *    reference.remote? -> true or false
 *
 *  Return whether a given reference is a remote
 */
static VALUE rb_git_ref_is_remote(VALUE self)
{
	git_reference *ref;
	Data_Get_Struct(self, git_reference, ref);
	return git_reference_is_remote(ref) ? Qtrue : Qfalse;
}

void Init_rugged_reference(void)
{
	rb_cRuggedReference = rb_define_class_under(rb_mRugged, "Reference", rb_cObject);

	rb_define_singleton_method(rb_cRuggedReference, "lookup", rb_git_ref_lookup, 2);
	rb_define_singleton_method(rb_cRuggedReference, "exist?", rb_git_ref_exist, 2);
	rb_define_singleton_method(rb_cRuggedReference, "exists?", rb_git_ref_exist, 2);
	rb_define_singleton_method(rb_cRuggedReference, "create", rb_git_ref_create, -1);
	rb_define_singleton_method(rb_cRuggedReference, "each", rb_git_ref_each, -1);
	rb_define_singleton_method(rb_cRuggedReference, "each_name", rb_git_ref_each_name, -1);
	rb_define_singleton_method(rb_cRuggedReference, "valid_name?", rb_git_ref_valid_name, 1);

	rb_define_method(rb_cRuggedReference, "target", rb_git_ref_target, 0);
	rb_define_method(rb_cRuggedReference, "peel", rb_git_ref_peel, 0);
	rb_define_method(rb_cRuggedReference, "set_target", rb_git_ref_set_target, 1);

	rb_define_method(rb_cRuggedReference, "type", rb_git_ref_type, 0);

	rb_define_method(rb_cRuggedReference, "name", rb_git_ref_name, 0);
	rb_define_method(rb_cRuggedReference, "rename", rb_git_ref_rename, -1);

	rb_define_method(rb_cRuggedReference, "resolve", rb_git_ref_resolve, 0);
	rb_define_method(rb_cRuggedReference, "delete!", rb_git_ref_delete, 0);

	rb_define_method(rb_cRuggedReference, "branch?", rb_git_ref_is_branch, 0);
	rb_define_method(rb_cRuggedReference, "remote?", rb_git_ref_is_remote, 0);

	rb_define_method(rb_cRuggedReference, "log", rb_git_reflog, 0);
	rb_define_method(rb_cRuggedReference, "log?", rb_git_has_reflog, 0);
	rb_define_method(rb_cRuggedReference, "log!", rb_git_reflog_write, -1);
}
