/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri <bascule@inferno.tusculum.edu>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

/* XXX This file is NOT strtok() clean */

#include <cdaudio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int cdindex_read_line(int sock, char *inbuffer, int len);

/* Connect to a cover art server */
int
coverart_connect_server(struct cddb_host host,
			struct cddb_server *proxy,
			char *http_string, int len)
{
  int sock;

  if(proxy != NULL) {
    if((sock = cddb_connect(proxy)) < 0)
      return -1;
  } else {
    if((sock = cddb_connect(&host.host_server)) < 0)
      return -1;
  }
    
  snprintf(http_string, len, "GET http://%s:%d/%s",
	   host.host_server.server_name,
	   host.host_server.server_port,
	   host.host_addressing);
   
  return sock;
}

/* Internal function to make strings HTTP/1.0 compliant */
static void
coverart_httpize(char *out, char *in, int length)
{
  int index, outdex = 0;
  char hexval[3];

  for(index = 0; index < length; index++) {
    if(outdex >= length - 1) {
      out[outdex] = '\0';
      return;
    }
    if(isalpha(in[index]) || isdigit(in[index]) ||
       in[index] == '.' || in[index] == '-' || in[index] == '_')
      out[outdex++] = in[index];
    else if(in[index] == ' ')
      out[outdex++] = '+';
    else if(in[index] == '\n') {
      out[outdex++] = '\0';
      return;
    } else if(in[index] =='\0') {
      out[outdex++] = '\0';
      return;
    } else {
      if(outdex >= length - 3) {
	out[outdex] = '\0';
	return;
      }

      snprintf(hexval, 3, "%02x", in[index]);
      out[outdex++] = '%';
      out[outdex++] = hexval[0];
      out[outdex++] = hexval[1];
    }
  }

  out[outdex] = '\0';
  return;
}

/* Internal function to process a line of cover art data */
static void
coverart_process_line(char *line, struct art_query *query)
{
  int index = 0, outdex = 0;
  char procbuffer[128];
   
  if(strchr(line, ':') == NULL)
    return;
   
  while(line[index++] != ':');
  line[index - 1] = '\0';
  index++;
   
  while(line[index] != '\0')
    procbuffer[outdex++] = line[index++];
  procbuffer[outdex] = '\0';
     
  if(strcmp(line, "NumMatches") == 0) {
    query->query_matches = strtol(procbuffer, NULL, 10);
    if(query->query_matches < 1) {
      query->query_match = QUERY_NOMATCH;
      query->query_matches = 0;
      return;
    } else if(query->query_matches == 1) {
      query->query_match = QUERY_EXACT;
      return;
    } else {
      query->query_match = QUERY_INEXACT;
      return;
    }
  } else if(strncmp(line, "Album", 5) == 0) {
    long n = strtol((char *)line + 5, NULL, 10);
    if(parse_disc_artist && strchr(procbuffer, '/') != NULL) {
      strtok(procbuffer, "/");
      strncpy(query->query_list[n].list_artist, procbuffer,
	      (strlen(procbuffer) < 64) ? (strlen(procbuffer) - 1) : 64); 
      strncpy(query->query_list[n].list_album,
	      (char *)strtok(NULL, "/") + 1, 64);
    } else {
      strncpy(query->query_list[n].list_album, procbuffer, 64);
      query->query_list[n].list_artist[0] = '\0';
    }
  } else if(strncmp(line, "Url", 3) == 0) {
    long n = strtol((char *)line + 3, NULL, 10);
    cddb_process_url(&query->query_list[n].list_host, procbuffer);
  }

  return;
}

/* Internal function to read the results of a cover art request */
static int
coverart_read_results(int sock, struct art_query *query)
{
  char inbuffer[512];
   
  cdindex_read_line(sock, inbuffer, 512);
  if(strcmp("NumMatches: 0", inbuffer) == 0 ||
     strncmp("NumMatches: ", inbuffer, 12) != 0) {
    query->query_match = QUERY_NOMATCH;
    query->query_matches = 0;
    return 0;
  }
   
  do
    coverart_process_line(inbuffer, query);
  while(cdindex_read_line(sock, inbuffer, 512) > -1);
   
  return 0;
}

