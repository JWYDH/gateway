#pragma once

#include <string>
#include <stdint.h>
#include <random>
#include <time.h>

#include "SHA1.h"
#include "base64.h"

 // 0                   1                   2                   3
 // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 //+-+-+-+-+-------+-+-------------+-------------------------------+
 //|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 //|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 //|N|V|V|V|       |S|             |   (if payload len==126/127)   |
 //| |1|2|3|       |K|             |                               |
 //+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 //|     Extended payload length continued, if payload len == 127  |
 //+ - - - - - - - - - - - - - - - +-------------------------------+
 //|                               |Masking-key, if MASK set to 1  |
 //+-------------------------------+-------------------------------+
 //| Masking-key (continued)       |          Payload Data         |
 //+-------------------------------- - - - - - - - - - - - - - - - +
 //:                     Payload Data continued ...                :
 //+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 //|                     Payload Data continued ...                |
 //+---------------------------------------------------------------+

//FIN      1bit 表示信息的最后一帧，flag，也就是标记符
//RSV 1 - 3  1bit each 以后备用的 默认都为 0
//Opcode   4bit 帧类型
//Mask     1bit 掩码，是否加密数据，默认必须置为1  表示净荷是否有掩码（只适用于客户端发送给服务器的消息）。
//Payload  7bit 数据的长度
//Masking - key      1 or 4 bit 掩码 用于给净荷加掩护，客户端到服务器标记
//Payload data(x + y) bytes 数据
//Extension data   x bytes  扩展数据
//Application data y bytes  程序数据


