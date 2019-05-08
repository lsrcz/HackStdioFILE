#ifndef __APPLE__
#define _IO_MTSAFE_IO
typedef struct {
  int lock;
  int cnt;
  void *owner;
} _IO_lock_t;

#define _IO_lock_initializer                                                   \
  { 0, 0, NULL }

#define _IO_lock_init(_name)                                                   \
  ((void)((_name) = (_IO_lock_t)_IO_lock_initializer))

#include "hack.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _IO_pos_BAD ((off64_t)-1)

extern const struct _IO_jump_t *
IO_validate_vtable(const struct _IO_jump_t *vtable);

#define _IO_seek_set 0
#define _IO_seek_cur 1
#define _IO_seek_end 2

/* THE JUMPTABLE FUNCTIONS.

 * The _IO_FILE type is used to implement the FILE type in GNU libc,
 * as well as the streambuf class in GNU iostreams for C++.
 * These are all the same, just used differently.
 * An _IO_FILE (or FILE) object is allows followed by a pointer to
 * a jump table (of pointers to functions).  The pointer is accessed
 * with the _IO_JUMPS macro.  The jump table has an eccentric format,
 * so as to be compatible with the layout of a C++ virtual function table.
 * (as implemented by g++).  When a pointer to a streambuf object is
 * coerced to an (FILE*), then _IO_JUMPS on the result just
 * happens to point to the virtual function table of the streambuf.
 * Thus the _IO_JUMPS function table used for C stdio/libio does
 * double duty as the virtual function table for C++ streambuf.
 *
 * The entries in the _IO_JUMPS function table (and hence also the
 * virtual functions of a streambuf) are described below.
 * The first parameter of each function entry is the _IO_FILE/streambuf
 * object being acted on (i.e. the 'this' parameter).
 */

/* Setting this macro to 1 enables the use of the _vtable_offset bias
   in _IO_JUMPS_FUNCS, below.  This is only needed for new-format
   _IO_FILE in libc that must support old binaries (see oldfileops.c).  */
#define _IO_JUMPS_OFFSET 0

/* Type of MEMBER in struct type TYPE.  */
#define _IO_MEMBER_TYPE(TYPE, MEMBER) __typeof__(((TYPE){}).MEMBER)
#define offsetof(type, ident) ((size_t) & (((type *)0)->ident))
/* Essentially ((TYPE *) THIS)->MEMBER, but avoiding the aliasing
   violation in case THIS has a different pointer type.  */
#define _IO_CAST_FIELD_ACCESS(THIS, TYPE, MEMBER)                              \
  (*(_IO_MEMBER_TYPE(TYPE, MEMBER) *)(((char *)(THIS)) +                       \
                                      offsetof(TYPE, MEMBER)))

#define _IO_JUMPS(THIS) (THIS)->vtable
#define _IO_JUMPS_FILE_plus(THIS)                                              \
  _IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE_plus, vtable)
#define _IO_WIDE_JUMPS(THIS)                                                   \
  _IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE, _wide_data)->_wide_vtable
#define _IO_CHECK_WIDE(THIS)                                                   \
  (_IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE, _wide_data) != NULL)

#if _IO_JUMPS_OFFSET
#define _IO_JUMPS_FUNC(THIS)                                                   \
  (IO_validate_vtable(                                                         \
      *(struct _IO_jump_t **)((void *)&_IO_JUMPS_FILE_plus(THIS) +             \
                              (THIS)->_vtable_offset)))
#define _IO_vtable_offset(THIS) (THIS)->_vtable_offset
#else
#define _IO_JUMPS_FUNC(THIS) (_IO_JUMPS_FILE_plus(THIS))
#define _IO_vtable_offset(THIS) 0
#endif
#define _IO_WIDE_JUMPS_FUNC(THIS) _IO_WIDE_JUMPS(THIS)
#define JUMP_FIELD(TYPE, NAME) TYPE NAME
#define JUMP0(FUNC, THIS) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS)
#define JUMP1(FUNC, THIS, X1) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1)
#define JUMP2(FUNC, THIS, X1, X2) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2)
#define JUMP3(FUNC, THIS, X1, X2, X3)                                          \
  (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2, X3)
#define JUMP_INIT(NAME, VALUE) VALUE
#define JUMP_INIT_DUMMY JUMP_INIT(dummy, 0), JUMP_INIT(dummy2, 0)

