#include "ptl_utility.h"
#include "utility/string.h"
#include "utility/sd_assert.h"

static	_u16	g_tcp_port = 0;
static	_u16	g_udp_port = 0;

void ptl_set_local_tcp_port(_u16 tcp_port)
{
	g_tcp_port = tcp_port;
}

_u16 ptl_get_local_tcp_port()
{
	return g_tcp_port;
}

void ptl_set_local_udp_port(_u16 udp_port)
{
	g_udp_port = udp_port;
}

_u16 ptl_get_local_udp_port()
{
	return g_udp_port;
}

