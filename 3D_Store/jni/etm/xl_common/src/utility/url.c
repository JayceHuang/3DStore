#if !defined(__URL_C_20080805)
#define __URL_C_20080805

#include "utility/url.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "platform/sd_fs.h"
#include "utility/sd_iconv.h"
#include "utility/local_ip.h"
#include "utility/md5.h"
#include "utility/base64.h"
#include "utility/logid.h"
#ifdef LOGID
#undef LOGID
#endif

#define LOGID LOGID_COMMON

#include "utility/logger.h"

#define POSTFIXS_SIZE 56

const static  char* postfixs[POSTFIXS_SIZE] =
{
    ".apk", ".jpg",".png",".gif",".bmp",".tiff",".raw",             /* 图片 */
    ".chm",".doc",".docx",".pdf",".txt",    ".xls",".xlsx",".ppt",  ".pptx",                /* 文本 */
    ".aac",".ape",".amr",".mp3",".ogg",".wav",".wma",               /* 音频 */
    ".xv",".avi",".asf",".mpeg",".mpga", ".mpg",".mp4",".m3u",".mov",".wmv",".rmvb",".rm",".3gp", ".dat",".flv",".mkv",".xlmv",".swf", /* 视频 */
    ".exe", ".msi", ".jar" , ".ipa", ".img",                    /* 应用程序 */
    ".rar",  ".zip",   ".iso",  ".bz2", ".tar",".ra",  ".gz" ,".7z",        /* 压缩文档 */
    ".bin",".torrent"                                  /* 其他 */
};

///HTTP=0, FTP , MMS, HTTPS, MMST, PEER, RTSP, RTSPT, FTPS, SFTP, UNKNOWN
const static char *g_url_type[] =
{
    "http://",
    "ftp://",
    "mms://",
    "https://",
    "mmst://",
    "peer://",
    "rtsp://",
    "rtspt://",
    "ftps://",
    "sftp://",
    "UNKNOWN"
};

/* @Simple Function@
* Return : TRUE or FALSE
*/
BOOL sd_is_dynamic_url(char* _url)
{
    LOG_DEBUG( "sd_is_dynamic_url" );

    if((( sd_strstr(_url, "http://", 0)!=NULL)||( sd_strstr(_url, "https://", 0)!=NULL))&&( sd_strchr(_url, '?', 0)!=NULL))
    {
        /* Dynamic URL */
        return TRUE;
    }
    return FALSE;
}