#define WJUMP0(FUNC, THIS) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS)
#define WJUMP1(FUNC, THIS, X1) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1)
#define WJUMP2(FUNC, THIS, X1, X2)                                             \
  (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2)
#define WJUMP3(FUNC, THIS, X1, X2, X3)                                         \
  (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2, X3)

/* The 'finish' function does any final cleaning up of an _IO_FILE object.
   It does not delete (free) it, but does everything else to finalize it.
   It matches the streambuf::~streambuf virtual destructor.  */
#define _IO_FINISH(FP) JUMP1(__finish, FP, 0)
#define _IO_WFINISH(FP) WJUMP1(__finish, FP, 0)

/* The 'overflow' hook flushes the buffer.
   The second argument is a character, or EOF.
   It matches the streambuf::overflow virtual function. */
#define _IO_OVERFLOW(FP, CH) JUMP1(__overflow, FP, CH)
#define _IO_WOVERFLOW(FP, CH) WJUMP1(__overflow, FP, CH)

/* The 'underflow' hook tries to fills the get buffer.
   It returns the next character (as an unsigned char) or EOF.  The next
   character remains in the get buffer, and the get position is not changed.
   It matches the streambuf::underflow virtual function. */
#define _IO_UNDERFLOW(FP) JUMP0(__underflow, FP)
#define _IO_WUNDERFLOW(FP) WJUMP0(__underflow, FP)

/* The 'uflow' hook returns the next character in the input stream
   (cast to unsigned char), and increments the read position;
   EOF is returned on failure.
   It matches the streambuf::uflow virtual function, which is not in the
   cfront implementation, but was added to C++ by the ANSI/ISO committee. */
#define _IO_UFLOW(FP) JUMP0(__uflow, FP)
#define _IO_WUFLOW(FP) WJUMP0(__uflow, FP)

/* The 'pbackfail' hook handles backing up.
   It matches the streambuf::pbackfail virtual function. */
#define _IO_PBACKFAIL(FP, CH) JUMP1(__pbackfail, FP, CH)
#define _IO_WPBACKFAIL(FP, CH) WJUMP1(__pbackfail, FP, CH)

/* The 'xsputn' hook writes upto N characters from buffer DATA.
   Returns EOF or the number of character actually written.
   It matches the streambuf::xsputn virtual function. */
#define _IO_XSPUTN(FP, DATA, N) JUMP2(__xsputn, FP, DATA, N)
#define _IO_WXSPUTN(FP, DATA, N) WJUMP2(__xsputn, FP, DATA, N)

/* The 'xsgetn' hook reads upto N characters into buffer DATA.
   Returns the number of character actually read.
   It matches the streambuf::xsgetn virtual function. */
#define _IO_XSGETN(FP, DATA, N) JUMP2(__xsgetn, FP, DATA, N)
#define _IO_WXSGETN(FP, DATA, N) WJUMP2(__xsgetn, FP, DATA, N)

/* The 'seekoff' hook moves the stream position to a new position
   relative to the start of the file (if DIR==0), the current position
   (MODE==1), or the end of the file (MODE==2).
   It matches the streambuf::seekoff virtual function.
   It is also used for the ANSI fseek function. */
#define _IO_SEEKOFF(FP, OFF, DIR, MODE) JUMP3(__seekoff, FP, OFF, DIR, MODE)
#define _IO_WSEEKOFF(FP, OFF, DIR, MODE) WJUMP3(__seekoff, FP, OFF, DIR, MODE)

/* The 'seekpos' hook also moves the stream position,
   but to an absolute position given by a fpos64_t (seekpos).
   It matches the streambuf::seekpos virtual function.
   It is also used for the ANSI fgetpos and fsetpos functions.  */
/* The _IO_seek_cur and _IO_seek_end options are not allowed. */
#define _IO_SEEKPOS(FP, POS, FLAGS) JUMP2(__seekpos, FP, POS, FLAGS)
#define _IO_WSEEKPOS(FP, POS, FLAGS) WJUMP2(__seekpos, FP, POS, FLAGS)

/* The 'setbuf' hook gives a buffer to the file.
   It matches the streambuf::setbuf virtual function. */