/* Cover art query which uses a CD Index ID */
int
coverart_query(int cd_desc, int sock,
	       struct art_query *query,
	       char *http_string)
{
  char cdindex_id[CDINDEX_ID_SIZE], outbuffer[512];
   
  cdindex_discid(cd_desc, cdindex_id, CDINDEX_ID_SIZE);
  snprintf(outbuffer, 512, "%s?id=%s\n", http_string, cdindex_id);
  write(sock, outbuffer, strlen(outbuffer));
     
  return coverart_read_results(sock, query);
}

/* Cover art query using a CDDB ID */
int
coverart_name_query(int sock, struct art_query *query,
		    char *http_string,
		    char *album,
		    char *artist)
{
  char artistbuffer[64], albumbuffer[64], outbuffer[512];
   
  if(artist != NULL)
    coverart_httpize(artistbuffer, artist, 64);
  coverart_httpize(albumbuffer, album, 64);
   
  if(artist != NULL)
    snprintf(outbuffer, 512, "%s?artist=%s&album=%s\n",
	     http_string, artistbuffer, albumbuffer);
  else
    snprintf(outbuffer, 512, "%s?album=%s\n", http_string, albumbuffer);
  write(sock, outbuffer, strlen(outbuffer));
   
  return coverart_read_results(sock, query);
}

/* Read cover art from a specified host into an art_data structure */
int
coverart_read(struct art_data *art,
	      struct cddb_server *proxy,
	      struct cddb_host host)
{
  int sock, bytes_read;
  char outbuffer[512], inbuffer[512], *artptr;
   
  art->art_present = 0;
   
  if(proxy == NULL) {
    if((sock = cddb_connect(&host.host_server)) < 0)
      return -1;
      
    snprintf(outbuffer, 512, "GET /%s HTTP/1.0\n\n",
	     host.host_addressing);
  } else {
    if((sock = cddb_connect(proxy)) < 0)
      return -1;
      
    snprintf(outbuffer, 512,
	     "GET http://%s:%d/%s HTTP/1.0\n\n",
	     host.host_server.server_name,
	     host.host_server.server_port,
	     host.host_addressing);
  }
   
  write(sock, outbuffer, strlen(outbuffer));
   
  while(cdindex_read_line(sock, inbuffer, 512) >= 0) {
    if(strlen(inbuffer) <= 1) {
      break;
    }
    if(strchr(inbuffer, ' ') != NULL) {
      strtok(inbuffer, " ");
      if(strcmp(inbuffer, "Content-Type:") == 0) {
	strncpy(art->art_mime_type, strtok(NULL, " "), 16);
	if(art->art_mime_type[strlen(art->art_mime_type) - 1] == 0xD)
	  art->art_mime_type[strlen(art->art_mime_type) - 1] = '\0';
      }
    }
  }
   
  artptr = art->art_image;
  art->art_length = 0;
   
  while((bytes_read = read(sock, inbuffer, 512)) > 0) {
    if(art->art_length < DISC_ART_SIZE - bytes_read) {
      memcpy(artptr, inbuffer, bytes_read);
      art->art_length += bytes_read;
      artptr += bytes_read;
    } else {
      return -1;
    }
  }
   
  if(art->art_length > 0)
    art->art_present = 1;
  else
    return -1;
   
  return 0;
}

