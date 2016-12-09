/*
 * crosslink.c
 *
 *  Created on: 2010-12-25
 *      Author: lihai
 */
#include "utility/crosslink.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"

static SLAB *gp_crosslinkslab = NULL;


_int32 crosslink_alloctor_init(void)
{
	_int32 ret_val = SUCCESS;

	if(!gp_crosslinkslab)
	{
		ret_val = mpool_create_slab(sizeof(CROSSLINK_NODE), MIN_CROSSLINK_MEMORY, 0, &gp_crosslinkslab);
		CHECK_VALUE(ret_val);
	}

	return ret_val;
}

_int32 crosslink_alloctor_uninit(void)
{
	_int32 ret_val = SUCCESS;

	if(gp_crosslinkslab)
	{
		ret_val = mpool_destory_slab(gp_crosslinkslab);
		CHECK_VALUE(ret_val);
		gp_crosslinkslab = NULL;
	}

	return ret_val;
}

void crosslink_init(CROSSLINK *crosslink, _int32 row, _int32 col)
{
	_int32 ret_val = SUCCESS;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE left_sibiling = NULL;
	pCROSSLINK_NODE up_sibiling = NULL;
	pCROSSLINK_NODE up_sibiling_bak = NULL;
	_int32 i,j;
	
	sd_memset(crosslink, 0, sizeof(CROSSLINK));
	
	crosslink->row = row;
	crosslink->col = col;
	
	for(j=0; j<row; j++)
	{
		for(i=0; i<col; i++)
		{
			ret_val = mpool_get_slip(gp_crosslinkslab, (void**)&temp);
			sd_assert(ret_val == SUCCESS);

			sd_memset(temp, 0, sizeof(CROSSLINK_NODE));

			if(i==0 && j==0)
			{
				crosslink->head = temp;
			}
			if(i==col-1 && j==row-1)
			{
				crosslink->tail = temp;
			}
			if(i == 0)
			{
				up_sibiling_bak = temp;
			}
			if(left_sibiling != NULL)
			{
				left_sibiling->right_sibiling = temp;
			}
			temp->left_sibiling = left_sibiling;
			left_sibiling = temp;
			if(up_sibiling != NULL)
			{
				up_sibiling->down_sibiling = temp;
			}
			temp->up_sibiling = up_sibiling;
			if(up_sibiling != NULL)
			{
				up_sibiling = up_sibiling->right_sibiling;
			}
		}
		up_sibiling = up_sibiling_bak;
		left_sibiling = NULL;
	}
}

void crosslink_destroy(CROSSLINK *crosslink)
{
	_int32 ret_val = SUCCESS;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE right_sibiling = NULL;
	pCROSSLINK_NODE down_sibiling = NULL;
	
	temp = crosslink->head;
	if(temp == NULL)
	{
		return ;
	}
	while(temp != NULL)
	{
		down_sibiling = temp->down_sibiling;
		while(temp != NULL)
		{
			right_sibiling = temp->right_sibiling;
			ret_val = mpool_free_slip(gp_crosslinkslab, temp);
			sd_assert(ret_val == SUCCESS);
			temp = right_sibiling;
		}
		temp = down_sibiling;
	}	
}

_u32 crosslink_row(const CROSSLINK *crosslink)
{
	if(crosslink)
		return crosslink->row;
	else
		return 0;
}

_u32 crosslink_col(const CROSSLINK *crosslink)
{
	if(crosslink)
		return crosslink->col;
	else
		return 0;
}