#define _IO_SETBUF(FP, BUFFER, LENGTH) JUMP2(__setbuf, FP, BUFFER, LENGTH)
#define _IO_WSETBUF(FP, BUFFER, LENGTH) WJUMP2(__setbuf, FP, BUFFER, LENGTH)

/* The 'sync' hook attempts to synchronize the internal data structures
   of the file with the external state.
   It matches the streambuf::sync virtual function. */
#define _IO_SYNC(FP) JUMP0(__sync, FP)
#define _IO_WSYNC(FP) WJUMP0(__sync, FP)

/* The 'doallocate' hook is used to tell the file to allocate a buffer.
   It matches the streambuf::doallocate virtual function, which is not
   in the ANSI/ISO C++ standard, but is part traditional implementations. */
#define _IO_DOALLOCATE(FP) JUMP0(__doallocate, FP)
#define _IO_WDOALLOCATE(FP) WJUMP0(__doallocate, FP)

/* The following four hooks (sysread, syswrite, sysclose, sysseek, and
   sysstat) are low-level hooks specific to this implementation.
   There is no correspondence in the ANSI/ISO C++ standard library.
   The hooks basically correspond to the Unix system functions
   (read, write, close, lseek, and stat) except that a FILE*
   parameter is used instead of an integer file descriptor;  the default
   implementation used for normal files just calls those functions.
   The advantage of overriding these functions instead of the higher-level
   ones (underflow, overflow etc) is that you can leave all the buffering
   higher-level functions.  */

/* The 'sysread' hook is used to read data from the external file into
   an existing buffer.  It generalizes the Unix read(2) function.
   It matches the streambuf::sys_read virtual function, which is
   specific to this implementation. */
#define _IO_SYSREAD(FP, DATA, LEN) JUMP2(__read, FP, DATA, LEN)
#define _IO_WSYSREAD(FP, DATA, LEN) WJUMP2(__read, FP, DATA, LEN)

/* The 'syswrite' hook is used to write data from an existing buffer
   to an external file.  It generalizes the Unix write(2) function.
   It matches the streambuf::sys_write virtual function, which is
   specific to this implementation. */
#define _IO_SYSWRITE(FP, DATA, LEN) JUMP2(__write, FP, DATA, LEN)
#define _IO_WSYSWRITE(FP, DATA, LEN) WJUMP2(__write, FP, DATA, LEN)

/* The 'sysseek' hook is used to re-position an external file.
   It generalizes the Unix lseek(2) function.
   It matches the streambuf::sys_seek virtual function, which is
   specific to this implementation. */
#define _IO_SYSSEEK(FP, OFFSET, MODE) JUMP2(__seek, FP, OFFSET, MODE)
#define _IO_WSYSSEEK(FP, OFFSET, MODE) WJUMP2(__seek, FP, OFFSET, MODE)

/* The 'sysclose' hook is used to finalize (close, finish up) an
   external file.  It generalizes the Unix close(2) function.
   It matches the streambuf::sys_close virtual function, which is
   specific to this implementation. */
#define _IO_SYSCLOSE(FP) JUMP0(__close, FP)
#define _IO_WSYSCLOSE(FP) WJUMP0(__close, FP)

/* The 'sysstat' hook is used to get information about an external file
   into a struct stat buffer.  It generalizes the Unix fstat(2) call.
   It matches the streambuf::sys_stat virtual function, which is
   specific to this implementation. */
#define _IO_SYSSTAT(FP, BUF) JUMP1(__stat, FP, BUF)
#define _IO_WSYSSTAT(FP, BUF) WJUMP1(__stat, FP, BUF)

/* The 'showmany' hook can be used to get an image how much input is
   available.  In many cases the answer will be 0 which means unknown
   but some cases one can provide real information.  */
#define _IO_SHOWMANYC(FP) JUMP0(__showmanyc, FP)
#define _IO_WSHOWMANYC(FP) WJUMP0(__showmanyc, FP)

/* The 'imbue' hook is used to get information about the currently
   installed locales.  */
#define _IO_IMBUE(FP, LOCALE) JUMP1(__imbue, FP, LOCALE)
#define _IO_WIMBUE(FP, LOCALE) WJUMP1(__imbue, FP, LOCALE)

/* We always allocate an extra word following an _IO_FILE.
   This contains a pointer to the function jump table used.
   This is for compatibility with C++ streambuf; the word can
   be used to smash to a pointer to a virtual function table. */