char* sd_find_url_head(char* _url,_u32 url_length)
{
    char * head=NULL;
    char  _url_tmp [MAX_URL_LEN]= {0};

    if(_url==NULL||url_length<MIN_URL_LEN) return NULL;

    sd_assert(url_length<MAX_URL_LEN);
    sd_memcpy(_url_tmp, _url, url_length);
    _url_tmp[url_length]='\0';

    sd_string_to_low_case(_url_tmp);

    head = sd_strstr(_url_tmp, "http://", 0);
    if((head!=NULL )&&(head-_url_tmp<url_length-MIN_URL_LEN)) return _url+(head-_url_tmp);

    head = sd_strstr(_url_tmp, "ftp://", 0);
    if((head!=NULL )&&(head-_url_tmp<url_length-MIN_URL_LEN)) return _url+(head-_url_tmp);

    head = sd_strstr(_url_tmp, "https://", 0);
    if((head!=NULL )&&(head-_url_tmp<url_length-MIN_URL_LEN)) return _url+(head-_url_tmp);

    return NULL;
}
_int32 sd_url_check_host(char *host)
{
    char * point = sd_strchr(host, '.', 0);
    if((point!=NULL)&&(point!=host)&&(sd_strlen(point)>1))
    {
        return SUCCESS;
    }
    return -1;
}
/* Change URL from char* to URL_OBJECT
* Return : 0: success; other:error
*/
_int32 sd_url_to_object(const char* _url,_u32 url_length,URL_OBJECT* _url_o)
{
    // _u32 url_length=strlen(_url);
    //_int32 pos = 0,pos1 = 0,pos2 = 0,pos3 =0;
    char  _url_tmp [MAX_URL_LEN],temp3[MAX_URL_LEN];
    char *_url_t=_url_tmp, *_url_t2=NULL,*pos=NULL,*pos1=NULL ,*pos2=NULL ,*pos3=NULL,*qm=NULL ,*url_head=NULL;
    char * _eq_pos =NULL,*_qm2_pos =NULL,*_point_pos =NULL,*_underline_pos =NULL,*full_path_pos=NULL,*dd_pos=NULL;
    _u32 _port = HTTP_DEFAULT_PORT;
    BOOL is_dynamic_domain=FALSE;

    LOG_DEBUG( "sd_url_to_object[%s]",_url);

    if((_url==NULL)||(_url_o==NULL)||(url_length<MIN_URL_LEN)||(url_length>=MAX_URL_LEN))
        return -1;

    url_head = sd_find_url_head( (char*)_url, url_length);
    if(url_head==NULL) return -1;

    sd_memset(_url_tmp,0,MAX_URL_LEN);
    sd_memcpy(_url_tmp,url_head,url_length-(url_head-_url));

    sd_memset(_url_o,0,sizeof(URL_OBJECT));
    ///HTTP=0, FTP , MMS, HTTPS, MMST, PEER, RTSP, RTSPT, FTPS, SFTP, UNKNOWN
    _url_o->_schema_type = sd_get_url_type(_url_t, sd_strlen(_url_t));

    if(_url_o->_schema_type == UNKNOWN)
    {
        LOG_DEBUG("URL error...");
        return -1;
    }

    if(_url_o->_schema_type!=FTP)
    {
        if(_url_o->_schema_type==HTTP)
        {
            pos = sd_strstr(_url_t, "://",  0);
            if(pos==NULL)
                return -1;

            _url_t=pos+sd_strlen("://");
        }
        else
        {
            pos = sd_strstr(_url_t, "://",  0);
            if(pos==NULL)
                return -1;

            _url_t=pos+sd_strlen("://");
            _port = HTTPS_DEFAULT_PORT;
        }

        pos=sd_strchr(_url_t, '/',0);
        qm=sd_strchr(_url_t, '?',  0);
        if((pos!=NULL)&&( qm!=NULL)&&(qm>pos))
        {
            /* Dynamic URL */
            _url_o->_is_dynamic_url = TRUE;
        }
        else if( qm!=NULL)
        {
            dd_pos=sd_strstr(_url_t, "?domain=",  0);
            if(dd_pos!=NULL)
            {
                /* Dynamic domain */
                is_dynamic_domain = TRUE;
                qm=sd_strchr(dd_pos+1, '?',  0);
                pos=sd_strchr(dd_pos+1, '/',0);
                if((pos!=NULL)&&( qm!=NULL)&&(qm>pos))
                {
                    /* Dynamic URL */
                    _url_o->_is_dynamic_url = TRUE;
                }
                else if( qm!=NULL)
                {
                    LOG_DEBUG("URL qm error...");
                    return -1;
                }
            }
            else
            {
                LOG_DEBUG("URL domain error...");
                return -1;
            }
        }


    }
    else
    {
        pos = sd_strstr(_url_t, "://",  0);
        if(pos==NULL)
            return -1;

        _url_t=pos+sd_strlen("://");;
        _port = 21;
    }



    /* Find the user name and password */
    pos = sd_strchr(_url_t, '/', 0);

    if(pos!=NULL)
    {
        sd_memset(temp3,0,MAX_URL_LEN);
        sd_memcpy(temp3,_url_t,pos-_url_t);
        pos1 = sd_strrchr(temp3, '@');
        if(pos1!=NULL)
            pos1 =_url_t+( pos1-temp3);
        else
        {
            pos2 = sd_strchr(temp3, ':', 0);
            if( pos2!=NULL)
            {
                _point_pos= sd_strrchr(temp3, '.');
                if( _point_pos==NULL)
                {
                    LOG_DEBUG("URL ERROR:no domain...[%s]",_url_t);
                    return -1 ;
                }

                if(pos2<_point_pos)
                {
                    /* Get user name but no passwords */
                    pos1 =_url_t+( pos2-temp3)+1;
                    pos2=NULL;
                    _point_pos=NULL;
                }
            }
        }
    }
    else
    {
        pos1 = sd_strrchr(_url_t, '@');
        if(pos1==NULL)
        {
            pos2 = sd_strchr(_url_t, ':', 0);
            if( pos2!=NULL)
            {
                _point_pos= sd_strrchr(_url_t, '.');
                if( _point_pos==NULL)
                {
                    LOG_DEBUG("URL ERROR:no domain...[%s]",_url_t);
                    return -1 ;
                }

                if(pos2<_point_pos)
                {
                    /* Get user name but no passwords */
                    pos1 =pos2+1;
                    pos2=NULL;
                    _point_pos=NULL;
                }
            }
        }
    }

    if((( pos1 != NULL)&&( pos != NULL)&&( pos1 < pos))
       ||(( pos1 != NULL)&&( pos ==NULL)))
    {
        pos2 = sd_strchr(_url_t, ':', 0);
        if( pos2!=NULL)
        {
            if(pos2<pos1)
            {
                /* User name */
                if( pos2-_url_t>=MAX_USER_NAME_LEN)
                {
                    LOG_DEBUG("URL ERROR:User name too long...[%s]",_url_t);
                    return -1 ;
                }

                sd_memset(_url_o->_user,0,MAX_USER_NAME_LEN);
                if(pos2-_url_t>MAX_USER_NAME_LEN-1)
                    sd_strncpy(_url_o->_user, _url_t, MAX_USER_NAME_LEN-1);
                else
                    sd_memcpy(_url_o->_user, _url_t, pos2-_url_t);

                /* password */
                if(pos1!=pos2+1)
                {
                    if( pos1-pos2-1>=MAX_PASSWORD_LEN)
                    {
                        LOG_DEBUG("URL ERROR:Pass word too long...[%s]",pos2+1);
                        return -1 ;
                    }

                    sd_memset(_url_o->_password,0,MAX_PASSWORD_LEN);
                    if(pos1-pos2-1>MAX_PASSWORD_LEN-1)
                        sd_strncpy(_url_o->_password, pos2+1,MAX_PASSWORD_LEN-1);
                    else
                        sd_memcpy(_url_o->_password, pos2+1, pos1-pos2-1);
                }
            }
            else
            {
                /* No password??*/
                LOG_DEBUG("WARNING:no pass words in this url!");
                /* User name */
                if( pos1-_url_t>=MAX_USER_NAME_LEN)
                {
                    LOG_DEBUG("URL ERROR:User name too long...[%s]",_url_t);
                    return -1 ;
                }

                sd_memset(_url_o->_user,0,MAX_USER_NAME_LEN);
                if(pos1-_url_t>MAX_USER_NAME_LEN-1)
                    sd_strncpy(_url_o->_user, _url_t,MAX_USER_NAME_LEN-1);
                else
                    sd_memcpy(_url_o->_user, _url_t, pos1-_url_t);
            }
        }
        else
        {
            /* No password??*/
            LOG_DEBUG("WARNING:no pass words in this url!");
            /* User name */
            if( pos1-_url_t>=MAX_USER_NAME_LEN)
            {
                LOG_DEBUG("URL ERROR:User name too long...[%s]",_url_t);
                return -1 ;
            }

            sd_memset(_url_o->_user,0,MAX_USER_NAME_LEN);
            if(pos1-_url_t>MAX_USER_NAME_LEN-1)
                sd_strncpy(_url_o->_user, _url_t,MAX_USER_NAME_LEN-1);
            else
                sd_memcpy(_url_o->_user, _url_t, pos1-_url_t);
        }

        if(pos1[0]=='@')
            _url_t=pos1+1;
        else
            _url_t=pos1;

        pos1 = NULL;
        pos2 = NULL;

        sd_memset(temp3,0,MAX_URL_LEN);
        if(url_object_decode_ex(_url_o->_user,temp3,MAX_URL_LEN)!=-1)
        {
            sd_memset(_url_o->_user, 0,MAX_USER_NAME_LEN);
            sd_strncpy(_url_o->_user, temp3,MAX_USER_NAME_LEN);
        }
        else
            return -1;

        if(_url_o->_password[0]!='\0')
        {
            sd_memset(temp3,0,MAX_URL_LEN);
            if(url_object_decode_ex(_url_o->_password,temp3,MAX_URL_LEN)!=-1)
            {
                sd_memset(_url_o->_password, 0,MAX_PASSWORD_LEN);
                sd_strncpy(_url_o->_password, temp3,MAX_PASSWORD_LEN);
            }
            else
                return -1;
        }

    }
    else
    {
        LOG_DEBUG("WARNING:no user name and pass words in this url!");
    }
    /* Discard the 2nd url like this: "http://www.abc.com/my_file.exe  http://www.def.com" */
    pos = sd_strstr(_url_t, "http://",  0);
    if((pos!=NULL)&&(qm==NULL))
        pos[0]='\0';

    /*  Discard all the \t \r \n ' ' from the tail of url */
    sd_trim_postfix_lws( _url_t );

    /* Find the start positon of path */
    pos1 = sd_strchr(_url_t, '/', 0);
    if( pos1 == NULL)
        pos1 =_url_t+ sd_strlen(_url_t);

    /*  Port position */
    pos2 = sd_strchr(_url_t,':', 0);
    if(( pos2 == NULL)||( pos2 > pos1))
    {
        /* No port */
        pos2=pos1;
    }

    /* The length of the host name must be larger than 2 bytes */
    if(pos2-_url_t<3)
        return -1;

    /* Host */
    sd_memset(_url_o->_host,0,MAX_HOST_NAME_LEN);
    if(is_dynamic_domain!=TRUE)
    {
        if( pos2-_url_t>=MAX_HOST_NAME_LEN)
        {
            LOG_DEBUG("URL ERROR:Host too long...[%s]-[%s]>%d",pos2,_url_t,MAX_HOST_NAME_LEN);
            return -1 ;
        }

        sd_memcpy(_url_o->_host, _url_t, pos2-_url_t);
        if(sd_url_check_host(_url_o->_host)!=SUCCESS)
        {
            LOG_DEBUG("URL ERROR:Host error...[%s]",_url_o->_host);
            return -1 ;
        }
    }
    else
    {
        //http://59.37.54.74?domain=xianexs.mail.qq.com
        if((dd_pos>pos2)||(dd_pos<_url_t)||( dd_pos-_url_t>=MAX_HOST_NAME_LEN)||( dd_pos-_url_t<3))
        {
            LOG_DEBUG("URL ERROR:dynamic_domain error...[%s],[%s],[%s]",_url_t,dd_pos,pos2);
            return -1 ;
        }

        sd_memcpy(_url_o->_host, _url_t, dd_pos-_url_t);
        if(sd_url_check_host(_url_o->_host)!=SUCCESS)
        {
            LOG_DEBUG("URL ERROR:Host error...[%s]",_url_o->_host);
            return -1 ;
        }
    }

    if(pos2[0]!='\0')
    {
        /* Port */
        pos2++;
        for(; pos2<pos1; pos2++)
        {
            if((_url_t[pos2-_url_t]<'0')||(_url_t[pos2-_url_t]>'9'))
            {
                LOG_DEBUG("URL ERROR:port error...%s[%d]=%c",_url_t,pos2-_url_t,_url_t[pos2-_url_t]);
                return -1;
            }
            else
            {
                _url_o->_port   *=   10;
                _url_o->_port   +=   _url_t[pos2-_url_t]   -   '0';

            }
        }
    }

    if(_url_o->_port == 0) _url_o->_port = _port;  /* Default port */
    /* Path and file name */
    if(pos1-_url_t != sd_strlen(_url_t))
    {
        _url_t=pos1;
        if(sd_strlen(_url_t) == 0)
            return -1;
        /* Full Path */
        if( sd_strlen(_url_t)>=MAX_FULL_PATH_BUFFER_LEN)
        {
            LOG_DEBUG("URL ERROR:Full path too long...[%s]",_url_t);
            return -1;
        }

        sd_memset(_url_o->_full_path,0,MAX_FULL_PATH_BUFFER_LEN);
        full_path_pos = sd_strchr(_url_t, '#',0);
        if (full_path_pos == NULL)
        {
            sd_strncpy(_url_o->_full_path, _url_t,MAX_FULL_PATH_BUFFER_LEN);
        }
        else
        {
            //sd_strncpy(_url_o->_full_path, _url_t, full_path_pos - _url_t);
            sd_strncpy(_url_o->_full_path, _url_t,MAX_FULL_PATH_BUFFER_LEN);
            LOG_DEBUG("URL WARNING:url with '#'!!...[%s]",_url_t);
            //full_path_pos[0]='\0';
            //#ifdef _DEBUG
            //CHECK_VALUE(-1);
            //#endif
        }

        //_url_t指向路径的第一个字符
        //_url_t2指向文件名的第一个字符
        _url_t2 = _url_t;
	 if (_url_t[0]!='\0')//考虑到只有域名，没有根目录/的情况
	 {
	        _url_t2 = _url_t+1;
	        pos3 = sd_strchr(_url_t2, '/', 0);

	        if(qm!=NULL)
	        {

	            while((pos3!=NULL)&&(pos3<qm))
	            {
	                _url_t2=pos3+1;
	                pos3 = sd_strchr(_url_t2, '/', 0);
	            }
	        }
	        else
	        {
	            while(pos3!=NULL)
	            {
	                _url_t2=pos3+1;
	                pos3 = sd_strchr(_url_t2, '/', 0);
	            }
	        }
	 }
	 
        /* Path */
        if( (_url_t2 - _url_t) >0 )
        {
            if( _url_t2-_url_t>=MAX_FULL_PATH_LEN)
            {
                LOG_DEBUG("URL ERROR:Path too long...[%s]",_url_t);
                return -1;
            }

            _url_o->_path = _url_o->_full_path;
            _url_o->_path_len = _url_t2-_url_t;

            //sd_memset(_url_o->_path,0,MAX_FILE_PATH_LEN);
            //if(_url_t2-_url_t>MAX_FILE_PATH_LEN-1)
            //sd_strncpy(_url_o->_path, _url_t, MAX_FILE_PATH_LEN-1);
            //else
            //sd_strncpy(_url_o->_path, _url_t, _url_t2-_url_t);
            _url_o->_path_encode_mode = url_get_encode_mode( _url_o->_path ,_url_o->_path_len );
        }

        /* File name */
        if((_url_t2-_url_t)!= sd_strlen(_url_t))
        {
            if(qm==NULL)
            {
                if(sd_strlen(_url_t2) == 0)
                    return -1;

                if( sd_strlen(_url_t2)>=MAX_FULL_PATH_LEN)
                {
                    LOG_DEBUG("URL ERROR:File name too long...[%s]",_url_t2);
                    return -1;
                }

                _url_o->_file_name = _url_o->_full_path+(_url_t2-_url_t);
                _url_o->_file_name_len = sd_strlen(_url_t2);
                //sd_memset(_url_o->_file_name,0,MAX_FILE_NAME_LEN);
                //sd_strncpy(_url_o->_file_name, _url_t2, MAX_FILE_NAME_LEN-1);
                _url_o->_filename_encode_mode= url_get_encode_mode( _url_o->_file_name ,_url_o->_file_name_len );
            }
            else
            {
                if(qm-_url_t2 == 0)
                {
                    _eq_pos = sd_strrchr(qm, '=');
                    _qm2_pos = sd_strrchr(qm, '?');
                    if((_eq_pos!=NULL)&&(_qm2_pos!=NULL)&&(_qm2_pos<_eq_pos)&&(sd_strlen(_eq_pos+1)!=0))
                    {
                        _url_o->_file_name = _url_o->_full_path+(_eq_pos+1-_url_t);
                        _url_o->_file_name_len = sd_strlen(_eq_pos+1);
                        if(_url_o->_file_name_len>=MAX_FULL_PATH_LEN) return -1;
                        _url_o->_filename_encode_mode= url_get_encode_mode( _url_o->_file_name ,_url_o->_file_name_len );
                        //return 0;
                    }
                    else
                    {
                        _point_pos =  sd_strchr(qm, '.',0);
                        _underline_pos =  sd_strchr(qm, '_',0);
                        if((_point_pos!=NULL)&&(_underline_pos!=NULL)&&(_point_pos<_underline_pos)&&(_underline_pos-(qm+1)!=0))
                        {
                            _url_o->_file_name = _url_o->_full_path+(qm+1-_url_t);
                            _url_o->_file_name_len = _underline_pos-(qm+1);
                            if(_url_o->_file_name_len>=MAX_FULL_PATH_LEN) return -1;
                            _url_o->_filename_encode_mode= url_get_encode_mode( _url_o->_file_name ,_url_o->_file_name_len );
                            //return 0;
                        }
                        else
                        {
                            _url_o->_file_name = _url_o->_full_path+(qm+1-_url_t);
                            _url_o->_file_name_len = sd_strlen(qm+1);
                            if(_url_o->_file_name_len>=MAX_FULL_PATH_LEN) return -1;
                            _url_o->_filename_encode_mode= url_get_encode_mode( _url_o->_file_name ,_url_o->_file_name_len );
                            //return 0;
                        }
                    }
                }
                else
                {

                    if( qm-_url_t2>=MAX_FULL_PATH_LEN)
                    {
                        LOG_DEBUG("URL ERROR:File name too long...[%s]",_url_t2);
                        return -1;
                    }

                    _url_o->_file_name = _url_o->_full_path+(_url_t2-_url_t);
                    _url_o->_file_name_len = qm-_url_t2;
                    pos = qm-4;
                    pos1 = qm-5;
                    pos2 = qm-6;
                    if((sd_strnicmp(pos, ".php", sd_strlen(".php"))==0)
                       ||(sd_strnicmp(pos, ".htm", sd_strlen(".htm"))==0)
                       ||(sd_strnicmp(pos1, ".html", sd_strlen(".html"))==0)
                       ||(sd_strnicmp(pos2, ".shtml", sd_strlen(".shtml"))==0))
                    {
                        /* 不能用这个php 后缀的名字 */
                        char suffix_buffer[16] = {0};
                        sd_memset(temp3,0,MAX_URL_LEN);
                        pos1 = qm+1;
                        sd_strncpy(temp3, pos1, sd_strlen(pos1));
                        if( sd_is_binary_file(temp3,suffix_buffer))

                        {

                            pos2 = sd_strstr(temp3,suffix_buffer,0);
                            sd_assert(pos2!=NULL);
                            if(pos2!=NULL)
                            {
                                temp3[pos2-temp3+sd_strlen(suffix_buffer)] = '\0';
                                pos3 = sd_strrchr(temp3, '=');
                                if(pos3!=NULL&&(pos3+1-temp3>1))
                                {
                                    _url_o->_file_name = _url_o->_full_path+(qm+1-_url_t)+(pos3+1-temp3);
                                    _url_o->_file_name_len = sd_strlen(temp3);
                                }
                            }
                        }

                    }
                    //sd_memset(_url_o->_file_name,0,MAX_FILE_NAME_LEN);
                    //sd_strncpy(_url_o->_file_name, _url_t2, qm-_url_t2);
                    _url_o->_filename_encode_mode= url_get_encode_mode( _url_o->_file_name ,_url_o->_file_name_len );
                }
            }

            sd_get_file_suffix(_url_o);

            return 0;
        }
    }//else

    /* No path and file name, just host and port */
    if(_url_o->_schema_type==FTP)
    {
        /* 没有文件名的FTP URL,比如:ftp://down:down@down.77ds.net:53344/ */
        if(_url_o->_full_path[0]=='\0')
        {
            _url_o->_full_path[0]='/';
            _url_o->_full_path[1]='\0';
        }
        _url_o->_path = _url_o->_full_path;
        _url_o->_path_len = sd_strlen(_url_o->_full_path);
        _url_o->_file_name = _url_o->_full_path;
        _url_o->_file_name_len = _url_o->_path_len;
        return 0;
    }
    else
    {
        if(_url_o->_full_path[0]=='\0')
        {
            _url_o->_full_path[0]='/';
            _url_o->_full_path[1]='\0';
        }

        sd_strncpy(_url_o->_file_suffix,".html",MAX_SUFFIX_LEN);
        _url_o->_is_binary_file = FALSE;
        return 0;

    }

    return -1;
}

