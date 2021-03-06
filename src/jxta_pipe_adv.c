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
 * $Id$
 */


/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c DiscoveryResponse.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_pipe_adv.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    PipeAdvertisement_,
    Id_,
    Type_,
    Name_,
    Desc_
};

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */

struct _jxta_pipe_adv {
    Jxta_advertisement jxta_advertisement;
    char *Id;
    char *Type;
    char *Name;
    char *Desc;
};

void jxta_pipe_adv_delete(Jxta_pipe_adv *);

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handlePipeAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_pipe_adv * ad = (Jxta_pipe_adv*)userdata; */
}

static void handleId(void *userdata, const XML_Char * cd, int len)
{
    Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0)
        jxta_pipe_adv_set_Id(ad, tok);

    free(tok);
}

static void handleType(void *userdata, const XML_Char * cd, int len)
{
    Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;
    char *tok = (char *) malloc(len + 1);

    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0)
        jxta_pipe_adv_set_Type(ad, tok);
    
    free(tok);
}

static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;
    char *tok = (char *) calloc(len + 1, sizeof(char));

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0)
        jxta_pipe_adv_set_Name(ad, tok);
    
    free(tok);
}

static void handleDesc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_pipe_adv *ad = (Jxta_pipe_adv *) userdata;
    JString * string;

    if(len == 0)
        return;

    string = jstring_new_0();
    jstring_append_0(string, cd, len);
    jstring_trim(string);

    if(jstring_length(string) > 0)
        jxta_pipe_adv_set_Desc(ad, jstring_get_string(string));

    JXTA_OBJECT_RELEASE(string);
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

JXTA_DECLARE(Jxta_id *) jxta_pipe_adv_get_pipeid(Jxta_pipe_adv * ad)
{
    Jxta_id *pipeid = NULL;
    JString *tmps = jstring_new_2(ad->Id);

    jxta_id_from_jstring(&pipeid, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    return pipeid;
}

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Id(Jxta_pipe_adv * ad)
{
    return ad->Id;
}

char *JXTA_STDCALL jxta_pipe_adv_get_Id_string(Jxta_advertisement * ad)
{
    char *res;

    res = strdup(((Jxta_pipe_adv *) ad)->Id);
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Id(Jxta_pipe_adv * ad, const char *val)
{
    if (val != NULL) {
        if (NULL != ad->Id) {
            free(ad->Id);
        }
        ad->Id = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Id, val);
    } else {
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Type(Jxta_pipe_adv * ad)
{
    return ad->Type;
}

char *JXTA_STDCALL jxta_pipe_adv_get_Type_string(Jxta_advertisement * ad)
{
    return strdup(((Jxta_pipe_adv *) ad)->Type);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Type(Jxta_pipe_adv * ad, const char *val)
{
    if (val != NULL) {
        if(ad->Type){
            free(ad->Type);
        }
        
        ad->Type = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Type, val);
    } else {
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Name(Jxta_pipe_adv * ad)
{
    return ad->Name;
}

char *JXTA_STDCALL jxta_pipe_adv_get_Name_string(Jxta_advertisement * ad)
{
    if (((Jxta_pipe_adv *) ad)->Name != NULL) {
        return strdup(((Jxta_pipe_adv *) ad)->Name);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Name(Jxta_pipe_adv * ad, const char *val)
{
    if (val != NULL) {
        if(ad->Name){
            free(ad->Name);
        }

        ad->Name = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Name, val);
    } else {
        return JXTA_INVALID_ARGUMENT;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Desc(Jxta_pipe_adv * ad)
{
    return ad->Desc;
}

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Desc(Jxta_pipe_adv * ad, const char *val)
{
    if (val != NULL) {
        if(ad->Desc){
            free(ad->Desc);
        }

        ad->Desc = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Desc, val);
    } else {
        return JXTA_INVALID_ARGUMENT;
    }
    return JXTA_SUCCESS;
}

/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab PipeAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PipeAdvertisement", PipeAdvertisement_, *handlePipeAdvertisement, NULL, NULL},
    {"Id", Id_, *handleId, jxta_pipe_adv_get_Id_string, NULL},
    {"Type", Type_, *handleType, jxta_pipe_adv_get_Type_string, NULL},
    {"Name", Name_, *handleName, jxta_pipe_adv_get_Name_string, NULL},
    {"Desc", Desc_, *handleDesc, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_get_xml(Jxta_pipe_adv * ad, JString ** xml)
{
    JString *string;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:PipeAdvertisement>\n");

    jstring_append_2(string, "<jxta:PipeAdvertisement xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(string, "<Id>");
    jstring_append_2(string, jxta_pipe_adv_get_Id(ad));
    jstring_append_2(string, "</Id>\n");

    jstring_append_2(string, "<Type>");
    jstring_append_2(string, jxta_pipe_adv_get_Type(ad));
    jstring_append_2(string, "</Type>\n");

    jstring_append_2(string, "<Name>");
    jstring_append_2(string, jxta_pipe_adv_get_Name(ad));
    jstring_append_2(string, "</Name>\n");

    if(jxta_pipe_adv_get_Desc(ad)){
        jstring_append_2(string, "<Desc>");
        jstring_append_2(string, jxta_pipe_adv_get_Desc(ad));
        jstring_append_2(string, "</Desc>\n");
    }

    jstring_append_2(string, "</jxta:PipeAdvertisement>\n");
    *xml = string;
    return JXTA_SUCCESS;
}

/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_pipe_adv *) jxta_pipe_adv_new(void)
{
    Jxta_pipe_adv *ad;

    ad = (Jxta_pipe_adv *) malloc(sizeof(Jxta_pipe_adv));
    memset(ad, 0x0, sizeof(Jxta_pipe_adv));

    /*
       JXTA_OBJECT_INIT((Jxta_advertisement*)ad,jxta_pipe_adv_delete, 0);
     */

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:PipeAdvertisement",
                                  PipeAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_pipe_adv_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_pipe_adv_get_pipeid,
                                  (JxtaAdvertisementGetIndexFunc) jxta_pipe_adv_get_indexes, (FreeFunc) jxta_pipe_adv_delete);

    ad->Id = NULL;
    ad->Type = NULL;
    ad->Name = NULL;
    ad->Desc = NULL;

    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
void jxta_pipe_adv_delete(Jxta_pipe_adv * ad)
{
    /* Fill in the required freeing functions here. */
    if (ad->Id) {
        free(ad->Id);
        ad->Id = NULL;
    }

    if (ad->Type) {
        free(ad->Type);
        ad->Type = NULL;
    }

    if (ad->Name) {
        free(ad->Name);
        ad->Name = NULL;
    }

    if (ad->Desc) {
        free(ad->Desc);
        ad->Desc = NULL;
    }

    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0x00, sizeof(Jxta_pipe_adv));
    free(ad);
}

JXTA_DECLARE(void) jxta_pipe_adv_parse_charbuffer(Jxta_pipe_adv * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_pipe_adv_parse_file(Jxta_pipe_adv * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *) jxta_pipe_adv_get_indexes(Jxta_advertisement * dummy)
{
    static const char *idx[][2] = {
        {"Name", NULL},
        {"Id", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_pipe_adv *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_pipe_adv_new();

    testfile = fopen(argv[1], "r");
    jxta_pipe_adv_parse_file(ad, testfile);
    fclose(testfile);

    /* Jxta_pipe_adv_print_xml(ad,fprintf,stdout); */
    jxta_pipe_adv_delete(ad);

    return 0;
}
#endif
