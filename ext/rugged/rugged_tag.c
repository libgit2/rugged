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

static VALUE rb_git_tag_allocate(VALUE klass)
{
	git_tag *tag = NULL;
	return Data_Wrap_Struct(klass, NULL, NULL, tag);
}

static VALUE rb_git_tag_init(int argc, VALUE *argv, VALUE self)
{
	return rb_git_object_init(GIT_OBJ_TAG, argc, argv, self);
}

static VALUE rb_git_tag_target_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rugged_object_c2rb((git_object*)git_tag_target(tag));
}

static VALUE rb_git_tag_target_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	git_object *target;
	Data_Get_Struct(self, git_tag, tag);

	target = rugged_object_rb2c(git_object_owner((git_object *)tag), val, GIT_OBJ_ANY);
	git_tag_set_target(tag, target);
	return Qnil;
}

static VALUE rb_git_tag_target_type_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_otype_tostring(git_tag_type(tag)));
}

static VALUE rb_git_tag_name_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_tag_name(tag));
}

static VALUE rb_git_tag_name_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	Check_Type(val, T_STRING);
	git_tag_set_name(tag, RSTRING_PTR(val));
	return Qnil;
}

static VALUE rb_git_tag_tagger_GET(VALUE self)
{
	git_tag *tag;
	git_person *person;
	Data_Get_Struct(self, git_tag, tag);

	person = (git_person *)git_tag_tagger(tag);
	return rugged_person_c2rb(person);
}

static VALUE rb_git_tag_tagger_SET(VALUE self, VALUE rb_person)
{
	const char *name, *email;
	time_t time;
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	rugged_person_rb2c(rb_person, &name, &email, &time);
	git_tag_set_tagger(tag, name, email, time);
	return Qnil;
}

static VALUE rb_git_tag_message_GET(VALUE self)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	return rb_str_new2(git_tag_message(tag));
}

static VALUE rb_git_tag_message_SET(VALUE self, VALUE val)
{
	git_tag *tag;
	Data_Get_Struct(self, git_tag, tag);

	Check_Type(val, T_STRING);
	git_tag_set_message(tag, RSTRING_PTR(val));
	return Qnil;
}


void Init_rugged_tag()
{
	rb_cRuggedTag = rb_define_class_under(rb_mRugged, "Tag", rb_cRuggedObject);
	rb_define_alloc_func(rb_cRuggedTag, rb_git_tag_allocate);
	rb_define_method(rb_cRuggedTag, "initialize", rb_git_tag_init, -1);

	rb_define_method(rb_cRuggedTag, "message", rb_git_tag_message_GET, 0);
	rb_define_method(rb_cRuggedTag, "message=", rb_git_tag_message_SET, 1);

	rb_define_method(rb_cRuggedTag, "name", rb_git_tag_name_GET, 0);
	rb_define_method(rb_cRuggedTag, "name=", rb_git_tag_name_SET, 1);

	rb_define_method(rb_cRuggedTag, "target", rb_git_tag_target_GET, 0);
	rb_define_method(rb_cRuggedTag, "target=", rb_git_tag_target_SET, 1);

	/* Read only */
	rb_define_method(rb_cRuggedTag, "target_type", rb_git_tag_target_type_GET, 0);

	rb_define_method(rb_cRuggedTag, "tagger", rb_git_tag_tagger_GET, 0);
	rb_define_method(rb_cRuggedTag, "tagger=", rb_git_tag_tagger_SET, 1);
}
