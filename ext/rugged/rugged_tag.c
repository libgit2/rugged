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
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;
extern VALUE rb_cRuggedReference;

VALUE rb_cRuggedTag;
VALUE rb_cRuggedTagAnnotation;

/*
 *  call-seq:
 *    annotation.target -> object
 *
 *  Return the +object+ pointed at by this tag annotation, as a <tt>Rugged::Object</tt>
 *  instance.
 *
 *    annotation.target #=> #<Rugged::Commit:0x108828918>
 */
static VALUE rb_git_tag_annotation_target(VALUE self)
{
	git_tag *tag;
	git_object *target;
	int error;
	VALUE owner;

	Data_Get_Struct(self, git_tag, tag);
	owner = rugged_owner(self);

	error = git_tag_target(&target, tag);
	rugged_exception_check(error);

	return rugged_object_new(owner, target);
}

/*
 *  call-seq:
 *    annotation.target_oid -> oid
 *    annotation.target_id -> oid
 *
 *  Return the oid pointed at by this tag annotation, as a <tt>String</tt>
 *  instance.
 *
 *    annotation.target_id #=> "2cb831a8aea28b2c1b9c63385585b864e4d3bad1"
 */
static VALUE rb_git_tag_annotation_target_id(VALUE self)
{
	git_tag *tag;
	const git_oid *target_oid;

	Data_Get_Struct(self, git_tag, tag);

	target_oid = git_tag_target_id(tag);

	return rugged_create_oid(target_oid);
}

/*
 *  call-seq:
 *    annotation.type -> t
 *
 *  Return a symbol representing the type of the objeced pointed at by
 *  this +annotation+. Possible values are +:blob+, +:commit+, +:tree+ and +:tag+.
 *
 *  This is always the same as the +type+ of the returned <tt>annotation.target</tt>
 *
 *    annotation.type #=> :commit
 *    annotation.target.type == annotation.type #=> true
 */
static VALUE rb_git_tag_annotation_target_type(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rugged_otype_new(git_tag_target_type(tag));
}

/*
 *  call-seq:
 *    annotation.name -> name
 *
 *  Return a string with the name of this tag +annotation+.
 *
 *    annotation.name #=> "v0.16.0"
 */
static VALUE rb_git_tag_annotation_name(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new_utf8(git_tag_name(tag));
}

/*
 *  call-seq:
 *    annotation.tagger -> signature
 *
 *  Return the signature for the author of this tag +annotation+. The signature
 *  is returned as a +Hash+ containing +:name+, +:email+ of the author
 *  and +:time+ of the tagging.
 *
 *    annotation.tagger #=> {:email=>"tanoku@gmail.com", :time=>Tue Jan 24 05:42:45 UTC 2012, :name=>"Vicent Mart\303\255"}
 */
static VALUE rb_git_tag_annotation_tagger(VALUE self)
{
	git_tag *tag;
	const git_signature *tagger;

	Data_Get_Struct(self, git_tag, tag);
	tagger = git_tag_tagger(tag);

	if (!tagger)
		return Qnil;

	return rugged_signature_new(tagger, NULL);
}

/*
 *  call-seq:
 *    annotation.message -> msg
 *
 *  Return the message of this tag +annotation+. This includes the full body of the
 *  message and any optional footers or signatures after it.
 *
 *    annotation.message #=> "Release v0.16.0, codename 'broken stuff'"
 */
static VALUE rb_git_tag_annotation_message(VALUE self)
{
	git_tag *tag;
	const char *message;

	Data_Get_Struct(self, git_tag, tag);
	message = git_tag_message(tag);

	if (!message)
		return Qnil;

	return rb_str_new_utf8(message);
}

/*
 *  call-seq:
 *    Tag.lookup(repo, name) -> tag
 *
 *  Lookup a tag in +repo+, with the given +name+.
 *
 *  +name+ can be a short or canonical tag name
 *  (e.g. +v0.1.0+ or +refs/tags/v0.1.0+).
 *
 *  Returns the looked up tag, or +nil+ if the tag doesn't exist.
 */
static VALUE rb_git_tag_lookup(VALUE self, VALUE rb_repo, VALUE rb_name)
{
	git_reference *tag;
	git_repository *repo;
	int error;

	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	error = git_reference_lookup(&tag, repo, StringValueCStr(rb_name));
	if (error == GIT_ENOTFOUND || error == GIT_EINVALIDSPEC) {
		char *canonical_ref = xmalloc((RSTRING_LEN(rb_name) + strlen("refs/tags/") + 1) * sizeof(char));
		strcpy(canonical_ref, "refs/tags/");
		strcat(canonical_ref, StringValueCStr(rb_name));

		error = git_reference_lookup(&tag, repo, canonical_ref);
		xfree(canonical_ref);
		if (error == GIT_ENOTFOUND)
			return Qnil;
	}

	rugged_exception_check(error);

	return rugged_ref_new(rb_cRuggedTag, rb_repo, tag);
}

