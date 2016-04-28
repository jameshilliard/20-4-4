// Copyright (C) 2012 TiVo Inc. All Rights Reserved.

char * cmdline_lookup(char *cmdline, char *param); 
void cmdline_delete(char *cmdline, char *first, ...);
void cmdline_append(char *cmdline, int cmdline_size, char * fmt, ...);
void cmdline_whitelist(char *cmdline);