/* Read cover art from local cache */
int
coverart_read_data(int cd_desc, struct art_data *art)
{
  DIR *coverart_dir;
  int index, coverart_fd;
  char root_dir[256], procbuffer[256], id[CDINDEX_ID_SIZE];
  struct stat st;
  struct dirent *d;
   
  if(getenv("HOME") == NULL) {
    strncpy(cddb_message, "$HOME is not set!", 256);
    return -1;
  }
   
  snprintf(root_dir, 256, "%s/.coverart", getenv("HOME"));
  cdindex_discid(cd_desc, id, CDINDEX_ID_SIZE);
   
  if(stat(root_dir, &st) < 0) {
    if(errno != ENOENT)
      return -1;
    else {
      art->art_present = 0;
      return 0;
    }
  } else if(!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    return -1;
  }
   
  if((coverart_dir = opendir(root_dir)) == NULL)
    return -1;
   
  while((d = readdir(coverart_dir)) != NULL) {
    if(strncmp(id, d->d_name, strlen(id) - 1) == 0) {
      strncpy(procbuffer, d->d_name, 256);
      index = strlen(d->d_name);
      if(strchr(d->d_name, '.') != NULL) {
	while(d->d_name[index--] != '.');
	index += 2;
	snprintf(art->art_mime_type, 16, "image/%s", d->d_name + index);
	art->art_mime_type[6 + (strlen(d->d_name) - index)] = '\0';
      } else
	art->art_mime_type[0] = '\0';
	 
      snprintf(procbuffer, 256, "%s/%s", root_dir, d->d_name);
      if(stat(procbuffer, &st) < 0)
	return -1;
	 
      art->art_length = st.st_size;
      if((coverart_fd = open(procbuffer, O_RDONLY)) < 0)
	return -1;
	 
      if(read(coverart_fd, art->art_image, art->art_length) < 0)
	return -1;
	 
      art->art_present = 1;
	 
      return 0;
    }
  }
   
  art->art_present = 0;
   
  return 0;
}

/* Write cover art to local cache */
int
coverart_write_data(int cd_desc, struct art_data art)
{
  int coverart_fd;
  char root_dir[256], file[256], id[CDINDEX_ID_SIZE],
    extension[16], procbuffer[16];
  struct stat st;
   
  if(!art.art_present)
    return 0;
   
  if(getenv("HOME") == NULL) {
    strncpy(cddb_message, "$HOME is not set!", 256);
    return -1;
  }
   
  memset(extension, '\0', 16);
  memset(file, '\0', 256);
   
  cdindex_discid(cd_desc, id, CDINDEX_ID_SIZE);
  strncpy(procbuffer, art.art_mime_type, 16);
  if(strchr(procbuffer, '/') != NULL) {
    strtok(procbuffer, "/");
    strncpy(extension, strtok(NULL, "/"), 16);
  } else
    strncpy(extension, procbuffer, 16);
   
  snprintf(root_dir, 256, "%s/.coverart", getenv("HOME"));
  snprintf(file, 256, "%s/%s.%s", root_dir, id, extension);
   
  if(stat(root_dir, &st) < 0) {
    if(errno != ENOENT)
      return -1;
    else
      mkdir(root_dir, 0755);
  } else if(!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    return -1;
  }
   
  if((coverart_fd = creat(file, 0644)) < 0)
    return -1;
   
  if(write(coverart_fd, art.art_image, art.art_length) < 0)
    return -1;
   
  return 0;
}

/* Erase a cover art entry directly */
int
coverart_direct_erase_data(char *cdindex_id, struct art_data *art)
{
  char *filename, *mime_type;
   
  if(getenv("HOME") == NULL) {
    strncpy(cddb_message, "$HOME is not set!", 256);
    return -1;
  }
   
  if((filename = malloc(108)) == NULL)
    return -1;
   
  if(strchr(art->art_mime_type, '/') == NULL)
    return -1;
   
  snprintf(filename, 108, "%s/.coverart/%s.%s",
	   getenv("HOME"), cdindex_id,
	   strchr(art->art_mime_type, '/') + 1);
  if(unlink(filename) < 0) {
    free(filename);
    return -1;
  }
	   
  free(filename);
  return 0;
}

int
coverart_erase_data(int cd_desc)
{
  struct art_data art;
  char cdindex_id[CDINDEX_ID_SIZE];
   
  if(cdindex_discid(cd_desc, cdindex_id, CDINDEX_ID_SIZE) < 0)
    return -1;
   
  if(coverart_read_data(cd_desc, &art) < 0)
    return -1;
   
  if(!art.art_present)
    return 0;
   
  if(coverart_direct_erase_data(cdindex_id, &art) < 0)
    return -1;
   
  return 0;
}
