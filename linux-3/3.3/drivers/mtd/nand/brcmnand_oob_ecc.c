/****************************************************************************
*
*  File        :  brcmnand_oob_ecc.c
*
*  Description :  Low level read/write functions for NAND flash using the
*                 7401 NAND flash controller
*
*  Notes       :
*
*  Author      :  John Smith
*
*  History     :
*
*  Copyright   :  Pace Micro Technology 2007 (c)
*
****************************************************************************
***************************************************************************/

/* I think this ECC scheme is known as a Hamming code.
 *
 * It is capable of detecting and fixing all single bit errors, and
 * detecting double bit errors. Multiple bit errors can be 
 * incorrectly diagnosed
 *
 * A hint of the theory:
 *
 * given a sequence of bits
 *    b(0), b(1), b(2), b(3), ... b(n-1)
 *
 * calculate bits of a syndrome using
 *    c0(i)  the exclusive or of all the bits b(j) where the i-th binary
 *           digit of the index j is a zero
 *    c1(i)  the exclusive or of all the bits b(j) where the i-th binary 
 *           digit of the index j is a one
 *
 * Thus
 *
 *  c0(0) = b(0) ^ b(2) ^ b(4) ^ b(6) ^ b(8) ^ ... ^ b(n-2) 
 *  c0(1) = b(0) ^ b(1) ^ b(3) ^ b(4) ^ b(8) ^ ... ^ b(something) 
 *  c0(2) = b(0) ^ b(1) ^ b(2) ^ b(3) ^ b(8) ^ ...
 *
 *  c1(0) = b(1) ^ b(3) ^ b(5) ^ b(7) ^ b(9)  ^ ... ^ b(n-2) 
 *  c1(1) = b(2) ^ b(3) ^ b(6) ^ b(7) ^ b(10) ^ ... ^ b(something) 
 *  c1(2) = b(4) ^ b(5) ^ b(6) ^ b(7) ^ b(12) ^ ... ^ b(something)
 *
 * And then the bits of the syndrome are formed as
 *
 *  c0(0) c1(0) c0(1) c0(1) c0(2) c1(2)
 *
 * The following results are nearly obvious:
 *
 * 1. The syndrome of a sequence of zero bits is zero.
 *
 * 2. The syndrome of the exclusive-or of two sequences of bits is equal
 *    to the exclusive-or of the syndromes of the separate sequences
 *
 * 3. If b(0) b(1) b(2) ... is a sequence of bits with exactly one one in the
 *    i-th position and all the other bits zero, 
 *    and if
 *      c0(0) c0(1) c0(2) ... and
 *      c1(0) c1(1) c1(2) ... are the bits from the syndrome, 
 *    then the binary number formed from the binary bits
 *      ... c1(3) c1(2) c1(1) c1(0)
 *    is the binary expansion of the index i of the one bit
 *    and for all the bits in the syndrome, 
 *      c0(j) = 1-c1(j)
 *
 * So the encoding process is simply a matter of doing lots of exclusive 
 * or operations.
 *
 * The decoding process is:
 *   Calculate the syndrome of the supplied vector
 *   Exclusive or the calculated syndrome and supplied syndrome.
 *   If result is all zero
 *      then no bit errors
 *   If result is not all zero, then the exclusive or of the syndromes is 
 *   the syndrome of the error vector. So calculate the error vector using 
 *   result 3 above - and if that is not possible, then there might be a single
 *   bit error in the syndrome, and if that is not true, then the whole thing 
 *  is uncorrectable
 *
 */

//#include "brcmnand.h"
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include "brcmnand_oob.h"

typedef uint32_t PCT_UInt32 ;
typedef uint8_t  PCT_UInt8 ;

/* ************************************************************
 *
 * function: ecc_get_syndrome_length_bytes
 *
 * Purpose:  Calculate the syndrome length in bytes
 *
 * Answers:
 *         Vec Length     Syndrome bits     Syndrome bytes    
 *             1                6                 1
 *             2                8                 1
 *           3-4               10                 2
 *           5-8               12                 2
 *          9-16               14                 2
 *         17-32               16                 2
 *         33-64               18                 3
 *
 *  2**(n-1)+1 .. -2^n       6+2n            (6+2n+7)/8 round down
 * ************************************************************ */  
static int ecc_get_syndrome_length_bytes( uint32_t ui32VecLength )
{
   unsigned int nbytes = 0 ;
   unsigned int j;
   for (j=0; (1<<(4*j)) < (ui32VecLength<<3); j++)
   {
      nbytes++ ; ;
   }   
   return nbytes ;
}


/* ************************************************************
 *
 * function: ecc_calculate_syndrome
 *
 * Purpose:  Calculate the syndrome of a vector on bytes
 *
 * Note:     We actually calculate the opposite of the syndrome
 *           described above - this is so that all-ones is
 *           the ECC syndrome for an all-ones vector
 * 
 * ************************************************************ */  