_int32 crosslink_push(CROSSLINK *crosslink, void *data, _int32 row, _int32 col)
{
	int i;
	CROSSLINK_ITERATOR iterator;
	if(row < 0 || row >= crosslink->row || col < 0 || col >= crosslink->col)
	{
		return SD_ERROR;
	}
	for(i=0, iterator=CROSSLINK_HEAD(crosslink); iterator!=NULL&&i!=row; iterator=CROSSLINK_DOWN(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return SD_ERROR;
	}
	for(i=0; iterator!=NULL&&i!=col; iterator=CROSSLINK_RIGHT(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return SD_ERROR;
	}
	iterator->_data = data;
	return SUCCESS;
}

_int32 crosslink_pop(CROSSLINK *crosslink, void **data, _int32 row, _int32 col)
{
	int i;
	CROSSLINK_ITERATOR iterator;
	if(row < 0 || row >= crosslink->row || col < 0 || col >= crosslink->col)
	{
		return SD_ERROR;
	}
	for(i=0, iterator=CROSSLINK_HEAD(crosslink); iterator!=NULL&&i!=row; iterator=CROSSLINK_RIGHT(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return SD_ERROR;
	}
	for(i=0; iterator!=NULL&&i!=col; iterator=CROSSLINK_DOWN(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return SD_ERROR;
	}
	*data = iterator->_data;
	iterator->_data = NULL;
	return SUCCESS;
}

_int32 crosslink_insert(CROSSLINK *crosslink, void *data, CROSSLINK_ITERATOR iterator)
{
	iterator->_data = data;
	return SUCCESS;
}

_int32 crosslink_erase(CROSSLINK *crosslink, CROSSLINK_ITERATOR iterator)
{
	iterator->_data = NULL;

	return SUCCESS;
}

_int32 crosslink_clear(CROSSLINK *crosslink)
{
	//_int32 ret_val = SUCCESS;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE right_sibiling = NULL;
	pCROSSLINK_NODE down_sibiling = NULL;
	
	temp = crosslink->head;
	if(temp == NULL)
	{
		return SUCCESS;
	}
	while(temp != NULL)
	{
		down_sibiling = temp->down_sibiling;
		while(temp != NULL)
		{
			right_sibiling = temp->right_sibiling;
			temp->_data = NULL;
			temp = right_sibiling;
		}
		temp = down_sibiling;
	}	
	return SUCCESS;
}

CROSSLINK_ITERATOR get_crosslink_cell(CROSSLINK *crosslink, _int32 row, _int32 col)
{
	int i;
	CROSSLINK_ITERATOR iterator;
	if(row < 0 || row >= crosslink->row || col < 0 || col >= crosslink->col)
	{
		return NULL;
	}
	for(i=0, iterator=CROSSLINK_HEAD(crosslink); iterator!=NULL&&i!=col; iterator=CROSSLINK_RIGHT(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return NULL;
	}
	for(i=0; iterator!=NULL&&i!=row; iterator=CROSSLINK_DOWN(iterator), i++)
	{
		;
	}
	if(iterator==NULL)
	{
		return NULL;
	}
	return iterator;
}
_int32 crosslink_add_row(CROSSLINK *crosslink, _int32 pos/*ROW_TOP,ROW_BOTTOM*/)
{
	_int32 ret_val = SUCCESS;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE left_sibiling = NULL;
	pCROSSLINK_NODE up_sibiling = NULL;
	pCROSSLINK_NODE down_sibiling = NULL;
	_int32 i;
	
	if(pos != ROW_TOP && pos != ROW_BOTTOM)
		return SD_ERROR;
	
	if(crosslink->col == 0)
		crosslink->col++;
	crosslink->row++;
	
	if(pos == ROW_TOP)
	{
		down_sibiling = crosslink->head;
		for(i=0; i<crosslink->col; i++)
		{
			ret_val = mpool_get_slip(gp_crosslinkslab, (void**)&temp);
			CHECK_VALUE(ret_val);

			sd_memset(temp, 0, sizeof(CROSSLINK_NODE));

			if(i==0)
			{
				crosslink->head = temp;
			}
			if(crosslink->col == i+1 && crosslink->row == 1)
			{
				crosslink->tail = temp;
			}
			if(left_sibiling != NULL)
			{
				left_sibiling->right_sibiling = temp;
			}
			temp->left_sibiling = left_sibiling;
			left_sibiling = temp;
			if(down_sibiling != NULL)
			{
				down_sibiling->up_sibiling = temp;
			}
			temp->down_sibiling = down_sibiling;
			if(down_sibiling != NULL)
			{
				down_sibiling = down_sibiling->right_sibiling;
			}
		}
	}
	if(pos == ROW_BOTTOM)
	{
		up_sibiling = crosslink->head;
		if(up_sibiling != NULL)
		{
			while(up_sibiling->down_sibiling != NULL)
				up_sibiling = up_sibiling->down_sibiling;
		}
		for(i=0; i<crosslink->col; i++)
		{
			ret_val = mpool_get_slip(gp_crosslinkslab, (void**)&temp);
			CHECK_VALUE(ret_val);

			sd_memset(temp, 0, sizeof(CROSSLINK_NODE));

			if(crosslink->row == 1)
			{
				crosslink->head = temp;
			}
			if(crosslink->col == i+1)
			{
				crosslink->tail = temp;
			}
			if(left_sibiling != NULL)
			{
				left_sibiling->right_sibiling = temp;
			}
			temp->left_sibiling = left_sibiling;
			left_sibiling = temp;
			if(up_sibiling != NULL)
			{
				up_sibiling->down_sibiling = temp;
			}
			temp->up_sibiling = up_sibiling;
			if(up_sibiling != NULL)
			{
				up_sibiling = up_sibiling->right_sibiling;
			}
		}
	}
	return SUCCESS;
}

_int32 crosslink_add_col(CROSSLINK *crosslink, _int32 pos/*COL_LEFT,COL_RIGHT*/)
{
	_int32 ret_val = SUCCESS;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE up_sibiling = NULL;
	pCROSSLINK_NODE left_sibiling = NULL;
	pCROSSLINK_NODE right_sibiling = NULL;
	_int32 i;
	
	if(pos != COL_LEFT && pos != COL_RIGHT)
		return SD_ERROR;
	
	if(crosslink->row == 0)
		crosslink->row++;
	crosslink->col++;
	
	if(pos == COL_LEFT)
	{
		right_sibiling = crosslink->head;
		for(i=0; i<crosslink->row; i++)
		{
			ret_val = mpool_get_slip(gp_crosslinkslab, (void**)&temp);
			CHECK_VALUE(ret_val);

			sd_memset(temp, 0, sizeof(CROSSLINK_NODE));

			if(i==0)
			{
				crosslink->head = temp;
			}
			if(crosslink->row == i+1 && crosslink->col == 1)
			{
				crosslink->tail = temp;
			}
			if(up_sibiling != NULL)
			{
				up_sibiling->down_sibiling = temp;
			}
			temp->up_sibiling = up_sibiling;
			up_sibiling = temp;
			temp->right_sibiling = right_sibiling;
			if(right_sibiling != NULL)
			{
				right_sibiling->left_sibiling = temp;
			}
			if(right_sibiling != NULL)
			{
				right_sibiling = right_sibiling->down_sibiling;
			}
		}
	}
	if(pos == COL_RIGHT)
	{
		left_sibiling = crosslink->head;
		if(left_sibiling != NULL)
		{
			while(left_sibiling->right_sibiling != NULL)
				left_sibiling = left_sibiling->right_sibiling;
		}
		for(i=0; i<crosslink->row; i++)
		{
			ret_val = mpool_get_slip(gp_crosslinkslab, (void**)&temp);
			CHECK_VALUE(ret_val);

			sd_memset(temp, 0, sizeof(CROSSLINK_NODE));

			if(crosslink->col == 1 && i == 0)
			{
				crosslink->head = temp;
			}
			if(crosslink->row == i+1)
			{
				crosslink->tail = temp;
			}
			if(up_sibiling != NULL)
			{
				up_sibiling->down_sibiling = temp;
			}
			temp->up_sibiling = up_sibiling;
			up_sibiling = temp;
			if(left_sibiling != NULL)
			{
				left_sibiling->right_sibiling = temp;
			}
			temp->left_sibiling = left_sibiling;
			if(left_sibiling != NULL)
			{
				left_sibiling = left_sibiling->down_sibiling;
			}
		}
	}
	return SUCCESS;
}

_int32 crosslink_delete_row(CROSSLINK *crosslink, _int32 row)
{
	_int32 ret_val = SUCCESS;
	_int32 i = 0;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE bak = NULL;
	
	if(crosslink == NULL)
		return SD_ERROR;
	temp = crosslink->head;
	if(temp == NULL)
	{
		return SD_ERROR;
	}
	while(temp != NULL)
	{
		if(row == i)
		{
			if(row == 0)
			{
				crosslink->head = crosslink->head->down_sibiling;
			}
			if(row == crosslink->row-1)
			{
				crosslink->tail = crosslink->tail->up_sibiling;
			}
			while(temp != NULL)
			{
				bak = temp;
				if(temp->up_sibiling != NULL)
					temp->up_sibiling->down_sibiling = temp->down_sibiling;
				if(temp->down_sibiling != NULL)
					temp->down_sibiling->up_sibiling = temp->up_sibiling;
				temp = temp->right_sibiling;
				ret_val = mpool_free_slip(gp_crosslinkslab, bak);
				CHECK_VALUE(ret_val);
			}
			crosslink->row--;
			if(crosslink->row == 0)
				crosslink->col = 0;
			return SUCCESS;
		}
		temp = temp->down_sibiling;
		i++;
	}
	return SD_ERROR;
}

_int32 crosslink_delete_col(CROSSLINK *crosslink, _int32 col)
{
	_int32 ret_val = SUCCESS;
	_int32 i = 0;
	pCROSSLINK_NODE temp = NULL;
	pCROSSLINK_NODE bak = NULL;
	
	if(crosslink == NULL)
		return SD_ERROR;
	temp = crosslink->head;
	if(temp == NULL)
	{
		return SD_ERROR;
	}
	while(temp != NULL)
	{
		if(col == i)
		{
			if(col == 0)
			{
				crosslink->head = crosslink->head->right_sibiling;
			}
			if(col == crosslink->col-1)
			{
				crosslink->tail = crosslink->tail->left_sibiling;
			}
			while(temp != NULL)
			{
				bak = temp;
				if(temp->left_sibiling != NULL)
					temp->left_sibiling->right_sibiling = temp->right_sibiling;
				if(temp->right_sibiling != NULL)
					temp->right_sibiling->left_sibiling = temp->left_sibiling;
				temp = temp->down_sibiling;
				ret_val = mpool_free_slip(gp_crosslinkslab, bak);
				CHECK_VALUE(ret_val);
			}
			crosslink->col--;
			if(crosslink->col == 0)
				crosslink->row = 0;
			return SUCCESS;
		}
		temp = temp->right_sibiling;
		i++;
	}
	return SD_ERROR;
}