// |Opcode  | Meaning                             | Reference |
//-+--------+-------------------------------------+-----------|
// | 0      | Continuation Frame                  | RFC 6455  |
//-+--------+-------------------------------------+-----------|
// | 1      | Text Frame                          | RFC 6455  |
//-+--------+-------------------------------------+-----------|
// | 2      | Binary Frame                        | RFC 6455  |
//-+--------+-------------------------------------+-----------|
// | 8      | Connection Close Frame              | RFC 6455  |
//-+--------+-------------------------------------+-----------|
// | 9      | Ping Frame                          | RFC 6455  |
//-+--------+-------------------------------------+-----------|
// | 10     | Pong Frame                          | RFC 6455  |
//-+--------+-------------------------------------+-----------|
class WebSocketFormat
    {
    public:
        enum class WebSocketFrameType {
            ERROR_FRAME = 0xff,
            CONTINUATION_FRAME = 0x00,
            TEXT_FRAME = 0x01,
            BINARY_FRAME = 0x02,
            CLOSE_FRAME = 0x08,
            PING_FRAME = 0x09,
            PONG_FRAME = 0x0A
        };

        static std::string wsHandshake(std::string secKey)
        {
            secKey.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

            CSHA1 s1;
            s1.Update((unsigned char*)secKey.c_str(), static_cast<unsigned int>(secKey.size()));
            s1.Final();
            unsigned char puDest[20];
            s1.GetHash(puDest);

            std::string base64Str = base64_encode((const unsigned char *)puDest, 20);

            std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: ";

            response += base64Str;
            response += "\r\n\r\n";

            return response;
        }

        static bool wsFrameBuild(const char* payload,
            size_t payloadLen,
            std::string& frame,
            WebSocketFrameType frame_type = WebSocketFrameType::TEXT_FRAME,
            bool isFin = true,
            bool masking = false)
        {
            static std::mt19937 random(time(0));

            static_assert(std::is_same<std::string::value_type, char>::value, "");

            const uint8_t head = static_cast<uint8_t>(frame_type) | (isFin ? 0x80 : 0x00);

            frame.clear();
            frame.push_back(static_cast<char>(head));
            if (payloadLen <= 125)
            {
                // mask << 7 | payloadLen, mask = 0
                frame.push_back(static_cast<uint8_t>(payloadLen));
            }
            else if (payloadLen <= 0xFFFF)
            {
                // 126 + 16bit len
                frame.push_back(126);
                frame.push_back((payloadLen & 0xFF00) >> 8);
                frame.push_back(payloadLen & 0x00FF);
            }
            else
            {
                // 127 + 64bit len
                frame.push_back(127);
                // assume payload len is less than u_int32_max
                frame.push_back(0x00);
                frame.push_back(0x00);
                frame.push_back(0x00);
                frame.push_back(0x00);
                frame.push_back(static_cast<char>((payloadLen & 0xFF000000) >> 24));
                frame.push_back(static_cast<char>((payloadLen & 0x00FF0000) >> 16));
                frame.push_back(static_cast<char>((payloadLen & 0x0000FF00) >> 8));
                frame.push_back(static_cast<char>(payloadLen & 0x000000FF));
            }

            if (masking)
            {
                frame[1] = ((uint8_t)frame[1]) | 0x80;
                uint8_t mask[4];
                for (auto& m : mask)
                {
                    m = random();
                    frame.push_back(m);
                }

                frame.reserve(frame.size() + payloadLen);

                for (size_t i = 0; i < payloadLen; i++)
                {
                    frame.push_back(static_cast<uint8_t>(payload[i]) ^ mask[i % 4]);
                }
            }
            else
            {
                frame.append(payload, payloadLen);
            }

            return true;
        }

        static bool wsFrameExtractBuffer(const char* inbuffer,
            const size_t bufferSize,
            std::string& payload,
            WebSocketFrameType& outopcode,
            size_t& frameSize,
            bool& outfin)
        {
            const auto buffer = (const unsigned char*)inbuffer;

            if (bufferSize < 2)
            {
                return false;
            }

			//最高位用于描述消息是否结束,如果为1则该消息为消息尾部,如果为零则还有后续数据包;
			//后面3位是用于扩展定义的,如果没有扩展约定的情况则必须为0.
            outfin = (buffer[0] & 0x80) != 0;
			//最低4位用于描述消息类型,消息类型暂定有15种,其中有几种是预留设置
            outopcode = (WebSocketFrameType)(buffer[0] & 0x0F);
			//消息的第二个字节主要用一描述掩码和消息长度,最高位用0或1来描述是否有掩码处理,
            const bool isMasking = (buffer[1] & 0x80) != 0;
            uint32_t payloadlen = buffer[1] & 0x7F;

			//剩下的后面7位用来描述消息长度, 由于7位最多只能描述127所以这个值会代表三种情况, 
			//一种是消息内容少于126存储消息长度, 如果消息长度少于UINT16的情况此值为126, 
			//当消息长度大于UINT16的情况下此值为127; 这两种情况的消息长度存储到紧随后面的byte[], 
			//分别是UINT16(2位byte)和UINT64(4位byte).
            uint32_t pos = 2;
            if (payloadlen == 126)
            {
                if (bufferSize < 4)
                {
                    return false;
                }

                payloadlen = (buffer[2] << 8) + buffer[3];
                pos = 4;
            }
            else if (payloadlen == 127)
            {
                if (bufferSize < 10)
                {
                    return false;
                }

                if (buffer[2] != 0 ||
                    buffer[3] != 0 ||
                    buffer[4] != 0 ||
                    buffer[5] != 0)
                {
                    return false;
                }

                if ((buffer[6] & 0x80) != 0)
                {
                    return false;
                }

                payloadlen = (buffer[6] << 24) + (buffer[7] << 16) + (buffer[8] << 8) + buffer[9];
                pos = 10;
            }

            uint8_t mask[4];
            if (isMasking)
            {
                if (bufferSize < (pos + 4))
                {
                    return false;
                }

                mask[0] = buffer[pos++];
                mask[1] = buffer[pos++];
                mask[2] = buffer[pos++];
                mask[3] = buffer[pos++];
            }

            if (bufferSize < (pos + payloadlen))
            {
                return false;
            }

			//获取消息体还有一个需要注意的地方就是掩码,如果存在掩码的情况下接收的byte[]要做如下转换处理:
            if (isMasking)
            {
                payload.reserve(payloadlen);
                for (size_t i = pos, j = 0; j < payloadlen; i++, j++)
                    payload.push_back(buffer[i] ^ mask[j % 4]);
            }
            else
            {
                payload.append((const char*)(buffer + pos), payloadlen);
            }

            frameSize = payloadlen + pos;

            return true;
        }


    };