void brcmnand_oobecc_calc( uint8_t* pui8Buffer, uint32_t ui32Length,
                           uint8_t* pui8EccBuff ) 
{
   unsigned int i,j ;
   uint8_t sum ;         /* Exclusive or of all the bytes */
   uint8_t t ;

   unsigned int nbytes = ecc_get_syndrome_length_bytes( ui32Length ) ;

   for (j=0; j<nbytes; j++)
   {
      pui8EccBuff[j] = 0xFF ;
   }   

   sum = 0 ;
   for (i=0; i<ui32Length; i++)
   {
      uint8_t b = pui8Buffer[i] ;      
      uint8_t all_bits = b ;

      all_bits ^= b>>4 ;
      all_bits ^= all_bits>>2 ;
      all_bits ^= all_bits>>1 ;      /* Exclusing or all the bits */
      all_bits &= 1 ;                /* of the byte together */ 

      sum ^= b ;                     

      if (all_bits & 1)
      {
         for (j=3; (1<<(j-3)) < ui32Length; j++)
         {
            /* bit 2j of the syndrome is the exclusive or of all bits b(i) of
             * the index where i has a zero in the j-th bit of its binary
             * expansion
             *
             * bit 2j+1 of the syndrome is the exclusive or of all bits b(i)
             * of the index where i has a one in the j-th bit of its binary
             * expansion
             */
            unsigned int bit_pos = 2*j ;
            unsigned int byte_index = (bit_pos)/8 ;
            uint8_t mask = 1<<(bit_pos%8) ;

            /* Look at the j-th bit of the bit index i. If it is a 0, then
               we are affecting 2j of the syndrome, otherwise bit 2j+1 */
            if ((i>>(j-3))&1) mask <<= 1 ;

            /* Now accumumate the syndrome */
            pui8EccBuff[byte_index] ^= mask ;
         }
      }
   }

   /*
    * sum contains the exclusive or of all the bytes of the original
    * vector. Some bit shifting can form the exclusive or of all
    * bits b(i) of the original vector where i has a zero in the 
    * first, second or third bits of its binary expansion
    */
   pui8EccBuff[0] ^= 3 & (sum ^ (sum>>2) ^ (sum>>4) ^ (sum>>6)) ;
   sum ^= sum>>1 ;
   
   t = sum ^ sum>>4 ;
   if (t&1) pui8EccBuff[0] ^= (1<<2) ;
   if (t&4) pui8EccBuff[0] ^= (1<<3) ;

   t = sum ^ sum>>2 ;
   if (t&1)  pui8EccBuff[0] ^= (1<<4) ;
   if (t&16) pui8EccBuff[0] ^= (1<<5) ;
}


void brcmnand_oobecc_fix( uint8_t* pui8Buffer, uint32_t ui32Length,
                          uint8_t* pui8EccBuff, enum BRCMNAND_OOBECC_STATUS* penStatus ) 
{
   unsigned int i,j ;
   uint8_t aui8Syndrome[4] ;

   unsigned int nSyndromeLengthBytes = 0 ;
   uint32_t ui32SyndromeMask ;
   uint8_t sum ;
   uint32_t m1, m2 ;
   uint32_t bit ;

   nSyndromeLengthBytes = ecc_get_syndrome_length_bytes( ui32Length ) ;

   ui32SyndromeMask = 1 ;
   for (ui32SyndromeMask=1;
        ui32SyndromeMask < ui32Length*8;
        ui32SyndromeMask <<= 1 )
   {
   }   
   ui32SyndromeMask -= 1 ;

   brcmnand_oobecc_calc( pui8Buffer, ui32Length, aui8Syndrome ) ;

   sum = 0 ;
   for (i=0; i<nSyndromeLengthBytes; i++)
   {
      aui8Syndrome[i] ^= pui8EccBuff[i] ;
      sum |= aui8Syndrome[i] ;
   }
   if (sum == 0)
   {
      /* Calculated and supplied syndrome match exactly -
       * vector must be good
       */
      *penStatus = BRCMNAND_ECC_GOOD ;
   }
   else
   {
      /* The bits of the syndrome are stored in a awkward order */         
      m1 = 0 ;
      m2 = 0 ;
      for (j=0; j<32; j++)
      {
         if (aui8Syndrome[j/4] & (1<<((2*j)%8)))   m1 |= 1<<j ;
         if (aui8Syndrome[j/4] & (1<<((2*j+1)%8))) m2 |= 1<<j ;
      }

      /* If there is a single bit error, then m2 gives the error and
       *  m1 = n-m2 
       */
      if ((m1&ui32SyndromeMask) + (m2&ui32SyndromeMask) == ui32SyndromeMask )
      {
         bit = m2&ui32SyndromeMask ;
         if (bit < 8*ui32Length)
         {
            *penStatus = BRCMNAND_ECC_FIXED ;
            pui8Buffer[bit/8] ^= 1<<(bit%8) ;
         }
         else
         {
            *penStatus = BRCMNAND_ECC_UNCORRECTABLE ;
         }
      }
      else
      {
         m1 &= ui32SyndromeMask ;
         m2 &= ui32SyndromeMask ;         

         /* If there is a single bit error in the syndrome, then either m1 is zero
          * and m2 has a single 1 set, or m2 is zero and m1 has a single bit set.
          * Note that if m has has just 0 or 1 bits set, them m&(m-1) == 0
          */
         if ( ((m1 == 0) && (m2&(m2-1)) == 0) ||
              ((m2 == 0) && (m1&(m1-1)) == 0) )
         {
            *penStatus = BRCMNAND_ECC_FIXED ;
            for (i=0; i<nSyndromeLengthBytes; i++)
            {
               pui8EccBuff[i] ^= aui8Syndrome[i] ;
            }
         }
         else
         {
            *penStatus = BRCMNAND_ECC_UNCORRECTABLE ;
         }
      }
   }
}
