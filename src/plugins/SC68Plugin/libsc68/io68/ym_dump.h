/**
 * @ingroup   emu68_devel
 * @file      emu68/ym_dump.h
 * @author    Benjamin Gerard
 * @date      2009/02/10
 * @brief     sc68 register dump YM-2149 emulator header.
 *
 */

IO68_EXTERN
/** Setup function for sc68 dump ym engine.
 *
 *    The ym_dump_setup() function sets ym dump engine for this ym
 *    emulator instance.
 *
 *  @parm    ym  ym emulator instance to setup
 *  @retval   0  on success
 *  @retval  -1  on failure
 */
int ym_dump_setup(ym_t * const ym);

/** Get/Set sc68 dump ym engine active state.
 *
 *  @parm    ym   ym emulator instance to setup
 *  @parm    val  0:disable 1:enable -1:current
 *  @return  previous status
 */
int ym_dump_active(ym_t * const ym, int val);

typedef void (*ym_dump_filter_t)(ym_t * const);

/** YM-2149 internal data structure for original emulator. */
struct ym2149_dump_s
{
  uint64_t base_cycle;
  uint68_t pass;
  int      active;
};

/** YM-2149 emulator instance type */
typedef struct ym2149_dump_s ym_dump_t;
