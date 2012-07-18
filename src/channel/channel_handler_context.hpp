/*
 * ChannelHandlerContext.hpp
 *
 *  Created on: 2011-1-7
 *      Author: qiyingwang
 */

#ifndef NOVA_CHANNELHANDLERCONTEXT_HPP_
#define NOVA_CHANNELHANDLERCONTEXT_HPP_
#include <string>
#include "channel/channel_handler.hpp"
#include "timer/timer.hpp"
using std::string;
using arch::timer::Timer;

namespace arch
{
	namespace channel
	{
		class ChannelPipeline;
		class Channel;
		/**
		 * ChannleHandler�������Ķ��󣬿��Ծݴ˵õ�ע���handler����һ��/��һ��handler����
		 */
		class ChannelHandlerContext
		{
			private:
				ChannelHandlerContext* m_prev;
				ChannelHandlerContext* m_next;
				ChannelPipeline& m_pipeline;
				string m_name;
				ChannelHandler* m_handler;
				bool m_canHandleUpstream;
				bool m_canHandleDownstream;
			public:
				ChannelHandlerContext(ChannelPipeline& pipeline,
				        ChannelHandlerContext* prev,
				        ChannelHandlerContext* next, const string& name,
				        ChannelHandler* handler);
				inline const string& GetName()
				{
					return m_name;
				}
				inline bool CanHandleUpstream()
				{
					return m_canHandleUpstream;
				}
				inline bool CanHandleDownstream()
				{
					return m_canHandleDownstream;
				}
				inline ChannelHandlerContext* GetPrev()
				{
					return m_prev;
				}
				inline void SetPrev(ChannelHandlerContext* prev)
				{
					m_prev = prev;
				}
				inline ChannelHandlerContext* GetNext()
				{
					return m_next;
				}
				inline void SetNext(ChannelHandlerContext* next)
				{
					m_next = next;
				}
				inline ChannelHandler* GetHandler()
				{
					return m_handler;
				}
				inline ChannelPipeline& GetPipeline()
				{
					return m_pipeline;
				}
				Timer& GetTimer();
				Channel* GetChannel();

				/**
				 * ������������event
				 * @see channel_pipeline.hpp
				 */
				template<typename T>
				bool SendDownstream(T& e);

                /**
                 * ������������event
                 * @see channel_pipeline.hpp
                 */
				template<typename T>
				bool SendUpstream(T& e);
		};

	}
}

#endif /* CHANNELHANDLERCONTEXT_HPP_ */
