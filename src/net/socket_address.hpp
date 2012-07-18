/*
 * SocketAddress.hpp
 *
 *  Created on: 2011-1-9
 *      Author: wqy
 */

#ifndef NOVA_SOCKETADDRESS_HPP_
#define NOVA_SOCKETADDRESS_HPP_
#include "net/socket_host_address.hpp"
#include "net/socket_inet_address.hpp"
namespace arch
{
	namespace net
	{
		class SocketAddress: public SocketHostAddress
		{
			private:
				SocketInetAddress m_inet_addr;
			public:
				SocketAddress(const string& host, uint32 port);
		};
	}
}

#endif /* SOCKETADDRESS_HPP_ */
