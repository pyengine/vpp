
#ifndef __SIP_PARSER_H_
#define __SIP_PARSER_H_

#include <sip/sip.h>

#include <vnet/vnet.h>

#include <strings.h>

#define SIP_VERSION "SIP/2.0"
#define SIP_VERSION_LEN 7

#define SUBSCRIBE "SUBSCRIBE"
#define REGISTER  "REGISTER"
#define MESSAGE   "MESSAGE"
#define OPTIONS   "OPTIONS"
#define PUBLISH   "PUBLISH"
#define CANCEL    "CANCEL"
#define INVITE    "INVITE"
#define NOTIFY    "NOTIFY"
#define UPDATE    "UPDATE"
#define PRACK     "PRACK"
#define REFER     "REFER"
#define INFO      "INFO"
#define ACK       "ACK"
#define BYE       "BYE"

#define SUBSCRIBE_LEN 9
#define REGISTER_LEN  8
#define MESSAGE_LEN   7
#define OPTIONS_LEN   7
#define PUBLISH_LEN   7
#define CANCEL_LEN    6
#define INVITE_LEN    6
#define NOTIFY_LEN    6
#define UPDATE_LEN    6
#define PRACK_LEN     5
#define REFER_LEN     5
#define INFO_LEN      4
#define ACK_LEN       3
#define BYE_LEN       3



#define if_sip_method(methodname,firstchar)                               \
if (  (*tmp==(firstchar) || *tmp==((firstchar) | 32)) &&                  \
        strncasecmp( tmp+1, #methodname +1, methodname##_LEN-1)==0 &&     \
        *(tmp+methodname##_LEN)==' ') {                                   \
                ms->type=SIP_REQUEST;                                     \
                ms->u.request.method=methodname##_METHOD;                 \
                ms->u.request.method_len=methodname##_LEN;                \
                tmp = tmp + methodname##_LEN;                             \
}

inline static char * next_token(const char * p, const char * pend)
{
  for (;(p<pend)&&(*p!=' ')&&(*p!='\t')&&(*p!='\n')&&(*p!='\r'); ++p);
  return (char *)p;
}

inline static char * skip_sp(const char * p, const char * pend)
{
  for (;(p<pend)&&(*p==' ' || *p=='\t');++p);
    return (char *)p;
}

inline static char * find_header(const char * p, const char * pend)
{
  for (;(p<pend)&&(*p!=':'); ++p);
  return (char *)p;
}

inline static char * find_eol(const char * p, const char * pend)
{
  for (;(p<pend)&&(*p!='\r'); ++p);
  return (char *)p;
}

#endif
