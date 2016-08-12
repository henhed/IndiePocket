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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "util.h"

char *
pckt_vstrdupf (const char *format, va_list ap)
{
  char *str;
  char c;
  int length;
  va_list aq;

  va_copy (aq, ap);
  length = vsnprintf (&c, 1, format, aq);
  va_end (aq);
  if (length < 0)
    return NULL;

  str = malloc (length + 1);
  if (str)
    vsprintf (str, format, ap);
  
  return str;
}

char *
pckt_strdupf (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  char *str = pckt_vstrdupf (format, ap);
  va_end (ap);
  return str;
}

static inline float
parse_digits (const char **c)
{
  float sign = 1.f;
  float value = 0.f;

  if (**c == '+')
    ++(*c);
  else if (**c == '-')
    {
      sign = -1.f;
      ++(*c);
    }

  for (; isdigit (**c); ++(*c))
    {
      value *= 10.f;
      value += **c - '0';
    }

  return sign * value;
}

float
pckt_strtof (const char *str, char **endptr)
{
  float value = 0.f;
  float sign = 1.f;
  float div = 10.f;
  const char* c = str;

  while (isspace (*c))
    ++c;

  value = parse_digits (&c);
  if (value < 0.f)
    {
      sign = -1.f;
      value *= sign;
    }

  if (*c == '.')
    {
      for (++c; isdigit (*c); ++c)
        {
          value += (float) (*c - '0') / div;
          div *= 10.f;
        }
    }

  if (tolower (*c) == 'e')
    {
      ++c;
      value *= powf (10.f, parse_digits (&c));
    }

  if (endptr)
    *endptr = (char *) c;

  return sign * value;
}
