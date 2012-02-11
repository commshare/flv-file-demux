#include <memory.h>
#include <malloc.h>
#include <string.h>
#include "byte_parse.h"
#include "amf_parse.h"
#include "flv_demux.h"
#include "../mp_msg.h"

BOOL amf_parse_elem_name    (FLVDemuxInf* dmx);
BOOL amf_parse_number       (FLVDemuxInf* dmx);
BOOL amf_parse_boolean      (FLVDemuxInf* dmx);
BOOL amf_parse_string       (FLVDemuxInf* dmx);
BOOL amf_parse_long_string  (FLVDemuxInf* dmx);
BOOL amf_parse_object       (FLVDemuxInf* dmx)
{
    UI16 elem_name_length;
    UI8* elem_name;

    if (dmx->m_CurrentPacket == NULL)
    {
        return FALSE;
    }
}
BOOL amf_parse_ecma_array   (FLVDemuxInf* dmx);
BOOL amf_parse_strict_array (FLVDemuxInf* dmx);


int amf_parse_elem_name (unsigned char** data, int* size, char* name, unsigned short* lens)
{
    *lens = get_ui16(*data, *size);
    (*data) += 2;
    (*size) -= 2;

    memcpy(name, *data, *lens);
    name[*lens] = '\0';

    (*data) += *lens;
    (*size) -= *lens;
    return 0;
}

/** 0x00 */
double amf_parse_double (unsigned char** data, int* size)
{
    double num = get_fl64(*data + 1, *size - 1);

    (*data) += 9;
    (*size) -= 9;
    return num;
}
/** 0x02 */
int amf_parse_string (unsigned char** data, int* size)
{
    unsigned short lens = get_ui16(*data + 1, *size - 1);
    (*data) += (1 + 2 + lens);
    (*size) -= (1 + 2 + lens);
    return 0;
}
/** 0x0C */
int amf_parse_long_string (unsigned char** data, int* size)
{
    unsigned long lens = get_ui32(*data + 1, *size - 1);
    (*data) += (1 + 4 + lens);
    (*size) -= (1 + 4 + lens);
    return 0;
}