_int32 sd_url_object_to_string_sub(URL_OBJECT  *url_o,char* url)
{
    char portBuffer[10];

    LOG_DEBUG("sd_url_object_to_string");
    if((url_o->_host[0]=='\0')||(url == NULL)) return -1;

    sd_snprintf(url,10,"%s",g_url_type[url_o->_schema_type]);

    if(url_o->_user[0]!='\0')
    {
        sd_strcat(url,url_o->_user,sd_strlen(url_o->_user));
        if(url_o->_password[0]!='\0')
        {
            sd_strcat(url,":",sd_strlen(":"));
            sd_strcat(url,url_o->_password,sd_strlen(url_o->_password));
        }

        sd_strcat(url,"@",sd_strlen("@"));
    }


    sd_strcat(url,url_o->_host,sd_strlen(url_o->_host));

    if((url_o->_port !=0)&&
       !((url_o->_schema_type==HTTP)&&(url_o->_port==HTTP_DEFAULT_PORT))&&
       !((url_o->_schema_type==HTTPS)&&(url_o->_port==HTTPS_DEFAULT_PORT))&&
       !((url_o->_schema_type==FTP)&&(url_o->_port==FTP_DEFAULT_PORT)))
    {
        sd_snprintf(portBuffer,10,":%u",url_o->_port );
        sd_strcat(url,portBuffer,sd_strlen(portBuffer));
    }

    return 0;
}

/* Change URL_OBJECT to char*
* Return : 0: success; other:error
* Notice: Make sure the length of the buffer:char* _url is larger than MAX_URL_LEN
*/
_int32 sd_url_object_to_string(URL_OBJECT  *url_o,char* url)
{
    _int32 ret_val = SUCCESS;
    LOG_DEBUG("sd_url_object_to_string");
    ret_val = sd_url_object_to_string_sub(url_o,url);
    CHECK_VALUE(ret_val);
    sd_strcat(url,url_o->_full_path,sd_strlen(url_o->_full_path));

    LOG_DEBUG("sd_url_object_to_string:_url[%d]=%s",sd_strlen(url),url);
    return 0;
}

/* Change URL_OBJECT to ref_url,without file name
* Return : 0: success; other:error
* Notice: Make sure the length of the buffer:char* _url is larger than MAX_URL_LEN
*/
_int32 sd_url_object_to_ref_url(URL_OBJECT  *url_o,char* url)
{
    _int32 ret_val = SUCCESS;
    LOG_DEBUG("sd_url_object_to_string");
    ret_val = sd_url_object_to_string_sub(url_o,url);
    CHECK_VALUE(ret_val);
    sd_strcat(url,url_o->_path,url_o->_path_len);

    LOG_DEBUG("sd_url_object_to_ref_url:_url[%d]=%s",sd_strlen(url),url);
    return 0;
}
/* Change the full path of URL_OBJECT
* Return : 0: success; other:error
*/
_int32 sd_url_object_set_path(URL_OBJECT  *url_o,char* full_path,_u32 full_path_len)
{
    char *_url_t=NULL,*_url_t2=NULL,*pos3=NULL,*qm=NULL,*_eq_pos=NULL,*_qm2_pos=NULL,*_point_pos=NULL,*_underline_pos=NULL;
    LOG_DEBUG("sd_url_object_set_path");
    if((url_o==NULL)||(full_path==NULL)||(full_path_len<1)||(full_path_len>MAX_FULL_PATH_BUFFER_LEN-1))
        return -1;

    sd_memset(url_o->_full_path,0,MAX_FULL_PATH_BUFFER_LEN);
    sd_memcpy(url_o->_full_path, full_path, full_path_len);

    LOG_DEBUG("sd_url_object_set_path=%s",url_o->_full_path);

    _url_t =url_o->_full_path;
    _url_t2 = _url_t+1;
    if(_url_t2[0]!='\0')
    {
        pos3 = sd_strchr(_url_t2, '/', 0);
        qm = sd_strchr(_url_t2, '?', 0);

        if(qm!=NULL)
        {
            url_o->_is_dynamic_url=TRUE;
            while((pos3!=NULL)&&(pos3<qm))
            {
                _url_t2=pos3+1;
                pos3 = sd_strchr(_url_t2, '/', 0);
            }
        }
        else
        {
            url_o->_is_dynamic_url=FALSE;
            while(pos3!=NULL)
            {
                _url_t2=pos3+1;
                pos3 = sd_strchr(_url_t2, '/', 0);
            }
        }

        /* Path */
        if(_url_t2 != _url_t+1)
        {
            url_o->_path = url_o->_full_path;
            url_o->_path_len = _url_t2-_url_t;
            url_o->_path_encode_mode = url_get_encode_mode( url_o->_path ,url_o->_path_len );
        }


        /* File name */
        if((_url_t2-_url_t)!= sd_strlen(_url_t))
        {
            if(qm==NULL)
            {
                if(sd_strlen(_url_t2) == 0)
                    return -1;

                if( sd_strlen(_url_t2)>=MAX_FULL_PATH_LEN)
                {
                    LOG_DEBUG("URL ERROR:File name too long...[%s]",_url_t2);
                    return -1;
                }

                url_o->_file_name = url_o->_full_path+(_url_t2-_url_t);
                url_o->_file_name_len = sd_strlen(_url_t2);
                url_o->_filename_encode_mode= url_get_encode_mode( url_o->_file_name ,url_o->_file_name_len );
            }
            else
            {
                if(qm-_url_t2 == 0)
                {
                    _eq_pos = sd_strrchr(qm, '=');
                    _qm2_pos = sd_strrchr(qm, '?');
                    if((_eq_pos!=NULL)&&(_qm2_pos!=NULL)&&(_qm2_pos<_eq_pos)&&(sd_strlen(_eq_pos+1)!=0))
                    {
                        url_o->_file_name = url_o->_full_path+(_eq_pos+1-_url_t);
                        url_o->_file_name_len = sd_strlen(_eq_pos+1);
                        url_o->_filename_encode_mode= url_get_encode_mode( url_o->_file_name ,url_o->_file_name_len );
                    }
                    else
                    {
                        _point_pos =  sd_strchr(qm, '.',0);
                        _underline_pos =  sd_strchr(qm, '_',0);
                        if((_point_pos!=NULL)&&(_underline_pos!=NULL)&&(_point_pos<_underline_pos)&&(_underline_pos-(qm+1)!=0))
                        {
                            url_o->_file_name = url_o->_full_path+(qm+1-_url_t);
                            url_o->_file_name_len = _underline_pos-(qm+1);
                            url_o->_filename_encode_mode= url_get_encode_mode( url_o->_file_name ,url_o->_file_name_len );
                            //return 0;
                        }
                        else
                        {
                            url_o->_file_name = url_o->_full_path+(qm+1-_url_t);
                            url_o->_file_name_len = sd_strlen(qm+1);
                            url_o->_filename_encode_mode= url_get_encode_mode( url_o->_file_name ,url_o->_file_name_len );
                        }
                    }
                }
                else
                {

                    if( qm-_url_t2>=MAX_FULL_PATH_LEN)
                    {
                        LOG_DEBUG("URL ERROR:File name too long...[%s]",_url_t2);
                        return -1;
                    }

                    url_o->_file_name = url_o->_full_path+(_url_t2-_url_t);
                    url_o->_file_name_len = qm-_url_t2;
                    url_o->_filename_encode_mode= url_get_encode_mode( url_o->_file_name ,url_o->_file_name_len );
                }
            }

            sd_get_file_suffix(url_o);

            return 0;
        }
        else
        {
            LOG_DEBUG("URL WARNNING:no file name...[%s]",_url_t);
            //return -1;
            url_o->_file_name=NULL;
            url_o->_file_name_len=0;
            url_o->_filename_encode_mode=0;
            sd_memset(url_o->_file_suffix,0,MAX_SUFFIX_LEN);
            sd_strncpy(url_o->_file_suffix,".html",MAX_SUFFIX_LEN);
            url_o->_is_binary_file = FALSE;
            url_o->_is_dynamic_url=FALSE;
            return 0;
        }
    }
    else
    {
        /* The full path is "/" */
        url_o->_path=NULL;
        url_o->_file_name=NULL;
        url_o->_path_len=0;
        url_o->_file_name_len=0;
        url_o->_path_encode_mode=0;
        url_o->_filename_encode_mode=0;
        sd_memset(url_o->_file_suffix,0,MAX_SUFFIX_LEN);
        sd_strncpy(url_o->_file_suffix,".html",MAX_SUFFIX_LEN);
        url_o->_is_binary_file = FALSE;
        url_o->_is_dynamic_url=FALSE;
        return 0;
    }
}

