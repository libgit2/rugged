/*
 * The MIT License
 *
 * Copyright (c) 2010 Scott Chacon
 * Copyright (c) 2010 Vicent Marti
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
VALUE rb_cRuggedTag;

static VALUE rb_git_tag_target_GET(VALUE self)
{
	git_tag *tag;
	git_object *target;
	int error;
	VALUE owner;

	RUGGED_OBJ_UNWRAP(self, git_tag, tag);
	RUGGED_OBJ_OWNER(self, owner);

	error = git_tag_target(&target, tag);
	rugged_exception_check(error);

	return rugged_object_new(owner, target);
}

static VALUE rb_git_tag_target_type_GET(VALUE self)
{
	git_tag *tag;
	RUGGED_OBJ_UNWRAP(self, git_tag, tag);

	return rugged_str_new2(git_object_type2string(git_tag_type(tag)), NULL);
}

static VALUE rb_git_tag_name_GET(VALUE self)
{
	git_tag *tag;
	RUGGED_OBJ_UNWRAP(self, git_tag, tag);

	return rugged_str_new2(git_tag_name(tag), NULL);
}

static VALUE rb_git_tag_tagger_GET(VALUE self)
{
	git_tag *tag;
	RUGGED_OBJ_UNWRAP(self, git_tag, tag);

	return rugged_signature_new(git_tag_tagger(tag));
}

static VALUE rb_git_tag_message_GET(VALUE self)
{
	git_tag *tag;
	RUGGED_OBJ_UNWRAP(self, git_tag, tag);

	return rugged_str_new2(git_tag_message(tag), NULL);
}

void Init_rugged_tag()
{
	rb_cRuggedTag = rb_define_class_under(rb_mRugged, "Tag", rb_cRuggedObject);

	rb_define_method(rb_cRuggedTag, "message", rb_git_tag_message_GET, 0);
	rb_define_method(rb_cRuggedTag, "name", rb_git_tag_name_GET, 0);
	rb_define_method(rb_cRuggedTag, "target", rb_git_tag_target_GET, 0);
	rb_define_method(rb_cRuggedTag, "target_type", rb_git_tag_target_type_GET, 0);
	rb_define_method(rb_cRuggedTag, "tagger", rb_git_tag_tagger_GET, 0);
}
