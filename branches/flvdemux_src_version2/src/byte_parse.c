#include <stddef.h>
#include "byte_parse.h"


BOOL is_little_endian ()
{
    UI16 data = 0x0001;
    if ((*((UI8*)(&data)))== 1)
    {
        return TRUE;
    }
    return FALSE;
}
BOOL get_Byte (UI8** buf, UI32 *size, UI8 * data)
{
    if ((*buf == NULL) || (*size < 1))
    {
        return FALSE;
    }

    if (data != NULL)
    {
        *data = **buf;
    }

    *buf  += 2;
    *size -= 2;
    return TRUE;
}
BOOL get_UI16 (UI8** buf, UI32* size, UI16* data)
{
    if ((*buf == NULL) || (*size < 2))
    {
        return FALSE;
    }

    if (data != NULL)
    {
        if (is_little_endian() == TRUE)
        {
            *data = **buf;
            *data = ((*data) << 8 ) | *(*buf + 1);
        }
        else
        {
            *data = *((UI16*)(*buf));
        }
    }

    *buf  += 2;
    *size -= 2;
    return TRUE;
}
BOOL get_UI24 (UI8** buf, UI32 *size, UI32* data)
{
    if ((*buf == NULL) || (*size < 3))
    {
        return FALSE;
    }

    if (data != NULL)
    {
        if (is_little_endian() == TRUE)
        {
            *data = **buf;
            *data = ((*data) << 8) | *(*buf + 1);
            *data = ((*data) << 8) | *(*buf + 2);
        }
        else
        {
            *data = *((UI32*)(*buf));
            *data = (*data) >> 8;
        }
    }

    *buf  += 3;
    *size -= 3;
    return TRUE;
}
BOOL get_UI32 (UI8** buf, UI32* size, UI32* data)
{
    if ((*buf == NULL) || (*size < 4))
    {
        return FALSE;
    }

    if (data != NULL)
    {
        if (is_little_endian() == TRUE)
        {
            *data = **buf;
            *data = ((*data) << 8) | *(*buf + 1);
            *data = ((*data) << 8) | *(*buf + 2);
            *data = ((*data) << 8) | *(*buf + 3);
        }
        else
        {
            *data = *((UI32*)(*buf));
        }
    }

    *buf  += 4;
    *size -= 4;
    return TRUE;
}
BOOL get_UI64 (UI8** buf, UI32 *size, UI64* data)
{
    if ((*buf == NULL) || (*size < 8))
    {
        return FALSE;
    }

    if (data != NULL)
    {
        if (is_little_endian() == TRUE)
        {
            *data = **buf;
            *data = ((*data) << 8) | *(*buf + 1);
            *data = ((*data) << 8) | *(*buf + 2);
            *data = ((*data) << 8) | *(*buf + 3);
            *data = ((*data) << 8) | *(*buf + 4);
            *data = ((*data) << 8) | *(*buf + 5);
            *data = ((*data) << 8) | *(*buf + 6);
            *data = ((*data) << 8) | *(*buf + 7);
        }
        else
        {
            *data = *((UI64*)(*buf));
        }
    }

    *buf  += 8;
    *size -= 8;
    return TRUE;
}