/** 0x03 */
int amf_parse_object (FLVDemuxInfo* dmx, unsigned char** data, int* size)
{
    unsigned short namelens = 0;
    char  elemname[128];
    Metadata* meta = dmx->m_Metadata;

    (*data) += 1;
    (*size) -= 1;

    while (((*data)[0] != 0x00 || (*data)[1] != 0x00 || (*data)[2] != 0x09) && (*size) > 3)
    {
        unsigned char tagtype;

        amf_parse_elem_name(data, size, elemname, &namelens);
        tagtype = (*data)[0];

        switch(tagtype)
        {
        case 0x00:  ///< double
            {
                if (0 == strcmp(elemname, "audiocodecid"))
                {
                    meta->audiocodec = (int)amf_parse_double(data, size);
                    switch(meta->audiocodec)
                    {
                    case 2: ///< mp3
                    case 14:///< mp3
                        meta->audiocodec = CODEC_ID_MP3;
                        break;
                    case 10:///< aac
                        meta->audiocodec = CODEC_ID_AAC;
                        break;
                    default:
                        meta->audiocodec = CODEC_ID_NONE;
                    }
                }
                else if (0 == strcmp(elemname, "videocodecid"))
                {
                    meta->videocodec = (int)amf_parse_double(data, size);
                    switch(meta->videocodec)
                    {
                    case 2:///< H.263
                        meta->videocodec = CODEC_ID_H263;
                        break;
                    case 7:///< AVC
                        meta->videocodec = CODEC_ID_H264;
                        break;
                    default:
                        meta->videocodec = CODEC_ID_NONE;
                    }
                }
                else if (0 == strcmp(elemname, "duration"))
                {
                    double duration = amf_parse_double(data, size);
                    meta->duation = (int)(1000 * duration);
                    dmx->m_Duration = (int)(1000 * duration);
                }
                else if (0 == strcmp(elemname, "videodatarate"))
                {
                    meta->vbitrate = (int)amf_parse_double(data, size);
                }
                else if (0 == strcmp(elemname, "audiodatarate"))
                {
                    meta->abitrate = (int)amf_parse_double(data, size);
                }
                else
                {
                    (*data) += 9;
                    (*size) -= 9;
                }
                break;
            }
        case 0x01:  ///< boolean
            {
                (*data) += 2;
                (*size) -= 2;
                break;
            }
        case 0x02:  ///< string
            {
                amf_parse_string(data, size);
                break;
            }
        case 0x03:  ///< object
            {
                if (amf_parse_object(dmx, data, size))
                {
                    return -1;
                }
                break;
            }
        case 0x08:  ///< ECMA array
            {
                if (amf_parse_ecma_array(dmx, data, size))
                {
                    return -1;
                }
                break;
            }
        case 0x0A:  ///< strict array
            {
                if ((0 == strcmp(elemname, "times") || 0 == strcmp(elemname, "filepositions")))
                {
                    if(amf_parse_strict_array(dmx, data, size, elemname))
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
                break;
            }
        case 0x0B:  ///< data
            {
                (*data) += 11;
                (*size) -= 11;
                break;
            }
        case 0x0C:  ///< long string
            {
                amf_parse_long_string(data, size);
                break;
            }
        default:
            return -1;
        }
    }
    (*data) += 3;
    (*size) -= 3;
    return 0;
}
/** 0x08 */
int amf_parse_ecma_array (FLVDemuxInfo* dmx, unsigned char** data, int* size)
{
    unsigned short namelens = 0;
    char  elemname[128];
    Metadata* meta = dmx->m_Metadata;

    (*data) += 5;
    (*size) -= 5;

    while (((*data)[0] != 0x00 || (*data)[1] != 0x00 || (*data)[2] != 0x09) && (*size) > 3)
    {
        unsigned char tagtype;

        amf_parse_elem_name(data, size, elemname, &namelens);
        tagtype = (*data)[0];

        switch(tagtype)
        {
        case 0x00:  ///< double
            {
                if (0 == strcmp(elemname, "audiocodecid"))
                {
                    meta->audiocodec = (int)amf_parse_double(data, size);
                    switch(meta->audiocodec)
                    {
                    case 2: ///< mp3
                    case 14:///< mp3
                        meta->audiocodec = CODEC_ID_MP3;
                        break;
                    case 10:///< aac
                        meta->audiocodec = CODEC_ID_AAC;
                        break;
                    default:
                        meta->audiocodec = CODEC_ID_NONE;
                    }
                }
                else if (0 == strcmp(elemname, "videocodecid"))
                {
                    meta->videocodec = (int)amf_parse_double(data, size);
                    switch(meta->videocodec)
                    {
                    case 2:///< H.263
                        meta->videocodec = CODEC_ID_H263;
                        break;
                    case 7:///< AVC
                        meta->videocodec = CODEC_ID_H264;
                        break;
                    default:
                        meta->videocodec = CODEC_ID_NONE;
                    }
                }
                else if (0 == strcmp(elemname, "duration"))
                {
                    double duration = amf_parse_double(data, size);
                    meta->duation = (int)(1000 * duration);
                    dmx->m_Duration = (int)(1000 * duration);
                }
                else if (0 == strcmp(elemname, "videodatarate"))
                {
                    meta->vbitrate = (int)amf_parse_double(data, size);
                }
                else if (0 == strcmp(elemname, "audiodatarate"))
                {
                    meta->abitrate = (int)amf_parse_double(data, size);
                }
                else
                {
                    (*data) += 9;
                    (*size) -= 9;
                }
                break;
            }
        case 0x01:  ///< boolean
            {
                (*data) += 2;
                (*size) -= 2;
                break;
            }
        case 0x02:  ///< string
            {
                amf_parse_string(data, size);
                break;
            }
        case 0x03:  ///< object
            {
                if (amf_parse_object(dmx, data, size))
                {
                    return -1;
                }
                break;
            }
        case 0x08:  ///< ECMA array
            {
                if (amf_parse_ecma_array(dmx, data, size))
                {
                    return -1;
                }
                break;
            }
        case 0x0A:  ///< strict array
            {
                if ((0 == strcmp(elemname, "times") || 0 == strcmp(elemname, "filepositions")))
                {
                    if(amf_parse_strict_array(dmx, data, size, elemname))
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
                break;
            }
        case 0x0B:  ///< data
            {
                (*data) += 11;
                (*size) -= 11;
                break;
            }
        case 0x0C:  ///< long string
            {
                amf_parse_long_string(data, size);
                break;
            }
        default:
            return -1;
        }
    }
    (*data) += 3;
    (*size) -= 3;
    return 0;
}
/** 0x0A */
int amf_parse_strict_array (FLVDemuxInfo* dmx, unsigned char** data, int* size, const char* flag)
{
    unsigned long  elemcount = 0;
    FLVIndexList* list = &dmx->m_IndexList;
    unsigned long i = 0;

    (*data) += 1;
    (*size) -= 1;

    elemcount = get_ui32(*data, *size);
    (*data) += 4;
    (*size) -= 4;

    list->count = elemcount;
    if (list->elems == NULL)
    {
        list->elems = (FLVIndex*)malloc(elemcount * sizeof(FLVIndex));
    }

    while (i < elemcount)
    {
        if ((*data)[0] == 0x00)
        {
            if (0 == strcmp(flag, "times"))
            {
                list->elems[i].ts = (long long)amf_parse_double(data, size);
            }
            else if (0 == strcmp(flag, "filepositions"))
            {
                list->elems[i].pos = (long long)amf_parse_double(data, size);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
        ++i;
    }
    return 0;
}
