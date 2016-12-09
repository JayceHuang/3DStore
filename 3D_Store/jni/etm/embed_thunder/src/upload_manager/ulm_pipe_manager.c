#include "ulm_pipe_manager.h"
#ifdef UPLOAD_ENABLE

#include "utility/time.h"
#include "utility/settings.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "ulm_file_manager.h"
#include "platform/sd_fs.h"
#include "utility/logid.h"
#define LOGID LOGID_UPLOAD_MANAGER
#include "utility/logger.h"

#include "p2p_data_pipe/p2p_pipe_interface.h"
#include "connect_manager/pipe_interface.h"
#ifdef ENABLE_BT
#include "bt_download/bt_data_pipe/bt_pipe_interface.h"
#endif
#include "emule/emule_pipe/emule_pipe_interface.h"
    
/*--------------------------------------------------------------------------*/
/*                Global declarations				        */
/*--------------------------------------------------------------------------*/
/* Pointer of the ulm_pipe_manager,only one ulm_pipe_manager when program is running */
static ULM_PIPE_MANAGER * gp_ulm_pipe_manager = NULL;
static SLAB * gp_upm_pipe_info_slab = NULL;
#ifdef _DEBUG
static _u32 my_count=0;
#endif

#ifdef ULM_DEBUG
static _u32  g_file_id=0;
static char * ulm_test_str="\npipe;type(0-u&d,1-u,2-bt_u&d,3-bt+u&d);up_stat;[type+file+ref+speed];up_speed,ex_ip;in_ip;connect_type;down_speed;down_score;state\n";
void upm_display_ulm_info(void);
#endif

/*--------------------------------------------------------------------------*/
/*                Interfaces provid for upload_manager				        */
/*--------------------------------------------------------------------------*/
_int32 init_upload_pipe_manager(void)
{
	_int32 ret_val = SUCCESS;
#ifdef ULM_DEBUG
	_u32 writesize=0;
#endif	
	LOG_DEBUG("init_upload_pipe_manager");
	
	ret_val = sd_malloc(sizeof(ULM_PIPE_MANAGER) ,(void**)&gp_ulm_pipe_manager);
	if(ret_val!=SUCCESS) return ret_val;

	sd_memset(gp_ulm_pipe_manager,0,sizeof(ULM_PIPE_MANAGER));

	ret_val = mpool_create_slab(sizeof(UPM_PIPE_INFO), MIN_UPM_PIPE_INFO_ITEM_MEMORY, 0, &gp_upm_pipe_info_slab);
	if(ret_val!=SUCCESS) goto ErrorHanle;
	
	list_init(&(gp_ulm_pipe_manager->_choke_pipe_list));
	list_init(&(gp_ulm_pipe_manager->_unchoke_pipe_list));
	map_init(&(gp_ulm_pipe_manager->_pipe_info_map), upm_map_compare_fun);

	gp_ulm_pipe_manager->_max_unchoke_pipes =ULM_MAX_UNCHOKE_PIPES_NUM;
	gp_ulm_pipe_manager->_max_choke_pipes =ULM_MAX_CHOKE_PIPES_NUM;
		
	settings_get_int_item( "upload_manager.max_unchoke_pipes",(_int32*)&(gp_ulm_pipe_manager->_max_unchoke_pipes));
	settings_get_int_item( "upload_manager.max_choke_pipes",(_int32*)&(gp_ulm_pipe_manager->_max_choke_pipes));
	
	gp_ulm_pipe_manager->_max_speed=0;
	//gp_ulm_pipe_manager->_scheduler_timer_id=INVALID_MSG_ID;
	gp_ulm_pipe_manager->_error_code=0;
	
#ifdef ULM_DEBUG
	sd_assert(g_file_id==0);
	ret_val = sd_open_ex("ulm_test.txt", O_FS_CREATE, &g_file_id);
	if(ret_val!=SUCCESS||g_file_id == -1)
	{
		/* Opening File error */
		LOG_DEBUG("ulm_test  Opening File error !");
		g_file_id=0;
	}
	sd_write( g_file_id,ulm_test_str, sd_strlen(ulm_test_str), &writesize);
#endif

	LOG_DEBUG("init_upload_pipe_manager:SUCCESS");
	return SUCCESS;
	
ErrorHanle:
	

	SAFE_DELETE(gp_ulm_pipe_manager);
	
	LOG_DEBUG("init_upload_pipe_manager:faild:ret_val=%d",ret_val);
	return ret_val;
}
_int32 uninit_upload_pipe_manager(void)
{
	
	LOG_DEBUG("uninit_upload_pipe_manager");

	if(gp_ulm_pipe_manager==NULL) return SUCCESS;
	
	upm_clear();

	if(gp_upm_pipe_info_slab!=NULL)
	{
		 mpool_destory_slab(gp_upm_pipe_info_slab);
		 gp_upm_pipe_info_slab = NULL;
	}
	 
	SAFE_DELETE(gp_ulm_pipe_manager);

	LOG_DEBUG("uninit_upload_pipe_manager:SUCCESS");
	return SUCCESS;
}
_int32 upm_close_upload_pipes(void)
{
	MAP_ITERATOR map_iterator=NULL;
	DATA_PIPE * p_pipe=NULL;
	LOG_DEBUG("upm_close_upload_pipes:pipe_number=%d",map_size(&(gp_ulm_pipe_manager->_pipe_info_map)));
	gp_ulm_pipe_manager->_wait_uninit=TRUE;
	while(map_size(&(gp_ulm_pipe_manager->_pipe_info_map))>0)
	{
		map_iterator = MAP_BEGIN( gp_ulm_pipe_manager->_pipe_info_map);
		sd_assert( map_iterator != MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) );
		if( map_iterator == MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
			return SUCCESS;
		
		p_pipe = (DATA_PIPE*)MAP_KEY( map_iterator );
		sd_assert(upm_get_pipe_type(p_pipe)==ULM_PU_P2P_PIPE);
		p2p_pipe_close( p_pipe);
	}

#ifdef ULM_DEBUG
	if(g_file_id!=0)
	{
		sd_close_ex(g_file_id);
		g_file_id=0;
	}
#endif

	return SUCCESS;
}

BOOL upm_is_pipe_full(void)
{
	BOOL ret_b=((map_size(&(gp_ulm_pipe_manager->_pipe_info_map))<(gp_ulm_pipe_manager->_max_unchoke_pipes+gp_ulm_pipe_manager->_max_choke_pipes))?FALSE:TRUE);
	LOG_DEBUG("upm_is_pipe_full:%d",ret_b);
	return ret_b;
}

_u32 upm_get_cur_upload_speed( void)
{
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	_u32 total_speed=0,pipe_speed=0;
	
	LOG_DEBUG("upm_get_cur_upload_speed");

	for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list);
		cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
		cur_node = LIST_NEXT(cur_node))
	{
		p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
		pipe_speed=upm_get_pipe_speed(p_pipe_info);
		total_speed+=pipe_speed;
	}

	if(total_speed>gp_ulm_pipe_manager->_max_speed)
		gp_ulm_pipe_manager->_max_speed = total_speed;

	LOG_DEBUG("upm_get_cur_upload_speed=%u,max_speed=%u",total_speed,gp_ulm_pipe_manager->_max_speed);

	return total_speed;
}
_u32 upm_get_max_upload_speed( void)
{
	return gp_ulm_pipe_manager->_max_speed;
}


