#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rtmp_publish.h"
#include "aac_file_parser.h"

#define AAC_HEADER_LENGTH 7
#define MAX_AAC_FRAME_SIZE 1024

static const int mpeg4audio_sample_rates[16] = {
   96000, 88200, 64000, 48000, 44100, 32000,
   24000, 22050, 16000, 12000, 11025, 8000, 7350
};

int main(int argc, char ** argv)
{
    int ret = 0;
    bool isAudioInit = false;
    unsigned int presentationTime = 0;
    unsigned int audioTimeStamp   = 0;

    if (argc < 3)
    {
	printf("usage: ./PusherModuleTest <url> <file>\n");
        return ret;
    }
    
    const char* url = argv[1];
    const char* path = argv[2];
    
    RtmpPubContext * pRtmpc = RtmpPubNew(url, 10, RTMP_PUB_AUDIO_AAC, RTMP_PUB_AUDIO_AAC, RTMP_PUB_TIMESTAMP_RELATIVE);
    if (NULL == pRtmpc)
    {
        printf("call RtmpPubNew failed.\n");
        return ret;
    }

    //初始化句柄
    if (0 != RtmpPubInit(pRtmpc))
    {
        printf("call RtmpPubInit failed.\n");
        return 0;
    }
   
    //连接服务器
    if (0 != RtmpPubConnect(pRtmpc))
    {
        printf("call RtmpPubConnect failed.\n");
        return 0;
    }
    
    //get AAC config data
    HANDLE_AAC_FILE handler = AacFileParser_Open((SCHAR *)path);
    if (handler == NULL)
    {
        printf("faild to open aac file\n");
        return 0;
    }
    
    if (TT_MP4_ADTS != AacFileParser_GetTransportType(handler))
    {
        printf("only support AAC ADTS format file.\n");
        return 0;
    }
   
    //read aac file ADTS header
    unsigned char buffer[256] = {0};
    unsigned int readSize = 0; 
    if ( 0 != AacFileParser_Read(handler, buffer, AAC_HEADER_LENGTH, &readSize))
    {
        printf("failed to read aac header.\n");
        return 0;
    }

    if (isAudioInit == false) 
    {
        presentationTime = audioTimeStamp;
        
	//初始化音频时间基准, presentationTime和第一个音频包的时间戳相同
	RtmpPubSetAudioTimebase(pRtmpc, presentationTime);
        
	//设置AAC配置数据
        char configData[2] = {0};
        AacFileParser_GetAudioSpecificConfig(buffer, AAC_HEADER_LENGTH + 1, (unsigned char*)configData);
	RtmpPubSetAac(pRtmpc, configData, readSize);
	isAudioInit = true;
    }

    if(0 != AacFileParser_DecodeHeader(handler, buffer, readSize))
    {
        printf("call AacFileParser_DecodeHeader failed.\n");
        return 0;
    }
  
    AdtsHeader* aacHeader = AacFileParser_GetHeader(handler);
    if (NULL == aacHeader)
    {
        printf("call AacFileParser_GetHeader failed.\n");
        return 0;
    }

    unsigned char frameData[MAX_AAC_FRAME_SIZE] = {0};

    //read aac frame data from file and then send aac frame to stream server
    while(1)
    {
        if (0 == aacHeader->nProtectionAbsent)
        {
            AacFileParser_Seek(handler, 1, 2);
        }
        
        unsigned int frameLength = AacFileParser_GetRawDataBlockLength(handler, 0);
        
        //read aac file ADTS header
        unsigned int byteReaded = 0;
        frameData[frameLength] = 0;
        if ( 0 != AacFileParser_Read(handler, frameData, frameLength, &byteReaded))
        {
            printf("failed to read aac ADTS frame.\n");
            break;
        }

        if (byteReaded < frameLength)
        {
            printf("readed %d bytes, not read enough frame data(%d) from file", byteReaded, frameLength);
            break;
        }

        float frameTimeSpace = (1024 * 1000 / (float)mpeg4audio_sample_rates[aacHeader->nSfIndex]);
        presentationTime = static_cast<unsigned int>(audioTimeStamp* frameTimeSpace);
        audioTimeStamp ++;
        
        if ( 0!= RtmpPubSendAudioFrame(pRtmpc,  (char*)frameData, frameLength,  presentationTime))
        {
            printf("send %d frame failed.\n");
            break;
        }
        
        //read aac file ADTS header
        memset(buffer, 0, AAC_HEADER_LENGTH + 1);
        readSize = 0; 
        if ( 0 != AacFileParser_Read(handler, buffer, AAC_HEADER_LENGTH, &readSize))
        {
            printf("failed to read aac header.\n");
            break;
        }
        
        if(0 != AacFileParser_DecodeHeader(handler, buffer, readSize))
        {
            printf("call AacFileParser_DecodeHeader failed.\n");
            break;
        }
        aacHeader = AacFileParser_GetHeader(handler);
        if (NULL == aacHeader)
        {
            printf("call AacFileParser_GetHeader failed.\n");
            break;
        }
        
        usleep(static_cast<unsigned int>(frameTimeSpace*1000));
    }

    printf("quit send frame loop.\n");
    
    AacFileParser_Close(handler);
    RtmpPubDel(pRtmpc);
    
    printf("stoped push aac frame to FMS.\n");
    
    return 0;
}
