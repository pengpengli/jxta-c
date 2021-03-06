/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id$
 */

static const char *__log_cat = "bytevector";

#include <stdlib.h>

#include "jxta_apr.h"
/* #include "jxta_object.h" */
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_bytevector.h"
#include "jxta_objecthashtable.h"

#ifdef WIN32
#pragma warning ( once : 4514)
#endif

 /**********************************************************************
 ** This file implements Jxta_bytevector. Please refer to jxta_bytevector.h
 ** for detail on the API.
 **********************************************************************/

#define INDEX_DEFAULT_SIZE 128
#define INDEX_DEFAULT_INCREASE 64

typedef unsigned char _byte_t;

struct _jxta_bytevector {
    JXTA_OBJECT_HANDLE;
    struct {
        _byte_t *content;
        size_t size;
        size_t capacity;
        apr_thread_mutex_t *mutex;
        apr_pool_t *jpr_pool;
    } usr;
};

typedef struct _jxta_bytevector Jxta_bytevector_mutable;

static Jxta_status ensure_capacity(Jxta_bytevector_mutable * vector, size_t requiredSize);
static void jxta_bytevector_free(Jxta_object * vector);

/**
 **/
static void jxta_bytevector_free(Jxta_object * vector)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (NULL == self)
        return;

    /* Free the contents */
    free(self->usr.content);

    /* Free the pool containing the mutex */
    if (NULL != self->usr.mutex) {
        apr_thread_mutex_destroy(self->usr.mutex);
        apr_pool_destroy(self->usr.jpr_pool);
    }

    /* Free the object itself */
    free(self);
}

static void jxta_bytevector_free_nothing(Jxta_object * vector)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (NULL == self)
        return;

    /* Free the pool containing the mutex */
    if (NULL != self->usr.mutex) {
        apr_thread_mutex_destroy(self->usr.mutex);
        apr_pool_destroy(self->usr.jpr_pool);
    }

    /* Free the object itself */
    free(self);
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_bytevector *) jxta_bytevector_new_0(void)
{
    return jxta_bytevector_new_1(INDEX_DEFAULT_SIZE);
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_bytevector *) jxta_bytevector_new_1(size_t initialSize)
{
    Jxta_bytevector_mutable *self;

    /* Create the vector object */
    self = (Jxta_bytevector_mutable *) malloc(sizeof(Jxta_bytevector_mutable));
    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    memset(self, 0, sizeof(Jxta_bytevector_mutable));

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, jxta_bytevector_free, 0);

    self->usr.content = (_byte_t *) calloc(initialSize + 1, sizeof(_byte_t));
    self->usr.capacity = initialSize;
    self->usr.size = 0;
    self->usr.jpr_pool = NULL;
    self->usr.mutex = NULL;

    return self;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_bytevector *) jxta_bytevector_new_2(unsigned char *initialPtr, size_t initialSize, size_t initialCapacity)
{
    Jxta_bytevector_mutable *self;

    if (NULL == initialPtr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "initial ptr must me non-NULL");
        return NULL;
    }

    if (initialCapacity == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Capacity must be greater than zero");
        return NULL;
    }

    if (initialCapacity < initialSize) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Capacity must be greater or equal to Size");
        return NULL;
    }

    /* Create the vector object */
    self = (Jxta_bytevector_mutable *) malloc(sizeof(Jxta_bytevector_mutable));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "object allocation failed.");
        return NULL;
    }

    memset(self, 0, sizeof(Jxta_bytevector_mutable));

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, jxta_bytevector_free, 0);

  /** 
   *  We require that the pointer be a malloc ptr and be completely given to us
   *  therefore, we are fully in our rights to do a realloc() on in. This realloc
   *  is a test. If we don't get a good pointer back, we don't continue.
   **/
    if (initialCapacity > initialSize) {
        initialPtr = (unsigned char *) realloc(initialPtr, initialCapacity);

        if (NULL == initialPtr) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "realloc failed, probably not a malloc ptr.");
            free(self);
            self = NULL;
            return NULL;
        }
    }

    self->usr.content = initialPtr;
    self->usr.capacity = initialCapacity;
    self->usr.size = initialSize;
    self->usr.jpr_pool = NULL;
    self->usr.mutex = NULL;

    return self;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_bytevector *) jxta_bytevector_new_3(char *content, size_t length, Jxta_boolean freedata)
{
    Jxta_bytevector_mutable *self;

    /* Create the vector object */
    self = (Jxta_bytevector_mutable *) malloc(sizeof(Jxta_bytevector_mutable));
    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    memset(self, 0, sizeof(Jxta_bytevector_mutable));

    /* Initialize it. */
    if (freedata)
        JXTA_OBJECT_INIT(self, jxta_bytevector_free, 0);
    else
        JXTA_OBJECT_INIT(self, jxta_bytevector_free_nothing, 0);

    self->usr.content = (_byte_t *) content;
    self->usr.capacity = length;
    self->usr.size = length;
    self->usr.jpr_pool = NULL;
    self->usr.mutex = NULL;


    return self;
}