// 添加pipe
_int32 upm_add_pipe(const _u8 *gcid  ,DATA_PIPE * p_pipe,  ULM_PIPE_TYPE type)
{
	_int32 ret_val=SUCCESS;
	UPM_PIPE_INFO * p_pipe_info=NULL;
	PAIR node;
	
	LOG_DEBUG("upm_add_pipe:0x%X,type=%d",p_pipe,type);

	if(gp_ulm_pipe_manager->_wait_uninit==TRUE)
		return UPM_ERR_UPM_WAIT_UNINIT;

	if(NULL!=upm_get_gcid( p_pipe))
	{
		LOG_DEBUG("upm_add_pipe:0x%X already exist!",p_pipe);
			return SUCCESS;
	}
	
	ret_val = upm_create_pipe_info(p_pipe,(_u8 *)gcid, type,&p_pipe_info);
	CHECK_VALUE(ret_val);

	node._key = (void*)p_pipe;
	node._value = (void*)p_pipe_info;
	ret_val = map_insert_node(&(gp_ulm_pipe_manager->_pipe_info_map), &node);
	if(ret_val!=SUCCESS)
	{
		upm_delete_pipe_info(p_pipe_info);
		CHECK_VALUE(ret_val);
	}

	ret_val = list_push(&(gp_ulm_pipe_manager->_choke_pipe_list),p_pipe_info);
	if(ret_val!=SUCCESS)
	{
		map_erase_node(&(gp_ulm_pipe_manager->_pipe_info_map), (void*)p_pipe);
		upm_delete_pipe_info(p_pipe_info);
		CHECK_VALUE(ret_val);
	}

	LOG_DEBUG("upm_add_pipe:0x%X success!",p_pipe);
	return SUCCESS;
}
// 删除pipe
_int32 upm_remove_pipe( DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	MAP_ITERATOR result_iterator=NULL;
	UPM_PIPE_INFO *p_pipe_info=NULL,*p_pipe_info_value=NULL;
	LIST_ITERATOR cur_node;
	BOOL remove_from_list=FALSE;
	
	LOG_DEBUG("upm_remove_pipe:0x%X",p_pipe);
	
	ret_val=map_find_iterator(&(gp_ulm_pipe_manager->_pipe_info_map), (void*)p_pipe, &result_iterator);
	CHECK_VALUE(ret_val);
	
	if( result_iterator == MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
	{
		LOG_DEBUG("upm_remove_pipe:0x%X not found!",p_pipe);
		return UPM_ERR_PIPE_NOT_FOUND;
	}
	
	p_pipe_info = (UPM_PIPE_INFO *)MAP_VALUE( result_iterator );
	sd_assert(p_pipe_info!=NULL);
	
	map_erase_iterator(&(gp_ulm_pipe_manager->_pipe_info_map), result_iterator );
		
	for(cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_choke_pipe_list); cur_node != LIST_END( gp_ulm_pipe_manager->_choke_pipe_list );
		cur_node = LIST_NEXT(cur_node))
	{
		p_pipe_info_value = (UPM_PIPE_INFO *)LIST_VALUE( cur_node );
		if(p_pipe_info_value==p_pipe_info)
		{
			list_erase(&(gp_ulm_pipe_manager->_choke_pipe_list), cur_node);
			remove_from_list=TRUE;
			break;
		}
	}

	if(remove_from_list==FALSE)
	{
		for(cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list); cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
			cur_node = LIST_NEXT(cur_node))
		{
			p_pipe_info_value = (UPM_PIPE_INFO *)LIST_VALUE( cur_node );
			if(p_pipe_info_value==p_pipe_info)
			{
				list_erase(&(gp_ulm_pipe_manager->_unchoke_pipe_list), cur_node);
				remove_from_list=TRUE;
				break;
			}
		}
	}
	
	sd_assert(remove_from_list==TRUE);
	/* Cancel the read command */	
	if(p_pipe_info->_type!=ULM_PU_P2P_PIPE)
	{
		sd_assert(FALSE==ufm_check_need_cancel_read_file(NULL, (void*)p_pipe));
	}
	else
	{
		sd_assert(FALSE==ufm_check_need_cancel_read_file(p_pipe_info->_gcid, (void*)p_pipe));
	}
	/* Close the pipe 
	if(p_pipe_info->_type==ULM_PU_P2P_PIPE)
	{
		p2p_pipe_close(p_pipe);
	}
*/
	upm_delete_pipe_info(p_pipe_info);
		
	return SUCCESS;
}

_int32 upm_del_pipes(const _u8 *gcid)
{
	_int32 ret_val = SUCCESS;
	_u32 listsize=0;
	LIST pipe_list;
	DATA_PIPE * p_pipe=NULL;
	
	LOG_DEBUG("upm_del_pipes");
	list_init(&pipe_list);
	ret_val=upm_get_pipe_list_by_gcid(gcid,&pipe_list);
	CHECK_VALUE(ret_val);

	listsize=list_size(&pipe_list);
	if(listsize>0)
	{
		while(listsize--)
		{
			ret_val=list_pop(&pipe_list,(void**)&p_pipe);
			CHECK_VALUE(ret_val);
		
				if(upm_get_pipe_type(p_pipe) == ULM_PU_P2P_PIPE)
				{
					p2p_pipe_close(p_pipe);
				}
				//else
				//{
				//	upm_remove_pipe( p_pipe);
				//}
		}
	}

	return SUCCESS;
}

_u8* upm_get_gcid(DATA_PIPE * p_pipe)
{
	_int32 ret_val = SUCCESS;
	UPM_PIPE_INFO *p_pipe_info = NULL;
	
	LOG_DEBUG("upm_get_gcid:0x%X",p_pipe);
	ret_val = map_find_node(&(gp_ulm_pipe_manager->_pipe_info_map), (void*)p_pipe, (void **)&p_pipe_info);
	sd_assert(ret_val==SUCCESS);
	if (ret_val != SUCCESS)
	{
	    return NULL;
	}
	
	if (p_pipe_info != NULL)
	{
		return p_pipe_info->_gcid;
	}
	
	LOG_DEBUG("upm_get_gcid:0x%X not found!",p_pipe);
	return NULL;
}

