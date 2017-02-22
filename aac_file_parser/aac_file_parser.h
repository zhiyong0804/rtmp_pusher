/*****************************  MPEG-4 AAC File Parser  ************************

   Author(s):   Arlon Lee
   Description: AAC File Parser interface

*******************************************************************************/
#ifndef AAC_FILE_PARSER_H_
#define AAC_FILE_PARSER_H_

#include <stdio.h>

#include "machine_type.h"

/**
 * File format identifiers.
 */
typedef enum
{
  FF_UNKNOWN           = -1, /**< Unknown format.        */
  FF_RAW               = 0,  /**< No container, bit stream data conveyed "as is". */

  FF_MP4_3GPP          = 3,  /**< 3GPP file format.      */
  FF_MP4_MP4F          = 4,  /**< MPEG-4 File format.     */

  FF_RAWPACKETS        = 5,  /**< Proprietary raw packet file. */

  FF_DRMCT             = 12  /**< Digital Radio Mondial (DRM30/DRM+) CT proprietary file format. */

} FILE_FORMAT;

/**
 * Transport type identifiers.
 */
typedef enum
{
  TT_UNKNOWN           = -1, /**< Unknown format.            */
  TT_MP4_RAW           = 0,  /**< "as is" access units (packet based since there is obviously no sync layer) */
  TT_MP4_ADIF          = 1,  /**< ADIF bitstream format.     */
  TT_MP4_ADTS          = 2,  /**< ADTS bitstream format.     */

  TT_MP4_LATM_MCP1     = 6,  /**< Audio Mux Elements with muxConfigPresent = 1 */
  TT_MP4_LATM_MCP0     = 7,  /**< Audio Mux Elements with muxConfigPresent = 0, out of band StreamMuxConfig */

  TT_MP4_LOAS          = 10, /**< Audio Sync Stream.         */

  TT_DRM               = 12  /**< Digital Radio Mondial (DRM30/DRM+) bitstream format. */

} TRANSPORT_TYPE;

typedef struct _AdtsHeader
{
    UINT nSyncWord;
    UINT nId;
    UINT nLayer;
    UINT nProtectionAbsent;
    UINT nProfile;
    UINT nSfIndex;
    UINT nPrivateBit;
    UINT nChannelConfiguration;
    UINT nOriginal;
    UINT nHome;

    UINT nCopyrightIdentificationBit;
    UINT nCopyrigthIdentificationStart;
    UINT nAacFrameLength;
    UINT nAdtsBufferFullness;

    UINT nNoRawDataBlocksInFrame;
} AdtsHeader;


typedef struct _MpegFileRead
{
    FILE* inFile;
    TRANSPORT_TYPE tt;
} MpegFileRead;

 struct AAC_File_Parser
{
    MpegFileRead   reader;
    AdtsHeader     header;
};

typedef struct AAC_File_Parser* HANDLE_AAC_FILE;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief           Open an AAC audio file and try to detect its format.
 * \param filename  String of the filename to be opened.
 * \return          AAC file parser handle.
 */
HANDLE_AAC_FILE AacFileParser_Open(const SCHAR *filename);

/**
 * \brief Read data from AAC file.
 *
 * \param hAacFile    AAC file parser handle.
 * \param inBuffer    Pointer to input buffer.
 * \param bufferSize  Size of input buffer.
 * \param bytesValid  Number of bytes that were read.
 * \return            0 on success, -1 if unsupported file format or file read error.
 */
INT AacFileParser_Read( HANDLE_AAC_FILE  hAacFile,
                        UCHAR            *inBuffer,
                        UINT              bufferSize,
                        UINT             *bytesValid
                      );

/**
 * \brief            Seek in file from origin by given offset in frames.
 * \param hMpegFile  AAC file parser handle.
 * \param origin     If 0, the origin is the file beginning (absolute seek).
 *                   If 1, the origin is the current position (relative seek).
 * \param offset     The amount of frames to seek from the given origin.
 * \return           0 on sucess, -1 if offset < 0 or file read error.
 */
INT AacFileParser_Seek( HANDLE_AAC_FILE  hAacFile,
                        INT origin, 
                        INT offset
                      );

/**
 * \brief           Get the transport type of the input file.
 * \param hDataSrc  MPEG file read handle.
 * \return          Transport type of the input file.
 */
TRANSPORT_TYPE AacFileParser_GetTransportType(HANDLE_AAC_FILE  hAacFile);

/**
 * \brief Check if we have a valid ADTS frame at the current bitbuffer position
 *
 * This function assumes enough bits in buffer for the current frame.
 * It reads out the header bits to prepare the bitbuffer for the decode loop.
 * In case the header bits show an invalid bitstream/frame, the whole frame is skipped.
 *
 * \param pAdts ADTS data handle which is filled with parsed ADTS header data
 * \param bs handle of bitstream from whom the ADTS header is read
 *
 * \return  0 on success, -1 if unsupported file format or file read error.
 */
INT AacFileParser_DecodeHeader(
        HANDLE_AAC_FILE       hAacFile,
        UCHAR                 *inBuffer,
        UINT                  bufferSize
        );


AdtsHeader* AacFileParser_GetHeader(HANDLE_AAC_FILE hAacFile);


INT AacFileParser_GetAudioSpecificConfig(UCHAR *inBuffer,
                                         UINT  bufferSize,
                                         UCHAR *asc
                                        );

/**
 * \brief Get the raw data block length of the given block number.
 *
 * \param pAdts ADTS data handle
 * \param blockNum current raw data block index
 *
 * \return  length of raw data block
 */
INT AacFileParser_GetRawDataBlockLength(
        HANDLE_AAC_FILE  hAacFile,
        INT              blockNum
        );

/**
 * \brief           Close AAC file.
 * \param hMpegFile Mpeg file read handle.
 * \return          0 on sucess.
 */
INT AacFileParser_Close(HANDLE_AAC_FILE hAacFile);

#ifdef __cplusplus
}
#endif

#endif //end of AAC_FILE_PARSER_H_