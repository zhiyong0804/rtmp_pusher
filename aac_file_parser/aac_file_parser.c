/*****************************  MPEG-4 AAC File Parser  ************************

   Author(s):   Arlon Lee
   Description: AAC File Parser interface

*******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "aac_file_parser.h"

#define MAX_CHANNELS 6 /* make this higher to support files with
                          more channels */

/* A decode call can eat up to FAAD_MIN_STREAMSIZE bytes per decoded channel,
   so at least so much bytes per channel should be available in this stream */
#define FAAD_MIN_STREAMSIZE 768 /* 6144 bits/channel */

HANDLE_AAC_FILE AacFileParser_Open(const SCHAR *filename)
{
    if ((filename == NULL) || (strlen(filename) <= 0) )
    {
        return NULL;
    }
    
    HANDLE_AAC_FILE handler = (HANDLE_AAC_FILE)malloc(sizeof(struct AAC_File_Parser));
    memset(handler, 0, sizeof(handler));
    
    handler->reader.inFile = fopen(filename, "rb");
    if (handler->reader.inFile == NULL)
    {
        free(handler);
        return NULL;
    }
    
    UCHAR* buffer = (UCHAR*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS);
    memset(buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS);
    int size = fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, handler->reader.inFile);
    if (size <= 0)
    {
        free(buffer);
        free(handler);
        return NULL;
    }

    //printf("buffer first two bytes is 0x%hhx, 0x%hhx\n", buffer[0], buffer[1]);
    
    // AAC ADTS file
    if ((buffer[0] == 0xFF) && ((buffer[1] & 0xF0) == 0xF0))
    {
        handler->reader.tt = TT_MP4_ADTS;
    }
    else if (memcmp(buffer, "ADIF", 4) == 0) 
    {
        handler->reader.tt = TT_MP4_ADIF;
    }
    else
    {
        free(buffer);
        free(handler);
        //printf("failed to open %s file\n", filename);
        return NULL;
    }
    
    free(buffer);
    fseek(handler->reader.inFile, 0, SEEK_SET);
    
    return handler;
}

INT AacFileParser_Read( HANDLE_AAC_FILE  hAacFile,
                        UCHAR            *inBuffer,
                        UINT              bufferSize,
                        UINT             *bytesValid
                      )
{
    if (hAacFile == NULL)
    {
        return -1;
    }
    
    //reach end of file
    if (feof(hAacFile->reader.inFile))
    {
        return -1;
    }
    
    *bytesValid = fread(inBuffer, 1, bufferSize, hAacFile->reader.inFile);
    
    if (0 == *bytesValid)
    {
        return -1;
    }

    return 0;
}

INT AacFileParser_Seek( HANDLE_AAC_FILE  hAacFile,
                        INT origin, 
                        INT offset
                      )
{
    if (0 == origin)
    {
        return fseek(hAacFile->reader.inFile, offset, SEEK_SET);
    }
    else if (1 == origin)
    {
        return fseek(hAacFile->reader.inFile, offset, SEEK_CUR);
    }
    
    return 0;
}

TRANSPORT_TYPE AacFileParser_GetTransportType(HANDLE_AAC_FILE  hAacFile)
{
    if (hAacFile == NULL)
    {
        return TT_UNKNOWN;
    }

    return hAacFile->reader.tt;
}

INT AacFileParser_DecodeHeader(
        HANDLE_AAC_FILE       hAacFile,
        UCHAR                 *inBuffer,
        UINT                  bufferSize
        )
{
    if (hAacFile == NULL )
    {
        return -1;
    }

    // headers begin with FFFxxxxx...
    if ((inBuffer[0] == 0xff) && ((inBuffer[1] & 0xf0) == 0xf0)) 
    {
        hAacFile->header.nSyncWord = (inBuffer[0] << 4) | (inBuffer[1] >> 4);
        hAacFile->header.nId = (inBuffer[1] & 0x08) >> 3;
        hAacFile->header.nLayer = (inBuffer[1] & 0x06) >> 1;
        hAacFile->header.nProtectionAbsent = inBuffer[1] & 0x01;
        hAacFile->header.nProfile = (inBuffer[2] & 0xc0) >> 6;
        hAacFile->header.nSfIndex = (inBuffer[2] & 0x3c) >> 2;
        hAacFile->header.nPrivateBit = (inBuffer[2] & 0x02) >> 1;
        hAacFile->header.nChannelConfiguration = (((inBuffer[2] & 0x01) << 2) | ((inBuffer[3] & 0xc0) >> 6));
        hAacFile->header.nOriginal = (inBuffer[3] & 0x20) >> 5;
        hAacFile->header.nHome = (inBuffer[3] & 0x10) >> 4;
        hAacFile->header.nCopyrightIdentificationBit = (inBuffer[3] & 0x08) >> 3;
        hAacFile->header.nCopyrigthIdentificationStart = inBuffer[3] & 0x04 >> 2;
        hAacFile->header.nAacFrameLength = ((((inBuffer[3]) & 0x03) << 11) |
                                            ((inBuffer[4] & 0xFF) << 3) |
                                            (inBuffer[5] & 0xE0) >> 5);
        hAacFile->header.nAdtsBufferFullness = ((inBuffer[5] & 0x1f) << 6 | (inBuffer[6] & 0xfc) >> 2);
        hAacFile->header.nNoRawDataBlocksInFrame = (inBuffer[6] & 0x03);
    }
    else {
        return -1;
    }

    return 0;
}

INT AacFileParser_GetAudioSpecificConfig(UCHAR *inBuffer,
                                         UINT  bufferSize,
                                         UCHAR *asc
                                        )
{
    if ((inBuffer[0] == 0xff) && ((inBuffer[1] & 0xf0) == 0xf0)) 
    {
        asc[0] = (((inBuffer[2] & 0xc0) >> 6) + 1) << 3;
        asc[0] = asc[0] | (((inBuffer[2] & 0x3c) >> 2) >> 1);
        asc[1] = ((inBuffer[2] & 0x3c) >> 2) << 7 ;
        asc[1] = asc[1] | ( ((((inBuffer[2] & 0x01) << 2) | ((inBuffer[3] & 0xc0) >> 6))) << 3);
        return 0;
    }

    return -1;
}

AdtsHeader * AacFileParser_GetHeader(HANDLE_AAC_FILE hAacFile)
{
    if (NULL == hAacFile)
    {
        return NULL;
    }
    return &hAacFile->header;
}

INT AacFileParser_GetRawDataBlockLength(
        HANDLE_AAC_FILE  hAacFile,
        INT              blockNum
        )
{
    if (NULL == hAacFile)
    {
        return -1;
    }

    INT length = -1;

    if (hAacFile->header.nNoRawDataBlocksInFrame == 0) 
    {
        length = hAacFile->header.nAacFrameLength - 7;    /* aac_frame_length subtracted by the header size (7 bytes). */
        if (hAacFile->header.nProtectionAbsent == 0)
          length -= 2;                                          /* substract 16 bit CRC */
    }

    return length;
}

INT AacFileParser_Close(HANDLE_AAC_FILE hAacFile)
{
    INT ret = 0;
    if (hAacFile == NULL || !hAacFile->reader.inFile)
    {
        return -1;
    }
    
    if (0 != fclose(hAacFile->reader.inFile))
    {
        ret = -1;
    }

    free(hAacFile);
    hAacFile = NULL;
    
    return ret;
}