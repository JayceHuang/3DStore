#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "emule_peer.h"
#include "emule_define.h"

void emule_peer_init(EMULE_PEER* peer)
{
	emule_tag_list_init(&peer->_tag_list);
}

void emule_peer_uninit(EMULE_PEER* peer)
{
	emule_tag_list_uninit(&peer->_tag_list, TRUE);
}

_u8 emule_peer_get_extended_requests_option(EMULE_PEER* peer)
{
	const EMULE_TAG* tag = emule_tag_list_get(&peer->_tag_list, CT_EMULE_MISCOPTIONS1);
	if(tag == NULL || emule_tag_is_u32(tag) == FALSE)
		return 0;	
	return GET_MISC_OPTION(tag->_value._val_u32, EXTENDREQUEST);
}

_u8 emule_peer_get_source_exchange_option(EMULE_PEER* peer)
{
	const EMULE_TAG* tag = emule_tag_list_get(&peer->_tag_list, CT_EMULE_MISCOPTIONS1);
	if(tag == NULL || emule_tag_is_u32(tag) == FALSE)
		return 0;
	return GET_MISC_OPTION(tag->_value._val_u32, SOURCEEXCHANGE);
}

#endif /* ENABLE_EMULE */

