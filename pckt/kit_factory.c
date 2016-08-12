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

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "kit_factory.h"
#include "util.h"

#define NUM_PARSERS 1

#ifdef _WIN32
# define DIR_SEP '\\'
# define BAD_DIR_SEP '/'
#else
# define DIR_SEP '/'
# define BAD_DIR_SEP '\\'
#endif

extern PcktKitParserIface *pckt_kit_parser_ttl_new (const PcktKitFactory *);

struct PcktKitFactoryImpl {
  char *filename;
  char *_basedir;
  char *basedir;
  PcktKitParserIface *parser;
};

PcktKitFactory *
pckt_kit_factory_new (const char *filename, PcktStatus *status)
{
  PcktKitFactory *factory = malloc (sizeof (PcktKitFactory));
  PcktKitParserCtor parsers[NUM_PARSERS] = {pckt_kit_parser_ttl_new};

  if (!factory)
    {
      if (status)
        *status = PCKTE_NOMEM;
      return NULL;
    }

  memset (factory, 0, sizeof (PcktKitFactory));
  factory->filename = realpath (filename, NULL);

  if (!factory->filename)
    {
      if (status)
        *status = PCKTE_INVAL;
      free (factory);
      return NULL;
    }

  /* `_basedir' is only kept as a reference because the pointer returned by
     `dirname' can not be safely passed to `free'.  */
  factory->_basedir = strdup (filename);
  factory->basedir = dirname (factory->_basedir);

  for (uint8_t i = 0; i < NUM_PARSERS; ++i)
    {
      factory->parser = parsers[i](factory);
      if (factory->parser)
        break;
    }

  if (!factory->parser)
    {
      pckt_kit_factory_free (factory);
      factory = NULL;
      if (status)
        *status = PCKTE_INVAL;
    }
  else if (status)
    *status = PCKTE_SUCCESS;

  return factory;
}

void
pckt_kit_factory_free (PcktKitFactory *factory)
{
  if (!factory)
    return;

  if (factory->parser && factory->parser->free)
    factory->parser->free (factory->parser, factory);
  if (factory->filename)
    free (factory->filename);
  if (factory->_basedir)
    free (factory->_basedir);
  free (factory);
}

PcktKit *
pckt_kit_factory_load (const PcktKitFactory *factory)
{
  if (factory && factory->parser && factory->parser->load)
    return factory->parser->load (factory->parser, factory);
  else
    return NULL;
}

const char *
pckt_kit_factory_get_filename (const PcktKitFactory *factory)
{
  return factory ? factory->filename : NULL;
}

const char *
pckt_kit_factory_get_basedir (const PcktKitFactory *factory)
{
  return factory ? factory->basedir : NULL;
}

char *
pckt_kit_factory_get_abspath (const PcktKitFactory *factory, const char *path)
{
  char *dirsep;
  char *relative;
  char *absolute;

  if (!factory || !path || (strlen (path) == 0))
    return NULL;

  absolute = pckt_strdupf ("%s%c%s", factory->basedir, DIR_SEP, path);
  if (!absolute)
    return NULL;

  relative = absolute + strlen (factory->basedir) + 1;
  while ((dirsep = strchr (relative, BAD_DIR_SEP)))
    *dirsep = DIR_SEP;

  return absolute;
}
