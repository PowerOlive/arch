/*
 * dispatcher_handler.hpp
 *
 *  Created on: 2011-7-23
 *      Author: wqy
 */

#ifndef IO_HANDLER_HPP_
#define IO_HANDLER_HPP_

#include "buffer/buffer.hpp"
#include "channel/all_includes.hpp"
#include "framework/framework.hpp"

using arch::buffer::Buffer;
using arch::channel::Channel;

namespace arch
{
    namespace framework
    {
        /**
         * IO���̺�ҵ���ӽ���֮���IO IPC event�߼�����
         * �û���Ҫʵ�ִ�����࣬��ע�ᵽmanager/dispatcher IO process��
         */
        struct IOProcessHandler
        {
                virtual bool OnInit()
                {
                    return true;
                }
                virtual bool OnDestroy()
                {
                    return true;
                }
                /**
                 * ���м��ص�
                 */
                virtual void OnRoutine()
                {
                }

                virtual void OnIPCChannelFlushComplete(Channel* ch)
                {

                }

                virtual void OnChannelWriteFailed(Channel* ch)
                {
                }

                /**
                 * �յ��������̵��ļ�������
                 */
                virtual void OnTransferFD(ServiceType src_type,
                        ServiceIndex src_idx, Channel* ch)
                {

                }

                /**
                 * �յ�ҵ���ӽ��̷�����ǰ���̵���Ϣ�ص������Ƿ�����ǰ���̵���Ҫ��ת����Ϣ��
                 */
                virtual void OnIPCMessage(ServiceType type, ServiceIndex idx,
                        Buffer& content)
                {
                }
                /**
                 * �յ�ҵ���ӽ��̴��������ⲿTCP/UDP/UNIX socket server������ص�
                 */
                virtual Channel* TCPSocketConnectRequest(
                        const std::string& host, uint16 port)
                {
                    return NULL;
                }

                virtual Channel* UDPSocketConnectRequest(
                        const std::string& host, uint16 port)
                {
                    return NULL;
                }
                virtual Channel* UnixSocketConnectRequest(
                        const std::string& remote)
                {
                    return NULL;
                }

                /**
                 * �յ�ҵ���ӽ��̴���TCP/UDP/UNIX socket����ص�
                 */
                virtual Channel* TCPSocketOpenRequest()
                {
                    return NULL;
                }
                virtual Channel* UDPSocketOpenRequest()
                {
                    return NULL;
                }
                virtual Channel * UnixSocketOpenRequest(
                        const std::string& local)
                {
                    return NULL;
                }
                virtual ~IOProcessHandler()
                {

                }
        };
    }
}
#endif /* DISPATCHER_HANDLER_HPP_ */
