/*
 * Copyright (c) 2017 Choonho Son and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sip/sip_parser.h>


/*
 * SIP-message  = Request / Response
 * Request      = Request-Line
 *                *( message-header )
 *                CRLF
 *                [ message-body ]
 * Request-Line = Method SP Request-URI SIP-Version CRLF
 * Request-URI  = SIP-URI / SIPS-URI / absoluteURI
 *
 * Response     = Status-Line
 * Status-Line  = SIP-Version SP Status-Code SP Reason-Phrase CRLF
 */
int
parse_msg(u8 *request, u32 req_len, msg_entry_t* me)
{
  int rv;
  char * tmp;
  char * second; 
  char * third;
  char * end;
  //char * hdr;
  char * field;
  char * value;
  char * d1, * d2, * d3;
  u8 offset;
  u8 value_len;
  msg_start_t *ms = (msg_start_t *)me;
  rv = 1;
  tmp = (char *)request;
  end = tmp + req_len;

  /* Parse Request-Line */
  if (  (*tmp=='S' || *tmp=='s') &&
    strncasecmp( tmp+1, SIP_VERSION+1, SIP_VERSION_LEN-1)==0 &&
    (*(tmp+SIP_VERSION_LEN)==' ')) {
      ms->type=SIP_RESPONSE;
      tmp = tmp + SIP_VERSION_LEN;
      rv = 0;
  } else if_sip_method( SUBSCRIBE, 'S')
  else if_sip_method( REGISTER, 'R')
  else if_sip_method( MESSAGE, 'M' )
  else if_sip_method( OPTIONS, 'O' )
  else if_sip_method( PUBLISH, 'P' )
  else if_sip_method( CANCEL, 'C')
  else if_sip_method( INVITE, 'I' )
  else if_sip_method( NOTIFY, 'N' )
  else if_sip_method( UPDATE, 'U' )
  else if_sip_method( PRACK, 'P' )
  else if_sip_method( REFER, 'R' )
  else if_sip_method( INFO, 'I' )
  else if_sip_method( ACK, 'A' )
  else if_sip_method( BYE, 'B' )

  else {
    /* This may be unkown method, */
    tmp = next_token(tmp, end);
    if ( (tmp == (char *)request) || (tmp >= end) ) 
      {
      goto invalid;
      }
    /* if unknown method, next token should be Space */
    if (*tmp != ' ')
      {
      goto invalid;
      }
    /* This is unknown method */
    ms->type = SIP_REQUEST;
    ms->u.request.method = UNKNOWN_METHOD;
    ms->u.request.method_len = tmp - (char *)request;
  }

  /* Parse second token */
  second = tmp + 1;
  tmp = next_token(second, end);

  if ( (tmp - second) > 256 ) goto invalid;
  offset = tmp - second;

  if ( (*tmp != ' ') || (tmp == second) || (tmp >= end) )
    {
      goto invalid;
    }
  if (ms->type & SIP_RESPONSE)
    {
      /* Every status_code is 3 digit characters */
      if ( *(second + 3) != ' ') {
       goto invalid;
      }
      d1 = (second);
      d2 = (second + 1);
      d3 = (second + 2);
      if (*d1 >= '0' && *d1 <= 9 &&
          *d2 >= '0' && *d2 <= 9 &&
          *d3 >= '0' && *d3 <= 9 ) 
        {
        /* Status_code SP Reason_Phrase */
        ms->u.response.status = second;
        ms->u.response.status_code = (*d1-'0')*100 + (*d2-'0')*10 + (*d3-'0');
        }
      else
        {
          goto invalid;
        }
    }
  else  /* SIP_REQUEST */
    {
      // TODO: 2. URI scheme
      /* If the Request-URI has a URI whose scheme is not understood by the
        proxy, the proxy SHOULD reject the request with a 416 (Unsupported
        URI Scheme) response.
      */
      ms->u.request.uri = second;
      ms->u.request.uri_len = offset;
    }

  third = tmp + 1;
  tmp = next_token(third, end);
  if ( (tmp - third) > 256 ) goto invalid;
  offset = tmp - third;

  if ( (*tmp != '\r' && *(tmp+1) != '\n') ) 
    {
    goto invalid;
    }

  if (ms->type & SIP_RESPONSE)
    {
      ms->u.response.reason = third;
      ms->u.response.reason_len = offset;
    }
  else /* SIP_REQUEST */
    {
    if (  (*third=='S' || *third=='s') &&
      strncasecmp( third+1, SIP_VERSION+1, SIP_VERSION_LEN-1)==0 &&
      (offset != SIP_VERSION_LEN)) {
        goto invalid; 
      }
    }

  /* End of start line */
  field = tmp + 2;
  while (tmp < end) 
    {
    tmp = find_header(field, end);
    offset = tmp - field;
    value = skip_sp(tmp+1, end);
    tmp = next_token(value, end);
    value_len = tmp - value;
    switch(offset)
      {
      case LEN_1:
        break;
      case LEN_2:
        if (strncasecmp (field, "to", 2) == 0)
          {
            me->fields.to.name = field;
            me->fields.to.value = value;
            me->fields.to.name_len = offset;
            me->fields.to.value_len = value_len;
          }
        break;
      case LEN_3:
        if (strncasecmp (field, "via", 3) == 0)
          {
            me->fields.via.name = field;
            me->fields.via.value = value;
            me->fields.via.name_len = offset;
            me->fields.via.value_len = value_len;
          }
        break;
      case LEN_4:
        if (strncasecmp (field, "from", 4) == 0)
          {
            me->fields.from.name = field;
            me->fields.from.value = value;
            me->fields.from.name_len = offset;
            me->fields.from.value_len = value_len;
          }
        break;
      }
    tmp = tmp + 2;
    field = tmp;
    }
  goto out;

invalid:
  ms->type = SIP_INVALID;
  rv = -1;

out:
  return rv;
}
