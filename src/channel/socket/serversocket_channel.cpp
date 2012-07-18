/*
 * ServerSocketChannel.cpp
 *
 *  Created on: 2011-2-1
 *      Author: wqy
 */
#include "channel/all_includes.hpp"
#include "util/file_helper.hpp"
#include "net/network_helper.hpp"
#include "logging/logger_macros.hpp"
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace arch::channel;
using namespace arch::channel::socket;
using namespace arch::util;
using namespace arch::net;

ServerSocketChannel::ServerSocketChannel(ChannelService& factory) :
        SocketChannel(factory), m_connected_socks(0), m_accepted_cb(NULL)
{
}

//void ServerSocketChannel::SetChannelAcceptOperationBarrierHook(
//        ChannelOperationBarrierHook* hook, void* data)
//{
//    //m_accept_hook = hook;
//    m_accept_hook_data = data;
//}

bool ServerSocketChannel::DoConfigure(const ChannelOptions& options)
{
    if (!SocketChannel::DoConfigure(options))
    {
        return false;
    }
    int flag = 1;
    if (options.reuse_address)
    {
        int ret = setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &flag,
                sizeof(flag));
        if (ret != 0)
        {
            //ERR_LOG("Failed to set send buf size(%u) for socket.", options.send_buffer_size);
            return false;
        }
    }
    //#ifdef HAVE_TCP_DEFER_ACCEPT
    //    /* linux only */
    //    if(options.post_accept_timeout > 0)
    //    {
    //        int timeout = options.post_accept_timeout;
    //        if (setsockopt(m_fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
    //                        &timeout, sizeof(int)) ==-1)
    //        {
    //            WARN_LOG("failed setsockopt TCP_DEFER_ACCEPT %s\n",
    //                    strerror(errno));
    //            /* continue since this is not critical */
    //        }
    //    }
    //#endif
    return true;
}

bool ServerSocketChannel::DoConnect(Address* remote)
{
    ERROR_LOG("DoConnect() is not supported in server socket channel.");
    return false;
}

bool ServerSocketChannel::DoBind(Address* local)
{
    if (m_fd > 0)
    {
        return true;
    }
    int on = 1;
    SocketInetAddress addr;
    if (InstanceOf<SocketHostAddress>(local).Value)
    {
        SocketHostAddress* host_addr = (SocketHostAddress*) local;
        addr = getInetAddress(host_addr->GetHost(), host_addr->GetPort());
    }
    else if (InstanceOf<SocketInetAddress>(local).Value)
    {
        SocketInetAddress* inet_addr = (SocketInetAddress*) local;
        addr = (*inet_addr);
    }
    else if (InstanceOf<SocketUnixAddress>(local).Value)
    {
        SocketUnixAddress* unix_addr = (SocketUnixAddress*) local;
        int ret = unlink(unix_addr->GetPath().c_str()); // in case it already exists
        if (ret == -1)
        {
            int e = errno;
            ERROR_LOG(
                    "unlink %s failed:%s", unix_addr->GetPath().c_str(), strerror(e));
        }
        addr = getInetAddress(*unix_addr);
    }
    else
    {
        return false;
    }
    int family = addr.GetRawSockAddr().sa_family;
    int fd = ::socket(family, SOCK_STREAM, 0);
    if (fd < 0)
    {
        return false;
    }

    if (make_fd_nonblocking(fd) < 0)
    {
        ::close(fd);
        return false;
    }
    if (!addr.IsUnix())
    {
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
        {
            ::close(fd);
            return false;
        }
    }
    else
    {
        struct sockaddr_un* pun = (struct sockaddr_un*) &(addr.GetRawSockAddr());
        DEBUG_LOG("Bind on %s", pun->sun_path);
//		int nZero = 0;
//		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &nZero, sizeof(nZero));
//		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &nZero, sizeof(int));

    }
    //setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*) &on, sizeof(on));
    if (::bind(fd, (struct sockaddr*) &(addr.GetRawSockAddr()),
            addr.GetRawSockAddrSize()) == -1)
    {
        int e = errno;
        ERROR_LOG("Failed to bind address for reason:%s", strerror(e));
        ::close(fd);
        return false;
    }
    if (::listen(fd, 511) == -1)
    { /* the magic 511 constant is from nginx */
        int e = errno;
        ERROR_LOG("Failed to listen for reason:%s", strerror(e));
        ::close(fd);
        return false;
    }
    if (aeCreateFileEvent(m_service.GetRawEventLoop(), fd, AE_READABLE,
            Channel::IOEventCallback, this) == AE_ERR)
    {
        ::close(fd);
        return false;
    }
    m_fd = fd;
    return true;
}

