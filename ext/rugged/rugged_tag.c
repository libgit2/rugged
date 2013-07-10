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
extern VALUE rb_cRuggedObject;
extern VALUE rb_cRuggedRepo;
VALUE rb_cRuggedTag;

/*
 *  call-seq:
 *    tag.target -> object
 *
 *  Return the +object+ pointed at by this tag, as a <tt>Rugged::Object</tt>
 *  instance.
 *
 *    tag.target #=> #<Rugged::Commit:0x108828918>
 */
static VALUE rb_git_tag_target_GET(VALUE self)
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
 *    tag.target_oid -> oid
 *    tag.target_id -> oid
 *
 *  Return the oid pointed at by this tag, as a <tt>String</tt>
 *  instance.
 *
 *    tag.target_id #=> "2cb831a8aea28b2c1b9c63385585b864e4d3bad1"
 */
static VALUE rb_git_tag_target_id_GET(VALUE self)
{
	git_tag *tag;
	const git_oid *target_oid;

	Data_Get_Struct(self, git_tag, tag);

	target_oid = git_tag_target_id(tag);

	return rugged_create_oid(target_oid);
}

/*
 *  call-seq:
 *    tag.type -> t
 *
 *  Return a symbol representing the type of the objeced pointed at by
 *  this +tag+. Possible values are +:blob+, +:commit+, +:tree+ and +:tag+.
 *
 *  This is always the same as the +type+ of the returned <tt>tag.target</tt>
 *
 *    tag.type #=> :commit
 *    tag.target.type == tag.type #=> true
 */
static VALUE rb_git_tag_target_type_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rugged_otype_new(git_tag_target_type(tag));
}

/*
 *  call-seq:
 *    tag.name -> name
 *
 *  Return a string with the name of this +tag+.
 *
 *    tag.name #=> "v0.16.0"
 */
static VALUE rb_git_tag_name_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new_utf8(git_tag_name(tag));
}

/*
 *  call-seq:
 *    tag.tagger -> signature
 *
 *  Return the signature for the author of this +tag+. The signature
 *  is returned as a +Hash+ containing +:name+, +:email+ of the author
 *  and +:time+ of the tagging.
 *
 *    tag.tagger #=> {:email=>"tanoku@gmail.com", :time=>Tue Jan 24 05:42:45 UTC 2012, :name=>"Vicent Mart\303\255"}
 */
static VALUE rb_git_tag_tagger_GET(VALUE self)
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
 *    tag.message -> msg
 *
 *  Return the message of this +tag+. This includes the full body of the
 *  message and any optional footers or signatures after it.
 *
 *    tag.message #=> "Release v0.16.0, codename 'broken stuff'"
 */
static VALUE rb_git_tag_message_GET(VALUE self)
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
 *    Tag.create(repo, data) -> oid
 *
 *  Create a new tag in +repo+.
 *
 *  If +data+ is a String, it has to contain the raw tag data.
 *
 *  If +data+ is a Hash, it has to contain the following key value pairs:
 *
 *  :name ::
 *    A String holding the tag's new name.
 *
 *  :force ::
 *    If +true+, existing tags will be overwritten. Defaults to +false+.
 *
 *  :tagger ::
 *    An optional Hash containing a git signature. Will cause the creation of
 *    an annotated tag object if present.
 *
 *  :message ::
 *    An optional string containing the message for the new tag. Will cause
 *    the creation of an annotated tag if present.
 *
 *  :target ::
 *    The OID of the object that the new tag should point to.
 *
 *  Returns the OID of the newly created tag.
 */