/* @Simple Function@
* Return : TRUE or FALSE
* Notice: Just for HTTP_RESOURCE and FTP_RESOURCE!
*/
/*
BOOL sd_is_server_res_equal(RESOURCE * _p_res1,RESOURCE *  _p_res2)
{
   HTTP_SERVER_RESOURCE * _p_http_res1 = NULL,* _p_http_res2 = NULL;
   FTP_SERVER_RESOURCE * _p_ftp_res1 = NULL,* _p_ftp_res2 = NULL;
BOOL _result = FALSE;

LOG_DEBUG("sd_is_server_res_equal");

if(_p_res1->_resource_type == HTTP_RESOURCE)
{
   _p_http_res1 =(HTTP_SERVER_RESOURCE * ) _p_res1;

   if(_p_res2->_resource_type == HTTP_RESOURCE)
   {
       _p_http_res2 =(HTTP_SERVER_RESOURCE * ) _p_res2;

       if(sd_is_url_object_equal(&(_p_http_res1->_url),&(_p_http_res2->_url))==TRUE)
       {
           _result =  TRUE;
       }
       else
       if(sd_is_url_object_equal(&(_p_http_res1->_redirect_url),&(_p_http_res2->_url))==TRUE)
       {
           _result =  TRUE;
       }
       else
       if(sd_is_url_object_equal(&(_p_http_res1->_url),&(_p_http_res2->_redirect_url))==TRUE)
       {
           _result =  TRUE;
       }
       else
       if(sd_is_url_object_equal(&(_p_http_res1->_redirect_url),&(_p_http_res2->_redirect_url))==TRUE)
       {
           _result =  TRUE;
       }
   }
}
else
if(_p_res1->_resource_type == FTP_RESOURCE)
{
   _p_ftp_res1 =(FTP_SERVER_RESOURCE * )_p_res1;

   if(_p_res2->_resource_type == FTP_RESOURCE)
   {
       _p_ftp_res2 =(FTP_SERVER_RESOURCE * ) _p_res2;

       _result =  sd_is_url_object_equal(&(_p_ftp_res1->_url),&(_p_ftp_res2->_url));
   }
}

LOG_DEBUG("sd_is_server_res_equal,result=%d",_result);

      return _result;


}
*/
/* @Simple Function@
* Return : TRUE or FALSE
* Notice: Just for HTTP_RESOURCE and FTP_RESOURCE!
*/
BOOL sd_is_url_object_equal(URL_OBJECT * _url_o1,URL_OBJECT * _url_o2)
{
    LOG_DEBUG("sd_is_url_object_equal");

    LOG_DEBUG("_url_o1:_schema_type=%d,_host=%s,_full_path=%s,_password=%s,_user=%s,_port=%u",_url_o1->_schema_type,_url_o1->_host,_url_o1->_full_path,_url_o1->_password,_url_o1->_user,_url_o1->_port);
    LOG_DEBUG("_url_o2:_schema_type=%d,_host=%s,_full_path=%s,_password=%s,_user=%s,_port=%u",_url_o2->_schema_type,_url_o2->_host,_url_o2->_full_path,_url_o2->_password,_url_o2->_user,_url_o2->_port);
    if((_url_o1->_host[0]=='\0')||(_url_o2->_host[0]=='\0'))
    {
        LOG_DEBUG("empty url object:FALSE!");
        return FALSE;
    }

    if((_url_o1->_schema_type==_url_o2->_schema_type)
       &&(sd_stricmp(_url_o1->_host,_url_o2->_host)==0)
       &&(sd_strcmp(_url_o1->_full_path,_url_o2->_full_path)==0)
       &&(sd_strcmp(_url_o1->_password,_url_o2->_password)==0)
       &&(sd_strcmp(_url_o1->_user,_url_o2->_user)==0)
       &&(_url_o1->_port==_url_o2->_port))
    {
        LOG_DEBUG("sd_is_url_object_equal:TRUE!");
        return TRUE;
    }

    LOG_DEBUG("sd_is_url_object_equal:FALSE!");
    return FALSE;


}


/* Get the schema type of URL
* Return :SCHEMA_TYPE{ HTTP=0, FTP , MMS, HTTPS, MMST, PEER, RTSP, RTSPT, FTPS, SFTP, UNKNOWN}
* Notice: Just for http,https and ftp now!
*/
enum SCHEMA_TYPE sd_get_url_type(char* _url,_u32 url_length)
{
    char _url_t[10];
    LOG_DEBUG( "sd_get_url_type" );

    if((_url==NULL)||(url_length<MIN_URL_LEN))
        return UNKNOWN;

    sd_memset(_url_t,0,10);

    sd_strncpy(_url_t,_url,9);

    sd_string_to_low_case(_url_t);

    if( sd_strstr(_url_t, "http://", 0)!=NULL)
    {
        return HTTP;
    }
    else if( sd_strstr(_url_t, "ftp://",  0)!=NULL)
    {
        return FTP;
    }
    else if( sd_strstr(_url_t, "https://",  0)!=NULL)
    {
        return HTTPS;
    }


    return UNKNOWN;

}

/* Get the hash value of URL
*/
_int32 sd_get_url_hash_value(char* _url,_u32 _url_size,_u32 * _p_hash_value)
{
    _u32  hash  =   0 ,x = 0,len=_url_size;
    char* str=_url;

    LOG_DEBUG( "sd_get_url_hash_value" );

    if((_url==NULL)||(_url_size==0))
    {
        return -1;
    }


    while  ( len!=0)
    {
        hash  =  (hash  <<   4 )  +  ( * str  );
        if  ((x  =  hash  &   0xF0000000L )  !=   0 )
        {
            hash  ^=  (x  >>   24 );
            hash  &=   ~ x;
        }
        str++;
        len--;
    }

    *_p_hash_value =  (hash  &   0x7FFFFFFF );

    LOG_DEBUG( "sd_get_url_hash_value=%u", *_p_hash_value);

    return 0;
}

void sd_string_to_low_case(char* str)
{
    _u32 i = 0,len=sd_strlen(str);
    for(  i=0; i<len; i++ )
    {
        if( (str[i]<='Z') && (str[i]>='A') )
            str[i] = str[i] + 32 ;
    }
}

void sd_string_to_uppercase(char* str)
{
    while(*str)
    {
        if(*str>='a' && *str<='z')
            *str+='A'-'a';
        ++str;
    }
}
BOOL sd_is_binary_file(char* _suffix,char * _ext_buffer)
{
    // const static char * ext[]={".exe",".mp3",".avi",".rmvb",".rm",".rar",".zip",".iso",".wma",".wmv",".msi",".asf",".mpeg",".mpga",".mpg",".ra",".tar",".wmp",".torrent"};
    _int32 i=0;
    LOG_DEBUG( "sd_is_binary_file" );

    sd_string_to_low_case(_suffix);

    for(i=0; i<POSTFIXS_SIZE; i++)
    {
        if(NULL!=sd_strstr(_suffix, postfixs[i],0))
        {
            if(_ext_buffer!=NULL)
                sd_memcpy(_ext_buffer, postfixs[i], sd_strlen(postfixs[i]));

            LOG_DEBUG( "sd_is_binary_file = TRUE" );
            return TRUE;

            break;
        }
    }
    LOG_DEBUG( "sd_is_binary_file = FALSE" );
    return FALSE;
}

_int32 sd_get_file_suffix(URL_OBJECT *_url_o)
{
    char url_buffer[MAX_URL_LEN],*dot_pos=NULL;

    LOG_DEBUG( "sd_get_file_suffix" );

    sd_memset(_url_o->_file_suffix, 0, MAX_SUFFIX_LEN);

    if(_url_o->_is_dynamic_url == FALSE)
    {
        if( url_object_decode_ex(_url_o->_file_name, url_buffer ,MAX_URL_LEN)==-1)
        {
            CHECK_VALUE(-1);
        }
        dot_pos=sd_strrchr(url_buffer, '.');
        if((dot_pos!=NULL)&&(sd_strlen(dot_pos)>1)&&(sd_strlen(dot_pos)<MAX_SUFFIX_LEN))
        {
            sd_memcpy(_url_o->_file_suffix, dot_pos, sd_strlen(dot_pos));
            _url_o->_is_binary_file = sd_is_binary_file(_url_o->_file_suffix,NULL);
        }

    }
    else
    {

        if( url_object_decode_ex(_url_o->_full_path, url_buffer ,MAX_URL_LEN)==-1)
        {
            CHECK_VALUE(-1);
        }

        _url_o->_is_binary_file = sd_is_binary_file(url_buffer,_url_o->_file_suffix);

        LOG_DEBUG("url.file_name = %s ", _url_o->_file_name);

        if(_url_o->_is_binary_file==FALSE)
        {
            /* Not found,get the suffix from file name */
            sd_memset(url_buffer,0,MAX_URL_LEN);
            sd_memcpy(url_buffer,_url_o->_file_name, _url_o->_file_name_len);

            dot_pos=sd_strrchr(url_buffer, '.');
            if((dot_pos!=NULL)&&(sd_strlen(dot_pos)>1)&&(sd_strlen(dot_pos)<MAX_SUFFIX_LEN))
            {
                sd_memcpy(_url_o->_file_suffix, dot_pos, sd_strlen(dot_pos));
                //_url_o->_is_binary_file = sd_is_binary_file(url_buffer,NULL);
            }

        }
    }
    LOG_DEBUG( "sd_get_file_suffix:suffix[%d]=%s", sd_strlen(_url_o->_file_suffix),_url_o->_file_suffix);
    return SUCCESS;
}

