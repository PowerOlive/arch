/*
 * BufferHelper.hpp
 * ���л�/�����л��������͵�Buffer�ĸ�����
 *
 *      Author: qiyingwang
 */

#ifndef NOVA_BufferHELPER_HPP_
#define NOVA_BufferHELPER_HPP_
#include "buffer/buffer.hpp"
#include <string>
using std::string;
#define MAX_BYTEARRAY_BUFFER_SIZE 4096
namespace arch
{
    namespace buffer
    {
        /**
         * ���л�/�����л��������͵�Buffer�ĸ�����
         */
        class BufferHelper
        {
            public:
                /**
                 * �����л��̶����ȵ�uint64
                 * @fromNetwork: �Ƿ��������ֽ���ת��
                 */
                static bool ReadFixUInt64(Buffer& buffer, uint64_t& i,
                        bool fromNetwork = true);
                static bool ReadFixInt64(Buffer& buffer, int64_t& i,
                        bool fromNetwork = true);
                static bool ReadFixUInt32(Buffer& buffer, uint32_t& i,
                        bool fromNetwork = true);
                static bool ReadFixInt32(Buffer& buffer, int32_t& i,
                        bool fromNetwork = true);
                static bool ReadFixUInt16(Buffer& buffer, uint16_t& i,
                        bool fromNetwork = true);
                static bool ReadFixInt16(Buffer& buffer, int16_t& i,
                        bool fromNetwork = true);
                static bool ReadFixUInt8(Buffer& buffer, uint8_t& i);
                static bool ReadFixInt8(Buffer& buffer, int8_t& i);
                static bool ReadFixString(Buffer& buffer, string& str,
                        bool fromNetwork = true);
                static bool ReadFixString(Buffer& buffer, char*& str,
                        bool fromNetwork = true);

                /**
                 * �����л��ǹ̶����ȵ�uint64�� ��ѭzigzag�㷨
                 */
                static bool ReadVarDouble(Buffer& buffer, double& i);
                static bool ReadVarUInt64(Buffer& buffer, uint64_t& i);
                static bool ReadVarInt64(Buffer& buffer, int64_t& i);
                static bool ReadVarUInt32(Buffer& buffer, uint32_t& i);
                static bool ReadVarInt32(Buffer& buffer, int32_t& i);
                static bool ReadVarUInt16(Buffer& buffer, uint16_t& i);
                static bool ReadVarInt16(Buffer& buffer, int16_t& i);
                static bool ReadVarString(Buffer& buffer, string& str);
                static bool ReadVarString(Buffer& buffer, char*& str);
                static bool ReadBool(Buffer& buffer, bool& value);

                static bool WriteBool(Buffer& buffer, bool value);
                /**
                 * ���л��̶����ȵ�uint64
                 * @toNetwork: �Ƿ��������ֽ���ת��
                 */
                static bool WriteFixUInt64(Buffer& buffer, uint64_t i,
                        bool toNetwork = true);
                static bool WriteFixInt64(Buffer& buffer, int64_t i,
                        bool toNetwork = true);
                static bool WriteFixUInt32(Buffer& buffer, uint32_t i,
                        bool toNetwork = true);
                static bool WriteFixInt32(Buffer& buffer, int32_t i,
                        bool toNetwork = true);
                static bool WriteFixUInt16(Buffer& buffer, uint16_t i,
                        bool toNetwork = true);

                static bool WriteFixInt16(Buffer& buffer, int16_t i,
                        bool toNetwork = true);
                static bool WriteFixUInt8(Buffer& buffer, uint8_t i);
                static bool WriteFixInt8(Buffer& buffer, int8_t i);
                static bool WriteFixString(Buffer& buffer, const string& str,
                        bool toNetwork = true);
                static bool WriteFixString(Buffer& buffer, const char* str,
                        bool toNetwork = true);
                /**
                 * ���л��ǹ̶����ȵ�uint64, ��ѭzigzag�㷨
                 */
                static bool WriteVarUInt64(Buffer& buffer, uint64_t i);
                static bool WriteVarInt64(Buffer& buffer, int64_t i);
                static bool WriteVarUInt32(Buffer& buffer, uint32_t i);
                static bool WriteVarUInt16(Buffer& buffer, uint16_t i);
                static bool WriteVarInt16(Buffer& buffer, int16_t i);
                static bool WriteVarInt32(Buffer& buffer, int32_t i);
                static bool WriteVarDouble(Buffer& buffer, double d);
                static bool WriteVarString(Buffer& buffer, const string& str);
                static bool WriteVarString(Buffer& buffer, const char* str);
        };

    }
}

#endif /* BufferHELPER_HPP_ */