#define JUMP_FIELD(TYPE, NAME) TYPE NAME
struct _IO_jump_t {
  JUMP_FIELD(size_t, __dummy);
  JUMP_FIELD(size_t, __dummy2);
  JUMP_FIELD(_IO_finish_t, __finish);
  JUMP_FIELD(_IO_overflow_t, __overflow);
  JUMP_FIELD(_IO_underflow_t, __underflow);
  JUMP_FIELD(_IO_underflow_t, __uflow);
  JUMP_FIELD(_IO_pbackfail_t, __pbackfail);
  /* showmany */
  JUMP_FIELD(_IO_xsputn_t, __xsputn);
  JUMP_FIELD(_IO_xsgetn_t, __xsgetn);
  JUMP_FIELD(_IO_seekoff_t, __seekoff);
  JUMP_FIELD(_IO_seekpos_t, __seekpos);
  JUMP_FIELD(_IO_setbuf_t, __setbuf);
  JUMP_FIELD(_IO_sync_t, __sync);
  JUMP_FIELD(_IO_doallocate_t, __doallocate);
  JUMP_FIELD(_IO_read_t, __read);
  JUMP_FIELD(_IO_write_t, __write);
  JUMP_FIELD(_IO_seek_t, __seek);
  JUMP_FIELD(_IO_close_t, __close);
  JUMP_FIELD(_IO_stat_t, __stat);
  JUMP_FIELD(_IO_showmanyc_t, __showmanyc);
  JUMP_FIELD(_IO_imbue_t, __imbue);
};

struct _IO_FILE_plus {
  FILE file;
  const struct _IO_jump_t *vtable;
};

#include <fcntl.h>
#include <gconv.h>

typedef union {
  struct __gconv_info __cd;
  struct {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _IO_iconv_t;

/* The order of the elements in the following struct must match the order
   of the virtual functions in the libstdc++ codecvt class.  */
struct _IO_codecvt {
  void (*__codecvt_destr)(struct _IO_codecvt *);

  enum __codecvt_result (*__codecvt_do_out)(struct _IO_codecvt *, __mbstate_t *,
                                            const wchar_t *, const wchar_t *,
                                            const wchar_t **, char *, char *,
                                            char **);

  enum __codecvt_result (*__codecvt_do_unshift)(struct _IO_codecvt *,
                                                __mbstate_t *, char *, char *,
                                                char **);

  enum __codecvt_result (*__codecvt_do_in)(struct _IO_codecvt *, __mbstate_t *,
                                           const char *, const char *,
                                           const char **, wchar_t *, wchar_t *,
                                           wchar_t **);

  int (*__codecvt_do_encoding)(struct _IO_codecvt *);

  int (*__codecvt_do_always_noconv)(struct _IO_codecvt *);

  int (*__codecvt_do_length)(struct _IO_codecvt *, __mbstate_t *, const char *,
                             const char *, size_t);

  int (*__codecvt_do_max_length)(struct _IO_codecvt *);

  _IO_iconv_t __cd_in;
  _IO_iconv_t __cd_out;
};

/* Extra data for wide character streams.  */
struct _IO_wide_data {
  wchar_t *_IO_read_ptr;   /* Current read pointer */
  wchar_t *_IO_read_end;   /* End of get area. */
  wchar_t *_IO_read_base;  /* Start of putback+get area. */
  wchar_t *_IO_write_base; /* Start of put area. */
  wchar_t *_IO_write_ptr;  /* Current put pointer. */
  wchar_t *_IO_write_end;  /* End of put area. */
  wchar_t *_IO_buf_base;   /* Start of reserve area. */
  wchar_t *_IO_buf_end;    /* End of reserve area. */
  /* The following fields are used to support backing up and undo. */
  wchar_t *_IO_save_base;   /* Pointer to start of non-current get area. */
  wchar_t *_IO_backup_base; /* Pointer to first valid character of
                              backup area */
  wchar_t *_IO_save_end;    /* Pointer to end of non-current get area. */

  __mbstate_t _IO_state;
  __mbstate_t _IO_last_state;
  struct _IO_codecvt _codecvt;

  wchar_t _shortbuf[1];

  const struct _IO_jump_t *_wide_vtable;
};

#define _IO_JUMPS(THIS) (THIS)->vtable
#define __set_errno(val) (errno = (val))
#define _IO_mask_flags(fp, f, mask)                                            \
  ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))

