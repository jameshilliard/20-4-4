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

#include <cdaudio.h>
#include <config.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* For Linux */
#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#endif

/* This contains the CDROM_SELECT_DISC ioctl in earlier kernels */
#ifdef HAVE_LINUX_UCDROM_H
#include <linux/ucdrom.h>
#endif

/* Choose a particular disc from the CD changer */
int
cd_changer_select_disc(int cd_desc, int disc)
{
#ifdef CDROM_SELECT_DISC
  if(ioctl(cd_desc, CDROM_SELECT_DISC, disc) < 0)
    return -1;
   
  return 0;
#else
  errno = ENOSYS;
   
  return -1;
#endif
}

/* Identify how many CD-ROMs the changer can hold */
int
cd_changer_slots(int cd_desc)
{
#ifdef CDROM_CHANGER_NSLOTS
  int slots;
	
  if((slots = ioctl(cd_desc, CDROM_CHANGER_NSLOTS)) < 0)
    slots = 1;
   
  if(slots == 0)
    return 1;
   
  return slots;
#else
  errno = ENOSYS;
	
  return 1;
#endif
}

/* This gives a brief summary of what the CD-ROM changer
   contains... this can take awhile to execute, especially on larger
   CD-ROM changers, so it shouldn't be called excessively */
int
cd_changer_stat(int cd_desc, struct disc_changer *changer)
{
  int discindex;
  struct disc_info disc;
  struct disc_data data;

  if((changer->changer_slots = cd_changer_slots(cd_desc)) < 0)
    return -1;
   
  for(discindex = 0; discindex < changer->changer_slots; discindex++) {
    if(cd_changer_select_disc(cd_desc, discindex) < 0)
      return -1;
      
    if(cd_stat(cd_desc, &disc) < 0)
      return -1;
      
    if(cddb_read_disc_data(cd_desc, &data) < 0)
      return -1;
      
#define CD changer->changer_disc[discindex]
#define DL CD.disc_length
    CD.disc_present = disc.disc_present;
    DL.minutes = disc.disc_length.minutes;
    DL.seconds = disc.disc_length.seconds;
    DL.frames = disc.disc_length.frames;
    CD.disc_total_tracks = disc.disc_total_tracks;
    CD.disc_id = data.data_id;
    strncpy(CD.data_cdindex_id, data.data_cdindex_id, CDINDEX_ID_SIZE);
      
    if(strlen(data.data_artist) > 0) {
      if(data.data_artist[strlen(data.data_artist) - 1] == ' ')
	snprintf(CD.disc_info, 128, "%s/ %s",
		 data.data_artist, data.data_title);
      else
	snprintf(CD.disc_info, 128, "%s / %s",
		 data.data_artist, data.data_title);
    } else
      strncpy(CD.disc_info, data.data_title, 128);
  }
#undef CD
#undef DL
   
  return 0;
}
