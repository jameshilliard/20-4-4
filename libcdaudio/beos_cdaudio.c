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

#include <config.h>

/* Oh yeah... it's done, but it could probably be integrated into
   cdaudio.c.  */

#if defined(BEOS_CDAUDIO)

#include <be/device/scsi.h>
#include <string.h>

#include <cdaudio.h>
#include <compat.h>

/* Return the version of libcdaudio.  */
void
cd_version (char *buffer, int len)
{
  snprintf (buffer, len, "%s %s", PACKAGE, VERSION);
}

/* Return the version of libcdaudio.  */
long
cdaudio_getversion (void)
{
  return LIBCDAUDIO_VERSION;
}

/* Initialize the CD-ROM for playing audio CDs.  */
int
cd_init_device (char *device_name)
{
  int cd_desc;

#ifdef NON_BLOCKING
  if ((cd_desc = open (device_name, O_RDONLY | O_NONBLOCK)) < 0)
#else
  if ((cd_desc = open (device_name, O_RDONLY)) < 0)
#endif
    return -1;
	
  return cd_desc;
}

/* Close a device handle and free its resources.  */
int
cd_finish (int cd_desc)
{
  close (cd_desc);
  return 0;
}

/* Update a CD status structure... because operating system interfaces vary
   so does this function.  */
int
cd_stat (int cd_desc, struct disc_info *disc)
{
  /* Since every platform does this a little bit differently this
     gets pretty complicated...  */

  scsi_toc toc;
  struct disc_status status;
  int readtracks, pos;
  unsigned char* byte;
   
  if (cd_poll (cd_desc, &status) < 0)
    return -1;
 
  if (!status.status_present) {
    disc->disc_present = 0;
    return 0;
  }

  disc->disc_present = 1;
   
  /* Read the Table Of Contents */
  if(ioctl(cd_desc, B_SCSI_GET_TOC, &toc) < 0)
    return -1;

  disc->disc_first_track  = toc.toc_data[2];
  disc->disc_total_tracks = toc.toc_data[3];

  for(readtracks = 0;
      readtracks <= disc->disc_total_tracks;
      ++readtracks) {

    // offset to track information for track i
    byte = &toc.toc_data[readtracks * 8] + 4;

    disc->disc_track[readtracks].track_pos.minutes = byte[5];
    disc->disc_track[readtracks].track_pos.seconds = byte[6];
    disc->disc_track[readtracks].track_pos.frames  = byte[7];
    
    disc->disc_track[readtracks].track_type =
      (byte[1] & CDROM_DATA_TRACK) ?
      CDAUDIO_TRACK_DATA : CDAUDIO_TRACK_AUDIO;

    disc->disc_track[readtracks].track_lba =
      cd_msf_to_lba(disc->disc_track[readtracks].track_pos);

  }

  for(readtracks = 0; readtracks <= disc->disc_total_tracks; readtracks++) {
    if(readtracks > 0) {
      pos = cd_msf_to_frames(disc->disc_track[readtracks].track_pos) -
	cd_msf_to_frames(disc->disc_track[readtracks - 1].track_pos);
      cd_frames_to_msf(&disc->disc_track[readtracks - 1].track_length, pos);
    }
  }

  disc->disc_length.minutes =
    disc->disc_track[disc->disc_total_tracks].track_pos.minutes;
  disc->disc_length.seconds =
    disc->disc_track[disc->disc_total_tracks].track_pos.seconds;
  disc->disc_length.frames  =
    disc->disc_track[disc->disc_total_tracks].track_pos.frames;
  
  cd_update(disc, status);
 
  return 0;
}

int
cd_poll(int cd_desc, struct disc_status *status)
{
  scsi_position cdsc;
  memset(&cdsc, 0, sizeof(cdsc));

  /* set up the header */
  cdsc.position[4] = 12;   /* length */
  cdsc.position[5] = 0x01; /* request track msf info */
   
  if(ioctl(cd_desc, B_SCSI_GET_POSITION, &cdsc) < 0)
    {
      status->status_present = 0;
      return 0;
    }

  status->status_present = 1;
  status->status_disc_time.minutes  = cdsc.position[ 9];
  status->status_disc_time.seconds  = cdsc.position[10];
  status->status_disc_time.frames   = cdsc.position[11];
  status->status_track_time.minutes = cdsc.position[13];
  status->status_track_time.seconds = cdsc.position[14];
  status->status_track_time.frames  = cdsc.position[15];
  status->status_current_track      = cdsc.position[ 8];

  switch(cdsc.position[1])
    {
    case 0x11:
      status->status_mode = CDAUDIO_PLAYING;
      break;
    case 0x12:
      status->status_mode = CDAUDIO_PAUSED;
      break;
    case 0x13:
      status->status_mode = CDAUDIO_COMPLETED;
      break;
    default:
      status->status_mode = CDAUDIO_NOSTATUS;
    }

  return 0;
}