ULM_PIPE_TYPE upm_get_pipe_type(DATA_PIPE * p_pipe)
{
	ULM_PIPE_TYPE type = ULM_UNDEFINE;
	PIPE_INTERFACE_TYPE pi_type = UNKNOWN_PIPE_INTERFACE_TYPE;
	if(NULL != p_pipe->_p_pipe_interface)
	{
		pi_type = pi_get_pipe_interface_type(p_pipe);
	}
	
	LOG_DEBUG("upm_get_pipe_type:0x%X, type = %d", p_pipe, p_pipe->_data_pipe_type);

	if (p_pipe->_data_pipe_type == P2P_PIPE)
	{
		if (p_pipe->_p_pipe_interface != NULL)
		{
			if (COMMON_PIPE_INTERFACE_TYPE == pi_type)
			{
				type= ULM_D_AND_U_P2P_PIPE;
			}
#ifdef ENABLE_EMULE
			else if (EMULE_SPEEDUP_PIPE_INTERFACE_TYPE == pi_type)
			{
				type = ULM_D_AND_U_EMULE_P2P_PIPE;
			}
#endif
#ifdef ENABLE_BT
			else
			{
				type = ULM_D_AND_U_BT_P2P_PIPE;
			}
#endif
		}
		else
		{
			type = ULM_PU_P2P_PIPE;
		}
	}    
#ifdef ENABLE_BT
	else if (BT_PIPE_INTERFACE_TYPE == pi_type)
	{
		type = ULM_D_AND_U_BT_PIPE;
	}
#endif
#ifdef ENABLE_EMULE
    else if (EMULE_PIPE_INTERFACE_TYPE == pi_type)
    {
		type = ULM_D_AND_U_EMULE_PIPE;
    }
#endif
    else
    {
        sd_assert(FALSE);
    }

	LOG_DEBUG("upm_get_pipe_type:0x%X,type=%d",p_pipe,type);
	return type;
}
	
void upm_scheduler(void)
{
	
	LOG_DEBUG("upm_scheduler");

	// remove all the failed pipes
	upm_clear_failure_pipes();

	if(0==list_size(&( gp_ulm_pipe_manager->_choke_pipe_list))) 
	{
		LOG_DEBUG("upm_scheduler:no choked pipe!");
#ifdef ULM_DEBUG
	upm_display_ulm_info();
#endif
		return; 
	}
	
	// Update unchoked pipes level
	upm_update_unchoked_pipes_score();

	// Update choked pipes level
	upm_update_choked_pipes_score();

#ifdef _DEBUG
	my_count=0;
#endif
#ifdef ULM_DEBUG
	upm_display_ulm_info();
#endif
	// Choke and unchke the pipes
	upm_assign_pipes();

	LOG_DEBUG("upm_scheduler end!");
	

}

BOOL upm_is_pipe_unchoked(DATA_PIPE * p_pipe)
{
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	BOOL ret_b=FALSE;

	if(list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list))!=0) 
	{

		for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list);
			cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
			cur_node = LIST_NEXT(cur_node))
		{
			p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
			if(p_pipe_info->_pipe==p_pipe)
			{
				ret_b=TRUE;
			}
		}
	}
	
	LOG_DEBUG("upm_is_pipe_unchoked:p_pipe=0x%X,unchoke_pipe_num=%u,ret_b=%d",p_pipe,list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list)),ret_b);

	return ret_b;
}
_int32 upm_notify_have_block( const _u8 *gcid)
{
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	_int32 ret_val=SUCCESS;

	LOG_DEBUG("upm_notify_have_block:unchoke_pipe_num=%u",list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list)));
	if(list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list))!=0) 
	{

		for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list);
			cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
			cur_node = LIST_NEXT(cur_node))
		{
			p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
			if((p_pipe_info->_type==ULM_D_AND_U_P2P_PIPE)
#ifdef ENABLE_BT
				||(p_pipe_info->_type==ULM_D_AND_U_BT_P2P_PIPE)
#endif
				)
			if(sd_is_cid_equal(gcid,p_pipe_info->_gcid))
			{
				LOG_DEBUG("p2p_pipe_notify_range_checked:p_pipe=0x%X",p_pipe_info->_pipe);
				if((PIPE_FAILURE!= dp_get_state(p_pipe_info->_pipe ))&&
					//(UPLOAD_FIN_STATE!= dp_get_upload_state(p_pipe_info->_pipe ))&&
					(UPLOAD_FAILED_STATE!= dp_get_upload_state(p_pipe_info->_pipe )))
				{
					ret_val = p2p_pipe_notify_range_checked( p_pipe_info->_pipe);
					sd_assert( ret_val == SUCCESS );
				}
			}
		}
	}
	

	return SUCCESS;
}


#ifdef _CONNECT_DETAIL
void upm_update_pipe_info(void)
{
	_u32 pipe_index = 0;
    _int32 ret_val = SUCCESS;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	DATA_PIPE *p_data_pipe =NULL;
    LIST_ITERATOR cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list );

    sd_memset( &gp_ulm_pipe_manager->_peer_pipe_info, 0, sizeof( gp_ulm_pipe_manager->_peer_pipe_info ) );

    while( cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list )
        && pipe_index < PEER_INFO_NUM )
    {
        	p_pipe_info = (UPM_PIPE_INFO *)LIST_VALUE( cur_node );
		p_data_pipe=p_pipe_info->_pipe;
		if( p_data_pipe->_data_pipe_type == P2P_PIPE )
		{
			ret_val = p2p_pipe_get_peer_pipe_info( p_data_pipe, &gp_ulm_pipe_manager->_peer_pipe_info._pipe_info_list[pipe_index] );
		}
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL
		else if( p_data_pipe->_data_pipe_type == BT_PIPE )
		{
			ret_val = bt_pipe_get_peer_pipe_info( p_data_pipe, &gp_ulm_pipe_manager->_peer_pipe_info._pipe_info_list[pipe_index] );
		}
#endif
#endif
        sd_assert( ret_val == SUCCESS );		
	 /* Replace the download score by upload score */
	 gp_ulm_pipe_manager->_peer_pipe_info._pipe_info_list[pipe_index]._score =p_pipe_info->_score;
	 
        pipe_index++;                
        cur_node = LIST_NEXT( cur_node );
    }

     gp_ulm_pipe_manager->_peer_pipe_info._pipe_info_num = pipe_index;
}
void* upm_get_pipe_info(void)
{
	return (void *)&gp_ulm_pipe_manager->_peer_pipe_info;
}


#endif

