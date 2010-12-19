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

#ifndef __H_RUGGED_BINDINGS__
#define __H_RUGGED_BINDINGS__

#include <ruby.h>

#include <assert.h>
#include <git2.h>
#include <git2/odb_backend.h>

/*
 * Initialization functions 
 */
void Init_rugged_object();
void Init_rugged_commit();
void Init_rugged_tree();
void Init_rugged_tag();
void Init_rugged_blob();
void Init_rugged_index();
void Init_rugged_repo();
void Init_rugged_revwalk();
void Init_rugged_signature();

void rugged_exception_check(int errorcode);

VALUE rb_git_object_init(git_otype type, int argc, VALUE *argv, VALUE self);

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid);

VALUE rugged_signature_new(const git_signature *sig);
VALUE rugged_object_new(VALUE repository, git_object *object);
VALUE rugged_index_new(VALUE owner, git_index *index);
VALUE rugged_rawobject_new(const git_rawobj *obj);

git_signature *rugged_signature_get(VALUE rb_person);
git_object *rugged_object_get(git_repository *repo, VALUE object_value, git_otype type);
void rugged_rawobject_get(git_rawobj *obj, VALUE rb_obj);


typedef struct {
	git_odb_backend parent;
	VALUE self;
} rugged_backend;

typedef struct {
	git_object	*object;
	VALUE owner;
} rugged_object;

typedef struct {
	git_repository *repo;
	VALUE backends;
} rugged_repository;

typedef struct {
	git_index *index;
	VALUE owner;
} rugged_index;

typedef struct {
	git_revwalk *walk;
	VALUE owner;
} rugged_walker;


#define RUGGED_OBJ_UNWRAP(_rb, _type, _c) {\
	rugged_object *_rugged_obj; \
	Data_Get_Struct(_rb, rugged_object, _rugged_obj); \
	_c = (_type *)(_rugged_obj->object); \
}

#define RUGGED_OBJ_OWNER(_rb, _val) {\
	rugged_object *_rugged_obj;\
	Data_Get_Struct(_rb, rugged_object, _rugged_obj);\
	_val = (_rugged_obj->owner);\
}


#endif