static void _IO_old_init(FILE *fp, int flags) {
  fp->_flags = _IO_MAGIC | flags;
  fp->_flags2 = 0;
  // if (stdio_needs_locking)
  fp->_flags2 |= 128; //_IO_FLAGS2_NEED_LOCK;
  fp->_IO_buf_base = NULL;
  fp->_IO_buf_end = NULL;
  fp->_IO_read_base = NULL;
  fp->_IO_read_ptr = NULL;
  fp->_IO_read_end = NULL;
  fp->_IO_write_base = NULL;
  fp->_IO_write_ptr = NULL;
  fp->_IO_write_end = NULL;
  fp->_chain = NULL; /* Not necessary. */

  fp->_IO_save_base = NULL;
  fp->_IO_backup_base = NULL;
  fp->_IO_save_end = NULL;
  fp->_markers = NULL;
  fp->_cur_column = 0;
#if _IO_JUMPS_OFFSET
  fp->_vtable_offset = 0;
#endif
#ifdef _IO_MTSAFE_IO
  if (fp->_lock != NULL)
    _IO_lock_init(*fp->_lock);
#endif
}

static void _IO_no_init(FILE *fp, int flags, int orientation,
                        struct _IO_wide_data *wd,
                        const struct _IO_jump_t *jmp) {
  _IO_old_init(fp, flags);
  fp->_mode = orientation;
  if (orientation >= 0) {
    fp->__pad2 = wd;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_buf_base = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_buf_end = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_read_base = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_read_ptr = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_read_end = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_write_base = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_write_ptr = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_write_end = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_save_base = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_backup_base = NULL;
    ((struct _IO_wide_data *)fp->__pad2)->_IO_save_end = NULL;

    ((struct _IO_wide_data *)fp->__pad2)->_wide_vtable = jmp;
  } else
    /* Cause predictable crash when a wide function is called on a byte
       stream.  */
    fp->__pad2 = (struct _IO_wide_data *)-1L;
  fp->__pad3 = NULL;
}

extern const struct _IO_jump_t _IO_wfile_jumps;
extern const struct _IO_jump_t _IO_file_jumps;
struct _IO_jump_t _IO_file_jumps_injected;

ssize_t original_read(FILE *file, void *buf, ssize_t size) {
  return _IO_file_jumps.__read(file, buf, size);
}

ssize_t original_write(FILE *file, const void *buf, ssize_t size) {
  return _IO_file_jumps.__write(file, buf, size);
}

off64_t original_seek(FILE *file, off64_t offset, int whence) {
  return _IO_file_jumps.__seek(file, offset, whence);
}

int original_close(FILE *file) { return _IO_file_jumps.__close(file); }

int original_stat(FILE *file, void *buf) {
  return _IO_file_jumps.__stat(file, buf);
}

void __attribute__((constructor)) init() {
  memcpy(&_IO_file_jumps_injected, &_IO_file_jumps, sizeof(struct _IO_jump_t));
}