uint32 ServerSocketChannel::ConnectedSockets()
{
    return m_connected_socks;
}

void ServerSocketChannel::OnChildClose(Channel* ch)
{
    m_connected_socks--;
}

//static void DelayAttach(ServerSocketChannel* ss)
//{
//    if (ss->IsDetached())
//    {
//        DEBUG_LOG("Attach read FD again.");
//        ss->AttachFD();
//    }
//}

void ServerSocketChannel::OnRead()
{
    int fd;
    char addrbuf[128];
    //continue run accept until it says it would block
    while (1)
    {
//        if (NULL != m_accept_hook)
//        {
//            int hook_ret = m_accept_hook(this, m_accept_hook_data);
//            if (hook_ret == CHANNEL_OPERATION_BREAK)
//            {
//                return;
//            }
//            else if (hook_ret == CHANNEL_OPERATION_DELAY_READ)
//            {
//                DetachFD();
//                m_service.GetTimer().ScheduleHeapTask(
//                        make_fun_runnable(DelayAttach, this),
//                        m_options.delay_read_mills, -1);
//                return;
//            }
//        }
        socklen_t salen = sizeof(addrbuf);
        struct sockaddr* sa = (struct sockaddr*) addrbuf;
#if (HAVE_ACCEPT4)
        fd = accept4(m_fd, sa, &salen,SOCK_NONBLOCK);
#else
        fd = ::accept(m_fd, sa, &salen);
        if (fd != -1 && -1 == make_fd_nonblocking(fd))
        {
            DEBUG_LOG("failed to set opt for accept socket");
            ::close(fd);
            return;
        }
#endif
        if (-1 == fd)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                return;
            }
        }
        ClientSocketChannel * ch = m_service.NewClientSocketChannel();
        if (aeCreateFileEvent(m_service.GetRawEventLoop(), fd, AE_READABLE,
                Channel::IOEventCallback, ch) == AE_ERR)
        {
            ERROR_LOG("Failed to add event for accepted client for fd:%d.", fd);
            m_service.DeleteChannel(ch);
            ::close(fd);
            return;
        }
        ch->m_fd = fd;
        if (m_user_configed)
        {
            ch->Configure(m_options);
        }
        if (NULL != m_pipeline_initializor)
        {
            DEBUG_LOG("Inherit pipeline initializor from server socket.");
            ch->SetChannelPipelineInitializor(m_pipeline_initializor,
                    m_pipeline_initailizor_user_data);
        }
        if (NULL != m_pipeline_finallizer)
        {
            DEBUG_LOG("Inherit pipeline finalizer from server socket.");
            ch->SetChannelPipelineFinalizer(m_pipeline_finallizer,
                    m_pipeline_finallizer_user_data);
        }
        ch->SetParent(this);

        DEBUG_LOG(
                "Server channel(%u) Accept a client channel(%u) for fd:%d", GetID(), ch->GetID(), fd);
        fire_channel_open(ch);
        fire_channel_connected(ch);
        m_connected_socks++;
        if(NULL != m_accepted_cb)
        {
            if(m_accepted_cb(this, ch) == -1)
            {
                break;
            }
        }
    }

}

ServerSocketChannel::~ServerSocketChannel()
{
}
