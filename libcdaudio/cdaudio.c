/*
This is part of the audio CD player library
Copyright (C)1998-99 Tony Arcieri <bascule@inferno.tusculum.edu>
Parts Copyright (C)1999 Quinton Dolan <q@OntheNet.com.au>

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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cdaudio.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

/* We can check to see if the CD-ROM is mounted if this is available */
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif

#ifdef HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#endif

#ifdef SOLARIS_GETMNTENT
#include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_UCRED_H
#include <sys/ucred.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

/* For Linux */
#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#define NON_BLOCKING
#endif

#ifdef HAVE_LINUX_UCDROM_H
#include <linux/ucdrom.h>
#endif

/* For FreeBSD, OpenBSD, and Solaris */
#ifdef HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#endif

/* For Digital UNIX */
#ifdef HAVE_IO_CAM_CDROM_H
#include <io/cam/cdrom.h>
#endif

#include "compat.h"


#if !defined(IRIX_CDAUDIO) && !defined(BEOS_CDAUDIO)
/*
Because of Irix's different interface, most of this program is
completely ignored when compiling under Irix.

BeOS is quite a bit different.  Not all of the necessary structure
exists for reading data from discs. 
*/

/* Return the version of libcdaudio */
void
cd_version(char *buffer, int len)
{
  snprintf(buffer, len, "%s %s", PACKAGE, VERSION);
}

/* Return the version of libcdaudio */
long
cdaudio_getversion(void)
{
  return LIBCDAUDIO_VERSION;
}

/* Initialize the CD-ROM for playing audio CDs */
int
cd_init_device(char *device_name)
{
  int cd_desc;

#if defined(SOLARIS_GETMNTENT)
  FILE *mounts;
  struct mnttab mnt;
#elif defined(HAVE_GETMNTENT)
  FILE *mounts;
  struct mntent *mnt;
#endif
#ifdef HAVE_GETMNTINFO
  int mounts;
  struct statfs *mnt;
#endif
  char devname[255];
  struct stat st;
  int len = 0;

  if(lstat(device_name, &st) < 0)
    return -1;
   
  if(S_ISLNK(st.st_mode)) {
    /* Some readlink implementations don't put a \0 on the end */
    len = readlink(device_name, devname, 255);
    devname[len] = '\0';
  } else {
    strncpy(devname, device_name, 255);
    len = strlen(devname);
  }
#ifdef SOLARIS_GETMNTENT
  if((mounts = fopen(MNTTAB, "r")) == NULL)
    return -1;

  while(getmntent(mounts, &mnt) == 0) {
    if(strncmp(mnt.mnt_fstype, devname, len) == 0) {
      errno = EBUSY;
      return -1;
    }
  }

  if(fclose(mounts) != 0)
    return -1;
#elif defined(HAVE_GETMNTENT)
  if((mounts = setmntent(MOUNTED, "r")) == NULL)
    return -1;
      
  while((mnt = getmntent(mounts)) != NULL) {
    if(strncmp(mnt->mnt_fsname, devname, len) == 0) {
      endmntent(mounts);
      errno = EBUSY;
      return -1;
    }
  }
  endmntent(mounts);
#endif
#ifdef HAVE_GETMNTINFO
  for ( (mounts = getmntinfo(&mnt, 0)); mounts > 0;)
    {
      mounts--;
      if (strncmp(mnt[mounts].f_mntfromname, devname, len) == 0)
	{
	  errno = EBUSY;
	  return -1;
	}
    }
#endif   

#ifdef NON_BLOCKING
  if((cd_desc = open(device_name, O_RDONLY | O_NONBLOCK)) < 0)
#else
  if((cd_desc = open(device_name, O_RDONLY)) < 0)
#endif
    return -1;
	
  return cd_desc;
}

/* Close a device handle and free its resources. */
int
cd_finish(int cd_desc)
{
  close(cd_desc);
  return 0;
}

/* Update a CD status structure... because operating system interfaces vary
   so does this function. */
