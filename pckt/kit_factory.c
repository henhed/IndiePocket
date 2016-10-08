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
#include <stdio.h>
#include "kit_factory.h"
#include "util.h"

#define NUM_PARSERS 2

extern PcktKitParserIface *pckt_kit_parser_bfk_new (const PcktKitFactory *);
extern PcktKitParserIface *pckt_kit_parser_ttl_new (const PcktKitFactory *);

typedef struct _DrumMetaHandle DrumMetaHandle;
struct _DrumMetaHandle {
  PcktDrumMeta *meta;
  const void *handle;
  DrumMetaHandle *next;
};

struct PcktKitFactoryImpl {
  char *filename;
  char *_basedir;
  char *basedir;
  PcktKitParserIface *parser;
  DrumMetaHandle *meta_handles;
};

PcktKitFactory *
pckt_kit_factory_new (const char *filename, PcktStatus *status)
{
  PcktKitFactory *factory = malloc (sizeof (PcktKitFactory));
  PcktKitParserCtor parsers[NUM_PARSERS] = {
    pckt_kit_parser_bfk_new,
    pckt_kit_parser_ttl_new
  };

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
  factory->_basedir = strdup (factory->filename);
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
  DrumMetaHandle **item;

  if (!factory)
    return;

  item = &factory->meta_handles;
  while (*item)
    {
      DrumMetaHandle *handle = *item;
      *item = (*item)->next;
      free (handle);
    }

  if (factory->parser && factory->parser->free)
    factory->parser->free (factory->parser, factory);
  if (factory->filename)
    free (factory->filename);
  if (factory->_basedir)
    free (factory->_basedir);
  free (factory);
}

static const void *
pckt_kit_factory_add_drum_meta (PcktKitFactory *factory, PcktDrumMeta *meta,
                                const void *handle)
{
  DrumMetaHandle **item;

  if (!factory || !meta)
    return NULL;

  item = &factory->meta_handles;
  while (*item)
    {
      if ((*item)->meta == meta)
        {
          const void *old_handle = (*item)->handle;
          (*item)->handle = handle;
          return old_handle ? old_handle : handle;
        }
      item = &(*item)->next;
    }

  *item = malloc (sizeof (DrumMetaHandle));
  if (*item)
    {
      (*item)->meta = meta;
      (*item)->handle = handle;
      (*item)->next = NULL;
      return handle;
    }

  return NULL;
}

PcktStatus
pckt_kit_factory_load_metas (PcktKitFactory *factory, PcktKit *kit)
{
  PcktStatus status;
  DrumMetaHandle **handle;

  if (!factory || !kit)
    return PCKTE_INVAL;

  if (!factory->parser || !factory->parser->load_metas)
    return PCKTE_INTERNAL;

  status = factory->parser->load_metas (factory->parser, factory,
                                        pckt_kit_factory_add_drum_meta);
  handle = &factory->meta_handles;

  while (*handle)
    {
      if (pckt_kit_add_drum_meta (kit, (*handle)->meta) < 0)
        {
          DrumMetaHandle *failed = *handle;
          *handle = (*handle)->next;
          fprintf (stderr, __FILE__ ": Could not add \"%s\" to kit\n",
                   pckt_drum_meta_get_name (failed->meta));
          pckt_drum_meta_free (failed->meta);
          free (failed);
          continue;
        }
      handle = &(*handle)->next;
    }

  return status;
}

PcktStatus
pckt_kit_factory_load_drums (PcktKitFactory *factory, PcktDrumMeta *meta,
                             PcktKitFactoryDrumCb callback, void *user_handle)
{
  DrumMetaHandle **item, *meta_handle = NULL;

  if (!factory || !meta || !callback)
    return PCKTE_INVAL;

  if (!factory->parser || !factory->parser->load_drums)
    return PCKTE_INTERNAL;

  item = &factory->meta_handles;

  while (*item)
    {
      if ((*item)->meta == meta)
        {
          meta_handle = *item;
          *item = (*item)->next;
          break;
        }
      item = &(*item)->next;
    }

  if (!meta_handle)
    return PCKTE_INVAL;

  factory->parser->load_drums (factory->parser, meta_handle->meta,
                               meta_handle->handle, callback, user_handle);
  free (meta_handle);

  return PCKTE_SUCCESS;
}

static void
pckt_kit_factory_add_drum (void *data, PcktDrum *drum, int8_t id,
                           const int8_t *chokers, size_t nchokers)
{
  PcktKit *kit = (PcktKit *) data;

  if (pckt_kit_get_drum (kit, id))
    {
      fprintf (stderr, __FILE__ ": Drum with ID %d already exists\n", id);
      pckt_drum_free (drum);
    }
  else
    {
      pckt_kit_add_drum (kit, drum, id);
      for (uint8_t i = 0; i < nchokers; ++i)
        pckt_kit_set_choke (kit, chokers[i], id, true);
    }
}

PcktKit *
pckt_kit_factory_load (PcktKitFactory *factory)
{
  PcktKit *kit;

  if (!factory)
    return NULL;

  kit = pckt_kit_new ();
  if (!kit)
    return NULL;

  if (pckt_kit_factory_load_metas (factory, kit) != PCKTE_SUCCESS)
    {
      pckt_kit_free (kit);
      return NULL;
    }

  PCKT_KIT_EACH_DRUM_META (kit, meta)
    pckt_kit_factory_load_drums (factory, meta, pckt_kit_factory_add_drum, kit);

  return kit;
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
  char *relative;
  char *absolute;

  if (!factory || !path || (strlen (path) == 0))
    return NULL;

  absolute = pckt_strdupf ("%s%c%s", factory->basedir, PCKT_DIR_SEP, path);
  if (!absolute)
    return NULL;

  relative = absolute + strlen (factory->basedir) + 1;
  pckt_fix_path (relative);

  return absolute;
}
