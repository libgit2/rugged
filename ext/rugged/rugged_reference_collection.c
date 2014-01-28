/*
 * The MIT License
 *
 * Copyright (c) 2014 GitHub, Inc
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

VALUE rb_cRuggedReferenceCollection;

/*
 *  call-seq:
 *    ReferenceCollection.new(repo) -> refs
 */
static VALUE rb_git_reference_collection_initialize(VALUE self, VALUE repo)
{
	rugged_set_owner(self, repo);
	return self;
}

/*
 *  call-seq:
 *    references.create(name, oid, force = false) -> new_ref
 *    references.create(name, target, force = false) -> new_ref
 *
 *  Create a symbolic or direct reference on the collection's +repository+ with the given +name+.
 *
 *  If the second argument is a valid OID, the reference will be created as direct.
 *  Otherwise, it will be assumed the target is the name of another reference.
 *
 *  If a reference with the given +name+ already exists and +:force+ is not +true+,
 *  an exception will be raised.
 *
 *  The following options can be passed in the +options+ Hash:
 *
 *  :force ::
 *    Overwrites the reference with the given +name+, if it already exists,
 *    instead of raising an exception.
 *
 *  :message ::
 *    A single line log message to be appended to the reflog.
 *
 *  :signature ::
 *    The signature to be used for populating the reflog entry.
 *
 *  The +:message+ and +:signature+ options are ignored if the reference does not
 *  belong to the standard set (+HEAD+, +refs/heads/*+, +refs/remotes/*+ or +refs/notes/*+)
 *  and it does not have a reflog.
 */
static VALUE rb_git_reference_collection_create(int argc, VALUE *argv, VALUE self)
{
	VALUE rb_repo = rugged_owner(self), rb_name, rb_target, rb_options;
	git_repository *repo;
	git_reference *ref;
	git_oid oid;
	git_signature *signature = NULL;
	char *log_message = NULL;
	int error, force = 0;

	rb_scan_args(argc, argv, "20:", &rb_name, &rb_target, &rb_options);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);
	Check_Type(rb_name, T_STRING);
	Check_Type(rb_target, T_STRING);

	if (!NIL_P(rb_options)) {
		VALUE rb_val;

		force = RTEST(rb_hash_aref(rb_options, CSTR2SYM("force")));

		rb_val = rb_hash_aref(rb_options, CSTR2SYM("signature"));
		if (!NIL_P(rb_val))
			signature = rugged_signature_get(rb_val, repo);

		rb_val = rb_hash_aref(rb_options, CSTR2SYM("message"));
		if (!NIL_P(rb_val))
			log_message = StringValueCStr(rb_val);
	}

	if (git_oid_fromstr(&oid, StringValueCStr(rb_target)) == GIT_OK) {
		error = git_reference_create(
			&ref, repo, StringValueCStr(rb_name), &oid, force, signature, log_message);
	} else {
		error = git_reference_symbolic_create(
			&ref, repo, StringValueCStr(rb_name), StringValueCStr(rb_target), force, signature, log_message);
	}

	git_signature_free(signature);
	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rb_repo, ref);
}

/*
 *  call-seq:
 *    references[ref_name] -> new_ref
 *
 *  Lookup a reference from the collection's repository.
 *  Returns a new Rugged::Reference object.
 */
static VALUE rb_git_reference_collection_aref(VALUE self, VALUE rb_name) {
	VALUE rb_repo = rugged_owner(self);
	git_repository *repo;
	git_reference *ref;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_reference_lookup(&ref, repo, StringValueCStr(rb_name));

	if (error == GIT_ENOTFOUND)
		return Qnil;

	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedReference, rb_repo, ref);
}

static VALUE rb_git_reference_collection__each(int argc, VALUE *argv, VALUE self, int only_names)
{
	VALUE rb_glob, rb_repo = rugged_owner(self);
	git_repository *repo;
	git_reference_iterator *iter;
	int error, exception = 0;

	rb_scan_args(argc, argv, "01", &rb_glob);

	if (!rb_block_given_p()) {
		return rb_funcall(self,
			rb_intern("to_enum"), 2,
			only_names ? CSTR2SYM("each_name") : CSTR2SYM("each"),
			rb_glob);
	}

	rugged_check_repo(rb_repo);

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
 *    references.each(glob = nil) { |ref| block } -> nil
 *    references.each(glob = nil) -> enumerator
 *
 *  Iterate through all the references in the collection's +repository+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +glob+, a standard Unix filename glob.
 *
 *  The given block will be called once with a Rugged::Reference
 *  instance for each reference.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_reference_collection_each(int argc, VALUE *argv, VALUE self)
{
	return rb_git_reference_collection__each(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    references.each_name(glob = nil) { |ref_name| block } -> nil
 *    references.each_name(glob = nil) -> enumerator
 *
 *  Iterate through all the reference names in the collection's +repository+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +glob+, a standard Unix filename glob.
 *
 *  The given block will be called once with the name of each reference.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_reference_collection_each_name(int argc, VALUE *argv, VALUE self)
{
	return rb_git_reference_collection__each(argc, argv, self, 1);
}

/*
 *  call-seq:
 *    references.exist?(repository, ref_name) -> true or false
 *    references.exists?(repository, ref_name) -> true or false
 *
 *  Check if a given reference exists in the collection's +repository+.
 */
static VALUE rb_git_reference_collection_exist_p(VALUE self, VALUE rb_name_or_ref)
{
	VALUE rb_repo = rugged_owner(self);
	git_repository *repo;
	git_reference *ref;
	int error;

	if (rb_obj_is_kind_of(rb_name_or_ref, rb_cRuggedReference))
		rb_name_or_ref = rb_funcall(rb_name_or_ref, rb_intern("canonical_name"), 0);

	if (TYPE(rb_name_or_ref) != T_STRING)
		rb_raise(rb_eTypeError, "Expecting a String or Rugged::Reference instance");

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_reference_lookup(&ref, repo, StringValueCStr(rb_name_or_ref));
	git_reference_free(ref);

	if (error == GIT_ENOTFOUND)
		return Qfalse;
	else
		rugged_exception_check(error);

	return Qtrue;
}

void Init_rugged_reference_collection(void)
{
	rb_cRuggedReferenceCollection = rb_define_class_under(rb_mRugged, "ReferenceCollection", rb_cObject);
	rb_include_module(rb_cRuggedReferenceCollection, rb_mEnumerable);

	rb_define_method(rb_cRuggedReferenceCollection, "initialize", rb_git_reference_collection_initialize, 1);

	rb_define_method(rb_cRuggedReferenceCollection, "create",     rb_git_reference_collection_create, -1);
	rb_define_method(rb_cRuggedReferenceCollection, "[]",         rb_git_reference_collection_aref, 1);

	rb_define_method(rb_cRuggedReferenceCollection, "each",       rb_git_reference_collection_each, -1);
	rb_define_method(rb_cRuggedReferenceCollection, "each_name",  rb_git_reference_collection_each_name, -1);

	rb_define_method(rb_cRuggedReferenceCollection, "exist?",     rb_git_reference_collection_exist_p, 1);
	rb_define_method(rb_cRuggedReferenceCollection, "exists?",    rb_git_reference_collection_exist_p, 1);
}