/*--------------------------------------------------------------------------*/
/*                Internal functions				        */
/*--------------------------------------------------------------------------*/
void upm_clear(void)
{
	MAP_ITERATOR cur_node, nxt_node;
	DATA_PIPE * p_pipe = NULL;
	
	LOG_DEBUG("upm_clear");
	
	sd_assert(map_size(&(gp_ulm_pipe_manager->_pipe_info_map))==0);
	
	cur_node = MAP_BEGIN( gp_ulm_pipe_manager->_pipe_info_map);

	while( cur_node != MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
	{
		nxt_node = MAP_NEXT( gp_ulm_pipe_manager->_pipe_info_map, cur_node );
		p_pipe = (void*)MAP_KEY( cur_node );
		sd_assert(upm_get_pipe_type(p_pipe)==ULM_PU_P2P_PIPE);
		upm_remove_pipe( p_pipe);
		cur_node=nxt_node;
	}

}

void upm_clear_failure_pipes(void)
{
	MAP_ITERATOR cur_node, nxt_node;
	DATA_PIPE * p_pipe = NULL;
	UPM_PIPE_INFO * p_pipe_info=NULL;
	
	LOG_DEBUG("upm_clear_failure_pipes");
	
	cur_node = MAP_BEGIN( gp_ulm_pipe_manager->_pipe_info_map);

	while( cur_node != MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
	{
		nxt_node = MAP_NEXT( gp_ulm_pipe_manager->_pipe_info_map, cur_node );
		p_pipe = (void*)MAP_KEY( cur_node );
		p_pipe_info = (UPM_PIPE_INFO *)MAP_VALUE( cur_node );
		if((PIPE_FAILURE== dp_get_state(p_pipe ))||(UPLOAD_FAILED_STATE== dp_get_upload_state(p_pipe )))
		{
			if(p_pipe_info->_type == ULM_PU_P2P_PIPE)
			{
				p2p_pipe_close(p_pipe);
			}
			else
			{
				upm_remove_pipe( p_pipe);
			}
		}
		else
		if(UPLOAD_FIN_STATE== dp_get_upload_state(p_pipe ))
		{
			if(p_pipe_info->_type == ULM_PU_P2P_PIPE)
			{
				p2p_pipe_close(p_pipe);
			}
		}
		cur_node=nxt_node;
	}


}

/* 由pipe的旧分数得到的参考分数算法：
	旧分数小于6，则参考分数0；
	旧分数大于10，则参考分数2；
	其余旧分数得到的参考分数为1。
*/
_u32 upm_get_pipe_referrence_score(UPM_PIPE_INFO* p_pipe_info)
{
	_u32 score=0;
	
	if(p_pipe_info->_score<7)
		score =0;
	else
	if(p_pipe_info->_score>10)
		score =2;
	else
		score =1;

	LOG_DEBUG("upm_get_pipe_referrence_score:score=%u",score);

	return score;

}

_u32 upm_get_pipe_speed(UPM_PIPE_INFO* p_pipe_info)
{
	_u32 pipe_speed=0;
	if(p_pipe_info->_pipe->_data_pipe_type==P2P_PIPE)
	{
		p2p_pipe_get_upload_speed(p_pipe_info->_pipe,&pipe_speed);
	}
	else
	{
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL
		bt_pipe_get_upload_speed(p_pipe_info->_pipe,&pipe_speed);
#endif
		
#endif
	}
	
	LOG_DEBUG("upm_get_pipe_speed:p_pipe=0x%X,score=%u",p_pipe_info->_pipe,pipe_speed);
	
	return pipe_speed;
}

/* 速度分数算法：
	速度为0，则分数也为0；
	速度低于平均速度的80%，则分数为2，
	速度高于平均速度的120%，则分数为6，
	其余速度对应的分数为4。
*/
_u32 upm_get_pipe_speed_score(UPM_PIPE_INFO* p_pipe_info,_u32 average_speed)
{
	_u32 speed=0,score=0;
	
	speed=upm_get_pipe_speed(p_pipe_info);
	
	if(speed==0)
	{
		score= 0;
	}
	else
	if(speed<(average_speed*8/10))
	{
		score= 2;
	}
	else
	if(speed>(average_speed*12/10))
	{
		score= 6;
	}
	else
	{
		score= 4;
	}
	
	LOG_DEBUG("upm_get_pipe_speed_score:score=%u",score);

	return score;
}

/* 由pipe对应的文件得到的分数算法：
	文件分数大于1，则得到的分数为1；
	否则，分数为0
*/
_u32 upm_get_file_score(const _u8 * gcid,ULM_PIPE_TYPE	_type)
{
	_u32 score=0;
	
	if(_type == ULM_PU_P2P_PIPE)
	{
		score= ufm_get_file_score(gcid);
	}
	else
	{
		score=1;
	}

	LOG_DEBUG("upm_get_file_score:score=%u",score);

	return score;
}

/* pipe类型分数算法：
	纯上传pipe的分数为1，
	边下边传pipe的分数为4。
*/
_u32 upm_get_pipe_type_score( ULM_PIPE_TYPE	_type)
{
	_u32 score=0;
	
	if(_type==ULM_PU_P2P_PIPE)
	{
		score= 1;
	}
	else
	{
		score= 4;
	}

	LOG_DEBUG("upm_get_pipe_type_score:score=%u",score);

	return score;
}

/* pipe的等待时间分数算法：
	每等多一秒钟，分数加1。
注意：只有被choked的pipe才用此算法
*/
_u32 upm_get_pipe_time_score(UPM_PIPE_INFO* p_pipe_info)
{
	_u32 cur_time=0,score=0;
	
	sd_time_ms(&(cur_time));
	
	score=(TIME_SUBZ(cur_time, p_pipe_info->_choke_time))/1000;
	
	LOG_DEBUG("upm_get_pipe_time_score:score=%u",score);

	return score;

}


void upm_update_unchoked_pipe_score(UPM_PIPE_INFO* p_pipe_info,_u32 average_speed)
{
	_u32 score =0;	
	_u32  _ref_score=0;
	_u32  _file_score=0;
	_u32  _type_score=0;
	_u32  _t_s_score=0;
	LOG_DEBUG("upm_update_unchoked_pipe_score:pipe=0x%X,p_pipe_info->_score =%u",p_pipe_info->_pipe,p_pipe_info->_score);

	if(p_pipe_info->_score==20)
	{
		if(upm_get_pipe_time_score(p_pipe_info)<5)
			return;
		else
			p_pipe_info->_choke_time=0;
	}

	if(UPLOAD_FIN_STATE!= dp_get_upload_state(p_pipe_info->_pipe))
	{
		_ref_score=upm_get_pipe_referrence_score(p_pipe_info);
		_file_score=upm_get_file_score(p_pipe_info->_gcid,p_pipe_info->_type);
		_type_score=upm_get_pipe_type_score(p_pipe_info->_type);
		_t_s_score=upm_get_pipe_speed_score(p_pipe_info,average_speed);
		
		score = _ref_score+  _file_score+   _type_score+  _t_s_score;
	}
	
	LOG_DEBUG("upm_update_unchoked_pipe_score:p_pipe_info->_score =%u",score);
	p_pipe_info->_score = score;
#ifdef ULM_DEBUG
	p_pipe_info->_ref_score = _ref_score;
	p_pipe_info->_file_score = _file_score;
	p_pipe_info->_type_score = _type_score;
	p_pipe_info->_t_s_score = _t_s_score;
#endif
}
/* 按照score从小到大的顺序排列 */
_int32 upm_sort_unchoked_pipes(void)
{
	LIST list_tmp,*p_pipe_list=&(gp_ulm_pipe_manager->_unchoke_pipe_list);
	LIST_ITERATOR p_list_node=NULL;
	_int32 ret_val=SUCCESS;
	_u32 ListSize = list_size(p_pipe_list),ListSize2=0,score=0;
	UPM_PIPE_INFO * p_pipe_info = NULL,*p_pipe_info_2=NULL;
	
	LOG_DEBUG( " upm_sort_unchoked_pipes:ListSize=%u",ListSize);

	if(ListSize<2) return SUCCESS;
	
	ListSize2 = ListSize;
	list_init( &(list_tmp));
	list_cat_and_clear(&(list_tmp), p_pipe_list);

	ret_val=list_pop(&(list_tmp), (void**)&p_pipe_info);
	CHECK_VALUE(ret_val);
	score=p_pipe_info->_score;
	ret_val=list_push(p_pipe_list, p_pipe_info);
	CHECK_VALUE(ret_val);
	
	while(--ListSize!=0)
	{
		ret_val=list_pop(&(list_tmp),(void**) &p_pipe_info);
		CHECK_VALUE(ret_val);
		if(p_pipe_info->_score<score)
		{
			ret_val=list_insert(p_pipe_list, p_pipe_info, LIST_BEGIN(*p_pipe_list));
			CHECK_VALUE(ret_val);
			score=p_pipe_info->_score;
		}
		else
		{
			for(p_list_node = LIST_BEGIN(*p_pipe_list); p_list_node != LIST_END(*p_pipe_list); p_list_node = LIST_NEXT(p_list_node))
			{
				p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
				/* Check the  p_pipe_info  ...*/
				if(p_pipe_info_2->_score>p_pipe_info->_score)
				{
					break;
				}
			}

			if(p_list_node!=LIST_END(*p_pipe_list))
			{
				p_list_node=LIST_PRE(p_list_node);
			}
			else
			{
				p_list_node=LIST_RBEGIN(*p_pipe_list);
			}
			
			p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
			while((p_pipe_info_2->_score==p_pipe_info->_score)&&
				(upm_get_pipe_speed(p_pipe_info_2) > upm_get_pipe_speed(p_pipe_info)))
			{
				p_list_node=LIST_PRE(p_list_node);
				if(p_list_node==LIST_END(*p_pipe_list))
				{
					p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(LIST_BEGIN(*p_pipe_list));
					sd_assert(p_pipe_info_2->_score==p_pipe_info->_score);
					break;
				}
				p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
			}
				
			ret_val=list_insert(p_pipe_list, p_pipe_info, LIST_NEXT(p_list_node));
			CHECK_VALUE(ret_val);
		}
	}
	sd_assert(ListSize2==list_size(p_pipe_list));
	sd_assert(0==list_size( &(list_tmp)));
	LOG_DEBUG( " upm_sort_unchoked_pipes end SUCCESS");
	return SUCCESS;

}

void upm_update_unchoked_pipes_score(void)
{
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	_u32 avg_speed=0;
	
	LOG_DEBUG("upm_update_unchoked_pipes_score:unchoke_pipe_num=%u",list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list)));

	gp_ulm_pipe_manager->_low_score_unchoked_pipe_num=0;

	if(list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list))==0) 
	{
		return;
	}
	
	avg_speed=upm_get_average_speed();

	for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list);
		cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
		cur_node = LIST_NEXT(cur_node))
	{
		p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
		upm_update_unchoked_pipe_score(p_pipe_info,avg_speed);
		if(p_pipe_info->_score<6)
		{
			gp_ulm_pipe_manager->_low_score_unchoked_pipe_num++;
		}
	}

	LOG_DEBUG("upm_update_unchoked_pipes_score:_low_score_unchoked_pipe_num=%u",gp_ulm_pipe_manager->_low_score_unchoked_pipe_num);

	upm_sort_unchoked_pipes();
	
}