BOOL sd_decode_file_name(char * _file_name,char * _file_suffix, _u32 file_name_buf_len)
{
    char str_buffer[MAX_URL_LEN],*pos=NULL,*dot_pos=NULL,*semicolon_pos=NULL;
    LOG_DEBUG( "sd_decode_file_name" );
    if((_file_name==NULL)||(sd_strlen(_file_name)<1))
        return FALSE;

    /* remove all the '%' */
    if( url_object_decode_ex(_file_name, str_buffer ,MAX_URL_LEN)!=-1)
    {
        /* Get the new file name after decode */
        pos=sd_strrchr(str_buffer, '/');
        if((pos!=NULL)&&(sd_strlen(pos)>2))
        {
            sd_strncpy(_file_name, pos, file_name_buf_len-1);
            //_file_name[sd_strlen(pos)]='\0';
            _file_name[file_name_buf_len-1] = '\0';
        }
        else
        {
            sd_strncpy(_file_name, str_buffer, file_name_buf_len);
            //_file_name[sd_strlen(str_buffer)]='\0';
            _file_name[file_name_buf_len-1] = '\0';
        }

        /* Check is there any semicolon in the file name */
        semicolon_pos = sd_strchr(_file_name, ';', 0);
        if(semicolon_pos!=NULL)
        {
            /* Cut all the chars after semicolon  */
            semicolon_pos[0]='\0';
        }

        /* Check the  file suffix  */
        dot_pos=sd_strrchr(_file_name, '.');
        if((dot_pos!=NULL)&&(dot_pos!=_file_name)&&(sd_strlen(dot_pos)>1))
        {
            if((_file_suffix!=NULL)&&(0==sd_stricmp(dot_pos, _file_suffix)))
            {
                /* file suffix is the same as extend name */
                LOG_DEBUG( "sd_decode_file_name:file suffix is the same as extend name,_file_name=%s",_file_name );
            }
            else if(_file_suffix!=NULL)
            {
                /* the file suffix is different from extend name */
                /* use the suffix as extend name */
                if ( sd_strlen(_file_name)+sd_strlen(_file_suffix)< file_name_buf_len-1)
                {
                    sd_strcat(_file_name, _file_suffix, sd_strlen(_file_suffix)+1);                   
                    LOG_DEBUG( "sd_decode_file_name:the file suffix is different from extend name,_file_name=%s",_file_name );
                }
                else
                {
                    LOG_ERROR( "sd_decode_file_name:the file suffix is different from extend name, append fail, cause is out of memory, _file_name=%s", _file_name );
                }
            }
            else
            {
                LOG_DEBUG( "sd_decode_file_name:_file_name=%s",_file_name );
            }
        }
        else if(_file_suffix!=NULL)
        {
            /* use the suffix as extend name */
            if ( sd_strlen(_file_name)+sd_strlen(_file_suffix)< file_name_buf_len-1)
            {
                sd_strcat(_file_name, _file_suffix, sd_strlen(_file_suffix)+1);
                LOG_DEBUG( "sd_decode_file_name:use the suffix as extend name,_file_name=%s",_file_name );
            }
            else
            {
                LOG_ERROR( "sd_decode_file_name:use the suffix as extend name, append fail, cause is out of memory, _file_name=%s",_file_name );                
            }
        }
        else
        {
            /* no extend name */
            LOG_DEBUG( "sd_decode_file_name:no extend name,_file_name=%s",_file_name );
        }


        return TRUE;
    }
    return FALSE;
}
_int32 sd_get_valid_name(char * _file_name,char * file_suffix)
{
    char  chr[2],*p=NULL;
    _int32 i=0;

    chr[0]=_file_name[0];
    chr[1]='\0';

    while(chr[0]!='\0')
    {
        if(sd_is_file_name_valid(chr)==FALSE)
        {
            _file_name[i]='_';
        }

        chr[0]=_file_name[++i];
    }

    if(file_suffix!=NULL)
    {
        i=0;
        chr[0]=file_suffix[0];

        while(chr[0]!='\0')
        {
            if(sd_is_file_name_valid(chr)==FALSE)
            {
                file_suffix[i]='_';
            }

            chr[0]=file_suffix[++i];
        }

        i=sd_strlen(file_suffix);
        p=sd_strrchr(_file_name, '.');
        if((p!=NULL)&&(i>1))
        {
            if(sd_stricmp(p, file_suffix)!=0)
            {
                sd_strcat(_file_name, file_suffix, i);
            }
        }
        else if(i>1)
        {
            sd_strcat(_file_name, file_suffix, i);
        }
    }
    if(sd_strlen(_file_name)==0)
    {
        sd_strncpy(_file_name,"UNKNOWN_FILE_NAME", sd_strlen("UNKNOWN_FILE_NAME"));
    }

    LOG_DEBUG( "sd_get_valid_name:%s",_file_name );
    return SUCCESS;
}
_int32 sd_get_file_name_from_url(char * _url,_u32 url_length,char * _file_name, _u32 _file_name_buf_len)
{
    URL_OBJECT _url_o;
    LOG_DEBUG( "sd_get_file_name_from_url" );
    if((_url==NULL) || (url_length<1) || (sd_strlen(_url)<url_length) || (_file_name==NULL) ) //去掉||(url_length>MAX_URL_LEN)，因为在sd_url_to_object会判断
        CHECK_VALUE(-1);

    if(sd_url_to_object( _url,url_length,&_url_o)!=SUCCESS)
    {
        return -1;
    }

    sd_memset(_file_name,0,_file_name_buf_len);

    LOG_DEBUG( "sd_get_file_name_from_url[%u]:%s",_url_o._file_name_len,_url_o._file_name);
    if(_url_o._file_name_len!=0)
    {
        /* Get the file name from the URL */
        sd_memcpy(_file_name, _url_o._file_name,( (_url_o._file_name_len>=_file_name_buf_len)?(_file_name_buf_len-1):_url_o._file_name_len));
        /* remove all the '%' */
        if(sd_decode_file_name(_file_name,_url_o._file_suffix, _file_name_buf_len)==TRUE)
        {
            if(sd_is_file_name_valid(_file_name)==TRUE)
            {
                return SUCCESS;
            }
        }

        /* Bad file name ! */
        sd_get_valid_name(_file_name,_url_o._file_suffix);
    }
    else
    {
        /* No filename from the URL */
        sd_memcpy(_file_name, _url_o._host, sd_strlen(_url_o._host));
        sd_strcat(_file_name,".html",sd_strlen(".html"));
        LOG_DEBUG( "sd_get_file_name_from_url:No filename from the URL ,_file_name=%s",_file_name );
    }
    return SUCCESS;
    /*
        qm=strchr(_url, '?');
        http_head=strstr(_url,"http://");

        if((qm!=NULL)&&(http_head!=NULL))
        {
            memset(t_buffer,0,512);
            strncpy(t_buffer,_url,qm-_url);

            tmp=strrchr(t_buffer, '/');
            if((tmp!=NULL)&&(strlen(tmp+1)!=0))
            {
                strncpy(_file_name,tmp+1,strlen(tmp+1));
                _file_name[strlen(tmp+1)]='\0';

            }
            else
            {

                            eq = strrchr(qm, '=');
                            _qm2_pos = strrchr(qm, '?');
                            if((eq!=NULL)&&(_qm2_pos!=NULL)&&(_qm2_pos<eq)&&(strlen(eq+1)!=0))
                            {
                                strncpy(_file_name,eq+1,strlen(eq+1));
                                _file_name[strlen(eq+1)]='\0';
                                return 0;
                            }
                            else
                            {
                                _point_pos =  strchr(qm, '.');
                                _underline_pos =  strchr(qm, '_');
                                if((_point_pos!=NULL)&&(_underline_pos!=NULL)&&(_point_pos<_underline_pos)&&(_underline_pos-(qm+1)!=0))
                                {
                                    strncpy(_file_name,qm+1,_underline_pos-(qm+1));
                                    _file_name[_underline_pos-(qm+1)]='\0';
                                    return 0;
                                }
                                else
                                {
                                    strncpy(_file_name,qm+1,strlen(qm+1));
                                    _file_name[strlen(qm+1)]='\0';
                                    return 0;
                                }
                            }
            }
        }
        else
        {
            tmp=strrchr(_url, '/');
            if((tmp!=NULL)&&(strlen(tmp+1)!=0)&&((tmp-1)[0]!='/'))
            {
                strncpy(_file_name,tmp+1,strlen(tmp+1));
                _file_name[strlen(tmp+1)]='\0';

            }
            else
            {
                if(http_head!=NULL)
                {
                    strncpy(_file_name,"index.html",strlen("index.html"));
                    _file_name[strlen("index.html")]='\0';
                }
                else
                    return -1;

            }
        }
    return 0;
    */
}

/* @Simple Function@
* Return : TRUE or FALSE
* Notice: Just for HTTP_RESOURCE and FTP_RESOURCE!
*/
BOOL sd_is_url_has_user_name(char * _url)
{
    URL_OBJECT _url_o;
    LOG_DEBUG("sd_is_url_has_user_name:%s",_url);

    if(sd_url_to_object( _url,sd_strlen(_url),&_url_o)!=SUCCESS)
        return FALSE;

    if(_url_o._user[0]!='\0')
    {
        return TRUE;
    }

    return FALSE;
}


_u32 url_object_default_port_for(enum SCHEMA_TYPE schema_type )
{
    _u32 result;
    switch(schema_type)
    {
        case HTTP:
            result = 80;
            break;
        case FTP:
            result = 21;
            break;
        case MMS:
            result = 1755;
            break;
        case RTSP:
            result = 554;
            break;
        case HTTPS:
            result = 443;
            break;
        case FTPS:
            result = 21;
            break;
        case MMST:
            result = 1755;
            break;
        case RTSPT:
            result = 554;
            break;
        case PEER:
            result = 3077;
            break;
        default:
            result = 80;
            break;
    }
    return result;

}


_int32 url_object_to_noauth_string(const char* src_str,  char * str_buffer ,_u32 buffer_len)
{
    URL_OBJECT _url_o;
    char portBuffer[10];
    _int32 count = 0;

    LOG_DEBUG( " enter url_object_to_noauth_string(%s)..." ,src_str);

    if((src_str==NULL)||(str_buffer==NULL)||(buffer_len<MAX_URL_LEN)) return -1;

    sd_memset(str_buffer,0,buffer_len);

    if(sd_url_to_object( src_str,sd_strlen(src_str),&_url_o)!=SUCCESS)
    {
        return -1;
    }

    /* Get the head of URL e.g. "http://","ftp://"... */
    sd_snprintf(str_buffer,MAX_URL_LEN,"%s",g_url_type[_url_o._schema_type]);
    /* Add the host name to the string */
    sd_strcat(str_buffer,_url_o._host,sd_strlen(_url_o._host));

    /* Add the port to the string */
    if(_url_o._port !=url_object_default_port_for(_url_o._schema_type ))
    {
        sd_snprintf(portBuffer,10,":%u",_url_o._port );
        sd_strcat(str_buffer,portBuffer,sd_strlen(portBuffer));
    }

    /* Remove '%' of the full_path and add  to the string */
    sd_strncpy(str_buffer+sd_strlen(str_buffer), _url_o._full_path, MAX_URL_LEN-sd_strlen(str_buffer)-1);
    //count = url_object_decode(_url_o._full_path,str_buffer+sd_strlen(str_buffer),MAX_URL_LEN-sd_strlen(str_buffer)-1);
    //if(count==-1)
    //   return -1;

    /* Remove all the '\t',' '... from the tail of  the string */
    sd_trim_postfix_lws(str_buffer );

    LOG_DEBUG( " url_object_to_noauth_string:url[%d]=%s", sd_strlen(str_buffer),str_buffer);

    if(sd_strcmp(src_str, str_buffer)==0)
    {
        return 0;
    }
    else
    {
        if(count==0)
            return 1;
        else
            return count;
    }

}

BOOL url_object_need_escape( char c )
{
    if( c==0X20 )
        return TRUE;
    switch( c )
    {
        case '<':
        case '>':
        case '#':
        case '%':
        case '\"':
        case '{':
        case '}':
        case '|':
        case '\\':
        case '^':
        case '[':
        case ']':
        case '`':
        case '~':
            return TRUE;
        default:
            break;
    }

#if defined(LINUX)  || defined(WINCE)
    if(c>=0x00 && c<= 0x1F )
        return TRUE;
#endif
    if( c==0x7F )
        return TRUE;

    return FALSE;
}

BOOL url_object_is_reserved( char c )
{
    switch( c )
    {
        case ';':
        case '/':
        case '?':
        case ':':
        case '@':
        case '&':
        case '=':
        case '+':
        case '$':
        case ',':
        case '%':
            return TRUE;
        default:
            return FALSE;
    }
}