int
cd_stat(int cd_desc, struct disc_info *disc)
{
  /* Since every platform does this a little bit differently this
     gets pretty complicated... */

  struct CDAUDIO_TOCHEADER cdth;
  struct CDAUDIO_TOCENTRY cdte;
#ifdef CDAUDIO_TOCENTRY_DATA
  struct CDAUDIO_TOCENTRY_DATA cdte_buffer[MAX_TRACKS];
#endif

  struct disc_status status;
  int readtracks, pos;
   
  if(cd_poll(cd_desc, &status) < 0)
    return -1;
 
  if(!status.status_present) {
    disc->disc_present = 0;
    return 0;
  }
   
  /* Read the Table Of Contents header */
  if(ioctl(cd_desc, CDAUDIO_READTOCHEADER, &cdth) < 0)
    return -1;

  disc->disc_first_track = cdth.CDTH_STARTING_TRACK;
  disc->disc_total_tracks = cdth.CDTH_ENDING_TRACK;

#ifdef CDAUDIO_READTOCENTRYS

  /* Read the Table Of Contents */
  cdte.CDTE_ADDRESS_FORMAT = CDAUDIO_MSF_FORMAT;
  cdte.CDTE_STARTING_TRACK = 0;
  cdte.CDTE_DATA = cdte_buffer;
  cdte.CDTE_DATA_LEN = sizeof(cdte_buffer);

  if(ioctl(cd_desc, CDAUDIO_READTOCENTRYS, &cdte) < 0)
    return -1;

  for(readtracks = 0;
      readtracks <= disc->disc_total_tracks;
      readtracks++) {
#define DT disc->disc_track[readtracks]
#define TP DT.track_pos
#define CDTE cdte.CDTE_DATA[readtracks]
    TP.minutes = CDTE.CDTE_MSF_M;
    TP.seconds = CDTE.CDTE_MSF_S;
    TP.frames = CDTE.CDTE_MSF_F;
    DT.track_type =
      (CDTE.CDTE_CONTROL & CDROM_DATA_TRACK)?
      CDAUDIO_TRACK_DATA: CDAUDIO_TRACK_AUDIO;
    DT.track_lba = cd_msf_to_lba(TP);
#if 0
    disc->disc_track[readtracks].track_type = (cdte.CDTE_DATA[readtracks].CDTE_CONTROL & CDROM_DATA_TRACK) ? CDAUDIO_TRACK_DATA : CDAUDIO_TRACK_AUDIO;
    disc->disc_track[readtracks].track_lba = cd_msf_to_lba(disc->disc_track[readtracks].track_pos);
#endif
#undef DT
#undef TP
#undef CDTE
  }

#else /* CDAUDIO_READTOCENTRYS */

  for(readtracks = 0;
      readtracks <= disc->disc_total_tracks;
      readtracks++) {
    cdte.CDTE_STARTING_TRACK =
      (readtracks == disc->disc_total_tracks) ?
      CDROM_LEADOUT : readtracks + 1;
    cdte.CDTE_ADDRESS_FORMAT = CDAUDIO_MSF_FORMAT;
    if(ioctl(cd_desc, CDAUDIO_READTOCENTRY, &cdte) < 0)
      return -1;

#define DT disc->disc_track[readtracks]
    DT.track_pos.minutes = cdte.CDTE_MSF_M;
    DT.track_pos.seconds = cdte.CDTE_MSF_S;
    DT.track_pos.frames = cdte.CDTE_MSF_F;
    DT.track_type =
      (cdte.CDTE_CONTROL & CDROM_DATA_TRACK) ?
      CDAUDIO_TRACK_DATA : CDAUDIO_TRACK_AUDIO;
    DT.track_lba = cd_msf_to_lba(DT.track_pos);
#undef DT
  }
#endif /* CDAUDIO_READTOCENTRY */
  for(readtracks = 0; readtracks <= disc->disc_total_tracks; readtracks++) {
    if(readtracks > 0) {
      pos = cd_msf_to_frames(disc->disc_track[readtracks].track_pos) -
	cd_msf_to_frames(disc->disc_track[readtracks - 1].track_pos);
      cd_frames_to_msf(&disc->disc_track[readtracks - 1].track_length, pos);
    }
  }

#define DL disc->disc_length
#define TP disc->disc_track[disc->disc_total_tracks].track_pos
  DL.minutes = TP.minutes;
  DL.seconds = TP.seconds;
  DL.frames = TP.frames;
#undef DL
#undef TP
  
  cd_update(disc, status);
 
  return 0;
}

