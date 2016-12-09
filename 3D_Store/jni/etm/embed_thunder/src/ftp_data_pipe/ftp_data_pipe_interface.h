/*--------------------------------------------------------------------------*/
/*                               IDENTIFICATION                             */
/*--------------------------------------------------------------------------*/
/*     Filename   : ftp_data_pipe_interface.h                               */
/*     Author     : ZengYuqing                                              */
/*     Project    : EmbedThunder                                         */
/*     Module     : FTP  													*/
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
/* This file contains the platforms provided by ftp data pipe             */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                              HISTORY                                     */
/*--------------------------------------------------------------------------*/
/*   Date     |    Author   | Modification                                  */
/*--------------------------------------------------------------------------*/
/* 2008.06.14 | ZengYuqing  | Creation                                      */
/*--------------------------------------------------------------------------*/

#if !defined(__FTP_DATA_PIPE_INTERFACE_H_20080614)
#define __FTP_DATA_PIPE_INTERFACE_H_20080614

#ifdef __cplusplus
extern "C" 
{
#endif

#include "ftp_data_pipe/ftp_data_pipe.h"


/*--------------------------------------------------------------------------*/
/*                Interfaces provid for other modules				        */
/*--------------------------------------------------------------------------*/
 _int32 init_ftp_pipe_module(  void);
 _int32 uninit_ftp_pipe_module(  void);

 _int32 ftp_resource_create(  char *_url,_u32 _url_size,BOOL _is_origin,RESOURCE ** _pp_ftp_resouce);
 _int32 ftp_resource_destroy(RESOURCE ** _pp_ftp_resouce);

/* Interface for connect_manager getting file suffix  */
  char* ftp_resource_get_file_suffix(RESOURCE * _p_resource,BOOL *_is_binary_file);

/* Interface for connect_manager getting file name from resource */
 BOOL ftp_resource_get_file_name(RESOURCE * _p_resource, char * _file_name, _u32 buffer_len,BOOL _compellent);

 BOOL ftp_resource_is_support_range(RESOURCE * p_resource);
 
/*****************************************************************************
 * Interface name : ftp_pipe_create
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *    DATA_MANAGER*  _p_data_manager : 
 * pointer to DATA_MANAGER which stores the received data
 *
 * Output parameters :
 * -------------------
 *   FTP_DATA_PIPE ** _pp_ftp_pipe : 
 *  pointer to new FTP_DATA_PIPE,release this memory by ftp_pipe_destroy(...)
 *
 * Return values :
 * -------------------
 *   0 - success;other values - error
 *
 *----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*
 *                                    DESCRIPTION
 *
 *   Create a new ftp data pipe and initiate the FTP_DATA_PIPE structure    
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 _int32 ftp_pipe_create(  void*  _p_data_manager,RESOURCE*  _p_resource,DATA_PIPE ** _pp_pipe);

/*****************************************************************************
 * Interface name : ftp_pipe_open
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   FTP_DATA_PIPE * _p_ftp_pipe 
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
 * Open the ftp data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 _int32 ftp_pipe_open(DATA_PIPE * _p_pipe);

 /*****************************************************************************
 * Interface name : ftp_pipe_close
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   FTP_DATA_PIPE * _p_ftp_pipe 
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
 * Close the ftp data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
_int32 ftp_pipe_close(DATA_PIPE * _p_pipe);

 /*****************************************************************************
 * Interface name : ftp_pipe_change_ranges
 *----------------------------------------------------------------------------*
 * Input parameters  :
 * -------------------
 *   const RANGE_LIST _range_list : New ranges assigned to the pipe
 *
 *   FTP_DATA_PIPE * _p_ftp_pipe 
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
 * Change the range ftp data pipe     
 *----------------------------------------------------------------------------*
 ******************************************************************************/
 _int32 ftp_pipe_change_ranges(DATA_PIPE * _p_pipe,const RANGE *_alloc_range);




/* Interface for connect_manager getting the pipe speed */
 _int32 ftp_pipe_get_speed(DATA_PIPE * _p_pipe);

#ifdef __cplusplus
}
#endif

#endif /* __FTP_DATA_PIPE_INTERFACE_H_20080614 */
