/**
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
 *    Alternately, this acknowledgment may appear in the software itmyself,
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
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 **/


/**
 *
 * This is the ad-hoc edege Rendezvous Service implementation
 *
 * An ad-hoc edge is able to send and receive propagated messages. 
 */
static const char *__log_cat = "RdvAdhoc";

#include <limits.h>

#include "jxta_apr.h"

#include "jxta_log.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jstring.h"
#include "jxta_vector.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_pm.h"
#include "jxta_peergroup.h"
#include "jxta_peerview.h"
#include "jxta_id.h"
#include "jxta_lease_request_msg.h"
#include "jxta_lease_response_msg.h"
#include "jxta_peer_private.h"
#include "jxta_peerview_priv.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"
#include "jxta_endpoint_service_priv.h"

const int MAX_TTL = 2;

/**
    Public typedef for the JXTA Rendezvous service adhoc object.
**/
typedef Jxta_rdv_service_provider Jxta_rdv_service_adhoc;

/**
 ** This is the Rendezvous Service ad-hoc internal object definition
 ** The first field in the structure MUST be of type _jxta_rdv_service_provider
 ** since that is the generic interface of a Rendezvous Service. 
 ** This base structure is a Jxta_object that needs to be initialized.
 **/
struct _jxta_rdv_service_adhoc {
    Extends(_jxta_rdv_service_provider);

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    volatile Jxta_boolean running;

    /* configuration */

    char *groupiduniq;
    char *assigned_id_str;
    Jxta_RdvConfigAdvertisement *rdvConfig;
    Jxta_PID *localPeerId;

    /* state */
};

/**
    Private typedef for the JXTA Rendezvous service adhoc object.
**/
typedef struct _jxta_rdv_service_adhoc _jxta_rdv_service_adhoc;

/**
 ** Forward definition to make the compiler happy
 **/

static _jxta_rdv_service_adhoc *jxta_rdv_service_adhoc_construct(_jxta_rdv_service_adhoc * myself,
                                                                 const _jxta_rdv_service_provider_methods * methods);
static void adhoc_delete(Jxta_object * service);
static void adhoc_destruct(_jxta_rdv_service_adhoc * myself);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer** peer );
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam);

static const _jxta_rdv_service_provider_methods JXTA_RDV_SERVICE_ADHOC_METHODS = {
    "_jxta_rdv_service_provider_methods",
    init,
    start,
    stop,
    get_peer,
    get_peers,
    propagate,
    walk
};

/**
 * Standard instantiator
 **/
Jxta_rdv_service_provider *jxta_rdv_service_adhoc_new(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) calloc(1, sizeof(_jxta_rdv_service_adhoc));

    if (myself == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, adhoc_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service_provider *) jxta_rdv_service_adhoc_construct(myself, &JXTA_RDV_SERVICE_ADHOC_METHODS);
}


/**
 * standard constructor. This is a "final" class so it's static.
 **/
static _jxta_rdv_service_adhoc *jxta_rdv_service_adhoc_construct(_jxta_rdv_service_adhoc * myself,
                                                                 const _jxta_rdv_service_provider_methods * methods)
{
    Jxta_status res;
    myself = (_jxta_rdv_service_adhoc *) jxta_rdv_service_provider_construct((_jxta_rdv_service_provider *) myself, methods);

    if (NULL != myself) {
        /* Set our rt type checking string */
        myself->thisType = "_jxta_rdv_service_adhoc";

        myself->pool = ((_jxta_rdv_service_provider *) myself)->pool;

        res = apr_thread_mutex_create(&myself->mutex, APR_THREAD_MUTEX_NESTED, myself->pool);

        if (res != APR_SUCCESS) {
            return NULL;
        }

        /** The following will be updated with initialized **/
        myself->assigned_id_str = NULL;

        myself->running = FALSE;
    }

    return (_jxta_rdv_service_adhoc *) myself;
}

/**
 * Frees an instance of the Rendezvous Service.
 * All objects and memory used by the instance must first be released/freed.
 *
 * @param service a pointer to the instance of the Rendezvous Service to free.
 * @return error code.
 **/