static VALUE rb_git_tag_create(VALUE self, VALUE rb_repo, VALUE rb_data)
{
	git_oid tag_oid;
	git_repository *repo = NULL;
	int error, force = 0;

	VALUE rb_name, rb_target, rb_tagger, rb_message, rb_force;

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, git_repository, repo);

	if (TYPE(rb_data) == T_STRING) {
		error = git_tag_create_frombuffer(
			&tag_oid,
			repo,
			StringValueCStr(rb_data),
			force
		);
	} else if (TYPE(rb_data) == T_HASH) {
		git_object *target = NULL;

		rb_name = rb_hash_aref(rb_data, CSTR2SYM("name"));
		Check_Type(rb_name, T_STRING);

		rb_force = rb_hash_aref(rb_data, CSTR2SYM("force"));
		if (!NIL_P(rb_force))
			force = rugged_parse_bool(rb_force);

		/* only for heavy tags */
		rb_tagger = rb_hash_aref(rb_data, CSTR2SYM("tagger"));
		rb_message = rb_hash_aref(rb_data, CSTR2SYM("message"));

		if (!NIL_P(rb_message))
			Check_Type(rb_message, T_STRING);

		rb_target = rb_hash_aref(rb_data, CSTR2SYM("target"));
		target = rugged_object_get(repo, rb_target, GIT_OBJ_ANY);

		if (!NIL_P(rb_tagger) && !NIL_P(rb_message)) {
			git_signature *tagger = NULL;

			tagger = rugged_signature_get(rb_tagger);

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
		} else {
			error = git_tag_create_lightweight(
				&tag_oid,
				repo,
				StringValueCStr(rb_name),
				target,
				force
			);
		}

		git_object_free(target);
	} else {
		rb_raise(rb_eTypeError, "Invalid tag data: expected a String or a Hash");
	}

	rugged_exception_check(error);
	return rugged_create_oid(&tag_oid);
}

/*
 *  call-seq:
 *    Tag.each(repo[, pattern]) { |name| block } -> nil
 *    Tag.each(repo[, pattern])                  -> enumerator
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
static VALUE rb_git_tag_each(int argc, VALUE *argv, VALUE self)
{
	git_repository *repo;
	git_strarray tags;
	size_t i;
	int error, exception = 0;
	VALUE rb_repo, rb_pattern;
	const char *pattern = NULL;

	rb_scan_args(argc, argv, "11", &rb_repo, &rb_pattern);

	if (!rb_block_given_p())
		return rb_funcall(self, rb_intern("to_enum"), 3, CSTR2SYM("each"), rb_repo, rb_pattern);

	if (!NIL_P(rb_pattern)) {
		Check_Type(rb_pattern, T_STRING);
		pattern = StringValueCStr(rb_pattern);
	}

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");

	Data_Get_Struct(rb_repo, git_repository, repo);

	error = git_tag_list_match(&tags, pattern ? pattern : "", repo);
	rugged_exception_check(error);

	for (i = 0; !exception && i < tags.count; ++i)
		rb_protect(rb_yield, rb_str_new_utf8(tags.strings[i]), &exception);

	git_strarray_free(&tags);

	if (exception)
		rb_jump_tag(exception);

	return Qnil;
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

	if (!rb_obj_is_kind_of(rb_repo, rb_cRuggedRepo))
		rb_raise(rb_eTypeError, "Expecting a Rugged::Repository instance");
	Data_Get_Struct(rb_repo, git_repository, repo);

	Check_Type(rb_name, T_STRING);

	error = git_tag_delete(repo, StringValueCStr(rb_name));
	rugged_exception_check(error);
	return Qnil;
}

void Init_rugged_tag(void)
{
	rb_cRuggedTag = rb_define_class_under(rb_mRugged, "Tag", rb_cRuggedObject);

	rb_define_singleton_method(rb_cRuggedTag, "create", rb_git_tag_create, 2);
	rb_define_singleton_method(rb_cRuggedTag, "each", rb_git_tag_each, -1);
	rb_define_singleton_method(rb_cRuggedTag, "delete", rb_git_tag_delete, 2);

	rb_define_method(rb_cRuggedTag, "message", rb_git_tag_message_GET, 0);
	rb_define_method(rb_cRuggedTag, "name", rb_git_tag_name_GET, 0);
	rb_define_method(rb_cRuggedTag, "target", rb_git_tag_target_GET, 0);
	rb_define_method(rb_cRuggedTag, "target_oid", rb_git_tag_target_id_GET, 0);
	rb_define_method(rb_cRuggedTag, "target_id", rb_git_tag_target_id_GET, 0);
	rb_define_method(rb_cRuggedTag, "target_type", rb_git_tag_target_type_GET, 0);
	rb_define_method(rb_cRuggedTag, "tagger", rb_git_tag_tagger_GET, 0);
}
