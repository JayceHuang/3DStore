#ifndef __HTTP_MINI_INTERFACE_H_20100701__
#define __HTTP_MINI_INTERFACE_H_20100701__

#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "utility/errcode.h"

#if 0
typedef enum t_http_resp_type
{
	HRT_STATUS_CODE = 0, 				
	HRT_HEADER,
	HRT_BODY,
	HRT_FINISHED
} HTTP_RESP_TYPE;

typedef struct t_http_resp_body
{
	const _u8 * _data;
	_u32 _data_len;
} HTTP_RESP_BODY;


typedef _int32 ( *http_resp_callback)(_u32 http_id,HTTP_RESP_TYPE type, void * resp);

typedef struct t_http_param
{
	const char * _url;
	_u32 _url_len;
	
	const _u8 * _data;	/* just for POST */
	_u32 _data_len;		/* just for POST */
	
	const char * _send_header;		/* ���ڷ���httpͷ�� */
	_u32  _send_header_len;		/* _send_header�Ĵ�С	 */
	
	BOOL  _gzip;					/* �Ƿ���ܻ���ѹ���ļ� */
	http_resp_callback   resp_callback;
} HTTP_PARAM;
#endif
/* HTTP�Ự�ӿڵ�������� */
typedef struct t_http_param
{
	char* _url;					/* ֻ֧��"http://" ��ͷ��url  */
	_u32 _url_len;
	
	char * _ref_url;				/* ����ҳ��URL*/
	_u32  _ref_url_len;		
	
	char * _cookie;			
	_u32  _cookie_len;		
	
	_u64  _range_from;			/* RANGE ��ʼλ��*/
	_u64  _range_to;			/* RANGE ����λ��*/
	
	_u64  _content_len;			/* Content-Length:*/
	
	BOOL  _send_gzip;			/* �Ƿ���ѹ������*/
	BOOL  _accept_gzip;			/* �Ƿ����ѹ���ļ� */
	
	_u8 * _send_data;				/* ָ����Ҫ�ϴ�������*/
	_u32  _send_data_len;			/* ��Ҫ�ϴ������ݴ�С*/

	void * _recv_buffer;			/* ���ڽ��շ��������ص���Ӧ���ݵĻ���*/
	_u32  _recv_buffer_size;		/* _recv_buffer �����С*/
	
	void *_callback_fun;			/* ������ɺ�Ļص����� : typedef int32 ( *ET_HTTP_CALL_BACK_FUNCTION)(ET_HTTP_CALL_BACK * p_http_call_back_param); */
	void * _user_data;			/* �û����� */
	_u32  _timeout;				/* ��ʱ,��λ��,����0ʱ180�볬ʱ*/
	_int32 _priority;				/* -1:�����ȼ�,������ͣ��������; 0:��ͨ���ȼ�,����ͣ��������;1:�����ȼ�,����ͣ������������������  */
} HTTP_PARAM;
/* HTTP�Ự�ӿڵĻص���������*/
typedef enum t_http_cb_type
{
	EHC_NOTIFY_RESPN=0, 
	EHC_GET_SEND_DATA, 	// Just for POST
	EHC_NOTIFY_SENT_DATA, 	// Just for POST
	EHC_GET_RECV_BUFFER,
	EHC_PUT_RECVED_DATA,
	EHC_NOTIFY_FINISHED
} HTTP_CB_TYPE;
typedef struct t_http_call_back
{
	_u32 _http_id;
	void * _user_data;			/* �û����� */
	HTTP_CB_TYPE _type;
	void * _header;				/* _type==EHCT_NOTIFY_RESPNʱ��Ч,ָ��http��Ӧͷ,����etm_get_http_header_value��ȡ�������ϸ��Ϣ*/
	
	_u8 ** _send_data;			/* _type==EHCT_GET_SEND_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ����Ҫ�ϴ�������*/
	_u32  * _send_data_len;		/* ��Ҫ�ϴ������ݳ��� */
	
	_u8 * _sent_data;			/* _type==EHCT_NOTIFY_SENT_DATAʱ��Ч, ���ݷֲ��ϴ�ʱ,ָ���Ѿ��ϴ�������*/
	_u32   _sent_data_len;		/* �Ѿ��ϴ������ݳ��� */
	
	void ** _recv_buffer;			/* _type==EHCT_GET_RECV_BUFFERʱ��Ч, ���ݷֲ�����ʱ,ָ�����ڽ������ݵĻ���*/
	_u32  * _recv_buffer_len;		/* �����С */
	
	_u8 * _recved_data;			/* _type==EHCT_PUT_RECVED_DATAʱ��Ч, ���ݷֲ�����ʱ,ָ���Ѿ��յ�������*/
	_u32  _recved_data_len;		/* �Ѿ��յ������ݳ��� */

	_int32 _result;					/* _type==EHCT_NOTIFY_FINISHEDʱ��Ч, 0Ϊ�ɹ�*/
} HTTP_CALL_BACK;
typedef _int32 ( *HTTP_CALL_BACK_FUNCTION)(HTTP_CALL_BACK * p_http_call_back_param); 


 _int32 init_mini_http(void);
 _int32 uninit_mini_http(void);
 
 _int32 sd_http_clear(void);

 _int32 sd_http_get(HTTP_PARAM * param,_u32 * http_id);

 _int32 sd_http_post(HTTP_PARAM * param,_u32 * http_id);

 _int32 sd_http_close(_u32 http_id);

_int32 sd_http_cancel(_u32 http_id);

#ifdef __cplusplus
}
#endif

#endif // __HTTP_MINI_INTERFACE_H_20100701__