FILE *fdopen_injected(int fd, const char *mode) {
  int read_write;
  struct locked_FILE {
    struct _IO_FILE_plus fp;
    _IO_lock_t lock;
    struct _IO_wide_data wd;
  } * new_f;
  int i;
  int use_mmap = 0;
  bool do_seek = false;

  switch (*mode) {
  case 'r':
    read_write = _IO_NO_WRITES;
    break;
  case 'w':
    read_write = _IO_NO_READS;
    break;
  case 'a':
    read_write = _IO_NO_READS | _IO_IS_APPENDING;
    break;
  default:
    __set_errno(EINVAL);
    return NULL;
  }
  for (i = 1; i < 5; ++i) {
    switch (*++mode) {
    case '\0':
      break;
    case '+':
      read_write &= _IO_IS_APPENDING;
      break;
    case 'm':
      use_mmap = 1;
      continue;
    case 'x':
    case 'b':
    default:
      /* Ignore */
      continue;
    }
    break;
  }
  int fd_flags = fcntl(fd, F_GETFL);
  if (fd_flags == -1)
    return NULL;
  if (((fd_flags & O_ACCMODE) == O_RDONLY && !(read_write & _IO_NO_WRITES)) ||
      ((fd_flags & O_ACCMODE) == O_WRONLY && !(read_write & _IO_NO_READS))) {
    __set_errno(EINVAL);
    return NULL;
  }

  if ((read_write & _IO_IS_APPENDING) && !(fd_flags & O_APPEND)) {
    do_seek = true;
    if (fcntl(fd, F_SETFL, fd_flags | O_APPEND) == -1)
      return NULL;
  }

  new_f = (struct locked_FILE *)malloc(sizeof(struct locked_FILE));
  if (new_f == NULL)
    return NULL;

  new_f->fp.file._lock = &new_f->lock;

  extern void _IO_no_init(FILE * fp, int flags, int orientation,
                          struct _IO_wide_data *wd,
                          const struct _IO_jump_t *jmp);
  _IO_no_init(&new_f->fp.file, 0, 0, &new_f->wd, &_IO_wfile_jumps);
  _IO_JUMPS(&new_f->fp) = &_IO_file_jumps_injected;

  extern void _IO_file_init(struct _IO_FILE_plus * fp);
  _IO_file_init(&new_f->fp);

  new_f->fp.file._fileno = fd;
  new_f->fp.file._flags &= ~_IO_DELETE_DONT_CLOSE;

  _IO_mask_flags(&new_f->fp.file, read_write,
                 _IO_NO_READS + _IO_NO_WRITES + _IO_IS_APPENDING);

  /* For append mode, set the file offset to the end of the file if we added
     O_APPEND to the file descriptor flags.  Don't update the offset cache
     though, since the file handle is not active.  */
  if (do_seek && ((read_write & (_IO_IS_APPENDING | _IO_NO_READS)) ==
                  (_IO_IS_APPENDING | _IO_NO_READS))) {
    off64_t new_pos = _IO_SYSSEEK(&new_f->fp.file, 0, _IO_seek_end);
    if (new_pos == _IO_pos_BAD && errno != ESPIPE)
      return NULL;
  }
  return &new_f->fp.file;
}

FILE *fopen_injected(const char *filename, const char *mode) {
  FILE *file = fopen(filename, mode);
  if (!file)
    return NULL;
  _IO_JUMPS_FILE_plus(file) = &_IO_file_jumps_injected;
  return file;
}

void inject_read(_IO_read_t func) { _IO_file_jumps_injected.__read = func; }

void inject_write(_IO_write_t func) { _IO_file_jumps_injected.__write = func; }

void inject_seek(_IO_seek_t func) { _IO_file_jumps_injected.__seek = func; }

void inject_close(_IO_close_t func) { _IO_file_jumps_injected.__close = func; }

void inject_stat(_IO_stat_t func) { _IO_file_jumps_injected.__stat = func; }
#else

#include "hack.h"
#include <errno.h>
#include <fcntl.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>

static _IO_read_t _saved_read = read;

static _IO_seek_t _saved_seek = lseek;

static _IO_close_t _saved_close = close;

static _IO_write_t _saved_write = write;

static _IO_open_t _saved_open = open;

static _IO_fcntl_t _saved_fcntl = fcntl;

void inject_read(_IO_read_t func) { _saved_read = func; }

void inject_write(_IO_write_t func) { _saved_write = func; }

void inject_seek(_IO_seek_t func) { _saved_seek = func; }

void inject_close(_IO_close_t func) { _saved_close = func; }

void inject_open(_IO_open_t func) { _saved_open = func; }

void inject_fcntl(_IO_fcntl_t func) { _saved_fcntl = func; }

int _read_vfunc(void *cookie, char *buf, int n) {
  FILE *fp = cookie;
  return (_saved_read(fp->_file, buf, (size_t)n));
}

int _write_vfunc(void *cookie, const char *buf, int n) {
  FILE *fp = cookie;
  return (_saved_write(fp->_file, buf, (size_t)n));
}

fpos_t _seek_vfunc(void *cookie, fpos_t offset, int whence) {
  FILE *fp = cookie;
  return (_saved_seek(fp->_file, (off_t)offset, whence));
}

int _close_vfunc(void *cookie) { return _saved_close(((FILE *)cookie)->_file); }

extern int __sflags(const char *, int *);
extern FILE *__sfp(int);
#define _counted _extra->counted