static void adhoc_delete(Jxta_object * service)
{
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(service, _jxta_rdv_service_adhoc);

    /* call the hierarchy of dtors */
    adhoc_destruct(myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "ad-hoc rendezvous destruction finished [%pp].\n", myself);

    /* free the object itmyself */
    memset(myself, 0xDD, sizeof(_jxta_rdv_service_adhoc));

    free((void *) myself);
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
static void adhoc_destruct(_jxta_rdv_service_adhoc * myself)
{
    if (myself->rdvConfig != NULL)
        JXTA_OBJECT_RELEASE(myself->rdvConfig);

    /* Release the local peer id */
    JXTA_OBJECT_RELEASE(myself->localPeerId);

    /* Release our references */
    if (NULL != myself->assigned_id_str)
        free(myself->assigned_id_str);

    /* Free the string that contains the name of the instance */
    if (NULL != myself->groupiduniq)
        free(myself->groupiduniq);

    apr_thread_mutex_destroy(myself->mutex);

    /* call the base classe's dtor. */
    jxta_rdv_service_provider_destruct((_jxta_rdv_service_provider *) myself);
}

/**
 * Initializes an instance of the Rendezvous Service. (a module method)
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service.
 * @param group a pointer to the PeerGroup the Rendezvous Service is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * @return error code.
 */
static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(provider, _jxta_rdv_service_adhoc);
    JString *tmp = NULL;
    Jxta_PGID *gid;
    JString *string = NULL;
    Jxta_PA *conf_adv = NULL;
    Jxta_RdvConfigAdvertisement *rdvConfig = NULL;
    Jxta_PG *parentgroup;
    Jxta_id *assigned_id = jxta_service_get_assigned_ID_priv((Jxta_service *) service);
    Jxta_PG *group = jxta_service_get_peergroup_priv((Jxta_service *) service);

    jxta_rdv_service_provider_init(provider, service);

    jxta_id_to_jstring(assigned_id, &string);

    myself->assigned_id_str = strdup(jstring_get_string(string));

    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_PID(group, &(myself->localPeerId));

    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &tmp);
    JXTA_OBJECT_RELEASE(gid);
    myself->groupiduniq = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    jxta_PG_get_configadv(group, &conf_adv);

    if (NULL == conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;

        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);
        if (NULL != svc) {
            rdvConfig = jxta_svc_get_RdvConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }
    if (NULL == rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Loading defaults\n");
        rdvConfig = jxta_RdvConfigAdvertisement_new();
    }

    myself->rdvConfig = rdvConfig;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Ad-hoc initialized for group %s\n", myself->groupiduniq);

    return JXTA_SUCCESS;
}


/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically).
 *
 * @param provider The provider base class for which we are working.
 **/
static Jxta_status start(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(provider, _jxta_rdv_service_adhoc);

    jxta_rdv_service_provider_lock_priv(provider);

    res = jxta_rdv_service_provider_start(provider);

    myself->running = TRUE;

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_rdv_generate_event((_jxta_rdv_service *)
                            jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) myself),
                            JXTA_RDV_BECAME_RDV, myself->localPeerId);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Ad-hoc started for group %s\n", myself->groupiduniq);

    return JXTA_SUCCESS;
}

/**
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static Jxta_status stop(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(provider, _jxta_rdv_service_adhoc);

    jxta_rdv_service_provider_lock_priv(provider);

    if (TRUE != myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Rendezvous adhoc is not started, ignore stop request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        return APR_SUCCESS;
    }

    myself->running = FALSE;

    res = jxta_rdv_service_provider_stop(provider);

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_rdv_generate_event((_jxta_rdv_service *)
                            jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) myself),
                            JXTA_RDV_FAILED, myself->localPeerId);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Ad-hoc stopped for group %s\n", myself->groupiduniq);

    return res;
}

/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param provider a pointer to the instance of the Rendezvous Service
 * @param peerid The peerID of the peer sought.
 * @param peer The result.
 * @return error code.
 **/
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer** peer )
{
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(provider, _jxta_rdv_service_adhoc);

    /* Nothing is ever found. */
    return JXTA_ITEM_NOTFOUND;
}

/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 **/
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist)
{
    _jxta_rdv_service_adhoc *myself = (_jxta_rdv_service_adhoc *) PTValid(provider, _jxta_rdv_service_adhoc);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /*
     * Allocate a new vector and copy the peers from our own vector.
     */
    *peerlist = jxta_vector_new(0);

    if (NULL == *peerlist) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a new jxta_vector\n");
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

/**
 * Propagates a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param ttl Maximum number of peers the propagated message can go through.
 * @return error code.
 **/
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl)
{
    Jxta_status res;

    res = jxta_rdv_service_provider_update_prophdr(provider, msg, serviceName, serviceParam, ttl > MAX_TTL ? MAX_TTL : ttl);

    if (JXTA_SUCCESS == res) {
        /* Send the message to ourself */
        jxta_rdv_service_provider_prop_listener((Jxta_object *) msg, provider);

        res = jxta_rdv_service_provider_prop_to_peers(provider, msg, TRUE);
    }

    return res;
}

/**
 * Walks a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @return error code.
 **/
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam)
{
    Jxta_status res;

    res = jxta_rdv_service_provider_update_prophdr(provider, msg, serviceName, serviceParam, MAX_TTL);

    if (JXTA_SUCCESS == res) {
        res = jxta_rdv_service_provider_prop_to_peers(provider, msg, TRUE);
    }

    return res;
}

/* vim: set ts=4 sw=4 tw=130 et: */