void upm_update_choked_pipe_score(UPM_PIPE_INFO* p_pipe_info)
{
	_u32 score =0;	
	_u32  _ref_score=0;
	_u32  _file_score=0;
	_u32  _type_score=0;
	_u32  _t_s_score=0;
	LOG_DEBUG("upm_update_choked_pipe_score:pipe=0x%X",p_pipe_info->_pipe);
	
	_ref_score=upm_get_pipe_referrence_score(p_pipe_info);
	_file_score=upm_get_file_score(p_pipe_info->_gcid,p_pipe_info->_type);
	_type_score=upm_get_pipe_type_score(p_pipe_info->_type);
	_t_s_score=upm_get_pipe_time_score(p_pipe_info);
	
	score = _ref_score+  _file_score+   _type_score+  _t_s_score;
	
	LOG_DEBUG("upm_update_choked_pipe_score:p_pipe_info->_score =%u",score);
	p_pipe_info->_score = score;
#ifdef ULM_DEBUG
	p_pipe_info->_ref_score = _ref_score;
	p_pipe_info->_file_score = _file_score;
	p_pipe_info->_type_score = _type_score;
	p_pipe_info->_t_s_score = _t_s_score;
#endif
}

/* 按照score从大到小的顺序排列 */
_int32 upm_sort_choked_pipes(void)
{
	LIST list_tmp,*p_pipe_list=&(gp_ulm_pipe_manager->_choke_pipe_list);
	LIST_ITERATOR p_list_node=NULL;
	_int32 ret_val=SUCCESS;
	_u32 ListSize = list_size(p_pipe_list),ListSize2=0,score=0;
	UPM_PIPE_INFO * p_pipe_info = NULL,*p_pipe_info_2=NULL;
	
	LOG_DEBUG( " upm_sort_choked_pipes:ListSize=%u",ListSize);

	if(ListSize<2) return SUCCESS;
	
	ListSize2 = ListSize;
	list_init( &(list_tmp));
	list_cat_and_clear(&(list_tmp), p_pipe_list);

	ret_val=list_pop(&(list_tmp), (void**)&p_pipe_info);
	CHECK_VALUE(ret_val);
	score=p_pipe_info->_score;
	ret_val=list_push(p_pipe_list, p_pipe_info);
	CHECK_VALUE(ret_val);
	
	while(--ListSize!=0)
	{
		ret_val=list_pop(&(list_tmp),(void**) &p_pipe_info);
		CHECK_VALUE(ret_val);
		if(p_pipe_info->_score>score)
		{
			ret_val=list_insert(p_pipe_list, p_pipe_info, LIST_BEGIN(*p_pipe_list));
			CHECK_VALUE(ret_val);
			score=p_pipe_info->_score;
		}
		else
		{
			for(p_list_node = LIST_BEGIN(*p_pipe_list); p_list_node != LIST_END(*p_pipe_list); p_list_node = LIST_NEXT(p_list_node))
			{
				p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
				/* Check the  p_pipe_info  ...*/
				if(p_pipe_info_2->_score<p_pipe_info->_score)
				{
					break;
				}
			}

			if(p_list_node!=LIST_END(*p_pipe_list))
			{
				p_list_node=LIST_PRE(p_list_node);
			}
			else
			{
				p_list_node=LIST_RBEGIN(*p_pipe_list);
			}
			
			p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
			while((p_pipe_info_2->_score==p_pipe_info->_score)&&
				(upm_get_pipe_time_score(p_pipe_info_2) < upm_get_pipe_time_score(p_pipe_info)))
			{
				p_list_node=LIST_PRE(p_list_node);
				if(p_list_node==LIST_END(*p_pipe_list))
				{
					p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(LIST_BEGIN(*p_pipe_list));
					sd_assert(p_pipe_info_2->_score==p_pipe_info->_score);
					break;
				}
				p_pipe_info_2 = (UPM_PIPE_INFO * )LIST_VALUE(p_list_node);
			}
				
			ret_val=list_insert(p_pipe_list, p_pipe_info, LIST_NEXT(p_list_node));
			CHECK_VALUE(ret_val);
		}
	}
	sd_assert(ListSize2==list_size(p_pipe_list));
	sd_assert(0==list_size( &(list_tmp)));
	LOG_DEBUG( " upm_sort_choked_pipes end SUCCESS");
	return SUCCESS;

}