//除保留字符'/','?','@',':'... 外，将所有的带有'%' 的字符去除'%'
_int32 url_object_decode(const char* src_str, char * str_buffer ,_u32 buffer_len)
{
    const char * p_temp=src_str;
    _u8 char_value=0;
    _int32 i=0,count=0,length=sd_strlen(src_str);

    LOG_DEBUG( " enter url_object_decode()..." );

    if((src_str==NULL)||(str_buffer==NULL)||(buffer_len<length)) return -1;

    sd_memset(str_buffer,0,buffer_len);

    while((p_temp[0]!='\0')&&(i<buffer_len))
    {
        if(p_temp[0]!='%')
        {
            str_buffer[i] = p_temp[0];
            p_temp++;
            i++;
        }
        else
        {
            char_value=0;
            if( ( (p_temp+2-src_str)<length )
                && IS_HEX( p_temp[1] )
                && IS_HEX( p_temp[2] )
              )
            {
                char_value = sd_hex_2_int(p_temp[1])*16 + sd_hex_2_int(p_temp[2]);
                if( url_object_is_reserved(char_value) ==TRUE)
                {
                    str_buffer[i] = p_temp[0];
                    p_temp++;
                    i++;
                }
                else
                {
                    str_buffer[i] = char_value;
                    p_temp+=3;
                    i++;
                    count++;
                }


            }
            else
            {
                str_buffer[i] = p_temp[0];
                p_temp++;
                i++;
            }

        }
    }

    LOG_DEBUG( "\n ---------------------" );
    LOG_DEBUG( "\n src_str[%d]=%s",length,src_str );
    LOG_DEBUG( "\n str_buffer[%d]=%s",sd_strlen(str_buffer),str_buffer );
    LOG_DEBUG( "\n ---------------------" );

    return count;

}
//将所有的带有'%' 的字符去除'%'，包括保留字符'/','?','@',':'...
_int32 url_object_decode_ex(const char* src_str, char * str_buffer ,_u32 buffer_len)
{
    const char * p_temp=src_str;
    _u8 char_value=0;
    _int32 i=0,count=0,length=sd_strlen(src_str);

    LOG_DEBUG( " enter url_object_decode_ex()..." );

    if((src_str==NULL)||(str_buffer==NULL))/*||(buffer_len<length))*/ return -1;

    sd_memset(str_buffer,0,buffer_len);

    while((p_temp[0]!='\0')&&(i<buffer_len))
    {
        if(p_temp[0]!='%')
        {
            str_buffer[i] = p_temp[0];
            p_temp++;
            i++;
        }
        else
        {
            char_value=0;
            if( ( (p_temp+2-src_str)<length )
                && IS_HEX( p_temp[1] )
                && IS_HEX( p_temp[2] )
              )
            {
                char_value = sd_hex_2_int(p_temp[1])*16 + sd_hex_2_int(p_temp[2]);
                str_buffer[i] = char_value;
                p_temp+=3;
                i++;
                count++;
            }
            else
            {
                str_buffer[i] = p_temp[0];
                p_temp++;
                i++;
            }

        }
    }

    if((p_temp[0]!='\0') && (i == buffer_len))
    {
        sd_memset(str_buffer,0,buffer_len);
        return -1;
    }

    LOG_DEBUG( "\n ---------------------" );
    LOG_DEBUG( "\n src_str[%d]=%s",length,src_str );
    LOG_DEBUG( "\n str_buffer[%d]=%s",sd_strlen(str_buffer),str_buffer );
    LOG_DEBUG( "\n ---------------------" );


    return count;
}



//encode:把汉字和'<'，'>'，'#'，'%'，'\"'，'{'，'}'，'|'，'\\'，'^'，'['，']'，'`'，'~'，0X20，c>=0X00 && c<= 0X1F，0X7F转换成%XX
// 即把除保留字符(';'，'/'，'?'，':'，'@'，'&'，'='，'+'，'$'，',')及英文数字等字符除外的所有其他字符转换成%XX
_int32 url_object_encode(const char* src_str, char * str_buffer ,_u32 buffer_len)
{
    const unsigned char * p_temp=(const unsigned char *)src_str;
    char hex_buffer[3];
    _int32 i=0,count=0,length=sd_strlen(src_str);

    LOG_DEBUG( " enter url_object_encode()..." );

    if((src_str==NULL)||(str_buffer==NULL)||(buffer_len<length)) return -1;

    sd_memset(str_buffer,0,buffer_len);

    while((p_temp[0]!='\0')&&(i<buffer_len))
    {
        if(p_temp[0]!='%')
        {
            if( url_object_need_escape(p_temp[0])||(p_temp[0]>0x7F))
            {
                str_buffer[i] ='%';
                char2hex( p_temp[0] , hex_buffer, 3);
                str_buffer[i+1] =hex_buffer[0];
                str_buffer[i+2] =hex_buffer[1];
                i+=3;
                count++;
            }
            else
            {
                str_buffer[i] = p_temp[0];
                i++;
            }

            p_temp++;
        }
        else
        {
            if( ( (p_temp[1]!='\0')&&(p_temp[2]!='\0') )
                && IS_HEX( p_temp[1] )
                && IS_HEX( p_temp[2] )
              )
            {
                str_buffer[i] = p_temp[0];
                str_buffer[i+1] = p_temp[1];
                str_buffer[i+2] = p_temp[2];
                p_temp+=3;
                i+=3;
            }
            else
            {
                str_buffer[i] = '%';
                str_buffer[i+1] = '2';
                str_buffer[i+2] = '5';
                p_temp++;
                i+=3;
                count++;
            }

        }
    }

    LOG_DEBUG( "\n ---------------------" );
    LOG_DEBUG( "\n src_str[%d]=%s",length,src_str );
    LOG_DEBUG( "\n str_buffer[%d]=%s",sd_strlen(str_buffer),str_buffer );
    LOG_DEBUG( "\n ---------------------" );

    return count;

}

// 即把所有字符转换成%XX,包括保留字符(';'，'/'，'?'，':'，'@'，'&'，'='，'+'，'$'，',')
_int32 url_object_encode_ex(const char* src_str, char * str_buffer ,_u32 buffer_len)
{
    const unsigned char * p_temp=(const unsigned char *)src_str;
    char hex_buffer[3];
    _int32 i=0,count=0,length=sd_strlen(src_str);

    LOG_DEBUG( " enter url_object_encode_ex()..." );

    if((src_str==NULL)||(str_buffer==NULL)||(buffer_len<length)) return -1;

    sd_memset(str_buffer,0,buffer_len);

    while((p_temp[0]!='\0')&&(i<buffer_len))
    {
        if(p_temp[0]!='%')
        {
            if( url_object_need_escape(p_temp[0])|| url_object_is_reserved(p_temp[0])||(p_temp[0]>0x7F))
            {
                str_buffer[i] ='%';
                char2hex( p_temp[0] , hex_buffer, 3);
                str_buffer[i+1] =hex_buffer[0];
                str_buffer[i+2] =hex_buffer[1];
                i+=3;
                count++;
            }
            else
            {
                str_buffer[i] = p_temp[0];
                i++;
            }

            p_temp++;
        }
        else
        {
            if( ( (p_temp[1]!='\0')&&(p_temp[2]!='\0') )
                && IS_HEX( p_temp[1] )
                && IS_HEX( p_temp[2] )
              )
            {
                str_buffer[i] = p_temp[0];
                str_buffer[i+1] = p_temp[1];
                str_buffer[i+2] = p_temp[2];
                p_temp+=3;
                i+=3;
            }
            else
            {
                str_buffer[i] = '%';
                str_buffer[i+1] = '2';
                str_buffer[i+2] = '5';
                p_temp++;
                i+=3;
                count++;
            }

        }
    }

    LOG_DEBUG( "\n ---------------------" );
    LOG_DEBUG( "\n src_str[%d]=%s",length,src_str );
    LOG_DEBUG( "\n str_buffer[%d]=%s",sd_strlen(str_buffer),str_buffer );
    LOG_DEBUG( "\n ---------------------" );

    return count;

}

_int32  sd_parse_kankan_vod_url(char* url, _int32 length ,_u8 *p_gcid,_u8 * p_cid, _u64 * p_file_size,char * file_name)
{
    /*
    点播url格式：以/分割字段
    版本：0
    http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count/tail_pos/url_mid/filename
    版本：1
    http://host:port/version/gcid/cid/file_size/head_size/index_size/bitrate/packet_size/packet_count
    /tail_pos/扩展项列表/url_mid/filename
    */


    char* pos1;
    char* pos2;
    char host[MAX_HOST_NAME_LEN];
    char version[MAX_CFG_VALUE_LEN];
    char gcid[CID_SIZE*2+1]= {0};
    char cid[CID_SIZE*2+1]= {0};
    _int32 ret_val=SUCCESS;
    char* pos3 = NULL; // 问号的位置
    int file_name_len = MAX_FILE_NAME_LEN-1;

    LOG_DEBUG("sd_parse_kankan_vod_url...");
    pos1 = url;

    if( 0 != sd_strncmp(pos1, "http://", sd_strlen("http://") ))
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not start with http:// ");
        return -1;
    }
    pos1 += sd_strlen("http://");

    /*host:port*/
    pos2 = sd_strstr(pos1, "/", 0);
    if( NULL == pos2)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after  http:// ");
        return -1;
    }
    if((pos2-pos1)>=MAX_HOST_NAME_LEN)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url:host:port to long! ");
        return -1;
    }
    sd_memcpy(host, pos1, (_int32)((_int32)pos2-(_int32)pos1));
    host[(_int32)pos2-(_int32)pos1] = '\0';
    //sd_strstr(url, "pubnet.sandai.net", 0);

    /*version*/
    pos1 = pos2+1;
    pos2 = sd_strstr(pos1, "/", 0);
    if( NULL == pos2)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after  host:port/ ");
        return -1;
    }
    if((pos2-pos1)>=MAX_CFG_VALUE_LEN)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url:version to long! ");
        return -1;
    }
    sd_memcpy(version, pos1,(_int32)pos2-(_int32)pos1);
    version[ (_int32)pos2-(_int32)pos1] = '\0';

    /*gcid*/
    pos1 = pos2+1;
    pos2 = sd_strstr(pos1, "/", 0);
    if( NULL == pos2)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after  version/ ");
        return -1;
    }
    if((pos2-pos1)>=(CID_SIZE*2+1))
    {
        LOG_DEBUG("sd_parse_kankan_vod_url:gcid to long! ");
        return -1;
    }
    sd_memcpy(gcid, pos1, (_int32)pos2-(_int32)pos1);
    gcid[ (_int32)pos2-(_int32)pos1] = '\0';
    ret_val=sd_string_to_cid(gcid,p_gcid);
    if(ret_val!=SUCCESS) return ret_val;

    /*cid*/
    pos1 = pos2+1;
    pos2 = sd_strstr(pos1, "/", 0);
    if( NULL == pos2)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after gcid/ ");
        return -1;
    }
    if((pos2-pos1)>=(CID_SIZE*2+1))
    {
        LOG_DEBUG("sd_parse_kankan_vod_url:cid to long! ");
        return -1;
    }
    sd_memcpy(cid, pos1, (_int32)pos2-(_int32)pos1);
    cid[ (_int32)pos2-(_int32)pos1] = '\0';
    ret_val=sd_string_to_cid(cid,p_cid);
    if(ret_val!=SUCCESS) return ret_val;

    /*file_size*/
    pos1 = pos2+1;
    pos2 = sd_strstr(pos1, "/", 0);
    if( NULL == pos2)
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after cid/ ");
        return -1;
    }
    if((pos2-pos1)>=(CID_SIZE*2+1))
    {
        LOG_DEBUG("sd_parse_kankan_vod_url:file_size to long! ");
        return -1;
    }
    sd_memset(cid,0,CID_SIZE*2+1);
    sd_memcpy(cid, pos1, (_int32)pos2-(_int32)pos1);
    cid[ (_int32)pos2-(_int32)pos1] = '\0';
    pos1= cid;
	/*
    while(IS_HEX(pos1[0]))
    {
        (*p_file_size)=(*p_file_size)*16+sd_hex_2_int(pos1[0]);
        pos1++;
    }
	*/
	*p_file_size = 0;
    /*file_name*/
    pos1 = sd_strrchr(pos2+1,'/');
    if(( NULL == pos1)||(sd_strlen(pos1+1)<1))
    {
        LOG_DEBUG("sd_parse_kankan_vod_url...not found / after file_size/ ");
        return -1;
    }
    pos3 = sd_strchr(pos1+1, '?', 0);
    if(pos3 != NULL)
    {
        file_name_len = pos3 - pos1 - 1;
    }
    LOG_DEBUG("pos1 = 0x%x, pos3 = 0x%x", pos1, pos3);
    if(file_name_len <= 0 )
    {
        LOG_DEBUG("file_name_len <= 0");
        return -1;   
    }
    
    sd_strncpy(file_name, pos1+1, file_name_len);
    LOG_DEBUG("sd_parse_kankan_vod_url... file_name=%s/ ", file_name);

    return SUCCESS;


    /*
    host:port           为CDN集群的域名，如pubnet1.sandai.net:80
    version         是vod url的版本号， 当前0/1
    gcid                文件的gcid信息，十六进制编码
    cid             文件的cid信息，十六进制编码
    file_size           文件大小, 十六进制编码
    head_size       头块数据大小, 十六进制编码
    index_size      索引块大小, 十六进制编码
    bitrate         比特率(bit) , 十六进制编码
    packet size     包大小 , 十六进制编码
    packet_count    包个数，十六进制编码
    tail_pos        尾部起始位置，十六进制编码
    url_mid         校验码
    url_mid 表示gcid[20] + cid[20] + file_size[8]，用MD5哈希，再进行十六进制编码得到
    filename            文件名, 如1.wmv
        扩展项列表      扩展项/扩展项/.../ext_mid, 以/分割扩展项
    第一个扩展项是custno（客户号），该url是为客户发行的
    ext_mid 表示所有扩展项+url_mid，用MD5哈希，再进行十六进制编码得到

    */

}


