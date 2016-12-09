/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename   : http_data_pipe_interface.h                              */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                         */
/*     Module     : HTTP													*/
/*     Version    : 1.0  													*/
/*--------------------------------------------------------------------------*/
/*                  Shenzhen XunLei Networks			                    */
/*--------------------------------------------------------------------------*/
/*                  - C (copyright) - www.xunlei.com -		     		    */
/*                                                                          */
/*      This document is the proprietary of XunLei                          */
/*                                                                          */
/*      All rights reserved. Integral or partial reproduction               */
/*      prohibited without a written authorization from the                 */
/*      permanent of the author rights.                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/*                         FUNCTIONAL DESCRIPTION                           */
/*--------------------------------------------------------------------------*/
/* This file contains the platforms provided by http data pipe             */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.06.14 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__HTTP_DATA_PIPE_INTERFACE_H_20080614)
#define __HTTP_DATA_PIPE_INTERFACE_H_20080614

#ifdef __cplusplus
extern "C" 
{
#endif


#include "http_data_pipe/http_data_pipe.h"


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
 _int32 init_http_pipe_module(  void);
 _int32 uninit_http_pipe_module(  void);


/*****************************************************************************
 * Interface name : http_pipe_create
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *    DATA_MANAGER*  _p_data_manager : 
 * pointer to DATA_MANAGER which stores the received data
 *
 * Output parameters :
 * -------------------
 *   HTTP_DATA_PIPE ** _pp_http_pipe : 
 *  pointer to new HTTP_DATA_PIPE,release this memory by http_pipe_destroy(...)
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 *   Create a new http data pipe and initiate the HTTP_DATA_PIPE structure    
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 _int32 http_pipe_create(  void*  p_data_manager,RESOURCE*  p_resource,DATA_PIPE ** pp_pipe);

/*****************************************************************************
 * Interface name : http_pipe_set_request
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   HTTP_DATA_PIPE * _p_http_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Open the http data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 
 _int32 http_pipe_set_request(DATA_PIPE * p_pipe,char* request,_u32 request_len);

/*****************************************************************************
 * Interface name : http_pipe_open
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   HTTP_DATA_PIPE * _p_http_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Open the http data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
_int32 http_pipe_open(DATA_PIPE * p_pipe);

 /*****************************************************************************
 * Interface name : http_pipe_close
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   HTTP_DATA_PIPE * _p_http_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Close the http data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
_int32 http_pipe_close(DATA_PIPE * p_pipe);

 /*****************************************************************************
 * Interface name : http_pipe_change_ranges
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   const RANGE_LIST _range_list : New ranges assigned to the pipe
 *
 *   HTTP_DATA_PIPE * _p_http_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Change the range http data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 _int32 http_pipe_change_ranges(DATA_PIPE * p_pipe,const RANGE* alloc_range );




 /*****************************************************************************
 * Interface name : http_pipe_get_speed
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *
 *   DATA_PIPE * p_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Interface for connect_manager getting the pipe speed   
 *----------------------------------------------------------------------------*
 ******************************************************************************/

 _int32 http_pipe_get_speed(DATA_PIPE * p_pipe);



 /*****************************************************************************
 * Interface name : http_pipe_destroy
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *
 *   HTTP_DATA_PIPE * _p_http_pipe 
 *
 * Output parameters :
 * -------------------
 *   NONE 
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 * Destroy the http data pipe and  release memory    
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 struct tag_HTTP_DATA_PIPE;
 _int32 http_pipe_destroy( struct tag_HTTP_DATA_PIPE * p_http_pipe);

 _int32 http_get_1024_buffer(void** pp_buffer);
 _int32 http_release_1024_buffer(void* p_buffer);
 _u32 http_get_every_time_reveive_range_num(void);

#ifdef __cplusplus
}
#endif

#endif /* __HTTP_DATA_PIPE_INTERFACE_H_20080614 */