int
cd_poll(int cd_desc, struct disc_status *status)
{
  struct CDAUDIO_SUBCHANNEL cdsc;
#ifdef CDAUDIO_SUBCHANNEL_DATA
  struct CDAUDIO_SUBCHANNEL_DATA cdsc_data;
#endif


#ifdef CDAUDIO_SUBCHANNEL_DATA
  memset(&cdsc_data, '\0', sizeof(cdsc_data));
  cdsc.CDSC_DATA = &cdsc_data;
  cdsc.CDSC_DATA_LEN = sizeof(cdsc_data);
  cdsc.CDSC_DATA_FORMAT = CDAUDIO_CDSC_DATA_FORMAT;
#endif
  cdsc.CDSC_ADDRESS_FORMAT = CDAUDIO_MSF_FORMAT;
   
  if(ioctl(cd_desc, CDAUDIO_READSUBCHANNEL, &cdsc) < 0)
    {
      status->status_present = 0;
	
      return 0;
    }
   
  status->status_present = 1;
   
#define DT status->status_disc_time
#define TT status->status_track_time
#define CT status->status_current_track

#ifdef CDAUDIO_SUBCHANNEL_DATA
#define CDSC(arg) cdsc_data.CDSC_DATA_ ## arg
#else
#define CDSC(arg) cdsc.CDSC_ ## arg
#endif

  DT.minutes = CDSC(ABS_MSF_M);
  DT.seconds = CDSC(ABS_MSF_S);
  DT.frames  = CDSC(ABS_MSF_F);
  TT.minutes = CDSC(REL_MSF_M);
  TT.seconds = CDSC(REL_MSF_S);
  TT.frames  = CDSC(REL_MSF_F);
  CT         = CDSC(TRK);

#undef DT
#undef TT
#undef CT
#undef CDSC

#ifdef CDAUDIO_SUBCHANNEL_DATA
#define SW cdsc_data.CDSC_AUDIO_STATUS
#else
#define SW cdsc.CDSC_AUDIO_STATUS
#endif

  switch(SW) {
  case CDAUDIO_AS_PLAY_IN_PROGRESS:
    status->status_mode = CDAUDIO_PLAYING;
    break;
  case CDAUDIO_AS_PLAY_PAUSED:
    status->status_mode = CDAUDIO_PAUSED;
    break;
  case CDAUDIO_AS_PLAY_COMPLETED:
    status->status_mode = CDAUDIO_COMPLETED;
    break;
  default:
    status->status_mode = CDAUDIO_NOSTATUS;
  }

#undef SW
   
  return 0;
}

/* Play frames from CD */
int
cd_play_frames(int cd_desc, int startframe, int endframe)
{
  struct CDAUDIO_MSF cdmsf;

#ifdef BROKEN_SOLARIS_LEADOUT
  endframe -= 1;
#endif

  cdmsf.CDMSF_START_M = startframe / 4500;
  cdmsf.CDMSF_START_S = (startframe % 4500) / 75;
  cdmsf.CDMSF_START_F = startframe % 75;
  cdmsf.CDMSF_END_M = endframe / 4500;
  cdmsf.CDMSF_END_S = (endframe % 4500) / 75;
  cdmsf.CDMSF_END_F = endframe % 75;

#ifdef CDAUDIO_START
  /* Some CDROM drives don't support this so lets try things without it */
  /* ioctl(cd_desc, CDAUDIO_START); */
#endif
   
  if(ioctl(cd_desc, CDAUDIO_PLAY_MSF, &cdmsf) < 0)
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
  if(ioctl(cd_desc, CDAUDIO_STOP) < 0)
    return -1;
   
  return 0;
}

/* Pause the CD */
int
cd_pause(int cd_desc)
{
  if(ioctl(cd_desc, CDAUDIO_PAUSE) < 0)
    return -1;
   
  return 0;
}

/* Resume playing */
int
cd_resume(int cd_desc)
{
  if(ioctl(cd_desc, CDAUDIO_RESUME) < 0)
    return -1;
   
  return 0;
}

/* Eject the tray */
int
cd_eject(int cd_desc)
{
  int r = ioctl(cd_desc, CDAUDIO_EJECT);
  if(r) {
    printf("ioctl returned %d\n", r);
    return -1;
  }
  return 0;
}

/* Close the tray */
int
cd_close(int cd_desc)
{
#ifdef CDAUDIO_CLOSE
  if(ioctl(cd_desc, CDAUDIO_CLOSE) < 0)
    return -1;
   
  return 0;
#else
  errno = ENOTTY;
  return -1;
#endif
}

/* Return the current volume setting */
int
cd_get_volume(int cd_desc, struct disc_volume *vol)
{
#ifdef CDAUDIO_GET_VOLUME
  struct CDAUDIO_VOLSTAT cdvol;
#ifdef CDAUDIO_VOLSTAT_DATA
  struct CDAUDIO_VOLSTAT_DATA cdvol_data;
  cdvol.CDVOLSTAT_DATA = &cdvol_data;
  cdvol.CDVOLSTAT_DATA_LEN = sizeof(cdvol_data);
#endif /* CDAUDIO_VOLSTAT_DATA */

  if(ioctl(cd_desc, CDAUDIO_GET_VOLUME, &cdvol) < 0)
    return -1;
   
#ifdef CDAUDIO_VOLSTAT_DATA
#define CDV cdvol_data
#else
#define CDV cdvol
#endif
  vol->vol_front.left = CDV.CDVOLSTAT_FRONT_LEFT;
  vol->vol_front.right = CDV.CDVOLSTAT_FRONT_RIGHT;
  vol->vol_back.left = CDV.CDVOLSTAT_BACK_LEFT;
  vol->vol_back.right = CDV.CDVOLSTAT_BACK_RIGHT;
#undef CDV
   
  return 0;
#else /* CDAUDIO_GET_VOLUME */
  errno = ENOTTY;
  return -1;
#endif /* CDAUDIO_GET_VOLUME */
}