_int32 sd_parse_lixian_url(char *url, _int32 len, _u8* p_gcid, _u8* p_cid, _u64* p_filesize, char* filename)
{
	int e = 1;
	char* p;
	char* q;
	char b64_out[128];
	char c = url[len];
	char t;
	_u32 threshold;
	_u32 vid;
	_u32 lower_filesize;
	char trailing_bytes[] = {47, 13, 94, 118, 39, 71, 156, 59};
	_u8 values[16];
	char hashed_md5[32];
	int i;
	ctx_md5 md5;

	url[len] = 0; // null termination
	
	p = strstr(url, "http://");
	if(p == NULL) return -(e++);
	p += strlen("http://");
	p = strchr(p, '/');
	if(p == NULL) return -(e++);
	p++;
	q = strchr(p, '?');
	if(q == NULL) return -(e++);
	memcpy(filename, p, q-p);
	
	p = strstr(url, "fid=");
	if(p == NULL) return -(e++);
	p += strlen("fid=");
	q = strchr(p, '&');
	if(q == NULL)
	{
		q = url + len;
	}
	
	t = q[0];
	q[0] = 0; // null termination
	;
	if(sd_base64_decode_v2((const _u8*)p, q-p, b64_out) > sizeof(b64_out))
	{
		return -(e++);
	}
	q[0] = t; // restore

	memcpy(p_cid, b64_out, CID_SIZE);
	memcpy(p_filesize, b64_out+CID_SIZE, sizeof(_u64));
	memcpy(p_gcid, b64_out+CID_SIZE+sizeof(_u64), CID_SIZE);

	p = strstr(url, "threshold=");
	if(p == NULL) return -(e++);
	p += strlen("threshold=");
	i = 0;
	threshold = 0;
	while(p[i] != '&' && p[i] != 0)
	{
		if(p[i] > '9' || p[i] < '0') 
			return -(e++);
		threshold *= 10;
		threshold += p[i] - '0';
		i++;
	}

	vid = ~threshold;
	lower_filesize = (_u32)*p_filesize;

	memcpy(values, &vid, sizeof(vid));
	memcpy(values+sizeof(vid), &lower_filesize, sizeof(lower_filesize));
	memcpy(values+sizeof(vid)+sizeof(lower_filesize), trailing_bytes, sizeof(trailing_bytes));

	md5_initialize(&md5);
	md5_update(&md5, values, sizeof(values));
	md5_finish(&md5, values);

	str2hex((const char*)values, sizeof(values), hashed_md5, sizeof(hashed_md5));

	p = strstr(url, "tid=");
	if(p == NULL) return -(e++);
	p += strlen("tid=");
	if(strncmp(p, hashed_md5, sizeof(hashed_md5)) == 0)
	{
#ifdef _DEBUG
		char tcid_str[CID_SIZE*2+1] = {0};
		char gcid_str[CID_SIZE*2+1] = {0};
		str2hex((const char*)p_cid, CID_SIZE, tcid_str, sizeof(tcid_str));
		str2hex((const char*)p_gcid, CID_SIZE, gcid_str, sizeof(gcid_str));
		LOG_DEBUG("parse lixian url success, tcid=%s, filesize=%llu, gcid=%s, filename=%s",
			tcid_str, *p_filesize, gcid_str, filename);
#endif
		url[len] = c; // restore
		return SUCCESS;
	}
	
	url[len] = c; // restore
	return -e;
}


_int32 url_convert_format( const char* src_str ,_int32 src_str_len,char* str_buffer ,_int32 buffer_len ,enum URL_CONVERT_STEP * convert_step)
{
    char  src_string[MAX_URL_LEN],_Tbuffer[MAX_URL_LEN],_Tbuffer2[MAX_URL_LEN];
    enum CODE_PAGE code_page;
    _u32 output_len = MAX_URL_LEN;
    _int32 ret_val = SUCCESS;
    enum URL_CONVERT_STEP step = *convert_step;

    LOG_DEBUG("url_convert_format(convert_step=%d)",*convert_step);

    *convert_step=UCS_END;
    sd_memset(src_string,0,MAX_URL_LEN);
    sd_memset(_Tbuffer,0,MAX_URL_LEN);
    sd_memset(_Tbuffer2,0,MAX_URL_LEN);

    sd_assert(src_str_len<MAX_URL_LEN);
    sd_memcpy(src_string, src_str, src_str_len);

    switch(step)
    {
        case UCS_ORIGIN:
            ret_val = url_object_decode(src_string,_Tbuffer,MAX_URL_LEN);
            if(ret_val==-1)
            {
                CHECK_VALUE(  -1);
            }
            else if(ret_val != 0)
            {
                step = UCS_DECODE;
                LOG_DEBUG("retry request step=UCS_DECODE");
                break;
            }

            LOG_DEBUG(" no char is decoded! go to next step ");

        case UCS_DECODE:
            ret_val = url_object_encode(src_string,_Tbuffer,MAX_URL_LEN);
            if(ret_val==-1)
            {
                CHECK_VALUE(  -1);
            }
            else if(ret_val != 0)
            {
                step = UCS_ENCODE;
                LOG_DEBUG("retry request step=UCS_ENCODE");
                break;
            }

            LOG_DEBUG("  no char is encoded! go to next step");

        case UCS_ENCODE:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);
            code_page = sd_conjecture_code_page(_Tbuffer );
            /* 由于sd_conjecture_code_page经常猜不准，所以除了纯ASCII格式的字符串，其它都假定为GBK格式进行第一步的格式转换处理 */
            if(code_page==_CP_GBK)
            {
                LOG_DEBUG(" The url is in GBK format ");
                sd_gbk_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),    _Tbuffer2, &output_len);
                if(output_len!=0)
                {
                    if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                        CHECK_VALUE(  -1);

                    step = UCS_GBK2UTF8_ENCODE;
                    LOG_DEBUG("retry request step=UCS_GBK2UTF8_ENCODE");
                }
                else
                {
                    LOG_DEBUG(" Maybe the full_path is in big5 format ");
                    output_len =MAX_URL_LEN;
                    sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),   _Tbuffer2, &output_len);
                    if(output_len!=0)
                    {
                        if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                            CHECK_VALUE(  -1);

                        step = UCS_BIG52UTF8_ENCODE;
                        LOG_DEBUG("retry request step=UCS_BIG52UTF8_ENCODE");
                    }
                    else
                    {
                        LOG_DEBUG(" Maybe the full_path is in UTF8 format ");
                        output_len =MAX_URL_LEN;
                        sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),_Tbuffer2, &output_len);
                        if(output_len!=0)
                        {
                            if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                                CHECK_VALUE(  -1);

                            step= UCS_UTF82GBK_ENCODE ;
                            LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
                        }
                        else
                        {
                            return -1;
                        }
                    }
                }
            }
            else if(code_page==_CP_BIG5)
            {
                LOG_DEBUG(" The url is in BIG5 format ");
                output_len =MAX_URL_LEN;
                sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),   _Tbuffer2, &output_len);
                if(output_len!=0)
                {
                    if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                        CHECK_VALUE(  -1);

                    step = UCS_BIG52UTF8_ENCODE;
                    LOG_DEBUG("retry request step=UCS_BIG52UTF8_ENCODE");
                }
                else
                {
                    LOG_DEBUG(" Maybe the full_path is in UTF8 format ");
                    output_len =MAX_URL_LEN;
                    sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),_Tbuffer2, &output_len);
                    if(output_len!=0)
                    {
                        if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                            CHECK_VALUE(  -1);

                        step = UCS_UTF82GBK_ENCODE ;
                        LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
                    }
                    else
                    {
                        return  -1;
                    }
                }
            }
            else if(code_page==_CP_UTF8)
            {
                sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),    _Tbuffer2, &output_len);
                if(output_len!=0)
                {
                    if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                        CHECK_VALUE(  -1);

                    step = UCS_UTF82GBK_ENCODE;
                    LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
                }
                else
                {
                    LOG_DEBUG(" Maybe the full_path should be converted to BIG5 format,but never try that any more! ");
                    return -1;
                }
            }
            else
                return -1;

            break;

        case UCS_GBK2UTF8_ENCODE:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_gbk_2_utf8(_Tbuffer2, sd_strlen(_Tbuffer2),_Tbuffer, &output_len);
            if(output_len!=0)
            {
                step = UCS_GBK2UTF8;
                LOG_DEBUG("retry request step=UCS_GBK2UTF8");
            }
            else
                return  -1;

            break;

        case UCS_UTF82GBK_ENCODE:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_utf8_2_gbk(_Tbuffer2, sd_strlen(_Tbuffer2),_Tbuffer, &output_len);
            if(output_len!=0)
            {
                step = UCS_UTF82GBK;
                LOG_DEBUG("retry request step=UCS_UTF82GBK");
            }
            else
                return  -1;

            break;

        case UCS_BIG52UTF8_ENCODE:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_big5_2_utf8(_Tbuffer2, sd_strlen(_Tbuffer2),_Tbuffer, &output_len);
            if(output_len!=0)
            {
                step = UCS_BIG52UTF8;
                LOG_DEBUG("retry request step=UCS_BIG52UTF8");
            }
            else
                return  -1;

            break;
        case UCS_GBK2UTF8:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            LOG_DEBUG(" Maybe the full_path is in BIG5 format ");

            sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),   _Tbuffer2, &output_len);
            if(output_len!=0)
            {
                if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                    CHECK_VALUE(  -1);

                step = UCS_BIG52UTF8_ENCODE ;
                LOG_DEBUG("retry request step=UCS_BIG52UTF8_ENCODE");
            }
            else
            {
                LOG_DEBUG(" Maybe the full_path is in UTF8 format ");

                sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),_Tbuffer2, &output_len);
                if(output_len!=0)
                {
                    if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                        CHECK_VALUE(  -1);

                    step = UCS_UTF82GBK_ENCODE;
                    LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
                }
                else
                {
                    return -1;
                }
            }

            break;

        case UCS_BIG52UTF8:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            LOG_DEBUG(" Maybe the full_path is in UTF8 format ");

            sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),_Tbuffer2, &output_len);
            if(output_len!=0)
            {
                if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                    CHECK_VALUE(  -1);

                step = UCS_UTF82GBK_ENCODE;
                LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
            }
            else
            {
                return  -1;
            }
            break;

        case UCS_UTF82GBK:
        default:
            return -1;


    }


    sd_memset(str_buffer ,0, buffer_len);
    sd_strncpy(str_buffer, _Tbuffer, buffer_len);
    *convert_step=step;

    return SUCCESS;
}

