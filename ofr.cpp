/*-
 * Copyright (c) 2015 Chris Spiegel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <cstdint>

#include <deadbeef/deadbeef.h>

#include <OptimFROG/OptimFROG.h>

#include "copyright.c"

#include "frogwrap.h"

DB_functions_t *deadbeef;

static DB_decoder_t plugin;

struct info
{
  DB_fileinfo_t info;
  FrogWrap *wrap;
};

class PlaylistLock
{
  public:
    explicit PlaylistLock(DB_functions_t *deadbeef) : deadbeef(deadbeef) { deadbeef->pl_lock(); }
    ~PlaylistLock() { deadbeef->pl_unlock(); }

  private:
    DB_functions_t *deadbeef;
};

static DB_fileinfo_t *ofr_open(std::uint32_t hints)
{
  return reinterpret_cast<DB_fileinfo_t *>(new struct info());
}

static int ofr_init(DB_fileinfo_t *info_, DB_playItem_t *it)
{
  const char *uri;
  struct info *info = reinterpret_cast<struct info *>(info_);

  {
    PlaylistLock lock(deadbeef);

    uri = deadbeef->pl_find_meta(it, ":URI");
    if(uri == nullptr) return -1;
  }

  try
  {
    info->wrap = new FrogWrap(uri);
  }
  catch(const FrogWrap::InvalidFile &)
  {
    return -1;
  }

  info->info.fmt.bps = info->wrap->depth();
  info->info.fmt.channels = info->wrap->channels();
  info->info.fmt.samplerate = info->wrap->rate();
  for(long i = 0; i < info->wrap->channels(); i++) info->info.fmt.channelmask |= 1 << i;
  info->info.readpos = 0;
  info->info.plugin = &plugin;

  return 0;
}

static void ofr_free(DB_fileinfo_t *info_)
{
  struct info *info = reinterpret_cast<struct info *>(info_);

  delete info->wrap;
  delete info;
}

static int ofr_read(DB_fileinfo_t *info_, char *buf, int size)
{
  struct info *info = reinterpret_cast<struct info *>(info_);

  return info->wrap->read(buf, size);
}

static int ofr_seek(DB_fileinfo_t *info_, float time)
{
  struct info *info = reinterpret_cast<struct info *>(info_);

  if(!info->wrap->seekable()) return -1;

  info->wrap->seek(time * 1000);

  return 0;
}

static DB_playItem_t *ofr_insert(ddb_playlist_t *plt, DB_playItem_t *after, const char *fname)
{
  try
  {
    FrogWrap wrap(fname);
    DB_playItem_t *it;

    it = deadbeef->pl_item_alloc_init(fname, plugin.plugin.id);
    deadbeef->plt_set_item_duration(plt, it, wrap.length() / 1000.0);
    deadbeef->pl_add_meta(it, ":FILETYPE", "OptimFROG");
    deadbeef->pl_set_meta_int(it, ":BPS", wrap.depth());
    deadbeef->pl_set_meta_int(it, ":CHANNELS", wrap.channels());
    deadbeef->pl_set_meta_int(it, ":SAMPLERATE", wrap.rate());
    deadbeef->pl_set_meta_int(it, ":BITRATE", wrap.depth());

    deadbeef->junk_apev2_read(it, wrap.get_file());

    after = deadbeef->plt_insert_item(plt, after, it);
    deadbeef->pl_item_unref(it);

    return after;
  }
  catch(const FrogWrap::InvalidFile &)
  {
    return nullptr;
  }
}

static int ofr_read_metadata(DB_playItem_t *it)
{
  int ret;
  DB_FILE *fp;

  {
    PlaylistLock lock(deadbeef);

    fp = deadbeef->fopen(deadbeef->pl_find_meta(it, ":URI"));
    if(fp == nullptr) return -1;
  }

  deadbeef->pl_delete_all_meta(it);
  ret = deadbeef->junk_apev2_read(it, fp);
  deadbeef->fclose(fp);

  return ret;
}

static int ofr_write_metadata(DB_playItem_t *it)
{
  return deadbeef->junk_rewrite_tags(it, JUNK_WRITE_APEV2, 0, nullptr);
}

extern "C"
{
  DB_plugin_t *cas_ofr_load(DB_functions_t *api);
}

DB_plugin_t *cas_ofr_load(DB_functions_t *api)
{
  static const char *exts[] = { "ofr", "ofs", nullptr };

  plugin.plugin.api_vmajor = 1,
  plugin.plugin.api_vminor = 0,
  plugin.plugin.version_major = 1,
  plugin.plugin.version_minor = 0,
  plugin.plugin.type = DB_PLUGIN_DECODER,
  plugin.plugin.id = "cas_ofr",
  plugin.plugin.name = "OptimFROG player",
  plugin.plugin.descr = "OptimFROG player",
  plugin.plugin.copyright = copyright,
  plugin.plugin.website = "https://github.com/cspiegel/deadbeef-optimfrog",
  plugin.open = ofr_open,
  plugin.init = ofr_init,
  plugin.free = ofr_free,
  plugin.read = ofr_read,
  plugin.seek = ofr_seek,
  plugin.insert = ofr_insert,
  plugin.read_metadata = ofr_read_metadata,
  plugin.write_metadata = ofr_write_metadata,
  plugin.exts = exts;

  deadbeef = api;

  return DB_PLUGIN(&plugin);
}
