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
 * $Id: jxta_discovery_service.c,v 1.13 2005/01/12 22:37:39 bondolo Exp $
 */

#include "jxta_discovery_service_private.h"

/**
 * The base discovery service ctor (not public: the only public way to make a
 * new pg is to instantiate one of the derived types).
 */
void jxta_discovery_service_construct(Jxta_discovery_service* service,
                                      Jxta_discovery_service_methods* methods) {

    PTValid(methods, Jxta_discovery_service_methods);
    jxta_service_construct((Jxta_service*) service,
                           (Jxta_service_methods*) methods);
    service->thisType = "Jxta_discovery_service";
}

/**
 * The base rsesolver service dtor (Not public, not virtual. Only called by
 * subclassers). We just pass it along.
 */
void jxta_discovery_service_destruct(Jxta_discovery_service* service) {
    jxta_service_destruct((Jxta_service*) service);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_discovery_service_methods*) JXTA_MODULE_VTBL(service))


long
discovery_service_get_remote_advertisements(Jxta_discovery_service* service,
        Jxta_id* peerid,
        short type,
        const char* attribute,
        const char* value,
        int threshold,
        Jxta_discovery_listener* listener) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->getRemoteAdv(service, peerid, type, attribute, value, threshold, listener);
}

Jxta_status
discovery_service_get_local_advertisements(Jxta_discovery_service* service,
        short type,
        const char* attribute,
        const char* value,
        Jxta_vector** advertisements) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->getLocalAdv(service, type, attribute, value, advertisements);
}

Jxta_status
discovery_service_publish(Jxta_discovery_service* service,
                          Jxta_advertisement* adv,
                          short type,
                          Jxta_expiration_time lifetime,
                          Jxta_expiration_time lifetimeForOthers) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->publish(service, adv, type, lifetime, lifetimeForOthers);

}

Jxta_status
discovery_service_remote_publish(Jxta_discovery_service* service,
                                 Jxta_id* peerid,
                                 Jxta_advertisement* adv,
                                 short type,
                                 Jxta_expiration_time expirationtime) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->remotePublish(service, peerid, adv, type, expirationtime);
}


Jxta_status
discovery_service_flush_advertisements(Jxta_discovery_service* service,
                                       char * id,
                                       short type) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->flush(service, id, type);

}


Jxta_status
discovery_service_add_discovery_listener(Jxta_discovery_service* service,
        Jxta_discovery_listener* listener) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->add_listener(service, listener);

}


Jxta_status
discovery_service_remove_discovery_listener(Jxta_discovery_service* service,
        Jxta_discovery_listener* listener) {
    PTValid(service, Jxta_discovery_service);
    return VTBL->remove_listener(service, listener);

}