/* Set the volume */
int
cd_set_volume(int cd_desc, struct disc_volume vol)
{
  struct CDAUDIO_VOLCTRL cdvol;
#ifdef CDAUDIO_VOLCTRL_DATA
  struct cd_playback cdvol_data;
  cdvol.CDVOLCTRL_DATA = &cdvol_data;
  cdvol.CDVOLCTRL_DATA_LEN = sizeof(cdvol_data);
#endif
   
#define OUTOFRANGE(x) (x > 255 || x < 0)
  if(OUTOFRANGE(vol.vol_front.left) || OUTOFRANGE(vol.vol_front.right) ||
     OUTOFRANGE(vol.vol_back.left) || OUTOFRANGE(vol.vol_back.right))
    return -1;
#undef OUTOFRANGE

  cdvol.CDVOLCTRL_FRONT_LEFT = vol.vol_front.left;
  cdvol.CDVOLCTRL_FRONT_RIGHT = vol.vol_front.right;
  cdvol.CDVOLCTRL_BACK_LEFT = vol.vol_back.left;
  cdvol.CDVOLCTRL_BACK_RIGHT = vol.vol_back.right;

#ifdef CDAUDIO_VOLCTRL_SELECT
  cdvol_data.CDVOLCTRL_FRONT_LEFT_SELECT = CDAUDIO_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_FRONT_RIGHT_SELECT = CDAUDIO_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_BACK_LEFT_SELECT = CDAUDIO_MAX_VOLUME;
  cdvol_data.CDVOLCTRL_BACK_RIGHT_SELECT = CDAUDIO_MAX_VOLUME;
#endif
   
  if(ioctl(cd_desc, CDAUDIO_SET_VOLUME, &cdvol) < 0)
    return -1;

  return 0;
}

#endif  /* IRIX_CDAUDIO */

/*
Because all these functions are solely mathematical and/or only make callbacks
to previously existing functions they can be used for any platform.
 */

/* Convert frames to a logical block address */
int
cd_frames_to_lba(int frames)
{
  if(frames >= 150)
    return frames - 150;
   
  return 0;
}

/* Convert a logical block address to frames */
int
cd_lba_to_frames(int lba)
{
  return lba + 150;
}

/* Convert disc_timeval to frames */
int
cd_msf_to_frames(struct disc_timeval time)
{
  return time.minutes * 4500 + time.seconds * 75 + time.frames;
}

/* Convert disc_timeval to a logical block address */
int
cd_msf_to_lba(struct disc_timeval time)
{
  if(cd_msf_to_frames(time) > 150)
    return cd_msf_to_frames(time) - 150;
   
  return 0;
}

/* Convert frames to disc_timeval */
void
cd_frames_to_msf(struct disc_timeval *time, int frames)
{
  time->minutes = frames / 4500;
  time->seconds = (frames % 4500) / 75;
  time->frames = frames % 75;
}

/* Convert a logical block address to disc_timeval */
void
cd_lba_to_msf(struct disc_timeval *time, int lba)
{
  cd_frames_to_msf(time, lba + 150);
}

/* Internal advance function */
int
__internal_cd_track_advance(int cd_desc, struct disc_info disc,
			    int endtrack, struct disc_timeval time)
{ 
  disc.disc_track_time.minutes += time.minutes;
  disc.disc_track_time.seconds += time.seconds;
  disc.disc_track_time.frames += time.frames;
   
  if(disc.disc_track_time.frames > 74) {
    disc.disc_track_time.frames -= 74;
    disc.disc_track_time.seconds++;
  }
   
  if(disc.disc_track_time.frames < 0) {
    disc.disc_track_time.frames += 75;
    disc.disc_track_time.seconds--;
  }
   
  if(disc.disc_track_time.seconds > 59) {
    disc.disc_track_time.seconds -= 59;
    disc.disc_track_time.minutes++;
  }
  if(disc.disc_track_time.seconds < 0) {
    disc.disc_track_time.seconds += 60;
    disc.disc_track_time.minutes--;
  }

  if(disc.disc_track_time.minutes < 0) {
    disc.disc_current_track--;
    if(disc.disc_current_track == 0)
      disc.disc_current_track = 1;

    return cd_play_track(cd_desc, disc.disc_current_track, endtrack);
  }

#define DTT disc.disc_track_time
#define TP disc.disc_track[disc.disc_current_track].track_pos
  if((DTT.minutes == TP.minutes && DTT.seconds > TP.seconds) ||
     DTT.minutes > TP.minutes) {
    disc.disc_current_track++;
    if(disc.disc_current_track > endtrack)
      disc.disc_current_track = endtrack;

    return cd_play_track(cd_desc, disc.disc_current_track, endtrack);
  }
#undef DTT
#undef TP

  return cd_play_track_pos(cd_desc,
			   disc.disc_current_track,
			   endtrack,
			   disc.disc_track_time.minutes * 60 +
			   disc.disc_track_time.seconds);
}

