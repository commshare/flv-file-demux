#include <memory.h>
#include <malloc.h>
#include <string.h>
#include "byte_parse.h"
#include "amf_parse.h"
#include "flv_demux.h"
#include "../mp_msg.h"

//NUMBER_MARKER       = 0x00,
//BOOLEAN_MARKER      = 0x01, ///< This program will skip this AMF data
//STRING_MARKER       = 0x02, ///< This program will skip this AMF data
//OBJECT_MARKER       = 0x03,
//MOVIECLIP_MARKER    = 0x04, ///< reserved, AMF_0 protocol does not supported
//NULL_MARKER         = 0x05, ///< This program will skip this AMF data
//UNDEFINED_MARKER    = 0x06, ///< This program will skip this AMF data
//REFERENCE_MARKER    = 0X07, ///< This program does not supported this AMF data type
//ECMA_ARRAY_MARKER   = 0x08,
//OBJECT_END_MARKER   = 0x09, ///< This program will skip this AMF data
//STRICT_ARRAY_MARKER = 0x0A,
//DATA_MARKER         = 0x0B, ///< This program will skip this AMF data
//LONG_STRING_MARKER  = 0x0C, ///< This program will skip this AMF data
//UNSUPPORTED_MARKER  = 0x0D, ///< This program does not supported this AMF data type
//RECORDSET_MARKER    = 0x0E, ///< reserved, AMF_0 protocol does not supported
//XML_DOCUMENT_MARKER = 0x0F, ///< This program does not supported this AMF data type
//TYPED_OBJECT_MARKER = 0x10  ///< This program does not supported this AMF data type