_int32 url_convert_format_to_step( const char* src_str ,_int32 src_str_len,char* str_buffer ,_int32 buffer_len,enum URL_CONVERT_STEP  convert_step )
{
    char src_string[MAX_URL_LEN],_Tbuffer[MAX_URL_LEN],_Tbuffer2[MAX_URL_LEN];
    _u32 output_len = MAX_URL_LEN;

    LOG_DEBUG("url_convert_format_to_step(step=%d)",convert_step);

    sd_memset(src_string,0,MAX_URL_LEN);
    sd_memset(_Tbuffer,0,MAX_URL_LEN);
    sd_memset(_Tbuffer2,0,MAX_URL_LEN);

    sd_assert(src_str_len<MAX_URL_LEN);
    sd_memcpy(src_string, src_str, src_str_len);

    switch(convert_step)
    {
        case UCS_DECODE:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            LOG_DEBUG("retry request step=UCS_DECODE");

            break;

        case UCS_ENCODE:
            if(url_object_encode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            LOG_DEBUG("retry request step=UCS_ENCODE");

            break;

        case UCS_GBK2UTF8_ENCODE:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_gbk_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),    _Tbuffer2, &output_len);
            if(output_len!=0)
            {
                if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                    CHECK_VALUE(  -1);
                LOG_DEBUG("retry request step=UCS_GBK2UTF8_ENCODE");
            }
            else
                CHECK_VALUE(  -1);

            break;
        case UCS_GBK2UTF8:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_gbk_2_utf8(_Tbuffer2, sd_strlen(_Tbuffer2),  _Tbuffer, &output_len);
            if(output_len!=0)
            {
                LOG_DEBUG("retry request step=UCS_GBK2UTF8");
            }
            else
                CHECK_VALUE(  -1);

            break;
        case UCS_UTF82GBK_ENCODE:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_utf8_2_gbk(_Tbuffer, sd_strlen(_Tbuffer),    _Tbuffer2, &output_len);
            if(output_len!=0)
            {
                if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                    CHECK_VALUE(  -1);
                LOG_DEBUG("retry request step=UCS_UTF82GBK_ENCODE");
            }
            else
                CHECK_VALUE(  -1);

            break;
        case UCS_UTF82GBK:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_utf8_2_gbk(_Tbuffer2, sd_strlen(_Tbuffer2),  _Tbuffer, &output_len);
            if(output_len!=0)
            {
                LOG_DEBUG("retry request step=UCS_UTF82GBK");
            }
            else
                CHECK_VALUE(  -1);

            break;
        case UCS_BIG52UTF8_ENCODE:
            if(url_object_decode(src_string,_Tbuffer,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_big5_2_utf8(_Tbuffer, sd_strlen(_Tbuffer),   _Tbuffer2, &output_len);
            if(output_len!=0)
            {
                if(url_object_encode(_Tbuffer2,_Tbuffer,MAX_URL_LEN)==-1)
                    CHECK_VALUE(  -1);

                LOG_DEBUG("retry request step=UCS_BIG52UTF8_ENCODE");
            }
            else
                CHECK_VALUE(  -1);

            break;
        case UCS_BIG52UTF8:
            if(url_object_decode(src_string,_Tbuffer2,MAX_URL_LEN)==-1)
                CHECK_VALUE(  -1);

            sd_big5_2_utf8(_Tbuffer2, sd_strlen(_Tbuffer2), _Tbuffer, &output_len);
            if(output_len!=0)
            {
                LOG_DEBUG("retry request step=UCS_BIG52UTF8");
            }
            else
                CHECK_VALUE( -1);

            break;

        default:
            CHECK_VALUE(  -1);


    }


    sd_memset(str_buffer ,0, buffer_len);
    sd_strncpy(str_buffer, _Tbuffer, buffer_len);

    return SUCCESS;
}


enum URL_ENCODE_MODE url_get_encode_mode( const char* src_str ,_int32 src_str_len )
{
    char  src_string[MAX_URL_LEN],_Tbuffer[MAX_URL_LEN];
    char * p_str=NULL;
    enum CODE_PAGE code_page;
    enum URL_ENCODE_MODE encode_mode=UEM_ASCII;
    BOOL is_encoded=FALSE;
    _int32 ret_val = 0;

    LOG_DEBUG("url_get_encode_mode(src_str[%d]=%s)",src_str_len,src_str);

    sd_memset(src_string,0,MAX_URL_LEN);
    sd_memset(_Tbuffer,0,MAX_URL_LEN);

    sd_assert(src_str_len<MAX_URL_LEN);
    sd_memcpy(src_string, src_str, src_str_len);
    p_str=src_string;

    ret_val = url_object_decode(p_str,_Tbuffer,MAX_URL_LEN);
    sd_assert(ret_val!=-1);
    if(ret_val != 0)
    {
        /* The string is encoded! */
        is_encoded=TRUE;
        p_str=_Tbuffer;
    }

    code_page = sd_conjecture_code_page(p_str );
    //_CP_ACP = 0, _CP_GBK , _CP_UTF8 ,_CP_BIG5
    switch(code_page)
    {
        case _CP_GBK:
            if(is_encoded==TRUE)
                encode_mode=UEM_GBK_ENCODED;
            else
                encode_mode=UEM_GBK;

            break;

        case _CP_BIG5:
            if(is_encoded==TRUE)
                encode_mode=UEM_BIG5_ENCODED;
            else
                encode_mode=UEM_BIG5;

            break;

        case _CP_UTF8:
            if(is_encoded==TRUE)
                encode_mode=UEM_UTF8_ENCODED;
            else
                encode_mode=UEM_UTF8;

            break;

        case _CP_ACP:
        default:
            if(is_encoded==TRUE)
                encode_mode=UEM_ENCODED;
    }

    LOG_DEBUG("url_get_encode_mode:%d",encode_mode);

    return encode_mode;
}


_int32 sd_get_url_sum(char* _url,_u32 _url_size,_u32 * _p_sum)
{
    _u32  sum  =   0 ,len=_url_size;
    char* str=_url;

    if((_url==NULL)||(_url_size==0)||( sd_strlen(_url)<_url_size))
    {
        return -1;
    }


    while  ( len!=0)
    {
        sum+=*str;
        str++;
        len--;
    }

    *_p_sum =  sum;

    return 0;
}

BOOL sd_check_if_in_nat_url(char * url)
{
    _u32 ip = 0;
    URL_OBJECT url_o;
    if(sd_url_to_object(url,sd_strlen(url),&url_o)!=SUCCESS)
    {
        return FALSE;
    }

    ip = sd_inet_addr(url_o._host);
    if(ip == MAX_U32)
    {
        return FALSE;
    }

    return sd_is_in_nat( ip);
}

BOOL sd_check_if_in_nat_host(const char * host)
{
    _u32 ip = 0;

    ip = sd_inet_addr(host);
    if(ip == MAX_U32)
    {
        return FALSE;
    }

    return sd_is_in_nat( ip);
}

/* 从局域网URL中解析出cid和file_size */
_int32 sd_get_cid_filesize_from_lan_url( char * url,_u8 cid[CID_SIZE], _u64  * file_size)
{
    _int32 ret_val = SUCCESS;
    char *pos_ = NULL,*pos_point = NULL;
    char * pos = sd_strrchr(url, '/');
    if(!pos) return -1;

    pos++;
    pos_ = sd_strchr(pos, '_',0);
    if(pos_-pos!=CID_SIZE*2) return -1;

    ret_val = sd_string_to_cid(pos,cid);

    if(ret_val!=SUCCESS) return -1;

    pos_++;
    pos_point = sd_strchr(pos_,'.', 0);
    if(pos_point==NULL) return -1;

    if(sd_stricmp(pos_point, ".png")==0) return -1;

    ret_val = sd_str_to_u64( pos_, pos_point-pos_,file_size );
    if(ret_val!=SUCCESS) return -1;
    return SUCCESS;
}

_int32 sd_get_filesize_from_emule_url(char *ed2k_link, _u64 *p_file_size)
{
	_int32 ret = 0;
	char *p_start = NULL, *p_end = NULL, *p_tmp = NULL;
	_int32 i = 0;
	char file_size[20] = {0};

	for (i = 0, p_start = ed2k_link; i < 3; i++, p_start = p_tmp + 1)
	{
		p_tmp = sd_strchr(p_start, '|', 0);
		if (NULL == p_tmp)
		{
			return -1;
		}
	}
	p_end = sd_strchr(p_start, '|', 0);
	sd_memcpy(file_size, p_start, p_end-p_start);
	sd_str_to_u64(file_size, sd_strlen(file_size), p_file_size);
	if(*p_file_size == 0)
	{
		return -2;
	}

	return SUCCESS;
}


#endif /* __URL_C_20080805 */