void upm_update_choked_pipes_score(void)
{
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	
	LOG_DEBUG("upm_update_choked_pipes_score:choke_pipe_num=%u",list_size(&( gp_ulm_pipe_manager->_choke_pipe_list)));

	gp_ulm_pipe_manager->_high_score_choked_pipe_num=0;
	
	for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_choke_pipe_list);
		cur_node != LIST_END( gp_ulm_pipe_manager->_choke_pipe_list );
		cur_node = LIST_NEXT(cur_node))
	{
		p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
		upm_update_choked_pipe_score(p_pipe_info);
		if(p_pipe_info->_score>10)
		{
			gp_ulm_pipe_manager->_high_score_choked_pipe_num++;
		}
	}

	LOG_DEBUG("upm_update_choked_pipes_score:_high_score_choked_pipe_num=%u",gp_ulm_pipe_manager->_high_score_choked_pipe_num);

	upm_sort_choked_pipes();
}

_int32 upm_choke_pipe(UPM_PIPE_INFO * p_pipe_info)
{
	_int32 ret_val=SUCCESS;
	LOG_DEBUG("upm_choke_pipe:_pipe=0x%X",p_pipe_info->_pipe);


	p_pipe_info->_score=0;


	if(p_pipe_info->_pipe->_data_pipe_type==P2P_PIPE)
	{
		ret_val=p2p_pipe_choke_remote(p_pipe_info->_pipe, TRUE);
		CHECK_VALUE(ret_val);
	}
	else if(p_pipe_info->_pipe->_data_pipe_type==BT_PIPE)
	{
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL
		ret_val=bt_pipe_choke_remote(p_pipe_info->_pipe, TRUE);
		CHECK_VALUE(ret_val);
#endif
#endif
		
	}
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
    else if(p_pipe_info->_pipe->_data_pipe_type==EMULE_PIPE)
    {
        ret_val=emule_pipe_choke_remote(p_pipe_info->_pipe, TRUE);
        CHECK_VALUE(ret_val);
    }
#endif
#endif
    else
    {
        sd_assert(FALSE);
    }
	

	ret_val=sd_time_ms(&(p_pipe_info->_choke_time));
	CHECK_VALUE(ret_val);

#ifdef ULM_DEBUG
	p_pipe_info->_ref_score = 0;
	p_pipe_info->_file_score = 0;
	p_pipe_info->_type_score = 0;
	p_pipe_info->_t_s_score = 0;
#endif
	return SUCCESS;
}

_int32 upm_unchoke_pipe(UPM_PIPE_INFO * p_pipe_info)
{
	_int32 ret_val=SUCCESS;
	LOG_DEBUG("upm_unchoke_pipe:_pipe=0x%X",p_pipe_info->_pipe);


	p_pipe_info->_score=20;


	if(p_pipe_info->_pipe->_data_pipe_type==P2P_PIPE)
	{
		ret_val=p2p_pipe_choke_remote(p_pipe_info->_pipe, FALSE);
		CHECK_VALUE(ret_val);
	}
	else if(p_pipe_info->_pipe->_data_pipe_type==BT_PIPE)
	{
#ifdef ENABLE_BT
#ifdef ENABLE_BT_PROTOCOL

		ret_val=bt_pipe_choke_remote(p_pipe_info->_pipe, FALSE);
		CHECK_VALUE(ret_val);
#endif
		
#endif
	}
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
    else if(p_pipe_info->_pipe->_data_pipe_type==EMULE_PIPE)
    {
        ret_val=emule_pipe_choke_remote(p_pipe_info->_pipe, FALSE);
		CHECK_VALUE(ret_val);
	}
#endif
#endif
    else
    {
        sd_assert(FALSE);
    }

	
	ret_val=sd_time_ms(&(p_pipe_info->_choke_time));
	CHECK_VALUE(ret_val);

#ifdef ULM_DEBUG
	p_pipe_info->_ref_score = 0;
	p_pipe_info->_file_score = 0;
	p_pipe_info->_type_score = 0;
	p_pipe_info->_t_s_score = 0;
#endif

	return SUCCESS;
}
_int32 upm_assign_pipes(void)
{
	_int32 ret_val=SUCCESS;
	UPM_PIPE_INFO *p_pipe_info = NULL,*p_pipe_info_2 = NULL;
	BOOL assign_end=FALSE;
	LIST_ITERATOR cur_node;
	
	LOG_DEBUG("upm_assign_pipes:max_unchoke_pipes=%u,unchoke_pipe_num=%u,low_score_unchoked_pipe_num=%u,choke_pipe_num=%u,high_score_choked_pipe_num=%u",
		gp_ulm_pipe_manager->_max_unchoke_pipes,list_size(&(gp_ulm_pipe_manager->_unchoke_pipe_list)),gp_ulm_pipe_manager->_low_score_unchoked_pipe_num,list_size(&(gp_ulm_pipe_manager->_choke_pipe_list)),gp_ulm_pipe_manager->_high_score_choked_pipe_num);

	/* 递归调用时需要判断是否要继续调度 */
	if(0==list_size(&( gp_ulm_pipe_manager->_choke_pipe_list))) 
	{
		LOG_DEBUG("upm_assign_pipes:no choked pipe!");
		return SUCCESS; 
	}
	else
	{
		/* 判断_choke_pipe_list里最上面的pipe是不是刚刚被choked的pipe，如果是，则本次调度完成！ */
		cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_choke_pipe_list);
		p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
		if(p_pipe_info->_score==0)
		{
			LOG_DEBUG("upm_assign_pipes:all old choked pipe are unchked!");
			return SUCCESS; 
		}
		p_pipe_info=NULL;
	}

	if(list_size(&(gp_ulm_pipe_manager->_unchoke_pipe_list))>=gp_ulm_pipe_manager->_max_unchoke_pipes)
	{
		if(gp_ulm_pipe_manager->_low_score_unchoked_pipe_num==0) 
			return SUCCESS;
	
		ret_val=list_pop(&(gp_ulm_pipe_manager->_unchoke_pipe_list), (void**)&p_pipe_info);
		CHECK_VALUE(ret_val);

		ret_val=upm_choke_pipe(p_pipe_info);
		CHECK_VALUE(ret_val);

		gp_ulm_pipe_manager->_low_score_unchoked_pipe_num--;
	}

	/* Get the highest score pipe from choke_list */
	ret_val=list_pop(&(gp_ulm_pipe_manager->_choke_pipe_list), (void**)&p_pipe_info_2);
	CHECK_VALUE(ret_val);
	
	if(((upm_get_file_score(p_pipe_info_2->_gcid,p_pipe_info_2->_type)!=0)||(FALSE==ufm_is_file_full()))&&
		(0!=p_pipe_info_2->_score))
	{
		ret_val=upm_unchoke_pipe(p_pipe_info_2);
		CHECK_VALUE(ret_val);
		
		if(gp_ulm_pipe_manager->_high_score_choked_pipe_num>0)
			gp_ulm_pipe_manager->_high_score_choked_pipe_num--;
	}
	else
	{
		ret_val=list_push(&(gp_ulm_pipe_manager->_choke_pipe_list), (void*)p_pipe_info_2);
		CHECK_VALUE(ret_val);
		p_pipe_info_2=NULL;
		assign_end=TRUE;
	}
	
	if(p_pipe_info!=NULL)
	{
		ret_val=list_push(&(gp_ulm_pipe_manager->_choke_pipe_list), (void*)p_pipe_info);
		CHECK_VALUE(ret_val);
	}
	
	if(p_pipe_info_2!=NULL)
	{
		ret_val=list_push(&(gp_ulm_pipe_manager->_unchoke_pipe_list), (void*)p_pipe_info_2);
		CHECK_VALUE(ret_val);
	}

	if(assign_end==FALSE)
	{
		/* Continue to assign next pipe */
#ifdef _DEBUG
		LOG_DEBUG("upm_assign_pipes:Continue to assign next pipe,my_count=%u",my_count);
		sd_assert((++my_count)<=gp_ulm_pipe_manager->_max_unchoke_pipes);
#endif
		return upm_assign_pipes();
	}
	
	return SUCCESS;
}



