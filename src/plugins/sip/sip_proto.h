
typedef struct 
{
  char * name;
  char * value;
  int name_len;
  int value_len;
} sip_header_t;


typedef enum {
  HDR_ERROR_F,
  HDR_EXTENSION_F,
  HDR_TO_F,               /*<< len: 2     */
  HDR_VIA_F,              /*<< len: 3     */
  HDR_CSEQ_F,             /*<< len: 4     */
  HDR_DATE_F,
  HDR_FROM_F,
  HDR_ALLOW_F,            /*<< len: 5     */
  HDR_ROUTE_F,            
  HDR_ACCEPT_F,           /*<< len: 6     */
  HDR_SERVER_F,
  HDR_CALL_ID_F,          /*<< len: 7     */
  HDR_CONTACT_F,
  HDR_EXPIRES_F,
  HDR_REQUIRE_F,
  HDR_SUBJECT_F,
  HDR_WARNING_F,
  HDR_PRIORITY_F,         /*<< len: 8     */
  HDR_REPLY_TO_F,
  HDR_CALL_INFO_F,        /*<< len: 9     */
  HDR_SUPPORTED_F,
  HDR_TIMESTAMP_F,
  HDR_ALERT_INFO_F,       /*<< len: 10    */
  HDR_ERROR_INFO_F,
  HDR_USER_AGENT_F,
  HDR_IN_REPLY_TO_F,      /*<<  len: 11   */
  HDR_MIN_EXPIRES_F,
  HDR_RETRY_AFTER_F,
  HDR_UNSUPPORTED_F,
  HDR_CONTENT_TYPE_F,     /*<< len: 12    */
  HDR_MAX_FORWARDS_F,
  HDR_MIME_VERSION_F,
  HDR_ORGANIZATION_F,
  HDR_RECORD_ROUTE_F,
  HDR_AUTHORIZATION_F,    /*<< len: 13    */
  HDR_PROXY_REQUIRE_F,
  HDR_CONTENT_LENGTH_F,         /*<< len: 14 */
  HDR_ACCEPT_ENCODING_F,        /*<< len: 15 */
  HDR_ACCEPT_LANGUAGE_F,
  HDR_CONTENT_ENCODING_F,       /*<< len: 16 */
  HDR_CONTENT_LANGUAGE_F,
  HDR_WWW_AUTHENTICATE_F,
  HDR_AUTHENTICATE_INFO_F,      /*<< len: 17 */
  HDR_PROXY_AUTHENTICATE_F,     /*<< len: 18 */
  HDR_PROXY_AUTHORIZATION_F,    /*<< len: 19 */
  HDR_CONTENT_DISPOSITION_F
} hdr_field_t;

enum {
  LEN_1 = 1,
  LEN_2,
  LEN_3,
  LEN_4,
  LEN_5,
  LEN_6,
  LEN_7,
  LEN_8,
  LEN_9,
  LEN_10,
  LEN_11,
  LEN_12,
  LEN_13,
  LEN_14,
  LEN_15,
  LEN_16,
  LEN_17,
  LEN_18,
  LEN_19
};