/* hold a buncha junk that would grow the ABI */
struct __sFILEX {
  unsigned char *up;        /* saved _p when _p is doing ungetc data */
  pthread_mutex_t fl_mutex; /* used for MT-safety */
  int orientation : 2;      /* orientation for fwide() */
  int counted : 1;          /* stream counted against STREAM_MAX */
  mbstate_t mbstate;        /* multibyte conversion state */
};

static int __scounted = 3;
static pthread_mutex_t filelist_lock = PTHREAD_MUTEX_INITIALIZER;
#define FILELIST_LOCK()                                                        \
  do {                                                                         \
    pthread_mutex_lock(&filelist_lock);                                        \
  } while (0)
#define FILELIST_UNLOCK()                                                      \
  do {                                                                         \
    pthread_mutex_unlock(&filelist_lock);                                      \
  } while (0)

extern void __sinit(void);
extern pthread_once_t __sdidinit;

static int __stream_max;
struct glue {
  struct glue *next;
  int niobs;
  FILE *iobs;
};
static FILE usual[FOPEN_MAX - 3];
static struct glue uglue = {NULL, FOPEN_MAX - 3, usual};
extern struct glue __sglue;
// assume f_prealloc is never called
static struct glue *lastglue = &uglue;

#define INITEXTRA(fp)                                                          \
  do {                                                                         \
    (fp)->_extra->up = NULL;                                                   \
    (fp)->_extra->fl_mutex =                                                   \
        (pthread_mutex_t)PTHREAD_RECURSIVE_MUTEX_INITIALIZER;                  \
    (fp)->_extra->orientation = 0;                                             \
    memset(&(fp)->_extra->mbstate, 0, sizeof(mbstate_t));                      \
    (fp)->_extra->counted = 0;                                                 \
  } while (0);

static struct glue *moreglue(int n) {
  struct glue *g;
  FILE *p;
  struct __sFILEX *fx;
  size_t align;

  align = __alignof__((struct {
    FILE f;
    struct __sFILEX s;
  }){});
  g = (struct glue *)malloc(sizeof(*g) + align + n * sizeof(FILE) +
                            n * sizeof(struct __sFILEX));
  if (g == NULL)
    return (NULL);
  p = (FILE *)roundup((uintptr_t)(g + 1), align);
  fx = (struct __sFILEX *)&p[n];
  g->next = NULL;
  g->niobs = n;
  g->iobs = p;

  while (--n >= 0) {
    bzero(p, sizeof(*p));
    p->_extra = fx;
    INITEXTRA(p);
    p++, fx++;
  }
  return (g);
}

#define NDYNAMIC 10

/*
 * Find a free FILE for fopen et al.
 */
FILE *__sfp(int count) {
  FILE *fp;
  int n;
  struct glue *g;

  pthread_once(&__sdidinit, __sinit);

  if (count) {
    int32_t new = OSAtomicIncrement32(&__scounted);
    __stream_max = sysconf(_SC_STREAM_MAX);
    if (new > __stream_max) {
      if (new > (__stream_max = sysconf(_SC_STREAM_MAX))) {
        OSAtomicDecrement32(&__scounted);
        errno = EMFILE;
        return NULL;
      }
    }
  }
  /*
   * The list must be locked because a FILE may be updated.
   */
  FILELIST_LOCK();
  for (g = &__sglue; g != NULL; g = g->next) {
    for (fp = g->iobs, n = g->niobs; --n >= 0; fp++)
      if (fp->_flags == 0)
        goto found;
  }
  FILELIST_UNLOCK(); /* don't hold lock while malloc()ing. */
  if ((g = moreglue(NDYNAMIC)) == NULL)
    return (NULL);
  FILELIST_LOCK();    /* reacquire the lock */
  lastglue->next = g; /* atomically append glue to list */
  lastglue = g;       /* not atomic; only accessed when locked */
  fp = g->iobs;
found:
  fp->_flags = 1; /* reserve this slot; caller sets real flags */
  FILELIST_UNLOCK();

  /* _flags = 1 means the FILE* is in use, and this thread owns the object while
   * it is being initialized */
  fp->_p = NULL; /* no current pointer */
  fp->_w = 0;    /* nothing to read or write */
  fp->_r = 0;
  fp->_bf._base = NULL; /* no buffer */
  fp->_bf._size = 0;
  fp->_lbfsize = 0;             /* not line buffered */
  fp->_file = -1;               /* no file */
  /*	fp->_cookie = <any>; */ /* caller sets cookie, _read/_write etc */
  fp->_ub._base = NULL;         /* no ungetc buffer */
  fp->_ub._size = 0;
  fp->_lb._base = NULL; /* no line buffer */
  fp->_lb._size = 0;
  /*	fp->_lock = NULL; */ /* once set always set (reused) */
  INITEXTRA(fp);
  fp->_extra->counted = count ? 1 : 0;
  return (fp);
}