/* Advance the position within a track */
int
cd_track_advance(int cd_desc, int endtrack, struct disc_timeval time)
{
  struct disc_info disc;
   
  if(cd_stat(cd_desc, &disc) < 0)
    return -1;
   
  if(!disc.disc_present)
    return -1;
   
  if(__internal_cd_track_advance(cd_desc, disc, endtrack, time) < 0)
    return -1;
   
  return 0;
}

/* Advance the position within the track without preserving an
   endtrack */
int
cd_advance(int cd_desc, struct disc_timeval time)
{
  struct disc_info disc;

  if(cd_stat(cd_desc, &disc) < 0)
    return -1;
   
  if(__internal_cd_track_advance(cd_desc, disc,
				 disc.disc_total_tracks, time) < 0)
    return -1;
   
  return 0;
}

/* Update information in a disc_info structure using a disc_status
   structure */
int
cd_update(struct disc_info *disc, struct disc_status status)
{
  if(!(disc->disc_present = status.status_present))
    return -1;
   
  disc->disc_mode = status.status_mode;
  memcpy(&disc->disc_time, &status.status_disc_time,
	 sizeof(struct disc_timeval));
  memcpy(&disc->disc_track_time, &status.status_track_time,
	 sizeof(struct disc_timeval));
   
#define CT disc->disc_current_track
  CT = 0;
  while(CT < disc->disc_total_tracks && 
	cd_msf_to_frames(disc->disc_time) >=
	cd_msf_to_frames(disc->disc_track[CT].track_pos) )
    CT++;
#undef CT
   
  return 0;
}

/* Universal play control function */
int
cd_playctl(int cd_desc, int options, int start_track, ...)
{
  int end_track;
  struct disc_info disc;
  struct disc_timeval *startpos, *endpos, start_position, end_position;
  va_list arglist;

  va_start(arglist, start_track);
  if(cd_stat(cd_desc, &disc) < 0)
    return -1;

  if(options & PLAY_END_TRACK)
    end_track = va_arg(arglist, int);
  else
    end_track = disc.disc_total_tracks;
   
  if(options & PLAY_START_POSITION)
    startpos = va_arg(arglist, struct disc_timeval *);
  else
    startpos = 0;
   
  if(options & PLAY_END_POSITION)
    endpos = va_arg(arglist, struct disc_timeval *);
  else
    endpos = 0;
   
  va_end(arglist);
   
#define TP disc.disc_track[start_track - 1].track_pos
  if(options & PLAY_START_POSITION) {
    start_position.minutes = TP.minutes + startpos->minutes;
    start_position.seconds = TP.seconds + startpos->seconds;
    start_position.frames = TP.frames + startpos->frames;
  } else {
    start_position.minutes = TP.minutes;
    start_position.seconds = TP.seconds;
    start_position.frames = TP.frames;
  }
#undef TP

  if(options & PLAY_END_TRACK) {
#define TP disc.disc_track[end_track].track_pos
    if(options & PLAY_END_POSITION) {
      end_position.minutes = TP.minutes + endpos->minutes;
      end_position.seconds = TP.seconds + endpos->seconds;
      end_position.frames = TP.frames + endpos->frames;
    } else {
      end_position.minutes = TP.minutes;
      end_position.seconds = TP.seconds;
      end_position.frames = TP.frames;
    }
#undef TP
  } else {
#define TP disc.disc_track[disc.disc_total_tracks].track_pos
    end_position.minutes = TP.minutes;
    end_position.seconds = TP.seconds;
    end_position.frames = TP.frames;
#undef TP
  }
   
  return cd_play_frames(cd_desc, cd_msf_to_frames(start_position),
			cd_msf_to_frames(end_position));
}