_int32 upm_failure_exit(_int32 err_code)
{
	
	LOG_DEBUG("upm_failure_exit");
	

	return SUCCESS;
}
_int32 upm_map_compare_fun( void *E1, void *E2 )
{
	return ((_int32)E1-(_int32)E2);
}
_u32 upm_get_average_speed(void)
{
	_u32 average_speed=0;
	
	LOG_DEBUG("upm_get_average_speed");

	sd_assert(0!=list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list)));
	average_speed=upm_get_cur_upload_speed()/list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list));

	LOG_DEBUG("upm_get_average_speed:pipe_num=%u,average_speed=%u",list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list)),average_speed);

	return average_speed;
}

_int32 upm_get_pipe_list_by_gcid(const _u8* gcid,LIST * pipe_list)
{
	UPM_PIPE_INFO * p_pipe_info = NULL;
	MAP_ITERATOR cur_node ;
	void * map_key = NULL;
	
	LOG_DEBUG("upm_get_pipe_list_by_gcid");
	
	cur_node = MAP_BEGIN( gp_ulm_pipe_manager->_pipe_info_map);

	while( cur_node != MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
	{
		p_pipe_info= (UPM_PIPE_INFO *)MAP_VALUE( cur_node );
		if(TRUE==sd_is_cid_equal(p_pipe_info->_gcid, gcid))
		{
			map_key = MAP_KEY( cur_node );
			list_push(pipe_list, map_key);
		}
		
		cur_node = MAP_NEXT( gp_ulm_pipe_manager->_pipe_info_map, cur_node );
	}

	return SUCCESS;
}

_int32 upm_create_pipe_info(DATA_PIPE	* p_pipe,_u8* gcid,  ULM_PIPE_TYPE type,UPM_PIPE_INFO ** pp_pipe_info)
{
	_int32 ret_val=SUCCESS;
	UPM_PIPE_INFO * p_pipe_info=NULL;
	LOG_DEBUG("upm_create_pipe_info:p_pipe=0x%X,type=%d",p_pipe,type);
	
		ret_val = mpool_get_slip(gp_upm_pipe_info_slab, (void**)&p_pipe_info);
		CHECK_VALUE(ret_val);
		//sd_assert(PIPE_CHOKED== dp_get_state(p_pipe ));
		p_pipe_info->_pipe= p_pipe;
		p_pipe_info->_type = type;
		sd_memcpy(p_pipe_info->_gcid, gcid, CID_SIZE);
		sd_time_ms(&(p_pipe_info->_choke_time));
		/* 为区分新加入的pipe和刚刚被choked的pipe(积分为0)，这里把pipe的初始积分计算出来！ */
		if(type==ULM_PU_P2P_PIPE)
			p_pipe_info->_score=1;
		else
			p_pipe_info->_score=5;
		
		*pp_pipe_info=p_pipe_info;


	return SUCCESS;
}

_int32 upm_delete_pipe_info(UPM_PIPE_INFO * p_pipe_info)
{
	
	LOG_DEBUG("upm_delete_pipe_info");
	
	if(p_pipe_info==NULL) return SUCCESS;
	
	mpool_free_slip(gp_upm_pipe_info_slab,(void*) p_pipe_info);

	return SUCCESS;
}


_int32 upm_close_pure_upload_pipes()
{
    MAP_ITERATOR cur_node, nxt_node;
    DATA_PIPE * p_pipe = NULL;
    UPM_PIPE_INFO * p_pipe_info=NULL;
    
    LOG_DEBUG("upm_close_pure_upload_pipes, upload pipe size = %d"
        , map_size(&gp_ulm_pipe_manager->_pipe_info_map) );
    
    cur_node = MAP_BEGIN( gp_ulm_pipe_manager->_pipe_info_map);

    while( cur_node != MAP_END( gp_ulm_pipe_manager->_pipe_info_map ) )
    {
        nxt_node = MAP_NEXT( gp_ulm_pipe_manager->_pipe_info_map, cur_node );
        p_pipe = (void*)MAP_KEY( cur_node );
        p_pipe_info = (UPM_PIPE_INFO *)MAP_VALUE( cur_node );

        if(p_pipe_info->_type == ULM_PU_P2P_PIPE)
        {
            LOG_DEBUG("close pure upload p2p_pipe, pipe = 0x%x", p_pipe );
            p2p_pipe_close(p_pipe);
        }
        else
        {
            LOG_DEBUG("remove not p2p_pipe, pipe = 0x%x", p_pipe );
            upm_remove_pipe( p_pipe);
        }
       
        cur_node=nxt_node;
    }

	return 0;
}



#ifdef ULM_DEBUG
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "utility/utility.h"

char * upm_get_cur_time(void)
{

	struct tm local_time;
	struct timeval now;
	static char tme_str[100];

	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &local_time);
	
//{tv_sec = 1248881670, tv_usec = 678166}
// {tm_sec = 30, tm_min = 34, tm_hour = 23, tm_mday = 29, tm_mon = 6, tm_year = 109, tm_wday = 3, tm_yday = 209, tm_isdst = 0, 
 // tm_gmtoff = 28800, tm_zone = 0x863f158 "CST"}

	sd_memset(tme_str,0,100);
	sprintf(tme_str, "%04d-%02d-%02d %02d:%02d:%02d", local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

	return tme_str; 

}
char * upm_get_file_name(_u8 * gcid)
{
	ID_RC_INFO *p_rc_info = NULL;
	char * file_name=NULL;

	p_rc_info = rclist_query( gcid );

	sd_assert(p_rc_info!=NULL );
	
	file_name=sd_strrchr(p_rc_info->_path, '/');
	if(file_name==NULL)
		file_name=p_rc_info->_path;
	else
		file_name+=1;

	return file_name;
}


void upm_display_ulm_info(void)
{
	char str_buffer[2048],str_buffer2[1024];;
	_u32 writesize=0,count=0;
	LIST_ITERATOR cur_node;
	UPM_PIPE_INFO * p_pipe_info = NULL;
	PEER_PIPE_INFO pipe_info;
	_int32 ret_val=SUCCESS;
	char * file_name=NULL;

	//LOG_DEBUG("upm_display_ulm_info");
	if(g_file_id==0)
		return;
		
	sd_memset(str_buffer2,0,1024);
	sprintf(str_buffer,"\n\n\n%s\n unchoke_pipes: %u,%u,%u", upm_get_cur_time(),
		gp_ulm_pipe_manager->_max_unchoke_pipes,list_size(&(gp_ulm_pipe_manager->_unchoke_pipe_list)),gp_ulm_pipe_manager->_low_score_unchoked_pipe_num); 

	sd_write( g_file_id,str_buffer, sd_strlen(str_buffer), &writesize);


	if(list_size(&( gp_ulm_pipe_manager->_unchoke_pipe_list))!=0) 
	{
		for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_unchoke_pipe_list);
			cur_node != LIST_END( gp_ulm_pipe_manager->_unchoke_pipe_list );
			cur_node = LIST_NEXT(cur_node))
		{
			p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
			sd_memset(&pipe_info,0, sizeof(PEER_PIPE_INFO));
			if( p_pipe_info->_pipe->_data_pipe_type == P2P_PIPE )
			{
				ret_val = p2p_pipe_get_peer_pipe_info( p_pipe_info->_pipe,&pipe_info );
			}
			else if( p_pipe_info->_pipe->_data_pipe_type == BT_PIPE )
			{
				ret_val = bt_pipe_get_peer_pipe_info( p_pipe_info->_pipe, &pipe_info );
			}
			else
			{
				sd_assert(FALSE);
			}
	        	sd_assert( ret_val == SUCCESS );	
				
			sd_memset(str_buffer,0,2048);
			sprintf(str_buffer,"\n0x%X,ty=%d,stat=%d,sco=%2u[%u+%u+%u+%2u],", 
				(_u32)(p_pipe_info->_pipe),p_pipe_info->_type,dp_get_upload_state(p_pipe_info->_pipe ),p_pipe_info->_score,p_pipe_info->_type_score,p_pipe_info->_file_score,p_pipe_info->_ref_score,p_pipe_info->_t_s_score); 

			sd_memset(str_buffer2,0,1024);
			sprintf(str_buffer2,"up:%7u,ex:%15s,in:%15s,ct:%2u,speed:%7u,score:%7u,state:%1u  ", 
			pipe_info._upload_speed,pipe_info._external_ip, pipe_info._internal_ip, pipe_info._connect_type, pipe_info._speed,pipe_info._score, pipe_info._pipe_state );

			sd_strcat(str_buffer,str_buffer2,sd_strlen(str_buffer2));
			if(p_pipe_info->_type==ULM_PU_P2P_PIPE)
			{
				sd_strcat(str_buffer,"\n",sd_strlen("\n"));
				file_name = upm_get_file_name(p_pipe_info->_gcid);
				sd_strcat(str_buffer,file_name,sd_strlen(file_name));
			}
			sd_write( g_file_id,str_buffer, sd_strlen(str_buffer), &writesize);
		}
	}
	

	sd_memset(str_buffer2,0,1024);
	sprintf(str_buffer,"\n\n choke_pipes %u,%u,%u", 
		gp_ulm_pipe_manager->_max_choke_pipes,list_size(&(gp_ulm_pipe_manager->_choke_pipe_list)),gp_ulm_pipe_manager->_high_score_choked_pipe_num); 

	sd_write( g_file_id,str_buffer, sd_strlen(str_buffer), &writesize);


	if(list_size(&( gp_ulm_pipe_manager->_choke_pipe_list))!=0) 
	{
		for( cur_node = LIST_BEGIN( gp_ulm_pipe_manager->_choke_pipe_list);
			cur_node != LIST_END( gp_ulm_pipe_manager->_choke_pipe_list );
			cur_node = LIST_NEXT(cur_node))
		{
			p_pipe_info = (UPM_PIPE_INFO*)LIST_VALUE( cur_node );
			sd_memset(&pipe_info,0, sizeof(PEER_PIPE_INFO));
			if( p_pipe_info->_pipe->_data_pipe_type == P2P_PIPE )
			{
				ret_val = p2p_pipe_get_peer_pipe_info( p_pipe_info->_pipe,&pipe_info );
			}
			else if( p_pipe_info->_pipe->_data_pipe_type == BT_PIPE )
			{
				ret_val = bt_pipe_get_peer_pipe_info( p_pipe_info->_pipe, &pipe_info );
			}
			else
			{
				sd_assert(FALSE);
			}
	        	sd_assert( ret_val == SUCCESS );	
				
			sd_memset(str_buffer,0,2048);
			sprintf(str_buffer,"\n0x%X,ty=%d,stat=%d,sco=%2u[%u+%u+%u+%2u],", 
				(_u32)(p_pipe_info->_pipe),p_pipe_info->_type,dp_get_upload_state(p_pipe_info->_pipe ),p_pipe_info->_score,p_pipe_info->_type_score,p_pipe_info->_file_score,p_pipe_info->_ref_score,p_pipe_info->_t_s_score); 

			sd_memset(str_buffer2,0,1024);
			sprintf(str_buffer2,"up:%7u,ex:%15s,in:%15s,ct:%2u,speed:%7u,score:%7u,state:%1u", 
			pipe_info._upload_speed,pipe_info._external_ip, pipe_info._internal_ip, pipe_info._connect_type, pipe_info._speed,pipe_info._score, pipe_info._pipe_state );

			sd_strcat(str_buffer,str_buffer2,sd_strlen(str_buffer2));
			if(p_pipe_info->_type==ULM_PU_P2P_PIPE)
			{
				sd_strcat(str_buffer,"\n",sd_strlen("\n"));
				file_name = upm_get_file_name(p_pipe_info->_gcid);
				sd_strcat(str_buffer,file_name,sd_strlen(file_name));
			}
			sd_write( g_file_id,str_buffer, sd_strlen(str_buffer), &writesize);

			if(count++>=5) break;
		}
	}
	
	sd_write( g_file_id,"\n-----------------------------", sd_strlen("\n-----------------------------"), &writesize);
}


#endif

#endif /* UPLOAD_ENABLE */

