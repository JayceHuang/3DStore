#if !defined(__MEMORYFILE_IMPL_IMPL_H_20130301)
#define __MEMORYFILE_IMPL_IMPL_H_20130301

#ifdef __cplusplus
extern "C" 
{
#endif

ClassMembers( CMemoryFile, Base )

	unsigned char* m_pBuffer;
	_u64 m_Size;
	BOOL m_bFreeOnClose;
	_int64 m_Position;	//current position
	_int64 m_Edge;		//buffer size

EndOfClassMembers;


#ifdef __cplusplus
}
#endif

#endif /* __MEMORYFILE_IMPL_IMPL_H_20130301 */
