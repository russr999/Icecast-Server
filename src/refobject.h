/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2018,      Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

/* This file contains the API for the refobject helper type.
 * The refobject helper type is a base type that allows building other types with safe reference counting.
 */

#ifndef __REFOBJECT_H__
#define __REFOBJECT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "common/thread/thread.h"

#include "icecasttypes.h"
#include "compat.h"

/* The following macros are defined. The definition depends on if the compiler
 * supports transparent unions. If supported full type checking is enabled.
 * If the compiler does not support transparent unions, we fall back to using
 * (void*) pointers.
 *
 * REFOBJECT_NULL
 *  Can be used to set an refobject to NULL.
 * REFOBJECT_IS_NULL(x)
 *  Can be used to check if the refobject x is NULL.
 *  Checking by doing (x == NULL) does not work with transparent unions
 *  as the operation is only defined for it's members.
 * REFOBJECT_TO_TYPE(type,x)
 *  This casts the refobject (x) to the type (type).
 */
#ifdef HAVE_TYPE_ATTRIBUTE_TRANSPARENT_UNION
#define REFOBJECT_NULL          ((refobject_t)(refobject_base_t*)NULL)
#define REFOBJECT_GET_BASE(x)   (((refobject_t)(x)).refobject_base)
#define REFOBJECT_IS_NULL(x)    (((refobject_t)(x)).refobject_base == NULL)
#define REFOBJECT_TO_TYPE(x,y)  ((y)(((refobject_t)(x)).refobject_base))
#else
#define REFOBJECT_NULL          NULL
#define REFOBJECT_GET_BASE(x)   ((refobject_base_t)(x))
#define REFOBJECT_IS_NULL(x)    ((x) == NULL)
#define REFOBJECT_TO_TYPE(x,y)  ((y)(x))
#endif

#define REFOBJECT_FROM_TYPE(x)  ((refobject_t)(refobject_base_t*)(x))

#define REFOBJECT_GET_TYPE(x)       (REFOBJECT_GET_BASE((x)) == NULL ? NULL : REFOBJECT_GET_BASE((x))->type)
#define REFOBJECT_GET_TYPENAME(x)   (REFOBJECT_GET_TYPE((x)) == NULL ? NULL : REFOBJECT_GET_TYPE((x))->type_name)

#define REFOBJECT_IS_VALID(x,type)  (!REFOBJECT_IS_NULL((x)) && REFOBJECT_GET_TYPE((x)) == (refobject_type__ ## type))

#define REFOBJECT_CONTROL_VERSION                   1
#define REFOBJECT_FORWARD_TYPE(type)                extern const refobject_type_t * refobject_type__ ## type;
#define REFOBJECT_DEFINE_TYPE__RAW(type, ...)       \
static const refobject_type_t refobject_typedef__ ## type = \
{ \
    .control_length = sizeof(refobject_type_t), \
    .control_version = REFOBJECT_CONTROL_VERSION, \
    .type_length = sizeof(type), \
    .type_name = # type \
    , ## __VA_ARGS__ \
}
#define REFOBJECT_DEFINE_TYPE(type, ...)            REFOBJECT_DEFINE_TYPE__RAW(type, ## __VA_ARGS__); const refobject_type_t * refobject_type__ ## type = &refobject_typedef__ ## type
#define REFOBJECT_DEFINE_PRIVATE_TYPE(type, ...)    REFOBJECT_DEFINE_TYPE__RAW(type, ## __VA_ARGS__); static const refobject_type_t * refobject_type__ ## type = &refobject_typedef__ ## type
#define REFOBJECT_DEFINE_TYPE_FREE(cb)              .type_freecb = (cb)

/* Type used for callback called then the object is actually freed
 * That is once all references to it are gone.
 *
 * If the callback does not set *userdata to NULL *userdata will
 * be freed automatically by calling free(3).
 *
 * This function must not try to deallocate or alter self.
 */
typedef void (*refobject_free_t)(refobject_t self, void **userdata);

/* Meta type used to defined types.
 * DO NOT use any of the members in here directly!
 */

typedef struct {
    /* Size of this control structure */
    size_t              control_length;
    /* ABI version of this structure */
    int                 control_version;

    /* Total length of the objects to be created */
    size_t              type_length;
    /* Name of type */
    const char *        type_name;
    /* Callback to be called on final free() */
    refobject_free_t    type_freecb;
} refobject_type_t;

/* Only defined here as the size must be publically known.
 * DO NOT use any of the members in here directly!
 */
struct refobject_base_tag {
    const refobject_type_t* type;
    size_t refc;
    mutex_t lock;
    void *userdata;
    char *name;
    refobject_t associated;
};

REFOBJECT_FORWARD_TYPE(refobject_base_t);

/* Create a new refobject
 * The total length of the new object is given by len (see malloc(3)),
 * the callback called on free is given by freecb (see refobject_free_t above),
 * the userdata us given by userdata,
 * the name for the object is given by name, and
 * the associated refobject is given by associated.
 *
 * All parameters beside len are optional and can be NULL/REFOBJECT_NULL.
 * If no freecb is given the userdata is freed (see refobject_free_t above).
 */
#define         refobject_new__new(type, userdata, name, associated) REFOBJECT_TO_TYPE(refobject_new__real((refobject_type__ ## type), (userdata), (name), (associated)), type*)
refobject_t     refobject_new__real(const refobject_type_t *type, void *userdata, const char *name, refobject_t associated);

/* This increases the reference counter of the object */
int             refobject_ref(refobject_t self);
/* This decreases the reference counter of the object.
 * If the object's reference counter reaches zero the object is freed.
 */
int             refobject_unref(refobject_t self);

/* This gets and sets the userdata */
void *          refobject_get_userdata(refobject_t self);
int             refobject_set_userdata(refobject_t self, void *userdata);

/* This gets the object's name */
const char *    refobject_get_name(refobject_t self);

/* This gets the object's associated object. */
refobject_t     refobject_get_associated(refobject_t self);

#endif
