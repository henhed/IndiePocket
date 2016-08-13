/* Copyright (C) 2016 Henrik Hedelund.

   This file is part of IndiePocket.

   IndiePocket is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   IndiePocket is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with IndiePocket.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef PCKT_UTIL_H
#define PCKT_UTIL_H 1

#include <stdarg.h>
#include <string.h>
#include "pckt.h"

#ifdef _WIN32
# define PCKT_DIR_SEP '\\'
# define PCKT_BAD_DIR_SEP '/'
#else
# define PCKT_DIR_SEP '/'
# define PCKT_BAD_DIR_SEP '\\'
#endif

__BEGIN_DECLS

extern char *pckt_vstrdupf (const char *, va_list);
extern char *pckt_strdupf (const char *, ...);
extern float pckt_strtof (const char *, char **);

static inline char *
pckt_fix_path (char *path)
{
  char *dirsep;
  if (path)
    {
      while ((dirsep = strchr (path, PCKT_BAD_DIR_SEP)))
        *dirsep = PCKT_DIR_SEP;
    }
  return path;
}

__END_DECLS

#endif /* ! PCKT_UTIL_H */
