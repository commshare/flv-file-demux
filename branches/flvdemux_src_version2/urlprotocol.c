#include "urlprotocol.h"
#include <string.h>
#include <malloc.h>

static URLProtocol * g_protocolList = 0;
///
void registerURLProtocol(URLProtocol *prot)
{
    if(!g_protocolList)
    {
        g_protocolList = prot;
        g_protocolList->next = 0;
    }
    else
    {
        prot->next = g_protocolList;
        g_protocolList = prot;
    }
}

URLProtocol * getURLProtocolByUrl(const char * url)
{
    URLProtocol * prot = g_protocolList;
    while(prot)
    {
        if( 0 == prot->url_can_handle(prot, url) )
        {
            return prot;
        }

        prot = prot->next;
    }
    return NULL;
}


char * getExtensionFromUrl(const char *url)
{
    //network protocol
    char * p = strstr(url, "://");
    if(p)
    {
        p = strchr(p+3, '?');
        if(p)
        {
            char *p1 = p--;
            while(*p != '.' && *p != '/') p--;
            if(*p == '.')
            {
                char * ext = (char*)malloc(p1 - p + 1); // one byte more '\0'
                strncpy(ext, p, p1 - p);
                return ext;
            }
            return 0;
        }
        else
        {
            p = strrchr(url,'.');
            if(p)
            {
                if(strchr(p+1, '/'))
                {
                    return 0;
                }
                return _strdup(p);
            }
            return 0;
        }
    }
    else
    {
        //file
        p = strrchr(url, '.');
        if(p)
        {
            return _strdup(p);
        }
    }

    return 0;
}
