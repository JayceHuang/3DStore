

/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/16
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_HUB_H_
#define _EMULE_HUB_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
/*
_int32 emule_query_hub(EMULE_QUERY_DEVICE* device, _u8 file_hash[FILE_HASH_SIZE], _u64 file_size, _u32 query_times);



class emule_cmd_query : public p2hub_command 
{
public:
	emule_cmd_query();
	virtual ~emule_cmd_query();

public:
	virtual unsigned __int32 length_parameters();
	virtual void encode_parameters(void * buff, unsigned __int32 &buff_size);
	virtual void decode_parameters(const void * buff, unsigned __int32 buff_size);
	virtual command* response();
	virtual void  reset_body();
	virtual void description_parameters(std::string &str);

public:
	std::string	_file_hash_id;
	unsigned __int64 _file_size;
	unsigned __int32 _query_sn;
	std::string	_partner_id;
	unsigned __int32 _product_flag;
};


//////////////////////////////////////////////////////////////////////////
// emule_cmd_query_response
//////////////////////////////////////////////////////////////////////////

class emule_cmd_query_response : public p2hub_command
{
public:
	emule_cmd_query_response();	
	virtual ~emule_cmd_query_response();

public:
	unsigned __int32 length_parameters();
	void encode_parameters(void * buff, unsigned __int32 & buff_size);
	void decode_parameters(const void * buff, unsigned __int32 buff_size);
	virtual void description_parameters(std::string &str);
    virtual void reset_body();

public:
	BYTE _result;
	unsigned __int32 _has_record;
	std::string _aich_hash;
	std::string _part_hash;
	std::string _cid;
	std::string _gcid;
	unsigned __int32 _gcid_part_size;
	unsigned __int32 _gcid_level;
	unsigned __int32 _control_flag;
};


//////////////////////////////////////////////////////////////////////////
// emule_hub_queryer
//////////////////////////////////////////////////////////////////////////

class emule_hub_queryer_result_handler;


class emule_hub_queryer : public cmd_tcp_handler, public timeout_handler
{
public:
	emule_hub_queryer(const std::string &str_srv_addr, 
		              unsigned short w_port, 
		              emule_hub_queryer_result_handler* p_hub_queryer_result_handler);
	virtual ~emule_hub_queryer();

	virtual void handle_timeout(unsigned timer_type);	
	
	void do_query(const std::string &str_xunlei_peer_id, 
		          const std::string &str_file_hash_id, 
		          unsigned __int64 n_file_size, 
				  unsigned __int32 n_query_sn,
				  bool set_product_info = false, // 是否设置产品信息
				  const std::string* product_version_ptr = NULL,
				  const unsigned long product_id = -1,
				  const std::string* partner_id_ptr = NULL);

protected:	
	virtual void handle_network_error(bool recv_error = false, unsigned error_code = 0);
	virtual void handle_response(command * response_ptr);
	
private:
	emule_hub_queryer_result_handler * _queryer_result_handler;
	emule_cmd_query _cmd;
	unsigned _cur_query_times;
	unsigned _query_max_times;
	unsigned _query_error_interval;
	LOG4CPLUS_CLASS_DECLARE( _s_logger );
};


//////////////////////////////////////////////////////////////////////////
// emule_hub_queryer_result_handler
//////////////////////////////////////////////////////////////////////////

class emule_hub_queryer_result_handler
{
public:
	virtual void handle_emule_hub_query_success(std::string &str_file_hash_id, 
		                                        unsigned __int64 n_file_size, 
												const std::string &str_aich_hash, 
		                                        const std::string &str_part_hash, 
												const std::string &str_cid, 
		                                        const std::string &str_gcid, 
												unsigned int n_gcid_part_size, 
												// unsigned int n_gcid_level,
												unsigned int n_control_flag, 
												emule_hub_queryer * p_sender 
												) = 0;  

	// error_code: 0 网络错误，1 服务器返回查询失败，2 服务器返回没有资源
	virtual void handle_emule_hub_query_failed(std::string &str_file_hash_id, 
		                                       emule_hub_queryer * p_sender, 
											   unsigned error_code
											   ) = 0;   
};

#endif // !defined(AFX_EMULE_HUB_QUERYER_H__C8D0127F_5B25_4331_B45A_06E461686EEC__INCLUDED_)


*/

#endif /* ENABLE_EMULE */

#endif

