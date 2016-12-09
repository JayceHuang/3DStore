#if !defined(__IOFILE_IMPL_IMPL_H_20130301)
#define __IOFILE_IMPL_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

ClassMembers( CIoFile, Base )

	FILE *m_fp;
	BOOL m_bCloseFile;

EndOfClassMembers;


#ifdef __cplusplus
}
#endif

#endif /* __IOFILE_IMPL_IMPL_H_20130301 */