static void __sfprelease(FILE *fp) {
  if (fp->_counted) {
    OSAtomicDecrement32(&__scounted);
    fp->_counted = 0;
  }

  pthread_mutex_destroy(&fp->_extra->fl_mutex);

  /* Make sure nobody else is enumerating the list while we clear the "in use"
   * _flags field. */
  FILELIST_LOCK();
  fp->_flags = 0;
  FILELIST_UNLOCK();
}
extern int _sread(FILE *, char *, int);
extern int _swrite(FILE *, const char *, int);
extern int _sseek(FILE *, fpos_t, int);

#define COUNT 1

FILE *fopen_injected(const char *filename, const char *mode) {
  FILE *fp;
  int f;
  int flags, oflags;

  if ((flags = __sflags(mode, &oflags)) == 0)
    return (NULL);
  if ((fp = __sfp(COUNT)) == NULL)
    return (NULL);
  if ((f = _saved_open(filename, oflags, DEFFILEMODE)) < 0) {
    __sfprelease(fp); /* release */
    return (NULL);
  }
  /*
   * File descriptors are a full int, but _file is only a short.
   * If we get a valid file descriptor that is greater than
   * SHRT_MAX, then the fd will get sign-extended into an
   * invalid file descriptor.  Handle this case by failing the
   * open.
   */
  if (f > SHRT_MAX) {
    fp->_flags = 0; /* release */
    _saved_close(f);
    errno = EMFILE;
    return (NULL);
  }
  fp->_file = f;
  fp->_flags = flags;
  fp->_cookie = fp;
  fp->_read = _read_vfunc;
  fp->_write = _write_vfunc;
  fp->_seek = _seek_vfunc;
  fp->_close = _close_vfunc;
  /*
   * When opening in append mode, even though we use O_APPEND,
   * we need to seek to the end so that ftell() gets the right
   * answer.  If the user then alters the seek pointer, or
   * the file extends, this will fail, but there is not much
   * we can do about this.  (We could set __SAPP and check in
   * fseek and ftell.)
   */
  if (oflags & O_APPEND)
    (void)_sseek(fp, (fpos_t)0, SEEK_END);
  return (fp);
}

FILE *fdopen_injected(int fd, const char *mode) {
  FILE *fp;
  int flags, oflags, fdflags, tmp;

  /*
   * File descriptors are a full int, but _file is only a short.
   * If we get a valid file descriptor that is greater than
   * SHRT_MAX, then the fd will get sign-extended into an
   * invalid file descriptor.  Handle this case by failing the
   * open.
   */
  if (fd > SHRT_MAX) {
    errno = EMFILE;
    return (NULL);
  }

  if ((flags = __sflags(mode, &oflags)) == 0)
    return (NULL);

  /* Make sure the mode the user wants is a subset of the actual mode. */
  if ((fdflags = _saved_fcntl(fd, F_GETFL, 0)) < 0)
    return (NULL);
  tmp = fdflags & O_ACCMODE;
  if (tmp != O_RDWR && (tmp != (oflags & O_ACCMODE))) {
    errno = EINVAL;
    return (NULL);
  }

  if ((fp = __sfp(COUNT)) == NULL)
    return (NULL);
  fp->_flags = flags;
  /*
   * If opened for appending, but underlying descriptor does not have
   * O_APPEND bit set, assert __SAPP so that __swrite() caller
   * will _sseek() to the end before write.
   */
  if ((oflags & O_APPEND) && !(fdflags & O_APPEND))
    fp->_flags |= __SAPP;
  fp->_file = fd;
  fp->_cookie = fp;
  fp->_read = _read_vfunc;
  fp->_write = _write_vfunc;
  fp->_seek = _seek_vfunc;
  fp->_close = _close_vfunc;
  return (fp);
}

#endif
