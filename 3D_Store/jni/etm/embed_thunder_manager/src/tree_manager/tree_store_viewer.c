#if 0

#include <stdio.h>

#ifdef LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h> 
#include <memory.h>
#include <malloc.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#elif WINCE
#include <winsock2.h>
#include <windows.h>
#endif

typedef unsigned char		_u8;
typedef unsigned short		_u16;
typedef unsigned int		_u32;
typedef  int		_int32;

typedef unsigned long long	_u64;

#if defined(LINUX)
#ifndef NULL
#define NULL	((void*)(0))
#endif
#endif

#ifndef TRUE
typedef _int32 BOOL;
#define TRUE	(1)
#define FALSE	(0)
#else
typedef _int32 BOOL;
#endif


#define SUCCESS							(0)

/* Structures for store file head */
typedef struct t_em_store_head
{
	_u16 _crc;	// 由于task_store.dat和tree_store.dat需要动态修改，无法进行全文的CRC校验，所以这个全文的CRC只在etm_store.dat中有效
	_u16 _ver;
	_u32  _len; //从_u32 _count开始到这个文件的最后一byte的长度
} EM_STORE_HEAD;

/* Structures for task storing */
typedef struct t_dt_task_store_head
{
	_u16 _valid; 	
	_u16 _crc;	
	_u32 _len;	
}DT_TASK_STORE_HEAD;


/* Structures of tree node for  storing */
typedef struct t_tree_node_store{
	_u16 _valid; 	
	_u16 _crc;		
	_u32 _len;	
	_u32 _node_id;		
	_u32 _parent_id;
	_u32 _pre_node_id;
	_u32 _nxt_node_id;
	_u32 _first_child_id;
	_u32 _last_child_id;
	_u32 _children_num;
	_u32 _data_len;
	_u32 _name_len;
	//void *_data;  
	//char *_name;  
}TREE_NODE_STORE;


_int32 em_pread(_u32 file_id, char *buffer, _int32 size, _u64 filepos, _u32 *readsize)
{
	_int32 ret_val = SUCCESS;

	(*readsize) = 0;

	do
	{
		ret_val = pread64(file_id, buffer, size, filepos);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*readsize = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
	}
	return ret_val;
}

#define TRM_POS_TOTAL_NODE_NUM 		(sizeof(EM_STORE_HEAD))
#define TRM_POS_NODE_START 			(TRM_POS_TOTAL_NODE_NUM+sizeof(_u32))

int main(int argc, char *argv[])
{
	int ret_val =0;
	unsigned int file_id = 0,i;
	unsigned int read_size = 0;
	char * file_name = argv[1];
	
	EM_STORE_HEAD file_head;
	_u32 total_node_num=0,node_count=0;		
	TREE_NODE_STORE node;
	_u64 g_pos_node_file_end=TRM_POS_NODE_START;
	char buffer[512];
	char * user_data=NULL;
	BOOL need_release = FALSE;

	file_id = open(file_name, O_RDONLY, 0640);
	if(file_id == -1)
	{
		/* Opening File error */
		printf("\nOpening tree file Error!\n");
		return -1;
	}
	
	printf("\n*******************************************************************\n");
	read_size = read(file_id, &file_head, sizeof(EM_STORE_HEAD));
	if(read_size!=sizeof(EM_STORE_HEAD) )
	{
		printf("\n Reading File Head Error:read_size=%u\n",read_size);
		goto ErrHandler;
	}

	printf("\nfile_crc=%d, file_version=%d, file_data_len=%u\n",file_head._crc,file_head._ver,file_head._len);
	
	read_size = read(file_id, &total_node_num, sizeof(_u32));
	if(read_size!=sizeof(_u32) )
	{
		printf("\n Reading Total Node Number Error:read_size=%u\n",read_size);
		goto ErrHandler;
	}
	printf("\ntotal_node_num=%u\n",total_node_num);
	
	
	
	while(TRUE)
	{
		ret_val = em_pread(file_id, (char*)&node, sizeof(TREE_NODE_STORE),g_pos_node_file_end,&read_size);
		if((ret_val!=SUCCESS)||(read_size!=sizeof(TREE_NODE_STORE) ))
		{
			if((ret_val!=SUCCESS)||(read_size!=0))
			{
				printf("\n Reading Node Error:ret_val=%d,read_size=%u\n",ret_val,read_size);
			}
			goto ErrHandler;
		}

		printf("\n----------------------------------------------\n");
		printf("\nFind a node:node._valid=%u, node._crc=%u, node._len=%u\n",node._valid,node._crc,node._len);
		if(node._valid==0)
		{
			g_pos_node_file_end+=sizeof(TREE_NODE_STORE);
			g_pos_node_file_end+=node._name_len+node._data_len;
			continue;
		}

		printf( "\nGet Node info[%5llu]:\nnode_id=%3u, parent=%3u, pre_node=%3u, nxt_node=%3u, first_child=%3u, last_child=%3u, children_num=%2u, data_len=%2u, name_len=%u\n",g_pos_node_file_end,
				node._node_id,node._parent_id,node._pre_node_id,node._nxt_node_id,node._first_child_id,node._last_child_id,node._children_num,node._data_len,node._name_len);

		g_pos_node_file_end+=read_size;
		
		if(node._data_len!=0)
		{
			if(node._data_len>512)
			{
				user_data=(char*) malloc(node._data_len);
				if(user_data==NULL)
				{
					printf("\n data:malloc Error!\n");
					goto ErrHandler;
				}
				need_release = TRUE;
			}
			else
			{
				user_data = buffer;
			}
			ret_val=em_pread(file_id, (char *)user_data,node._data_len, g_pos_node_file_end, &read_size);
			if((ret_val!=SUCCESS)||(read_size!=node._data_len))
			{
				if(need_release)
				{
					free(user_data);
					need_release=FALSE;
				}
				printf("\n Reading Data Error:ret_val=%d,read_size=%u\n",ret_val,read_size);
				goto ErrHandler;
			}
			printf("Node Data=");
			for(i=0;i<node._data_len;i++)
			{
				if(i%20==0)
				{
					printf("\n");
				}
				printf("0x%02X,",user_data[i]);
			}
			
			printf("\n\nNode Data in string format=%s\n",user_data);
			if(need_release)
			{
				free(user_data);
				need_release=FALSE;
			}
			
			g_pos_node_file_end+=read_size;
		}

		if(node._name_len!=0)
		{
			ret_val=em_pread(file_id, (char *)buffer,node._name_len, g_pos_node_file_end, &read_size);
			if((ret_val!=SUCCESS)||(read_size!=node._name_len))
			{
				printf("\n Reading Node Name Error:ret_val=%d,read_size=%u\n",ret_val,read_size);
				goto ErrHandler;
			}
			buffer[node._name_len]='\0';
			printf("\nnode_name=%s\n",buffer);
			g_pos_node_file_end+=read_size;
		}
		
		node_count++;
	}

ErrHandler:
	printf("\n************** End of reading nodes,valid node number=%u  *********************\n",node_count);
	close(file_id);	 
	return ret_val;
}

#endif

