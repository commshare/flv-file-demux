#include "avformat.h"
#include <string.h>

int fileExtensionToFileFormatId(const char * ext)
{
    if(!ext)
    {
        return FILEFORMAT_ID_NONE;
    }

    if(strncasecmp(ext, ".flv", 4) == 0 || strncasecmp(ext, ".f4v", 4) == 0)
    {
        return FILEFORMAT_ID_FLV;
    }
    else if(strncasecmp(ext, ".mp4", 4) == 0 || strncasecmp(ext, ".mov", 4) == 0)
    {
        return FILEFORMAT_ID_MP4;
    }
    else if(strncasecmp(ext, ".ts", 3) ==0 || strncasecmp(ext, ".m2ts", 5) == 0)
    {
        return FILEFORMAT_ID_TS;
    }
    else if(strncasecmp(ext, ".mp3", 4) == 0)
    {
        return FILEFORMAT_ID_MP3;
    }
    else if(strncasecmp(ext, ".asf", 4) ==0 || strncasecmp(ext, ".wmv", 4) == 0 || strncasecmp(ext, ".wma", 4)==0)
    {
        return FILEFORMAT_ID_ASF;
    }
    else if(strncasecmp(ext, ".mkv", 4) == 0)
    {
        return FILEFORMAT_ID_MKV;
    }
    else if(strncasecmp(ext, ".avi", 4) == 0)
    {
        return FILEFORMAT_ID_AVI;
    }
    else if(strncasecmp(ext, ".mpg", 4) == 0 || strncasecmp(ext, ".mpeg", 5) == 0)
    {
        return FILEFORMAT_ID_MPG;
    }
    else if(strncasecmp(ext, ".rmvb", 5) == 0)
    {
        return FILEFORMAT_ID_RMVB;
    }
    else if(strncasecmp(ext, ".ogg", 4) == 0)
    {
        return FILEFORMAT_ID_OGG;
    }
    else if(strncasecmp(ext, ".3gp", 4) == 0)
    {
        return FILEFORMAT_ID_3GP;
    }
    else if(strncasecmp(ext, ".pmp", 4) == 0)
    {
        return FILEFORMAT_ID_PMP;
    }

    return FILEFORMAT_ID_NONE;
}
