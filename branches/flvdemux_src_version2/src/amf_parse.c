#include <memory.h>
#include <malloc.h>
#include <string.h>
#include "byte_parse.h"
#include "amf_parse.h"
#include "flv_demux.h"
#include "../mp_msg.h"

BOOL amf_parse_elem_name    (UI8** buf, UI32* size, UI8** data, UI16* lens)
{
    UI16 _len = 0U; ///< current *data space length

    if (lens == NULL)
    {
        return FALSE;
    }

    _len = *lens;

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
    (*data)[*lens] = (UI8)'\0';

    return TRUE;
}
BOOL amf_parse_number       (UI8** buf, UI32* size, UI64* data)
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
BOOL amf_parse_object       (UI8** buf, UI32* size, TimestampInd* index, Metadata* mdata)
{
    UI16 elemlens   = 0UL;  ///< object element name length
    UI8* elemname   = NULL; ///< object element name string
    UI8  amf_data_type;      ///< AMF tag type

    while ((*size >= 3) && (*buf[0] != 0x00 || *buf[1] != 0x00 || *buf[2] != 0x09))
    {
        if (amf_parse_elem_name(buf, size, &elemname, &elemlens))
        {
            return FALSE;
        }
        if (get_Byte (buf, size, &amf_data_type))
        {
            return FALSE;
        }

        switch((AMFType)amf_data_type)
        {
        case NUMBER_MARKER      :   ///< 0x00 NUMBER_MARKER, Parse it
        {
            UI64 val = 0ULL;
            if (amf_parse_number (buf, size, &val) == FALSE)
            {
                return FALSE;
            }
            if (mdata == NULL)
            {
                break;
            }
            if (0 == strcmp((char*)elemname, "audiocodecid"))
            {
                switch(val)
                {
                case 2: ///< mp3
                case 14:///< mp3
                    mdata->audiocodec = CODEC_ID_MP3;
                    break;
                case 10:///< aac
                    mdata->audiocodec = CODEC_ID_AAC;
                    break;
                default:
                    mdata->audiocodec = CODEC_ID_NONE;
                    break;
                }
            }
            else if (0 == strcmp((char*)elemname, "videocodecid"))
            {
                switch(val)
                {
                case 2:///< H.263
                    mdata->videocodec = CODEC_ID_H263;
                    break;
                case 7:///< AVC
                    mdata->videocodec = CODEC_ID_H264;
                    break;
                default:
                    mdata->videocodec = CODEC_ID_NONE;
                    break;
                }
            }
            else if (0 == strcmp((char*)elemname, "duration"))
            {
                mdata->duation  = (int)val;
            }
            else if (0 == strcmp((char*)elemname, "videodatarate"))
            {
                mdata->vbitrate = (int)val;
            }
            else if (0 == strcmp((char*)elemname, "audiodatarate"))
            {
                mdata->abitrate = (int)val;
            }
            break;
        }
        case BOOLEAN_MARKER     :   ///< 0x01 BOOLEAN_MARKER, Skip it
        {
            if (*size < 1)
            {
                return FALSE;
            }
            *buf  += 1;
            *size -= 1;
            break;
        }
        case STRING_MARKER      :   ///< 0x02 STRING_MARKER, Skip it
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
            break;
        }
        case OBJECT_MARKER      :   ///< 0x03 OBJECT_MARKER, Parse it
        {
            if (amf_parse_object(buf, size, index, mdata) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case NULL_MARKER        :   ///< 0x05 NULL_MARKER, No more followed data
        case UNDEFINED_MARKER   :   ///< 0x06 UNDEFINED_MARKER, No more followed data
        {
            break;
        }
        case REFERENCE_MARKER   :   ///< 0x07 REFERENCE_MARKER, Skip it
        {
            if (get_UI16(buf, size, NULL) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case ECMA_ARRAY_MARKER  :   ///< 0x08 ECMA_ARRAY_MARKER, Parse it
        {
            if (amf_parse_ecma_array(buf, size, index, mdata) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case STRICT_ARRAY_MARKER:   ///< 0x0A STRICT_ARRAY_MARKER, Parse it
        {
            UI8* flag = NULL;
            if (0 == strcmp((char*)elemname, "filepositions")\
                || 0 == strcmp((char*)elemname, "times"))
            {
                flag = elemname;
            }
            if (amf_parse_strict_array (buf, size, index, flag) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case DATA_MARKER        :   ///< 0x0B DATA_MARKER, Skip it
        {
            if (*size < 10)
            {
                return FALSE;
            }
            if ((get_UI16 (buf, size, NULL) == FALSE)\
                || (get_UI64(buf, size, NULL) == FALSE))
            {
                return FALSE;
            }
            break;
        }
        case LONG_STRING_MARKER :   ///< 0x0C LONG_STRING_MARKER, Skip it
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
            break;
        }
        default:
            return FALSE;
        }
    }

    if (elemname != NULL)
    {
        free (elemname);
        elemname = NULL;
        elemlens = 0U;
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
BOOL amf_parse_ecma_array   (UI8** buf, UI32* size, TimestampInd* index, Metadata* mdata)
{
    UI16 elemlens   = 0UL;  ///< object element name length
    UI8* elemname   = NULL; ///< object element name string
    UI8  amf_data_type;      ///< AMF tag type
    UI32 elemcount  = 0UL;  ///< Element count

    if (get_UI32(buf, size, &elemcount) == FALSE)
    {
        return FALSE;
    }

    while ((*size >= 3) && (*buf[0] != 0x00 || *buf[1] != 0x00 || *buf[2] != 0x09))
    {
        if (amf_parse_elem_name(buf, size, &elemname, &elemlens))
        {
            return FALSE;
        }
        if (get_Byte (buf, size, &amf_data_type))
        {
            return FALSE;
        }

        switch((AMFType)amf_data_type)
        {
        case NUMBER_MARKER      :   ///< 0x00 NUMBER_MARKER, Parse it
        {
            UI64 val = 0ULL;
            if (amf_parse_number (buf, size, &val) == FALSE)
            {
                return FALSE;
            }
            if (mdata == NULL)
            {
                break;
            }
            if (0 == strcmp((char*)elemname, "audiocodecid"))
            {
                switch(val)
                {
                case 2: ///< mp3
                case 14:///< mp3
                    mdata->audiocodec = CODEC_ID_MP3;
                    break;
                case 10:///< aac
                    mdata->audiocodec = CODEC_ID_AAC;
                    break;
                default:
                    mdata->audiocodec = CODEC_ID_NONE;
                    break;
                }
            }
            else if (0 == strcmp((char*)elemname, "videocodecid"))
            {
                switch(val)
                {
                case 2:///< H.263
                    mdata->videocodec = CODEC_ID_H263;
                    break;
                case 7:///< AVC
                    mdata->videocodec = CODEC_ID_H264;
                    break;
                default:
                    mdata->videocodec = CODEC_ID_NONE;
                    break;
                }
            }
            else if (0 == strcmp((char*)elemname, "duration"))
            {
                mdata->duation  = (int)val;
            }
            else if (0 == strcmp((char*)elemname, "videodatarate"))
            {
                mdata->vbitrate = (int)val;
            }
            else if (0 == strcmp((char*)elemname, "audiodatarate"))
            {
                mdata->abitrate = (int)val;
            }
            break;
        }
        case BOOLEAN_MARKER     :   ///< 0x01 BOOLEAN_MARKER, Skip it
        {
            if (*size < 1)
            {
                return FALSE;
            }
            *buf  += 1;
            *size -= 1;
            break;
        }
        case STRING_MARKER      :   ///< 0x02 STRING_MARKER, Skip it
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
            break;
        }
        case OBJECT_MARKER      :   ///< 0x03 OBJECT_MARKER, Parse it
        {
            if (amf_parse_object(buf, size, index, mdata) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case NULL_MARKER        :   ///< 0x05 NULL_MARKER, No more followed data
        case UNDEFINED_MARKER   :   ///< 0x06 UNDEFINED_MARKER, No more followed data
        {
            break;
        }
        case REFERENCE_MARKER   :   ///< 0x07 REFERENCE_MARKER, Skip it
        {
            if (get_UI16(buf, size, NULL) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case ECMA_ARRAY_MARKER  :   ///< 0x08 ECMA_ARRAY_MARKER, Parse it
        {
            if (amf_parse_ecma_array(buf, size, index, mdata) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case STRICT_ARRAY_MARKER:   ///< 0x0A STRICT_ARRAY_MARKER, Parse it
        {
            UI8* flag = NULL;
            if (0 == strcmp((char*)elemname, "filepositions")\
                || 0 == strcmp((char*)elemname, "times"))
            {
                flag = elemname;
            }
            if (amf_parse_strict_array (buf, size, index, flag) == FALSE)
            {
                return FALSE;
            }
            break;
        }
        case DATA_MARKER        :   ///< 0x0B DATA_MARKER, Skip it
        {
            if (*size < 10)
            {
                return FALSE;
            }
            if ((get_UI16 (buf, size, NULL) == FALSE)\
                || (get_UI64(buf, size, NULL) == FALSE))
            {
                return FALSE;
            }
            break;
        }
        case LONG_STRING_MARKER :   ///< 0x0C LONG_STRING_MARKER, Skip it
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
            break;
        }
        default:
            return FALSE;
        }
    }

    if (elemname != NULL)
    {
        free (elemname);
        elemname = NULL;
        elemlens = 0U;
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
BOOL amf_parse_strict_array (UI8** buf, UI32* size, TimestampInd* index, UI8* flag)
{
    UI32 elemcount;
    UI8  amf_data_type;
    if (index == NULL || get_UI32(buf, size, &elemcount) == FALSE)
    {
        return FALSE;
    }

    if (elemcount == 0UL)
    {
        return TRUE;
    }

    if (flag == NULL)
    {
        UI32 i = 0;
        while (i < elemcount)
        {
            if (get_Byte(buf, size, &amf_data_type) == FALSE)
            {
                return FALSE;
            }
            if (amf_data_type != (UI8)NUMBER_MARKER)
            {
                return FALSE;
            }
            if (get_UI64 (buf, size, NULL) == FALSE)
            {
                return FALSE;
            }
            ++i;
        }
    }
    else if (0 == strcmp((char*)flag, "filepositions"))
    {
        UI32 i = 0;
        if (index->m_Index != NULL && index->m_Count != elemcount)
        {
            return FALSE;
        }
        else if (index->m_Index == NULL)
        {
            index->m_Count = elemcount;
            index->m_Index = (FLVTSInfo*)malloc(sizeof(FLVTSInfo) * elemcount);
            if (index->m_Index == NULL)
            {
                return FALSE;
            }
        }

        while (i > 0)
        {
            if (get_Byte(buf, size, &amf_data_type) == FALSE)
            {
                return FALSE;
            }
            if (amf_data_type != (UI8)NUMBER_MARKER)
            {
                return FALSE;
            }
            if (amf_parse_number(buf, size, &(index->m_Index[i].m_TimePos)) == FALSE)
            {
                return FALSE;
            }
            ++i;
        }
    }
    else if (0 == strcmp((char*)flag, "times"))
    {
        UI32 i = 0;
        if (index->m_Index != NULL && index->m_Count != elemcount)
        {
            return FALSE;
        }
        else if (index->m_Index == NULL)
        {
            index->m_Count = elemcount;
            index->m_Index = (FLVTSInfo*)malloc(sizeof(FLVTSInfo) * elemcount);
            if (index->m_Index == NULL)
            {
                return FALSE;
            }
        }

        while (i > 0)
        {
            if (get_Byte(buf, size, &amf_data_type) == FALSE)
            {
                return FALSE;
            }
            if (amf_data_type != (UI8)NUMBER_MARKER)
            {
                return FALSE;
            }
            if (amf_parse_number(buf, size, &(index->m_Index[i].m_TimePos)) == FALSE)
            {
                return FALSE;
            }
            ++i;
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}
