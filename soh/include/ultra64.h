#ifndef ULTRA64_H
#define ULTRA64_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "ultra64/types.h"
#include "unk.h"

#if !defined(__SWITCH__) && !defined(__linux__)
#include "libc/stdarg.h"
#include "libc/stdbool.h"
#include "libc/stddef.h"
#include "libc/stdlib.h"
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#endif

#include "libc/math.h"

#include "ultra64/exception.h"
#include "ultra64/rcp.h"
#include "ultra64/rdp.h"
#include "ultra64/rsp.h"
#include "ultra64/thread.h"
#include "ultra64/convert.h"
#include "ultra64/time.h"
#include "ultra64/message.h"
#include "ultra64/sptask.h"
#include "ultra64/gu.h"
#include "ultra64/vi.h"
#include "ultra64/pi.h"
#include "ultra64/controller.h"
#include "ultra64/printf.h"
#include "ultra64/mbi.h"
#include "ultra64/pfs.h"
#include "ultra64/motor.h"
#include "ultra64/r4300.h"

#endif