/* Play frames from CD */
int
cd_play_frames(int cd_desc, int startframe, int endframe)
{
  scsi_play_position cdmsf;

  cdmsf.start_m = startframe / 4500;
  cdmsf.start_s = (startframe % 4500) / 75;
  cdmsf.start_f = startframe % 75;
  cdmsf.end_m = endframe / 4500;
  cdmsf.end_s = (endframe % 4500) / 75;
  cdmsf.end_f = endframe % 75;
   
  if(ioctl(cd_desc, B_SCSI_PLAY_POSITION, &cdmsf) < 0)
    return -2;

  return 0;
}

/* Play starttrack at position pos to endtrack */
int
cd_play_track_pos(int cd_desc, int starttrack, int endtrack, int startpos)
{
  struct disc_timeval time;
   
  time.minutes = startpos / 60;
  time.seconds = startpos % 60;
  time.frames = 0;
   
  return cd_playctl(cd_desc,
		    PLAY_END_TRACK | PLAY_START_POSITION,
		    starttrack, endtrack, &time);
}

/* Play starttrack to endtrack */
int
cd_play_track(int cd_desc, int starttrack, int endtrack)
{
  return cd_playctl(cd_desc, PLAY_END_TRACK, starttrack, endtrack);
}

/* Play starttrack at position pos to end of CD */
int
cd_play_pos(int cd_desc, int track, int startpos)
{
  struct disc_timeval time;
   
  time.minutes = startpos / 60;
  time.seconds = startpos * 60;
  time.frames = 0;
   
  return cd_playctl(cd_desc, PLAY_START_POSITION, track, &time);
}

/* Play starttrack to end of CD */
int
cd_play(int cd_desc, int track)
{
  return cd_playctl(cd_desc, PLAY_START_TRACK, track);
}

/* Stop the CD, if it is playing */
int
cd_stop(int cd_desc)
{
  if(ioctl(cd_desc, B_SCSI_STOP_AUDIO) < 0)
    return -1;
   
  return 0;
}

/* Pause the CD */
int
cd_pause(int cd_desc)
{
  if(ioctl(cd_desc, B_SCSI_PAUSE_AUDIO) < 0)
    return -1;
   
  return 0;
}

/* Resume playing */
int
cd_resume(int cd_desc)
{
  if(ioctl(cd_desc, B_SCSI_RESUME_AUDIO) < 0)
    return -1;
   
  return 0;
}

/* Eject the tray */
int
cd_eject(int cd_desc)
{
  ioctl(cd_desc, B_EJECT_DEVICE);
  return 0;
}

/* Close the tray */
int
cd_close(int cd_desc)
{
  if(ioctl(cd_desc, B_LOAD_MEDIA) < 0)
    return -1;
   
  return 0;
}

/* Return the current volume setting */
int
cd_get_volume(int cd_desc, struct disc_volume *vol)
{
  scsi_volume cdvol;
  if(ioctl(cd_desc, B_SCSI_GET_VOLUME, &cdvol) < 0)
    return -1;
   
  vol->vol_front.left  = cdvol.port0_volume;
  vol->vol_front.right = cdvol.port1_volume;
  vol->vol_back.left   = cdvol.port2_volume;
  vol->vol_back.right  = cdvol.port3_volume;
   
  return 0;
}

/* Set the volume */
int
cd_set_volume(int cd_desc, struct disc_volume vol)
{
  scsi_volume cdvol;
   
#define OUTOFRANGE(x) (x > 255 || x < 0)
  if(OUTOFRANGE(vol.vol_front.left) || OUTOFRANGE(vol.vol_front.right) ||
     OUTOFRANGE(vol.vol_back.left)  || OUTOFRANGE(vol.vol_back.right))
    return -1;
#undef OUTOFRANGE

  cdvol.port0_volume = vol.vol_front.left;
  cdvol.port1_volume = vol.vol_front.right;
  cdvol.port2_volume = vol.vol_back.left;
  cdvol.port3_volume = vol.vol_back.right;

  cdvol.flags = B_SCSI_PORT0_VOLUME | B_SCSI_PORT1_VOLUME |
    B_SCSI_PORT2_VOLUME | B_SCSI_PORT3_VOLUME;

  if(ioctl(cd_desc, B_SCSI_SET_VOLUME, &cdvol) < 0)
    return -1;

  return 0;
}

#endif
