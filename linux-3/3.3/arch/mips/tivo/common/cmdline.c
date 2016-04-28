// Copyright (C) 2012 TiVo Inc. All Rights Reserved.
// command line access routines

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/ctype.h>

// skip whitespace
static char * __init just(char *s)
{
    if (s) while (isspace(*s)) s++;
    return s;
}    

// trim string trailing whitespace, return pointer to \0
static char * __init trim(char *s)
{
    char *p=strchr(s,0);
    while (p > s && isspace(*(p-1))) p--;
    *p=0;
    return p;
}    

// return pointer to end of current param, ie point to first following ' ' or '\0'
static char * __init end(char *s)
{
    while (*s && !isspace(*s)) s++;
    return s;
}   


// return pointer to next param, if any
static char * __init next(char *s)
{
    return just(end(s));
}   

// yank first "len" characters from string
static void __init yank(char *s, int len)
{
    char *t=s;
    while (len-- && *t) t++; // find s[len] or end of s
    while ((*s++=*t++)!=0);  // move the rest left
}    

// check if s matches specified "param" or "param="
static int __init match(char *s, char*param)
{
    int n=strlen(param);
    if (!strncmp(s,param,n))                        
    {
        // possible match
        if (s[n-1] == '=') return 2;                // if "param=", return 2
        if (isspace(s[n]) || s[n] == 0) return 1;   // if "param " or "param\0", return 1
    }    
    return 0;
}    

// delete all param from cmdline
static void __init delete(char *cmdline, char *param)
{
    char *p=just(cmdline);
    while (*p)
    {
        char *n=next(p);
        if (match(p, param)) yank(p,n-p);
        else p=n;
    }
}    

// Look for first "param" on cmdline, return NULL if not found.
// Otherwise return pointer to *static* buffer containing the parameter value, or "" if no value
char * __init cmdline_lookup(char *cmdline, char *param)  
{
    static char value[128];
    int x, plen=strlen(param);
    char *p=just(cmdline);
    while (*p)
    {
        switch(match(p, param))
        {
            case 1: return ""; // "param" has no value, returns "" 

            case 2:            // "param=" returns value
                x=end(&p[plen])-&p[plen];
                if (x >= sizeof(value)) 
                {
                    printk(KERN_WARNING "Warning, '%s' value is truncated...\n", param);
                    x = sizeof(value)-1;
                }    
                strncpy(value, &p[plen], x);
                value[x]=0;
                return value;
        }    
        p=next(p);
    }   
    return NULL;
}    

// Remove ALL occurrences of all specified params from the command line.
// Note param list must be NULL terminated.
void __init cmdline_delete(char *cmdline, char *first, ...)
{
    va_list args;
    char *p;

    delete(cmdline, first);
    va_start(args, first);
    while ((p=va_arg(args, char *)) != NULL) delete(cmdline, p);
    va_end(args);
    trim(cmdline);
} 

// Append formatted string to command line, or just complain if not enough room
void __init cmdline_append(char *cmdline, int cmdline_size, char *fmt, ...)
{
    va_list args;
    char *p;
    int max;
    
    p=trim(cmdline);                    
    max = cmdline_size-(p-cmdline); 
    if (max <= 0) 
    {   
        printk(KERN_WARNING "Warning, won't append '%s'\n", fmt);
        return;
    }    
    if (p != cmdline) *p++=' ', max--;              // append a space if necessary
    va_start(args, fmt);
    if (vsnprintf(p, max, fmt, args) >= max)
    {
        printk(KERN_WARNING "Warning, can't append '%s'\n", fmt); 
        *p=0;
    }    
    va_end(args);
    trim(cmdline);
}

// This must be defined by ../<platform>/setup.c
extern char *platform_whitelist[];

// Called during early init, delete or report invalid parameters on command line
void __init cmdline_whitelist(char *cmdline)
{
    char **wl, *p=just(cmdline);
    
    if (!*platform_whitelist)
    {
        // do nothing if whitelist is empty
        printk(KERN_WARNING "Whitelisting is disabled!\n");
        return;
    } 

    while(*p)                                       // for each param
    {
        char *n=next(p);                            // point to next param
        for (wl=platform_whitelist; *wl; wl++)      // check each whitelist entry
        if (match(p, *wl))                          // if match, say so and advance to next
        {
            printk(KERN_WARNING "'%.*s' is whitelisted.\n", end(p)-p, p);
            goto skip;                              
        }   
        printk(KERN_WARNING "'%.*s' is not whitelisted.\n", end(p)-p, p);
#ifndef CONFIG_TIVO_DEVEL       
        // no match, delete param and try again
        yank(p,n-p);   
        continue;
#endif   
        
        // advance to next
        skip: p=n;                                  
    }
    trim(cmdline);
}    


