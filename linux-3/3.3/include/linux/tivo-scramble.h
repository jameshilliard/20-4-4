#ifndef _LINUX_TIVO_SCRAMBLE_H
#define _LINUX_TIVO_SCRAMBLE_H

struct scrambleConfig {
   unsigned long int wyrd[4];
};

struct io_scramble {
   unsigned long magic;
   unsigned long version;
   unsigned long sectorsPerTransfer;
   long recordOffset;
   struct scrambleConfig config;
};

#define IO_SCRAMBLE_MAGIC 0x8F438DD2
#define IO_SCRAMBLE_VERSION_TIVO1 1

#define SCRAMBLER_MODE_MASK       0x0000000F
#define SCRAMBLER_MODE_3DES 	  0x00000001
#define SCRAMBLER_MODE_BLOWFISH	  0x00000002
#define SCRAMBLER_MODE_AES 	  0x00000004
#define SCRAMBLER_MODE_GOST 	  0x00000004
#define SCRAMBLER_MODE_BLUM_SHUB  0xFF000000
#define SCRAMBLER_EXPORT_MASK     0x7FFFFFFF
#define SCRAMBLER_GOST_MASK       0x1FFFFFFF

#ifdef __KERNEL__
static inline void prepare_scramble_regs(struct scrambleConfig *pconfig,
                                         struct io_scramble *pscramble)
{
    *pconfig = pscramble->config;
    pconfig->wyrd[0] += pscramble->recordOffset * 73221;
    pconfig->wyrd[0] |= SCRAMBLER_MODE_BLUM_SHUB;
    pconfig->wyrd[1] -= pscramble->recordOffset;
    pconfig->wyrd[1] |= SCRAMBLER_MODE_BLOWFISH;
    pconfig->wyrd[2] += pscramble->recordOffset * 85565;
    if (!(pconfig->wyrd[2] & SCRAMBLER_EXPORT_MASK)) {
        pconfig->wyrd[2] |= SCRAMBLER_MODE_AES;
    }
    pconfig->wyrd[3] -= pscramble->recordOffset * 27;
    pconfig->wyrd[3] |= ~SCRAMBLER_EXPORT_MASK;
    if (!(pconfig->wyrd[3] & SCRAMBLER_GOST_MASK)) {
        pconfig->wyrd[3] |= SCRAMBLER_MODE_GOST;
    }
}
#endif

#endif

