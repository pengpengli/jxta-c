/*
 * Copyright(c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
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

#ifndef __JXTA_BIDIPIPE_H__
#define __JXTA_BIDIPIPE_H__

#include "jxta_types.h"
#include "jxta_module.h"
#include "jxta_pipe_adv.h"
#include "jxta_message.h"
#include "jxta_listener.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0                           /* to cheat indent */
};
#endif

typedef struct Jxta_bidipipe Jxta_bidipipe;

enum Jxta_bidipipe_states {
    JXTA_BIDIPIPE_CLOSED = 0,
    JXTA_BIDIPIPE_ACCEPTING,
    JXTA_BIDIPIPE_CONNECTING,
    JXTA_BIDIPIPE_CONNECTED,
    JXTA_BIDIPIPE_CLOSING
};

typedef enum Jxta_bidipipe_states Jxta_bidipipe_state;

JXTA_DECLARE(Jxta_bidipipe *) jxta_bidipipe_new(Jxta_PG * pg);
Jxta_status jxta_bidipipe_delete(Jxta_bidipipe * self);

JXTA_DECLARE(Jxta_status) jxta_bidipipe_connect(Jxta_bidipipe * self, Jxta_pipe_adv * remote_adv, Jxta_listener * listener,
                                                Jxta_time_diff timeout);
JXTA_DECLARE(Jxta_status) jxta_bidipipe_close(Jxta_bidipipe * self);

JXTA_DECLARE(Jxta_status) jxta_bidipipe_accept(Jxta_bidipipe * self, Jxta_pipe_adv * local_adv, Jxta_listener * listener);

JXTA_DECLARE(Jxta_status) jxta_bidipipe_send(Jxta_bidipipe * self, Jxta_message * msg);

JXTA_DECLARE(Jxta_bidipipe_state) jxta_bidipipe_get_state(Jxta_bidipipe * self);
JXTA_DECLARE(Jxta_pipe_adv *) jxta_bidipipe_get_pipe_adv(Jxta_bidipipe * self);

#if 0   /* to cheat indent */
{
#endif
#ifdef __cplusplus
}   /* extern "C" */
#endif
#endif /* __JXTA_BIDIPIPE_H__ */
/* vi: set sw=4 ts=4 et wm=130: */