/*************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_set_synchronized(Jxta_bytevector * vector)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;
    apr_status_t res;

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        return JXTA_SUCCESS;

    /*
     * Allocate the mutex 
     * Note that a pool is created for each vector. This allows to have a finer
     * granularity when allocated the memory. This constraint comes from the APR.
     */
    res = apr_pool_create(&self->usr.jpr_pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        return JXTA_NOMEM;
    }
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->usr.mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->usr.jpr_pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->usr.jpr_pool);
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_clear(Jxta_bytevector * vector, size_t initialSize)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;
    Jxta_status res;

    /* Create the index */
    if (initialSize == 0)
        /* Use the default value instead */
        initialSize = INDEX_DEFAULT_SIZE;

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    if (NULL != self->usr.content)
        free(self->usr.content);
    self->usr.content = NULL;
    self->usr.capacity = 0;
    self->usr.size = 0;

    res = ensure_capacity(self, initialSize);

    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    return res;
}

/************************************************************************
 ** A new index needs to be created, increased by size.
 ** The old index is copied into the new one.
 ** The old index is freed.
 ** The caller of this function must make sure to be holding the mutex.
 *************************************************************************/
static Jxta_status ensure_capacity(Jxta_bytevector_mutable * vector, size_t requiredSize)
{
    _byte_t *newContent;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (requiredSize < vector->usr.capacity)
        return JXTA_SUCCESS;

    requiredSize =
        requiredSize >
        (vector->usr.capacity + INDEX_DEFAULT_INCREASE) ? requiredSize : vector->usr.capacity + INDEX_DEFAULT_INCREASE;

    /* Allocate a new index */
    if (NULL == vector->usr.content)
        newContent = (_byte_t *) malloc(sizeof(_byte_t) * requiredSize);
    else
        newContent = (_byte_t *) realloc(vector->usr.content, sizeof(_byte_t) * requiredSize);

    if (NULL == newContent)
        return JXTA_NOMEM;

    vector->usr.content = newContent;
    memset(vector->usr.content + (ptrdiff_t) vector->usr.capacity, 0, (sizeof(_byte_t) * (requiredSize - vector->usr.capacity)));
    vector->usr.capacity = requiredSize;

    return JXTA_SUCCESS;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(const char *) jxta_bytevector_content_ptr(Jxta_bytevector * vector)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    return (char *) self->usr.content;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_add_byte_at(Jxta_bytevector * vector, unsigned char byte, unsigned int at_index)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    err = jxta_bytevector_add_bytes_at(vector, &byte, at_index, sizeof(_byte_t));
    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_add_byte_first(Jxta_bytevector * vector, unsigned char byte)
{
    return jxta_bytevector_add_byte_at(vector, byte, 0);
}


/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_add_byte_last(Jxta_bytevector * vector, unsigned char byte)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);
    err = jxta_bytevector_add_byte_at(vector, byte, vector->usr.size);
    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_add_bytes_at(Jxta_bytevector * vector, unsigned char const *bytes,
                                                       unsigned int at_index, size_t length)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;
    Jxta_status res;

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    if (at_index > self->usr.size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "index %d is out of bounds (size of vector= %d)\n", at_index, self->usr.size);
        res = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    res = ensure_capacity(self, (self->usr.size + length));
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not allocate suffcient space for elements\n");
        goto Common_Exit;
    }

    if (self->usr.size >= length)
        memmove(self->usr.content + (ptrdiff_t) (at_index + length),
                self->usr.content + (ptrdiff_t) at_index, (self->usr.size - at_index) * sizeof(_byte_t));


    if (NULL != bytes) {
        memmove(self->usr.content + (ptrdiff_t) (at_index * sizeof(_byte_t)), bytes, length * sizeof(_byte_t));
    } else {
        memset(self->usr.content + (ptrdiff_t) (at_index * sizeof(_byte_t)), 0, length * sizeof(_byte_t));
    }

    /* Increase the size */
    self->usr.size += length;
    res = JXTA_SUCCESS;

  Common_Exit:
    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    return res;
}


