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

#ifndef OPTIMFROGWRAP_H
#define OPTIMFROGWRAP_H

#include <cstdio>
#include <exception>
#include <map>
#include <string>

#include <deadbeef/deadbeef.h>

#include <OptimFROG/OptimFROG.h>

extern DB_functions_t *deadbeef;

class FrogWrap
{
  public:
    class InvalidFile : public std::exception
    {
      public:
        InvalidFile() : std::exception() { }
    };

    explicit FrogWrap(const char *uri);
    FrogWrap(const FrogWrap &) = delete;
    FrogWrap &operator=(const FrogWrap &) = delete;
    ~FrogWrap();

    DB_FILE *get_file() { return file; }

    static bool can_play(std::string);

    long read(void *, long);
    bool seekable();
    void seek(int);

    long rate() { return info.samplerate; }
    long channels() { return info.channels; }
    long depth() { return info.bitspersample; }
    long length() { return info.length_ms; }
    long bitrate() { return info.bitrate; }
    long version() { return info.version; }
    double compression() { return 1000.0 * bitrate() / rate() / channels() / depth(); }

    bool has_tags() { return !tags.empty(); }
    std::string get_tag(std::string tag) { return tags.at(tag); }

  private:
    void *decoder = OptimFROG_createInstance();
    OptimFROG_Info info;
    bool is_signed;

    DB_FILE *file;

    std::map<std::string, std::string> tags;

    static condition_t ofr_close(void *f) { return C_TRUE; }
    static sInt32_t ofr_read(void *f, void *buf, uInt32_t n) { return deadbeef->fread(buf, 1, n, reinterpret_cast<DB_FILE *>(f)); }
    static condition_t ofr_eof(void* f) { return C_FALSE; }
    static condition_t ofr_seekable(void* f) { return !(reinterpret_cast<DB_FILE *>(f))->vfs->is_streaming(); }
    static sInt64_t ofr_length(void* f) { return deadbeef->fgetlength(reinterpret_cast<DB_FILE *>(f)); }
    static sInt64_t ofr_get_pos(void* f) { return deadbeef->ftell(reinterpret_cast<DB_FILE *>(f)); }
    static condition_t ofr_seek(void* f, sInt64_t offset) { return deadbeef->fseek(reinterpret_cast<DB_FILE *>(f), offset, SEEK_SET) >= 0; }
};

#endif
