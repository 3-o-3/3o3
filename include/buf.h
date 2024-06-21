/******************************************************************************
 *                       OS-3o3 operating system
 * 
 *                            growing buffer
 *
 *            20 June MMXXIV PUBLIC DOMAIN by Jean-Marc Lienher
 *
 *        The authors disclaim copyright and patents to this software.
 * 
 *****************************************************************************/

#ifndef BUF_H_
#define BUF_H_ 1

#include "os3.h"

struct buf {
    os_int32 size;
    os_int32 len;
    os_utf8* buf;
};


struct buf* buf__new(os_int32 size);
void buf__dispose(struct buf* self);
os_utf8* buf__getstr(struct buf* self);
void buf__addstr(struct buf* self, os_utf8* data);
void buf__addhex8(struct buf* self, os_int32 data);
void buf__addhex16(struct buf* self, os_int32 data);
void buf__addhex32(struct buf* self, os_int32 data);
void buf__addint(struct buf* self, os_int32 data);
void buf__add8(struct buf* self, os_int32 data);
void buf__add16(struct buf* self, os_int32 data);
void buf__add32(struct buf* self, os_int32 data);
void buf__add_utf8(struct buf* self, os_int32 data);
void buf__add_utf16(struct buf* self, os_int32 data);
os_int32 buf__utf8tocp(os_utf8* b, os_int32 len, os_int32* code_point);
os_int32 buf__utf16tocp(os_utf8* b, os_int32 len, os_int32* code_point);
#endif