/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_add_bytevector_at(Jxta_bytevector * vector, Jxta_bytevector * bytes,
                                                            unsigned int dstindex)
{
    Jxta_status err;
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid target vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (!JXTA_OBJECT_CHECK_VALID(bytes)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid source vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    if (NULL != bytes->usr.mutex)
        apr_thread_mutex_lock(bytes->usr.mutex);

    err = jxta_bytevector_add_bytes_at(self, bytes->usr.content, dstindex, bytes->usr.size);

    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    if (NULL != bytes->usr.mutex)
        apr_thread_mutex_unlock(bytes->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_bytevector_add_partial_bytevector_at(Jxta_bytevector * vector,
                                          Jxta_bytevector * bytes, unsigned int srcIndex, size_t length, unsigned int dstindex)
{
    Jxta_status err;
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid target vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (!JXTA_OBJECT_CHECK_VALID(bytes)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid source vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    if (NULL != bytes->usr.mutex)
        apr_thread_mutex_lock(bytes->usr.mutex);

    if (srcIndex >= bytes->usr.size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        err = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    length = length <= (bytes->usr.size - srcIndex) ? length : (bytes->usr.size - srcIndex);

    err = jxta_bytevector_add_bytes_at(self, bytes->usr.content + (ptrdiff_t) srcIndex, dstindex, length);

  Common_Exit:
    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    if (NULL != bytes->usr.mutex)
        apr_thread_mutex_unlock(bytes->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_bytevector_add_from_stream_at(Jxta_bytevector * vector, ReadFunc func, void *stream, size_t length, unsigned int at_index)
{
    Jxta_status err = JXTA_SUCCESS;
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (func == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    err = jxta_bytevector_add_bytes_at(self, NULL, at_index, length);

    if (JXTA_SUCCESS != err)
        goto Common_Exit;

    err = (func) (stream, (char *) (self->usr.content + (ptrdiff_t) at_index), length);

    if (JXTA_SUCCESS != err) {
        Jxta_status res = jxta_bytevector_remove_bytes_at(self, at_index, length);
        if (JXTA_SUCCESS == res)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Vector is probably corrupted.");
        goto Common_Exit;
    }

  Common_Exit:
    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    return err;
}

/************************************************************************
 ** No need to add bytes as the bytevector should have been correctly 
 ** allocated 
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_bytevector_set_from_stream_at(Jxta_bytevector * vector, ReadFunc func, void *stream, size_t length, unsigned int at_index)
{
    Jxta_status err = JXTA_SUCCESS;
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (func == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (!JXTA_OBJECT_CHECK_VALID(self)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (length > (self->usr.capacity - at_index)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid length");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    err = (func) (stream, (char *) (self->usr.content + (ptrdiff_t) at_index), length);

    self->usr.size += length;

    if (JXTA_SUCCESS != err) {
        Jxta_status res = jxta_bytevector_remove_bytes_at(self, at_index, length);
        if (JXTA_SUCCESS == res)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Vector is probably corrupted.");
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_get_byte_at(Jxta_bytevector * vector, unsigned char *byte, unsigned int at_index)
{
    return jxta_bytevector_get_bytes_at(vector, byte, at_index, 1);
}

JXTA_DECLARE(Jxta_status) jxta_bytevector_get_bytes_at(Jxta_bytevector * vector, unsigned char *bytes, unsigned int at_index,
                                                       size_t length)
{
    return jxta_bytevector_get_bytes_at_1(vector, bytes, at_index, &length);
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_get_bytes_at_1(Jxta_bytevector * vector, unsigned char *bytes, unsigned int at_index,
                                                         size_t * length)
{
    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == bytes) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);

    if (at_index >= vector->usr.size) {
        if (NULL != vector->usr.mutex)
            apr_thread_mutex_unlock(vector->usr.mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    *length = (vector->usr.size - at_index) > *length ? *length : (vector->usr.size - at_index);

    memmove(bytes, (vector->usr.content + (ptrdiff_t) at_index), *length);

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_unlock(vector->usr.mutex);
    return JXTA_SUCCESS;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_bytevector_get_bytevector_at(Jxta_bytevector * source, Jxta_bytevector ** dest, unsigned int at_index, size_t length)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(source)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid source vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == dest) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid dest vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != source->usr.mutex)
        apr_thread_mutex_lock(source->usr.mutex);

    if (at_index >= source->usr.size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        err = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    length = (source->usr.size - at_index) > length ? length : (source->usr.size - at_index);

    /* Create a new vector */
    *dest = jxta_bytevector_new_1(length);
    if (*dest == NULL) {
        err = JXTA_NOMEM;
        goto Common_Exit;
    }

    err = jxta_bytevector_add_bytes_at(*dest, source->usr.content + (ptrdiff_t) at_index, 0, length);

  Common_Exit:
    if (NULL != source->usr.mutex)
        apr_thread_mutex_unlock(source->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_write(Jxta_bytevector * vector, WriteFunc func, void *stream, unsigned int at_index,
                                                size_t length)
{
    Jxta_status err;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == func) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);

    if (at_index >= vector->usr.size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        err = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    length = (vector->usr.size - at_index) > length ? length : (vector->usr.size - at_index);

    err = (func) (stream, (char *) (vector->usr.content + (ptrdiff_t) at_index), length);

  Common_Exit:
    if (NULL != vector->usr.mutex)
        apr_thread_mutex_unlock(vector->usr.mutex);

    return err;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_remove_byte_at(Jxta_bytevector * vector, unsigned int at_index)
{
    return jxta_bytevector_remove_bytes_at(vector, at_index, 1);
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_bytevector_remove_bytes_at(Jxta_bytevector * vector, unsigned int at_index, size_t length)
{
    Jxta_bytevector_mutable *self = (Jxta_bytevector_mutable *) vector;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->usr.mutex)
        apr_thread_mutex_lock(self->usr.mutex);

    if (at_index >= self->usr.size) {
        if (NULL != self->usr.mutex)
            apr_thread_mutex_unlock(self->usr.mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    length = (self->usr.size - at_index) > length ? length : (self->usr.size - at_index);

    memmove(self->usr.content + (ptrdiff_t) at_index,
            self->usr.content + (ptrdiff_t) (at_index + length), self->usr.size - at_index - length);

    self->usr.size -= length;

    if (NULL != self->usr.mutex)
        apr_thread_mutex_unlock(self->usr.mutex);

    return JXTA_SUCCESS;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(size_t) jxta_bytevector_size(Jxta_bytevector * vector)
{
    size_t size;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);

    size = vector->usr.size;

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_unlock(vector->usr.mutex);

    return size;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_bytevector_equals(Jxta_bytevector * vector1, Jxta_bytevector * vector2)
{
    Jxta_boolean result;

    if (vector1 == vector2)
        return TRUE;

    if ((NULL == vector1) || (NULL == vector2))
        return FALSE;

    if (!JXTA_OBJECT_CHECK_VALID(vector1)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return FALSE;
    }

    if (!JXTA_OBJECT_CHECK_VALID(vector2)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
        return FALSE;
    }

    if (NULL != vector1->usr.mutex)
        apr_thread_mutex_lock(vector1->usr.mutex);

    if (NULL != vector2->usr.mutex)
        apr_thread_mutex_lock(vector2->usr.mutex);

    if (vector1->usr.size != vector2->usr.size)
        result = FALSE;
    else
        result = (0 == memcmp(vector1->usr.content, vector2->usr.content, vector1->usr.size)) ? TRUE : FALSE;


    if (NULL != vector1->usr.mutex)
        apr_thread_mutex_unlock(vector1->usr.mutex);

    if (NULL != vector2->usr.mutex)
        apr_thread_mutex_unlock(vector2->usr.mutex);

    return result;
}

/************************************************************************
 **
 *************************************************************************/
JXTA_DECLARE(unsigned int) jxta_bytevector_hashcode(Jxta_bytevector * vector)
{
    unsigned int hashcode;

    if (!JXTA_OBJECT_CHECK_VALID(vector)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "invalid vector");
		
        return 0;
    }

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_lock(vector->usr.mutex);

    hashcode = jxta_objecthashtable_simplehash(vector->usr.content, vector->usr.size);

    if (NULL != vector->usr.mutex)
        apr_thread_mutex_unlock(vector->usr.mutex);

    return hashcode;
}