BOOL amf_parse_elem_name    (UI8** buf, UI32 *size, UI8** data, UI16* lens)
{
    UI16 _len = 0U; ///< current *data space length

    if (*lens == NULL)
    {
        return FALSE;
    }

    _len = lens;

    if (get_UI16(buf, size, lens) == FALSE)
    {
        return FALSE;
    }

    if (*size < *lens)
    {
        return FALSE;
    }
    if (_len < *lens)
    {
        if (*data != NULL)
        {
            free(*data);
            *data = NULL;
        }
        *data = (UI8*)malloc(*lens + 1);
        if (*data == NULL)
        {
            return FALSE;
        }
    }
    memcpy(*data, *buf, *lens);
    *data[*lens] = NULL;

    return TRUE;
}
BOOL amf_parse_skip_bytes   (UI8** buf, UI32 *size, UI8   type)
{
    switch (type)
    {
    case BOOLEAN_MARKER     :
    {
        if (*size < 1)
        {
            return FALSE;
        }
        *buf  += 1;
        *size += 1;
        return TRUE;
    }
    case STRING_MARKER      :
    {
        UI16 lens = 0U;
        if (get_UI16 (buf, size, &lens) == FALSE)
        {
            return FALSE;
        }
        if (*size < lens)
        {
            return FALSE;
        }
        *buf  += lens;
        *size += lens;
        return TRUE;
    }
    case REFERENCE_MARKER   :
    {
        if (get_UI16(buf, size, NULL) == FALSE)
        {
            return FALSE;
        }
        return TRUE;
    }
    case OBJECT_END_MARKER  :
    {
        if (*size < 3)
        {
            return FALSE;
        }
        *buf  += 3;
        *size -= 3;
    }
    case DATA_MARKER        :
    {
        if ((get_UI16 (buf, size, NULL) == FALSE)\
            || (get_UI64(buf, size, NULL) == FALSE))
        {
            return FALSE;
        }
        return TRUE;
    }
    case LONG_STRING_MARKER :
    {
        UI32 lens = 0U;
        if (get_UI32 (buf, size, &lens) == FALSE)
        {
            return FALSE;
        }
        if (*size < lens)
        {
            return FALSE;
        }
        *buf  += lens;
        *size += lens;
        return TRUE;
    }
    default:
        return FALSE;
    }
}
BOOL amf_parse_number       (UI8** buf, UI32 *size, UI64* data)
{
    double float_data;
    if (get_UI64(buf, size, (UI64*)&float_data) == FALSE)
    {
        return FALSE;
    }
    if (data != NULL)
    {
        *data = (UI64)float_data;
    }
    return TRUE;
}
BOOL amf_parse_object       (UI8** buf, UI32 *size, Metadata* mdata, TimestampInd* index)
{
    UI16 elemlens   = 0UL;  ///< object element name length
    UI8* elemname   = NULL; ///< object element name string

    memset (mdata, 0, sizeof(Metadata));

    while ((*size >= 3) && (*buf[0] != 0x00 || *buf[1] != 0x00 || *buf[2] != 0x09))
    {
        UI8 amf_tag_type;

        if (amf_parse_elem_name(buf, size, &elemname, &elemlens))
        {
            return FALSE;
        }
        if (get_Byte (buf, size, &amf_tag_type))
        {
            return FALSE;
        }

        switch((AMFType)amf_tag_type)
        {
        case BOOLEAN_MARKER     :   ///< BOOLEAN_MARKER
        {
            if (*size < 1)
            {
                return FALSE;
            }
            *buf  += 1;
            *size += 1;
            return TRUE;
        }
        case STRING_MARKER      :   ///< STRING_MARKER
        {
            UI16 lens = 0U;
            if (get_UI16 (buf, size, &lens) == FALSE)
            {
                return FALSE;
            }
            if (*size < lens)
            {
                return FALSE;
            }
            *buf  += lens;
            *size += lens;
            return TRUE;
        }
        case REFERENCE_MARKER   :   ///< REFERENCE_MARKER
        {
            if (get_UI16(buf, size, NULL) == FALSE)
            {
                return FALSE;
            }
            return TRUE;
        }
        case DATA_MARKER        :   ///< DATA_MARKER
        {
            if ((get_UI16 (buf, size, NULL) == FALSE)\
                || (get_UI64(buf, size, NULL) == FALSE))
            {
                return FALSE;
            }
            return TRUE;
        }
        case LONG_STRING_MARKER :   ///< LONG_STRING_MARKER
        {
            UI32 lens = 0U;
            if (get_UI32 (buf, size, &lens) == FALSE)
            {
                return FALSE;
            }
            if (*size < lens)
            {
                return FALSE;
            }
            *buf  += lens;
            *size += lens;
            return TRUE;
        }
        case OBJECT_MARKER      :   ///< OBJECT_MARKER
        {
            if (amf_parse_object(buf, size, mdata) == FALSE)
            {
                return FALSE;
            }
        }
        case ECMA_ARRAY_MARKER  :   ///< ECMA_ARRAY_MARKER
        {
            if (amf_parse_ecma_array(buf, size, mdata) == FALSE)
            {
                return FALSE;
            }
        }
        case STRICT_ARRAY_MARKER:   ///<
        {
            if (amf_parse_strict_array(buf, size, index) == FALSE)
            {
                return FALSE;
            }
        }
        default:
            return FALSE;
        }
    }

    if ((*size >= 3) && (*buf[0] != 0x00 || *buf[1] != 0x00 || *buf[2] != 0x09))
    {
        *buf  += 3;
        *size -= 3;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
BOOL amf_parse_ecma_array   (FLVDemuxer* dmx);
BOOL amf_parse_strict_array (FLVDemuxer* dmx);


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
double amf_parse_double (unsigned char** data, int* size)
{
    double num = get_fl64(*data + 1, *size - 1);

    (*data) += 9;
    (*size) -= 9;
    return num;
}
int amf_parse_string (unsigned char** data, int* size)
{
    unsigned short lens = get_ui16(*data + 1, *size - 1);
    (*data) += (1 + 2 + lens);
    (*size) -= (1 + 2 + lens);
    return 0;
}
int amf_parse_long_string (unsigned char** data, int* size)
{
    unsigned long lens = get_ui32(*data + 1, *size - 1);
    (*data) += (1 + 4 + lens);
    (*size) -= (1 + 4 + lens);
    return 0;
}
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