/*
 *  call-seq:
 *    Tag.create(repo, name, target[, force = false][, annotation = nil]) -> oid
 *
 *  Create a new tag with the specified +name+ on +target+ in +repo+.
 *
 *  If +annotation+ is not +nil+, it will cause the creation of an annotated tag object.
 *  +annotation+ has to contain the following key value pairs:
 *
 *  :tagger ::
 *    An optional Hash containing a git signature. Defaults to the signature
 *    from the configuration if only `:message` is given. Will cause the
 *    creation of an annotated tag object if present.
 *
 *  :message ::
 *    An optional string containing the message for the new tag.
 *
 *  Returns the OID of the newly created tag.
 */
static VALUE rb_git_tag_create(int argc, VALUE *argv, VALUE self)
{
	git_oid tag_oid;
	git_repository *repo = NULL;
	git_object *target = NULL;
	int error, force = 0;

	VALUE rb_repo, rb_name, rb_target, rb_force, rb_annotation;

	rb_scan_args(argc, argv, "31:", &rb_repo, &rb_name, &rb_target, &rb_force, &rb_annotation);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	if (!NIL_P(rb_force))
		force = rugged_parse_bool(rb_force);

	target = rugged_object_get(repo, rb_target, GIT_OBJ_ANY);

	if (NIL_P(rb_annotation)) {
		error = git_tag_create_lightweight(
			&tag_oid,
			repo,
			StringValueCStr(rb_name),
			target,
			force
		);
	} else {
		git_signature *tagger = rugged_signature_get(
			rb_hash_aref(rb_annotation, CSTR2SYM("tagger")), repo
		);
		VALUE rb_message = rb_hash_aref(rb_annotation, CSTR2SYM("message"));

		Check_Type(rb_message, T_STRING);

		error = git_tag_create(
			&tag_oid,
			repo,
			StringValueCStr(rb_name),
			target,
			tagger,
			StringValueCStr(rb_message),
			force
		);

		git_signature_free(tagger);
	}

	git_object_free(target);
	rugged_exception_check(error);

	return rb_git_tag_lookup(self, rb_repo, rb_name);
}

/*
 *  call-seq:
 *    Tag.delete(repo, name) -> nil
 *
 *  Delete the tag reference identified by +name+ from +repo+.
 */
static VALUE rb_git_tag_delete(VALUE self, VALUE rb_repo, VALUE rb_name)
{
	git_repository *repo;
	int error;

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	error = git_tag_delete(repo, StringValueCStr(rb_name));
	rugged_exception_check(error);
	return Qnil;
}

static VALUE each_tag(int argc, VALUE *argv, VALUE self, int tag_names_only)
{
	git_repository *repo;
	git_strarray tags;
	size_t i;
	int error, exception = 0;
	VALUE rb_repo, rb_pattern;
	const char *pattern = NULL;

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_pattern);

	if (!rb_block_given_p()) {
		VALUE symbol = tag_names_only ? CSTR2SYM("each_name") : CSTR2SYM("each");
		return rb_funcall(self, rb_intern("to_enum"), 3, symbol, rb_repo, rb_pattern);
	}

	if (!NIL_P(rb_pattern)) {
		Check_Type(rb_pattern, T_STRING);
		pattern = StringValueCStr(rb_pattern);
	}

	rugged_check_repo(rb_repo);
	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_tag_list_match(&tags, pattern ? pattern : "", repo);
	rugged_exception_check(error);

	if (tag_names_only) {
		for (i = 0; !exception && i < tags.count; ++i)
			rb_protect(rb_yield, rb_str_new_utf8(tags.strings[i]), &exception);
	} else {
		for (i = 0; !exception && i < tags.count; ++i) {
			rb_protect(rb_yield, rb_git_tag_lookup(self, rb_repo,
				rb_str_new_utf8(tags.strings[i])), &exception);
		}
	}

	git_strarray_free(&tags);

	if (exception)
		rb_jump_tag(exception);

	return Qnil;
}

