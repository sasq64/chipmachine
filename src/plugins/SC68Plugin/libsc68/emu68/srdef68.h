/**
 * @ingroup   emu68_lib
 * @file      emu68/srdef68.h
 * @author    Benjamin Gerard
 * @date      1999/13/03
 * @brief     Status Register (SR) definition header.
 *
 */

/* $Id: srdef68.h 121 2009-06-30 17:30:22Z benjihan $ */

/* Copyright (C) 1998-2009 Benjamin Gerard */

#ifndef _EMU68_SRDEF68_H_
#define _EMU68_SRDEF68_H_

/** @defgroup  emu68_lib_srdef  Status Register (SR) definitions
 *  @ingroup   emu68_lib
 *
 *   68K status register definitions.
 *   The SR is composed of condition code register (CCR) located in
 *   the LSB and privileged processor status in MSB. EMU68 does not
 *   currently handle supervisor and trace mode. Internal processor level is
 *   partially managed. Each SR bit is defined by its bit number (SR_x_BIT)
 *   and the corresponding value (SR_x) where x is one of C,V,Z,N,X,S or T.
 *   SR_IPL_BIT is used to locate the less significant bit position of the 3
 *   IPL bits. Macros are available to help with SR bit manipulations.
 *
 *  SR := T.S. .IPL ...X NZVC
 *
 * @{
 */

/** @name  SR bit definitions.
 *  @{
 */

enum {
  SR_C_BIT = 0    /**< Carry bit number.                    */,
  SR_V_BIT = 1    /**< Overflow bit number.                 */,
  SR_Z_BIT = 2    /**< Zero bit number.                     */,
  SR_N_BIT = 3    /**< Negative bit number.                 */,
  SR_X_BIT = 4    /**< eXtended carry bit number.           */,
  SR_I_BIT = 8    /**< Internal Processor Level bit number. */,
  SR_S_BIT = 13   /**< Superuser bit number.                */,
  SR_T_BIT = 15   /**< Trace bit number.                    */,
};

enum {
  SR_C = (1<<SR_C_BIT)  /**< Carry value.          */,
  SR_V = (1<<SR_V_BIT)  /**< Overflow value.       */,
  SR_Z = (1<<SR_Z_BIT)  /**< Zero value.           */,
  SR_N = (1<<SR_N_BIT)  /**< Negative value.       */,
  SR_X = (1<<SR_X_BIT)  /**< eXtended carry value. */,
  SR_I = (7<<SR_I_BIT)  /**< IPL mask.             */,
  SR_S = (1<<SR_S_BIT)  /**< Superuser value.      */,
  SR_T = (1<<SR_T_BIT)  /**< Trace value.          */,
};

/** @} */


/** @name  SR manipulations.
 *  @{
 */

/** Get CCR value.
 *  @param  SR  current SR value
 *  @return CCR value
 *  @retval SR&255
 */
#define GET_CCR(SR) ( (u8) (SR) )

/** Set CCR in SR value.
 *  @param  SR  current SR value
 *  @param  CCR ccr value [0..255]
 *  @return new SR value
 *  @retval (SR&~255)|CCR
 */
#define SET_CCR(SR,CCR) (SR) = ( ( (SR) & ~255 ) | (CCR) )

/** Get IPL in SR value.
 *  @param  SR  current SR value
 *  @return IPL value
 *  @retval (SR>>SR_I_BIT)&7
 */
#define GET_IPL(SR) ( ( (SR) >> SR_I_BIT ) & 7 )

/** Change IPL value of SR.
 *  @param  SR   current SR value
 *  @param  IPL  new IPL value [0..7]
 *  @return new SR value
 *  @retval ((SR)&~SR_I)|((IPL)<<SR_IPL_BIT)
 */
#define SET_IPL(SR,IPL) (SR) = ( ( (SR) & ~SR_I ) | ( (IPL) << SR_I_BIT ) )

/** @} */

/**
 *  @}
 */

#endif /* #ifndef _EMU68_SRDEF68_H_ */
