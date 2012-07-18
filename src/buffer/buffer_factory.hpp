/*
 * ByteBufferFactory.hpp
 * ����slab�ڴ�ģ�͹����Buffer������
 *  Created on: 2011-1-26
 *      Author: wqy
 */

#ifndef NOVA_BYTEBUFFERFACTORY_HPP_
#define NOVA_BYTEBUFFERFACTORY_HPP_
#include "common/base.hpp"
#include "buffer/buffer.hpp"
#include <map>
#include <deque>

using std::map;
using std::deque;
namespace arch
{
    namespace buffer
    {
        /**
         * ����slab�ڴ�ģ�͹����Buffer����
         */
        class BufferFactory
        {
            private:
                deque<Buffer*> m_buffer_table;
            public:
                BufferFactory();
                /**
                 * ��ȡָ�����ȵ�Buffer������buffer���ȿ��ܳ���ָ��size
                 */
                Buffer* Acquire(uint32 size);
                /**
                 * ����Buffer
                 */
                void Release(Buffer* buf);
                ~BufferFactory();
        };
    }
}

#endif /* BYTEBUFFERFACTORY_HPP_ */