/*
 *  call-seq:
 *    Tag.each_name(repo[, pattern]) { |name| block } -> nil
 *    Tag.each_name(repo[, pattern])                  -> enumerator
 *
 *  Iterate through all the tag names in +repo+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +pattern+, a standard Unix filename glob.
 *
 *  If +pattern+ is empty or not given, all tag names will be returned.
 *
 *  The given block will be called once with the name for each tag.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_tag_each_name(int argc, VALUE *argv, VALUE self)
{
	return each_tag(argc, argv, self, 1);
}

/*
 *  call-seq:
 *    Tag.each(repo[, pattern]) { |name| block } -> nil
 *    Tag.each(repo[, pattern])                  -> enumerator
 *
 *  Iterate through all the tags in +repo+. Iteration
 *  can be optionally filtered to the ones matching the given
 *  +pattern+, a standard Unix filename glob.
 *
 *  If +pattern+ is empty or not given, all tag names will be returned.
 *
 *  The given block will be called once with the name for each tag.
 *
 *  If no block is given, an enumerator will be returned.
 */
static VALUE rb_git_tag_each(int argc, VALUE *argv, VALUE self)
{
	return each_tag(argc, argv, self, 0);
}

/*
 *  call-seq:
 *    tag.annotation -> annotation or nil
 */
static VALUE rb_git_tag_annotation(VALUE self)
{
	git_reference *ref, *resolved_ref;
	git_repository *repo;
	git_object *target;
	int error;
	VALUE rb_repo = rugged_owner(self);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(self, git_reference, ref);
	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_reference_resolve(&resolved_ref, ref);
	rugged_exception_check(error);

	error = git_object_lookup(&target, repo, git_reference_target(resolved_ref), GIT_OBJ_TAG);
	git_reference_free(resolved_ref);

	if (error == GIT_ENOTFOUND)
		return Qnil;

	return rugged_object_new(rb_repo, target);
}

/*
 *  call-seq:
 *    tag.target -> git_object
 */
static VALUE rb_git_tag_target(VALUE self)
{
	git_reference *ref, *resolved_ref;
	git_repository *repo;
	git_object *target;
	int error;
	VALUE rb_repo = rugged_owner(self);

	rugged_check_repo(rb_repo);
	Data_Get_Struct(self, git_reference, ref);
	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_reference_resolve(&resolved_ref, ref);
	rugged_exception_check(error);

	error = git_object_lookup(&target, repo, git_reference_target(resolved_ref), GIT_OBJ_ANY);
	git_reference_free(resolved_ref);
	rugged_exception_check(error);

	if (git_object_type(target) == GIT_OBJ_TAG) {
		git_object *annotation_target;

		error = git_tag_target(&annotation_target, (git_tag *)target);
		git_object_free(target);
		rugged_exception_check(error);

		return rugged_object_new(rb_repo, annotation_target);
	} else {
		return rugged_object_new(rb_repo, target);
	}
}

static VALUE rb_git_tag_annotated_p(VALUE self)
{
	return RTEST(rb_git_tag_annotation(self)) ? Qtrue : Qfalse;
}

void Init_rugged_tag(void)
{
	rb_cRuggedTag = rb_define_class_under(rb_mRugged, "Tag", rb_cRuggedReference);

	rb_define_singleton_method(rb_cRuggedTag, "create", rb_git_tag_create, -1);
	rb_define_singleton_method(rb_cRuggedTag, "each", rb_git_tag_each, -1);
	rb_define_singleton_method(rb_cRuggedTag, "each_name", rb_git_tag_each_name, -1);
	rb_define_singleton_method(rb_cRuggedTag, "delete", rb_git_tag_delete, 2);
	rb_define_singleton_method(rb_cRuggedTag, "lookup", rb_git_tag_lookup, 2);

	rb_define_method(rb_cRuggedTag, "annotation", rb_git_tag_annotation, 0);
	rb_define_method(rb_cRuggedTag, "annotated?", rb_git_tag_annotated_p, 0);
	rb_define_method(rb_cRuggedTag, "target", rb_git_tag_target, 0);

	rb_cRuggedTagAnnotation = rb_define_class_under(rb_mRugged, "TagAnnotation", rb_cRuggedObject);

	rb_define_method(rb_cRuggedTagAnnotation, "message", rb_git_tag_annotation_message, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "name", rb_git_tag_annotation_name, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "target", rb_git_tag_annotation_target, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "target_oid", rb_git_tag_annotation_target_id, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "target_id", rb_git_tag_annotation_target_id, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "target_type", rb_git_tag_annotation_target_type, 0);
	rb_define_method(rb_cRuggedTagAnnotation, "tagger", rb_git_tag_annotation_tagger, 0);
}
