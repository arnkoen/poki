#pragma once
#define PK_AUDIO_SINGLE_HEADER
#define SOKOL_NO_DEPRECATED

#if defined(POKI_IMPL) || defined(PK_AUDIO_IMPL)
#define SOKOL_AUDIO_IMPL
#define TMIXER_IMPL

// Only active for implementation.
//FILE_START:vorbis.c
// Ogg Vorbis audio decoder - v1.22 - public domain
// http://nothings.org/stb_vorbis/
//
// Original version written by Sean Barrett in 2007.
//
// Originally sponsored by RAD Game Tools. Seeking implementation
// sponsored by Phillip Bennefall, Marc Andersen, Aaron Baker,
// Elias Software, Aras Pranckevicius, and Sean Barrett.
//
// LICENSE
//
//   See end of file for license information.
//
// Limitations:
//
//   - floor 0 not supported (used in old ogg vorbis files pre-2004)
//   - lossless sample-truncation at beginning ignored
//   - cannot concatenate multiple vorbis streams
//   - sample positions are 32-bit, limiting seekable 192Khz
//       files to around 6 hours (Ogg supports 64-bit)
//
// Feature contributors:
//    Dougall Johnson (sample-exact seeking)
//
// Bugfix/warning contributors:
//    Terje Mathisen     Niklas Frykholm     Andy Hill
//    Casey Muratori     John Bolton         Gargaj
//    Laurent Gomila     Marc LeBlanc        Ronny Chevalier
//    Bernhard Wodo      Evan Balster        github:alxprd
//    Tom Beaumont       Ingo Leitgeb        Nicolas Guillemot
//    Phillip Bennefall  Rohit               Thiago Goulart
//    github:manxorist   Saga Musix          github:infatum
//    Timur Gagiev       Maxwell Koo         Peter Waller
//    github:audinowho   Dougall Johnson     David Reid
//    github:Clownacy    Pedro J. Estebanez  Remi Verschelde
//    AnthoFoxo          github:morlat       Gabriel Ravier
//
// Partial history:
//    1.22    - 2021-07-11 - various small fixes
//    1.21    - 2021-07-02 - fix bug for files with no comments
//    1.20    - 2020-07-11 - several small fixes
//    1.19    - 2020-02-05 - warnings
//    1.18    - 2020-02-02 - fix seek bugs; parse header comments; misc warnings etc.
//    1.17    - 2019-07-08 - fix CVE-2019-13217..CVE-2019-13223 (by ForAllSecure)
//    1.16    - 2019-03-04 - fix warnings
//    1.15    - 2019-02-07 - explicit failure if Ogg Skeleton data is found
//    1.14    - 2018-02-11 - delete bogus dealloca usage
//    1.13    - 2018-01-29 - fix truncation of last frame (hopefully)
//    1.12    - 2017-11-21 - limit residue begin/end to blocksize/2 to avoid large temp allocs in bad/corrupt files
//    1.11    - 2017-07-23 - fix MinGW compilation
//    1.10    - 2017-03-03 - more robust seeking; fix negative ilog(); clear error in open_memory
//    1.09    - 2016-04-04 - back out 'truncation of last frame' fix from previous version
//    1.08    - 2016-04-02 - warnings; setup memory leaks; truncation of last frame
//    1.07    - 2015-01-16 - fixes for crashes on invalid files; warning fixes; const
//    1.06    - 2015-08-31 - full, correct support for seeking API (Dougall Johnson)
//                           some crash fixes when out of memory or with corrupt files
//                           fix some inappropriately signed shifts
//    1.05    - 2015-04-19 - don't define __forceinline if it's redundant
//    1.04    - 2014-08-27 - fix missing const-correct case in API
//    1.03    - 2014-08-07 - warning fixes
//    1.02    - 2014-07-09 - declare qsort comparison as explicitly _cdecl in Windows
//    1.01    - 2014-06-18 - fix stb_vorbis_get_samples_float (interleaved was correct)
//    1.0     - 2014-05-26 - fix memory leaks; fix warnings; fix bugs in >2-channel;
//                           (API change) report sample rate for decode-full-file funcs
//
// See end of file for full version history.


//////////////////////////////////////////////////////////////////////////////
//
//  HEADER BEGINS HERE
//

#ifndef STB_VORBIS_INCLUDE_STB_VORBIS_H
#define STB_VORBIS_INCLUDE_STB_VORBIS_H

#if defined(STB_VORBIS_NO_CRT) && !defined(STB_VORBIS_NO_STDIO)
#define STB_VORBIS_NO_STDIO 1
#endif

#ifndef STB_VORBIS_NO_STDIO
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

///////////   THREAD SAFETY

// Individual stb_vorbis* handles are not thread-safe; you cannot decode from
// them from multiple threads at the same time. However, you can have multiple
// stb_vorbis* handles and decode from them independently in multiple thrads.


///////////   MEMORY ALLOCATION

// normally stb_vorbis uses malloc() to allocate memory at startup,
// and alloca() to allocate temporary memory during a frame on the
// stack. (Memory consumption will depend on the amount of setup
// data in the file and how you set the compile flags for speed
// vs. size. In my test files the maximal-size usage is ~150KB.)
//
// You can modify the wrapper functions in the source (setup_malloc,
// setup_temp_malloc, temp_malloc) to change this behavior, or you
// can use a simpler allocation model: you pass in a buffer from
// which stb_vorbis will allocate _all_ its memory (including the
// temp memory). "open" may fail with a VORBIS_outofmem if you
// do not pass in enough data; there is no way to determine how
// much you do need except to succeed (at which point you can
// query get_info to find the exact amount required. yes I know
// this is lame).
//
// If you pass in a non-NULL buffer of the type below, allocation
// will occur from it as described above. Otherwise just pass NULL
// to use malloc()/alloca()

typedef struct
{
   char *alloc_buffer;
   int   alloc_buffer_length_in_bytes;
} stb_vorbis_alloc;


///////////   FUNCTIONS USEABLE WITH ALL INPUT MODES

typedef struct stb_vorbis stb_vorbis;

typedef struct
{
   unsigned int sample_rate;
   int channels;

   unsigned int setup_memory_required;
   unsigned int setup_temp_memory_required;
   unsigned int temp_memory_required;

   int max_frame_size;
} stb_vorbis_info;

typedef struct
{
   char *vendor;

   int comment_list_length;
   char **comment_list;
} stb_vorbis_comment;

// get general information about the file
extern stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);

// get ogg comments
extern stb_vorbis_comment stb_vorbis_get_comment(stb_vorbis *f);

// get the last error detected (clears it, too)
extern int stb_vorbis_get_error(stb_vorbis *f);

// close an ogg vorbis file and free all memory in use
extern void stb_vorbis_close(stb_vorbis *f);

// this function returns the offset (in samples) from the beginning of the
// file that will be returned by the next decode, if it is known, or -1
// otherwise. after a flush_pushdata() call, this may take a while before
// it becomes valid again.
// NOT WORKING YET after a seek with PULLDATA API
extern int stb_vorbis_get_sample_offset(stb_vorbis *f);

// returns the current seek point within the file, or offset from the beginning
// of the memory buffer. In pushdata mode it returns 0.
extern unsigned int stb_vorbis_get_file_offset(stb_vorbis *f);

///////////   PUSHDATA API

#ifndef STB_VORBIS_NO_PUSHDATA_API

// this API allows you to get blocks of data from any source and hand
// them to stb_vorbis. you have to buffer them; stb_vorbis will tell
// you how much it used, and you have to give it the rest next time;
// and stb_vorbis may not have enough data to work with and you will
// need to give it the same data again PLUS more. Note that the Vorbis
// specification does not bound the size of an individual frame.

extern stb_vorbis *stb_vorbis_open_pushdata(
         const unsigned char * datablock, int datablock_length_in_bytes,
         int *datablock_memory_consumed_in_bytes,
         int *error,
         const stb_vorbis_alloc *alloc_buffer);
// create a vorbis decoder by passing in the initial data block containing
//    the ogg&vorbis headers (you don't need to do parse them, just provide
//    the first N bytes of the file--you're told if it's not enough, see below)
// on success, returns an stb_vorbis *, does not set error, returns the amount of
//    data parsed/consumed on this call in *datablock_memory_consumed_in_bytes;
// on failure, returns NULL on error and sets *error, does not change *datablock_memory_consumed
// if returns NULL and *error is VORBIS_need_more_data, then the input block was
//       incomplete and you need to pass in a larger block from the start of the file

extern int stb_vorbis_decode_frame_pushdata(
         stb_vorbis *f,
         const unsigned char *datablock, int datablock_length_in_bytes,
         int *channels,             // place to write number of float * buffers
         float ***output,           // place to write float ** array of float * buffers
         int *samples               // place to write number of output samples
     );
// decode a frame of audio sample data if possible from the passed-in data block
//
// return value: number of bytes we used from datablock
//
// possible cases:
//     0 bytes used, 0 samples output (need more data)
//     N bytes used, 0 samples output (resynching the stream, keep going)
//     N bytes used, M samples output (one frame of data)
// note that after opening a file, you will ALWAYS get one N-bytes,0-sample
// frame, because Vorbis always "discards" the first frame.
//
// Note that on resynch, stb_vorbis will rarely consume all of the buffer,
// instead only datablock_length_in_bytes-3 or less. This is because it wants
// to avoid missing parts of a page header if they cross a datablock boundary,
// without writing state-machiney code to record a partial detection.
//
// The number of channels returned are stored in *channels (which can be
// NULL--it is always the same as the number of channels reported by
// get_info). *output will contain an array of float* buffers, one per
// channel. In other words, (*output)[0][0] contains the first sample from
// the first channel, and (*output)[1][0] contains the first sample from
// the second channel.
//
// *output points into stb_vorbis's internal output buffer storage; these
// buffers are owned by stb_vorbis and application code should not free
// them or modify their contents. They are transient and will be overwritten
// once you ask for more data to get decoded, so be sure to grab any data
// you need before then.

extern void stb_vorbis_flush_pushdata(stb_vorbis *f);
// inform stb_vorbis that your next datablock will not be contiguous with
// previous ones (e.g. you've seeked in the data); future attempts to decode
// frames will cause stb_vorbis to resynchronize (as noted above), and
// once it sees a valid Ogg page (typically 4-8KB, as large as 64KB), it
// will begin decoding the _next_ frame.
//
// if you want to seek using pushdata, you need to seek in your file, then
// call stb_vorbis_flush_pushdata(), then start calling decoding, then once
// decoding is returning you data, call stb_vorbis_get_sample_offset, and
// if you don't like the result, seek your file again and repeat.
#endif


//////////   PULLING INPUT API

#ifndef STB_VORBIS_NO_PULLDATA_API
// This API assumes stb_vorbis is allowed to pull data from a source--
// either a block of memory containing the _entire_ vorbis stream, or a
// FILE * that you or it create, or possibly some other reading mechanism
// if you go modify the source to replace the FILE * case with some kind
// of callback to your code. (But if you don't support seeking, you may
// just want to go ahead and use pushdata.)

#if !defined(STB_VORBIS_NO_STDIO) && !defined(STB_VORBIS_NO_INTEGER_CONVERSION)
extern int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output);
#endif
#if !defined(STB_VORBIS_NO_INTEGER_CONVERSION)
extern int stb_vorbis_decode_memory(const unsigned char *mem, int len, int *channels, int *sample_rate, short **output);
#endif
// decode an entire file and output the data interleaved into a malloc()ed
// buffer stored in *output. The return value is the number of samples
// decoded, or -1 if the file could not be opened or was not an ogg vorbis file.
// When you're done with it, just free() the pointer returned in *output.

extern stb_vorbis * stb_vorbis_open_memory(const unsigned char *data, int len,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);
// create an ogg vorbis decoder from an ogg vorbis stream in memory (note
// this must be the entire stream!). on failure, returns NULL and sets *error

#ifndef STB_VORBIS_NO_STDIO
extern stb_vorbis * stb_vorbis_open_filename(const char *filename,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);
// create an ogg vorbis decoder from a filename via fopen(). on failure,
// returns NULL and sets *error (possibly to VORBIS_file_open_failure).

extern stb_vorbis * stb_vorbis_open_file(FILE *f, int close_handle_on_close,
                                  int *error, const stb_vorbis_alloc *alloc_buffer);
// create an ogg vorbis decoder from an open FILE *, looking for a stream at
// the _current_ seek point (ftell). on failure, returns NULL and sets *error.
// note that stb_vorbis must "own" this stream; if you seek it in between
// calls to stb_vorbis, it will become confused. Moreover, if you attempt to
// perform stb_vorbis_seek_*() operations on this file, it will assume it
// owns the _entire_ rest of the file after the start point. Use the next
// function, stb_vorbis_open_file_section(), to limit it.

extern stb_vorbis * stb_vorbis_open_file_section(FILE *f, int close_handle_on_close,
                int *error, const stb_vorbis_alloc *alloc_buffer, unsigned int len);
// create an ogg vorbis decoder from an open FILE *, looking for a stream at
// the _current_ seek point (ftell); the stream will be of length 'len' bytes.
// on failure, returns NULL and sets *error. note that stb_vorbis must "own"
// this stream; if you seek it in between calls to stb_vorbis, it will become
// confused.
#endif

extern int stb_vorbis_seek_frame(stb_vorbis *f, unsigned int sample_number);
extern int stb_vorbis_seek(stb_vorbis *f, unsigned int sample_number);
// these functions seek in the Vorbis file to (approximately) 'sample_number'.
// after calling seek_frame(), the next call to get_frame_*() will include
// the specified sample. after calling stb_vorbis_seek(), the next call to
// stb_vorbis_get_samples_* will start with the specified sample. If you
// do not need to seek to EXACTLY the target sample when using get_samples_*,
// you can also use seek_frame().

extern int stb_vorbis_seek_start(stb_vorbis *f);
// this function is equivalent to stb_vorbis_seek(f,0)

extern unsigned int stb_vorbis_stream_length_in_samples(stb_vorbis *f);
extern float        stb_vorbis_stream_length_in_seconds(stb_vorbis *f);
// these functions return the total length of the vorbis stream

extern int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output);
// decode the next frame and return the number of samples. the number of
// channels returned are stored in *channels (which can be NULL--it is always
// the same as the number of channels reported by get_info). *output will
// contain an array of float* buffers, one per channel. These outputs will
// be overwritten on the next call to stb_vorbis_get_frame_*.
//
// You generally should not intermix calls to stb_vorbis_get_frame_*()
// and stb_vorbis_get_samples_*(), since the latter calls the former.

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
extern int stb_vorbis_get_frame_short_interleaved(stb_vorbis *f, int num_c, short *buffer, int num_shorts);
extern int stb_vorbis_get_frame_short            (stb_vorbis *f, int num_c, short **buffer, int num_samples);
#endif
// decode the next frame and return the number of *samples* per channel.
// Note that for interleaved data, you pass in the number of shorts (the
// size of your array), but the return value is the number of samples per
// channel, not the total number of samples.
//
// The data is coerced to the number of channels you request according to the
// channel coercion rules (see below). You must pass in the size of your
// buffer(s) so that stb_vorbis will not overwrite the end of the buffer.
// The maximum buffer size needed can be gotten from get_info(); however,
// the Vorbis I specification implies an absolute maximum of 4096 samples
// per channel.

// Channel coercion rules:
//    Let M be the number of channels requested, and N the number of channels present,
//    and Cn be the nth channel; let stereo L be the sum of all L and center channels,
//    and stereo R be the sum of all R and center channels (channel assignment from the
//    vorbis spec).
//        M    N       output
//        1    k      sum(Ck) for all k
//        2    *      stereo L, stereo R
//        k    l      k > l, the first l channels, then 0s
//        k    l      k <= l, the first k channels
//    Note that this is not _good_ surround etc. mixing at all! It's just so
//    you get something useful.

extern int stb_vorbis_get_samples_float_interleaved(stb_vorbis *f, int channels, float *buffer, int num_floats);
extern int stb_vorbis_get_samples_float(stb_vorbis *f, int channels, float **buffer, int num_samples);
// gets num_samples samples, not necessarily on a frame boundary--this requires
// buffering so you have to supply the buffers. DOES NOT APPLY THE COERCION RULES.
// Returns the number of samples stored per channel; it may be less than requested
// at the end of the file. If there are no more samples in the file, returns 0.

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
extern int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts);
extern int stb_vorbis_get_samples_short(stb_vorbis *f, int channels, short **buffer, int num_samples);
#endif
// gets num_samples samples, not necessarily on a frame boundary--this requires
// buffering so you have to supply the buffers. Applies the coercion rules above
// to produce 'channels' channels. Returns the number of samples stored per channel;
// it may be less than requested at the end of the file. If there are no more
// samples in the file, returns 0.

#endif

////////   ERROR CODES

enum STBVorbisError
{
   VORBIS__no_error,

   VORBIS_need_more_data=1,             // not a real error

   VORBIS_invalid_api_mixing,           // can't mix API modes
   VORBIS_outofmem,                     // not enough memory
   VORBIS_feature_not_supported,        // uses floor 0
   VORBIS_too_many_channels,            // STB_VORBIS_MAX_CHANNELS is too small
   VORBIS_file_open_failure,            // fopen() failed
   VORBIS_seek_without_length,          // can't seek in unknown-length file

   VORBIS_unexpected_eof=10,            // file is truncated?
   VORBIS_seek_invalid,                 // seek past EOF

   // decoding errors (corrupt/invalid stream) -- you probably
   // don't care about the exact details of these

   // vorbis errors:
   VORBIS_invalid_setup=20,
   VORBIS_invalid_stream,

   // ogg errors:
   VORBIS_missing_capture_pattern=30,
   VORBIS_invalid_stream_structure_version,
   VORBIS_continued_packet_flag_invalid,
   VORBIS_incorrect_stream_serial_number,
   VORBIS_invalid_first_page,
   VORBIS_bad_packet_type,
   VORBIS_cant_find_last_page,
   VORBIS_seek_failed,
   VORBIS_ogg_skeleton_not_supported
};


#ifdef __cplusplus
}
#endif

#endif // STB_VORBIS_INCLUDE_STB_VORBIS_H
//
//  HEADER ENDS HERE
//
//////////////////////////////////////////////////////////////////////////////

#ifndef STB_VORBIS_HEADER_ONLY

///NOTE(mendsley): Adding allocation hooks
extern void* tm_vorbis_malloc(size_t sz);
extern void tm_vorbis_free(void* ptr);
extern void* tm_vorbis_temp_malloc(size_t sz);
extern void tm_vorbis_temp_free(void* ptr);

// global configuration settings (e.g. set these in the project/makefile),
// or just set them in this file at the top (although ideally the first few
// should be visible when the header file is compiled too, although it's not
// crucial)

// STB_VORBIS_NO_PUSHDATA_API
//     does not compile the code for the various stb_vorbis_*_pushdata()
//     functions
// #define STB_VORBIS_NO_PUSHDATA_API

// STB_VORBIS_NO_PULLDATA_API
//     does not compile the code for the non-pushdata APIs
// #define STB_VORBIS_NO_PULLDATA_API

// STB_VORBIS_NO_STDIO
//     does not compile the code for the APIs that use FILE *s internally
//     or externally (implied by STB_VORBIS_NO_PULLDATA_API)
// #define STB_VORBIS_NO_STDIO

// STB_VORBIS_NO_INTEGER_CONVERSION
//     does not compile the code for converting audio sample data from
//     float to integer (implied by STB_VORBIS_NO_PULLDATA_API)
// #define STB_VORBIS_NO_INTEGER_CONVERSION

// STB_VORBIS_NO_FAST_SCALED_FLOAT
//      does not use a fast float-to-int trick to accelerate float-to-int on
//      most platforms which requires endianness be defined correctly.
//#define STB_VORBIS_NO_FAST_SCALED_FLOAT


// STB_VORBIS_MAX_CHANNELS [number]
//     globally define this to the maximum number of channels you need.
//     The spec does not put a restriction on channels except that
//     the count is stored in a byte, so 255 is the hard limit.
//     Reducing this saves about 16 bytes per value, so using 16 saves
//     (255-16)*16 or around 4KB. Plus anything other memory usage
//     I forgot to account for. Can probably go as low as 8 (7.1 audio),
//     6 (5.1 audio), or 2 (stereo only).
#ifndef STB_VORBIS_MAX_CHANNELS
#define STB_VORBIS_MAX_CHANNELS    16  // enough for anyone?
#endif

// STB_VORBIS_PUSHDATA_CRC_COUNT [number]
//     after a flush_pushdata(), stb_vorbis begins scanning for the
//     next valid page, without backtracking. when it finds something
//     that looks like a page, it streams through it and verifies its
//     CRC32. Should that validation fail, it keeps scanning. But it's
//     possible that _while_ streaming through to check the CRC32 of
//     one candidate page, it sees another candidate page. This #define
//     determines how many "overlapping" candidate pages it can search
//     at once. Note that "real" pages are typically ~4KB to ~8KB, whereas
//     garbage pages could be as big as 64KB, but probably average ~16KB.
//     So don't hose ourselves by scanning an apparent 64KB page and
//     missing a ton of real ones in the interim; so minimum of 2
#ifndef STB_VORBIS_PUSHDATA_CRC_COUNT
#define STB_VORBIS_PUSHDATA_CRC_COUNT  4
#endif

// STB_VORBIS_FAST_HUFFMAN_LENGTH [number]
//     sets the log size of the huffman-acceleration table.  Maximum
//     supported value is 24. with larger numbers, more decodings are O(1),
//     but the table size is larger so worse cache missing, so you'll have
//     to probe (and try multiple ogg vorbis files) to find the sweet spot.
#ifndef STB_VORBIS_FAST_HUFFMAN_LENGTH
#define STB_VORBIS_FAST_HUFFMAN_LENGTH   10
#endif

// STB_VORBIS_FAST_BINARY_LENGTH [number]
//     sets the log size of the binary-search acceleration table. this
//     is used in similar fashion to the fast-huffman size to set initial
//     parameters for the binary search

// STB_VORBIS_FAST_HUFFMAN_INT
//     The fast huffman tables are much more efficient if they can be
//     stored as 16-bit results instead of 32-bit results. This restricts
//     the codebooks to having only 65535 possible outcomes, though.
//     (At least, accelerated by the huffman table.)
#ifndef STB_VORBIS_FAST_HUFFMAN_INT
#define STB_VORBIS_FAST_HUFFMAN_SHORT
#endif

// STB_VORBIS_NO_HUFFMAN_BINARY_SEARCH
//     If the 'fast huffman' search doesn't succeed, then stb_vorbis falls
//     back on binary searching for the correct one. This requires storing
//     extra tables with the huffman codes in sorted order. Defining this
//     symbol trades off space for speed by forcing a linear search in the
//     non-fast case, except for "sparse" codebooks.
// #define STB_VORBIS_NO_HUFFMAN_BINARY_SEARCH

// STB_VORBIS_DIVIDES_IN_RESIDUE
//     stb_vorbis precomputes the result of the scalar residue decoding
//     that would otherwise require a divide per chunk. you can trade off
//     space for time by defining this symbol.
// #define STB_VORBIS_DIVIDES_IN_RESIDUE

// STB_VORBIS_DIVIDES_IN_CODEBOOK
//     vorbis VQ codebooks can be encoded two ways: with every case explicitly
//     stored, or with all elements being chosen from a small range of values,
//     and all values possible in all elements. By default, stb_vorbis expands
//     this latter kind out to look like the former kind for ease of decoding,
//     because otherwise an integer divide-per-vector-element is required to
//     unpack the index. If you define STB_VORBIS_DIVIDES_IN_CODEBOOK, you can
//     trade off storage for speed.
//#define STB_VORBIS_DIVIDES_IN_CODEBOOK

#ifdef STB_VORBIS_CODEBOOK_SHORTS
#error "STB_VORBIS_CODEBOOK_SHORTS is no longer supported as it produced incorrect results for some input formats"
#endif

// STB_VORBIS_DIVIDE_TABLE
//     this replaces small integer divides in the floor decode loop with
//     table lookups. made less than 1% difference, so disabled by default.

// STB_VORBIS_NO_INLINE_DECODE
//     disables the inlining of the scalar codebook fast-huffman decode.
//     might save a little codespace; useful for debugging
// #define STB_VORBIS_NO_INLINE_DECODE

// STB_VORBIS_NO_DEFER_FLOOR
//     Normally we only decode the floor without synthesizing the actual
//     full curve. We can instead synthesize the curve immediately. This
//     requires more memory and is very likely slower, so I don't think
//     you'd ever want to do it except for debugging.
// #define STB_VORBIS_NO_DEFER_FLOOR




//////////////////////////////////////////////////////////////////////////////

#ifdef STB_VORBIS_NO_PULLDATA_API
   #define STB_VORBIS_NO_INTEGER_CONVERSION
   #define STB_VORBIS_NO_STDIO
#endif

#if defined(STB_VORBIS_NO_CRT) && !defined(STB_VORBIS_NO_STDIO)
   #define STB_VORBIS_NO_STDIO 1
#endif

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
#ifndef STB_VORBIS_NO_FAST_SCALED_FLOAT

   // only need endianness for fast-float-to-int, which we don't
   // use for pushdata

   #ifndef STB_VORBIS_BIG_ENDIAN
     #define STB_VORBIS_ENDIAN  0
   #else
     #define STB_VORBIS_ENDIAN  1
   #endif

#endif
#endif


#ifndef STB_VORBIS_NO_STDIO
#include <stdio.h>
#endif

#ifndef STB_VORBIS_NO_CRT
   #include <stdlib.h>
   #include <string.h>
   #include <assert.h>
   #include <math.h>

   // find definition of alloca if it's not in stdlib.h:
   #if defined(_MSC_VER) || defined(__MINGW32__)
      #include <malloc.h>
   #endif
   #if defined(__linux__) || defined(__linux) || defined(__sun__) || defined(__EMSCRIPTEN__) || defined(__NEWLIB__)
      #include <alloca.h>
   #endif
#else // STB_VORBIS_NO_CRT
   #define NULL 0
   #define malloc(s)   0
   #define free(s)     ((void) 0)
   #define realloc(s)  0
#endif // STB_VORBIS_NO_CRT

#include <limits.h>

#ifdef __MINGW32__
   // eff you mingw:
   //     "fixed":
   //         http://sourceforge.net/p/mingw-w64/mailman/message/32882927/
   //     "no that broke the build, reverted, who cares about C":
   //         http://sourceforge.net/p/mingw-w64/mailman/message/32890381/
   #ifdef __forceinline
   #undef __forceinline
   #endif
   #define __forceinline
   #ifndef alloca
   #define alloca __builtin_alloca
   #endif
#elif !defined(_MSC_VER)
   #if __GNUC__
      #define __forceinline inline
   #else
      #define __forceinline
   #endif
#endif

#if STB_VORBIS_MAX_CHANNELS > 256
#error "Value of STB_VORBIS_MAX_CHANNELS outside of allowed range"
#endif

#if STB_VORBIS_FAST_HUFFMAN_LENGTH > 24
#error "Value of STB_VORBIS_FAST_HUFFMAN_LENGTH outside of allowed range"
#endif


#if 0
#include <crtdbg.h>
#define CHECK(f)   _CrtIsValidHeapPointer(f->channel_buffers[1])
#else
#define CHECK(f)   ((void) 0)
#endif

#define MAX_BLOCKSIZE_LOG  13   // from specification
#define MAX_BLOCKSIZE      (1 << MAX_BLOCKSIZE_LOG)


typedef unsigned char  uint8;
typedef   signed char   int8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef float codetype;

#ifdef _MSC_VER
#define STBV_NOTUSED(v)  (void)(v)
#else
#define STBV_NOTUSED(v)  (void)sizeof(v)
#endif

// @NOTE
//
// Some arrays below are tagged "//varies", which means it's actually
// a variable-sized piece of data, but rather than malloc I assume it's
// small enough it's better to just allocate it all together with the
// main thing
//
// Most of the variables are specified with the smallest size I could pack
// them into. It might give better performance to make them all full-sized
// integers. It should be safe to freely rearrange the structures or change
// the sizes larger--nothing relies on silently truncating etc., nor the
// order of variables.

#define FAST_HUFFMAN_TABLE_SIZE   (1 << STB_VORBIS_FAST_HUFFMAN_LENGTH)
#define FAST_HUFFMAN_TABLE_MASK   (FAST_HUFFMAN_TABLE_SIZE - 1)

typedef struct
{
   int dimensions, entries;
   uint8 *codeword_lengths;
   float  minimum_value;
   float  delta_value;
   uint8  value_bits;
   uint8  lookup_type;
   uint8  sequence_p;
   uint8  sparse;
   uint32 lookup_values;
   codetype *multiplicands;
   uint32 *codewords;
   #ifdef STB_VORBIS_FAST_HUFFMAN_SHORT
    int16  fast_huffman[FAST_HUFFMAN_TABLE_SIZE];
   #else
    int32  fast_huffman[FAST_HUFFMAN_TABLE_SIZE];
   #endif
   uint32 *sorted_codewords;
   int    *sorted_values;
   int     sorted_entries;
} Codebook;

typedef struct
{
   uint8 order;
   uint16 rate;
   uint16 bark_map_size;
   uint8 amplitude_bits;
   uint8 amplitude_offset;
   uint8 number_of_books;
   uint8 book_list[16]; // varies
} Floor0;

typedef struct
{
   uint8 partitions;
   uint8 partition_class_list[32]; // varies
   uint8 class_dimensions[16]; // varies
   uint8 class_subclasses[16]; // varies
   uint8 class_masterbooks[16]; // varies
   int16 subclass_books[16][8]; // varies
   uint16 Xlist[31*8+2]; // varies
   uint8 sorted_order[31*8+2];
   uint8 neighbors[31*8+2][2];
   uint8 floor1_multiplier;
   uint8 rangebits;
   int values;
} Floor1;

typedef union
{
   Floor0 floor0;
   Floor1 floor1;
} Floor;

typedef struct
{
   uint32 begin, end;
   uint32 part_size;
   uint8 classifications;
   uint8 classbook;
   uint8 **classdata;
   int16 (*residue_books)[8];
} Residue;

typedef struct
{
   uint8 magnitude;
   uint8 angle;
   uint8 mux;
} MappingChannel;

typedef struct
{
   uint16 coupling_steps;
   MappingChannel *chan;
   uint8  submaps;
   uint8  submap_floor[15]; // varies
   uint8  submap_residue[15]; // varies
} Mapping;

typedef struct
{
   uint8 blockflag;
   uint8 mapping;
   uint16 windowtype;
   uint16 transformtype;
} Mode;

typedef struct
{
   uint32  goal_crc;    // expected crc if match
   int     bytes_left;  // bytes left in packet
   uint32  crc_so_far;  // running crc
   int     bytes_done;  // bytes processed in _current_ chunk
   uint32  sample_loc;  // granule pos encoded in page
} CRCscan;

typedef struct
{
   uint32 page_start, page_end;
   uint32 last_decoded_sample;
} ProbedPage;

struct stb_vorbis
{
  // user-accessible info
   unsigned int sample_rate;
   int channels;

   unsigned int setup_memory_required;
   unsigned int temp_memory_required;
   unsigned int setup_temp_memory_required;

   char *vendor;
   int comment_list_length;
   char **comment_list;

  // input config
#ifndef STB_VORBIS_NO_STDIO
   FILE *f;
   uint32 f_start;
   int close_on_free;
#endif

   uint8 *stream;
   uint8 *stream_start;
   uint8 *stream_end;

   uint32 stream_len;

   uint8  push_mode;

   // the page to seek to when seeking to start, may be zero
   uint32 first_audio_page_offset;

   // p_first is the page on which the first audio packet ends
   // (but not necessarily the page on which it starts)
   ProbedPage p_first, p_last;

  // memory management
   stb_vorbis_alloc alloc;
   int setup_offset;
   int temp_offset;

  // run-time results
   int eof;
   enum STBVorbisError error;

  // user-useful data

  // header info
   int blocksize[2];
   int blocksize_0, blocksize_1;
   int codebook_count;
   Codebook *codebooks;
   int floor_count;
   uint16 floor_types[64]; // varies
   Floor *floor_config;
   int residue_count;
   uint16 residue_types[64]; // varies
   Residue *residue_config;
   int mapping_count;
   Mapping *mapping;
   int mode_count;
   Mode mode_config[64];  // varies

   uint32 total_samples;

  // decode buffer
   float *channel_buffers[STB_VORBIS_MAX_CHANNELS];
   float *outputs        [STB_VORBIS_MAX_CHANNELS];

   float *previous_window[STB_VORBIS_MAX_CHANNELS];
   int previous_length;

   #ifndef STB_VORBIS_NO_DEFER_FLOOR
   int16 *finalY[STB_VORBIS_MAX_CHANNELS];
   #else
   float *floor_buffers[STB_VORBIS_MAX_CHANNELS];
   #endif

   uint32 current_loc; // sample location of next frame to decode
   int    current_loc_valid;

  // per-blocksize precomputed data

   // twiddle factors
   float *A[2],*B[2],*C[2];
   float *window[2];
   uint16 *bit_reverse[2];

  // current page/packet/segment streaming info
   uint32 serial; // stream serial number for verification
   int last_page;
   int segment_count;
   uint8 segments[255];
   uint8 page_flag;
   uint8 bytes_in_seg;
   uint8 first_decode;
   int next_seg;
   int last_seg;  // flag that we're on the last segment
   int last_seg_which; // what was the segment number of the last seg?
   uint32 acc;
   int valid_bits;
   int packet_bytes;
   int end_seg_with_known_loc;
   uint32 known_loc_for_packet;
   int discard_samples_deferred;
   uint32 samples_output;

  // push mode scanning
   int page_crc_tests; // only in push_mode: number of tests active; -1 if not searching
#ifndef STB_VORBIS_NO_PUSHDATA_API
   CRCscan scan[STB_VORBIS_PUSHDATA_CRC_COUNT];
#endif

  // sample-access
   int channel_buffer_start;
   int channel_buffer_end;
};

#if defined(STB_VORBIS_NO_PUSHDATA_API)
   #define IS_PUSH_MODE(f)   FALSE
#elif defined(STB_VORBIS_NO_PULLDATA_API)
   #define IS_PUSH_MODE(f)   TRUE
#else
   #define IS_PUSH_MODE(f)   ((f)->push_mode)
#endif

typedef struct stb_vorbis vorb;

static int error(vorb *f, enum STBVorbisError e)
{
   f->error = e;
   if (!f->eof && e != VORBIS_need_more_data) {
      f->error=e; // breakpoint for debugging
   }
   return 0;
}


// these functions are used for allocating temporary memory
// while decoding. if you can afford the stack space, use
// alloca(); otherwise, provide a temp buffer and it will
// allocate out of those.

#define array_size_required(count,size)  (count*(sizeof(void *)+(size)))

#define temp_alloc(f,size)              (f->alloc.alloc_buffer ? setup_temp_malloc(f,size) : alloca(size))
#define temp_free(f,p)                  (void)0
#define temp_alloc_save(f)              ((f)->temp_offset)
#define temp_alloc_restore(f,p)         ((f)->temp_offset = (p))

#define temp_block_array(f,count,size)  make_block_array(temp_alloc(f,array_size_required(count,size)), count, size)

// given a sufficiently large block of memory, make an array of pointers to subblocks of it
static void *make_block_array(void *mem, int count, int size)
{
   int i;
   void ** p = (void **) mem;
   char *q = (char *) (p + count);
   for (i=0; i < count; ++i) {
      p[i] = q;
      q += size;
   }
   return p;
}

static void *setup_malloc(vorb *f, int sz)
{
   sz = (sz+7) & ~7; // round up to nearest 8 for alignment of future allocs.
   f->setup_memory_required += sz;
   if (f->alloc.alloc_buffer) {
      void *p = (char *) f->alloc.alloc_buffer + f->setup_offset;
      if (f->setup_offset + sz > f->temp_offset) return NULL;
      f->setup_offset += sz;
      return p;
   }
   return sz ? tm_vorbis_malloc(sz) : NULL;
}

static void setup_free(vorb *f, void *p)
{
   if (f->alloc.alloc_buffer) return; // do nothing; setup mem is a stack
   tm_vorbis_free(p);
}

static void *setup_temp_malloc(vorb *f, int sz)
{
   sz = (sz+7) & ~7; // round up to nearest 8 for alignment of future allocs.
   if (f->alloc.alloc_buffer) {
      if (f->temp_offset - sz < f->setup_offset) return NULL;
      f->temp_offset -= sz;
      return (char *) f->alloc.alloc_buffer + f->temp_offset;
   }
   return tm_vorbis_temp_malloc(sz);
}

static void setup_temp_free(vorb *f, void *p, int sz)
{
   if (f->alloc.alloc_buffer) {
      f->temp_offset += (sz+7)&~7;
      return;
   }
   tm_vorbis_temp_free(p);
}

#define CRC32_POLY    0x04c11db7   // from spec

static uint32 crc_table[256];
static void crc32_init(void)
{
   int i,j;
   uint32 s;
   for(i=0; i < 256; i++) {
      for (s=(uint32) i << 24, j=0; j < 8; ++j)
         s = (s << 1) ^ (s >= (1U<<31) ? CRC32_POLY : 0);
      crc_table[i] = s;
   }
}

static __forceinline uint32 crc32_update(uint32 crc, uint8 byte)
{
   return (crc << 8) ^ crc_table[byte ^ (crc >> 24)];
}


// used in setup, and for huffman that doesn't go fast path
static unsigned int bit_reverse(unsigned int n)
{
  n = ((n & 0xAAAAAAAA) >>  1) | ((n & 0x55555555) << 1);
  n = ((n & 0xCCCCCCCC) >>  2) | ((n & 0x33333333) << 2);
  n = ((n & 0xF0F0F0F0) >>  4) | ((n & 0x0F0F0F0F) << 4);
  n = ((n & 0xFF00FF00) >>  8) | ((n & 0x00FF00FF) << 8);
  return (n >> 16) | (n << 16);
}

static float square(float x)
{
   return x*x;
}

// this is a weird definition of log2() for which log2(1) = 1, log2(2) = 2, log2(4) = 3
// as required by the specification. fast(?) implementation from stb.h
// @OPTIMIZE: called multiple times per-packet with "constants"; move to setup
static int ilog(int32 n)
{
   static signed char log2_4[16] = { 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 };

   if (n < 0) return 0; // signed n returns 0

   // 2 compares if n < 16, 3 compares otherwise (4 if signed or n > 1<<29)
   if (n < (1 << 14))
        if (n < (1 <<  4))            return  0 + log2_4[n      ];
        else if (n < (1 <<  9))       return  5 + log2_4[n >>  5];
             else                     return 10 + log2_4[n >> 10];
   else if (n < (1 << 24))
             if (n < (1 << 19))       return 15 + log2_4[n >> 15];
             else                     return 20 + log2_4[n >> 20];
        else if (n < (1 << 29))       return 25 + log2_4[n >> 25];
             else                     return 30 + log2_4[n >> 30];
}

#ifndef M_PI
  #define M_PI  3.14159265358979323846264f  // from CRC
#endif

// code length assigned to a value with no huffman encoding
#define NO_CODE   255

/////////////////////// LEAF SETUP FUNCTIONS //////////////////////////
//
// these functions are only called at setup, and only a few times
// per file

static float float32_unpack(uint32 x)
{
   // from the specification
   uint32 mantissa = x & 0x1fffff;
   uint32 sign = x & 0x80000000;
   uint32 exp = (x & 0x7fe00000) >> 21;
   double res = sign ? -(double)mantissa : (double)mantissa;
   return (float) ldexp((float)res, (int)exp-788);
}


// zlib & jpeg huffman tables assume that the output symbols
// can either be arbitrarily arranged, or have monotonically
// increasing frequencies--they rely on the lengths being sorted;
// this makes for a very simple generation algorithm.
// vorbis allows a huffman table with non-sorted lengths. This
// requires a more sophisticated construction, since symbols in
// order do not map to huffman codes "in order".
static void add_entry(Codebook *c, uint32 huff_code, int symbol, int count, int len, uint32 *values)
{
   if (!c->sparse) {
      c->codewords      [symbol] = huff_code;
   } else {
      c->codewords       [count] = huff_code;
      c->codeword_lengths[count] = len;
      values             [count] = symbol;
   }
}

static int compute_codewords(Codebook *c, uint8 *len, int n, uint32 *values)
{
   int i,k,m=0;
   uint32 available[32];

   memset(available, 0, sizeof(available));
   // find the first entry
   for (k=0; k < n; ++k) if (len[k] < NO_CODE) break;
   if (k == n) { assert(c->sorted_entries == 0); return TRUE; }
   assert(len[k] < 32); // no error return required, code reading lens checks this
   // add to the list
   add_entry(c, 0, k, m++, len[k], values);
   // add all available leaves
   for (i=1; i <= len[k]; ++i)
      available[i] = 1U << (32-i);
   // note that the above code treats the first case specially,
   // but it's really the same as the following code, so they
   // could probably be combined (except the initial code is 0,
   // and I use 0 in available[] to mean 'empty')
   for (i=k+1; i < n; ++i) {
      uint32 res;
      int z = len[i], y;
      if (z == NO_CODE) continue;
      assert(z < 32); // no error return required, code reading lens checks this
      // find lowest available leaf (should always be earliest,
      // which is what the specification calls for)
      // note that this property, and the fact we can never have
      // more than one free leaf at a given level, isn't totally
      // trivial to prove, but it seems true and the assert never
      // fires, so!
      while (z > 0 && !available[z]) --z;
      if (z == 0) { return FALSE; }
      res = available[z];
      available[z] = 0;
      add_entry(c, bit_reverse(res), i, m++, len[i], values);
      // propagate availability up the tree
      if (z != len[i]) {
         for (y=len[i]; y > z; --y) {
            assert(available[y] == 0);
            available[y] = res + (1 << (32-y));
         }
      }
   }
   return TRUE;
}

// accelerated huffman table allows fast O(1) match of all symbols
// of length <= STB_VORBIS_FAST_HUFFMAN_LENGTH
static void compute_accelerated_huffman(Codebook *c)
{
   int i, len;
   for (i=0; i < FAST_HUFFMAN_TABLE_SIZE; ++i)
      c->fast_huffman[i] = -1;

   len = c->sparse ? c->sorted_entries : c->entries;
   #ifdef STB_VORBIS_FAST_HUFFMAN_SHORT
   if (len > 32767) len = 32767; // largest possible value we can encode!
   #endif
   for (i=0; i < len; ++i) {
      if (c->codeword_lengths[i] <= STB_VORBIS_FAST_HUFFMAN_LENGTH) {
         uint32 z = c->sparse ? bit_reverse(c->sorted_codewords[i]) : c->codewords[i];
         // set table entries for all bit combinations in the higher bits
         while (z < FAST_HUFFMAN_TABLE_SIZE) {
             c->fast_huffman[z] = i;
             z += 1 << c->codeword_lengths[i];
         }
      }
   }
}

#ifdef _MSC_VER
#define STBV_CDECL __cdecl
#else
#define STBV_CDECL
#endif

static int STBV_CDECL uint32_compare(const void *p, const void *q)
{
   uint32 x = * (uint32 *) p;
   uint32 y = * (uint32 *) q;
   return x < y ? -1 : x > y;
}

static int include_in_sort(Codebook *c, uint8 len)
{
   if (c->sparse) { assert(len != NO_CODE); return TRUE; }
   if (len == NO_CODE) return FALSE;
   if (len > STB_VORBIS_FAST_HUFFMAN_LENGTH) return TRUE;
   return FALSE;
}

// if the fast table above doesn't work, we want to binary
// search them... need to reverse the bits
static void compute_sorted_huffman(Codebook *c, uint8 *lengths, uint32 *values)
{
   int i, len;
   // build a list of all the entries
   // OPTIMIZATION: don't include the short ones, since they'll be caught by FAST_HUFFMAN.
   // this is kind of a frivolous optimization--I don't see any performance improvement,
   // but it's like 4 extra lines of code, so.
   if (!c->sparse) {
      int k = 0;
      for (i=0; i < c->entries; ++i)
         if (include_in_sort(c, lengths[i]))
            c->sorted_codewords[k++] = bit_reverse(c->codewords[i]);
      assert(k == c->sorted_entries);
   } else {
      for (i=0; i < c->sorted_entries; ++i)
         c->sorted_codewords[i] = bit_reverse(c->codewords[i]);
   }

   qsort(c->sorted_codewords, c->sorted_entries, sizeof(c->sorted_codewords[0]), uint32_compare);
   c->sorted_codewords[c->sorted_entries] = 0xffffffff;

   len = c->sparse ? c->sorted_entries : c->entries;
   // now we need to indicate how they correspond; we could either
   //   #1: sort a different data structure that says who they correspond to
   //   #2: for each sorted entry, search the original list to find who corresponds
   //   #3: for each original entry, find the sorted entry
   // #1 requires extra storage, #2 is slow, #3 can use binary search!
   for (i=0; i < len; ++i) {
      int huff_len = c->sparse ? lengths[values[i]] : lengths[i];
      if (include_in_sort(c,huff_len)) {
         uint32 code = bit_reverse(c->codewords[i]);
         int x=0, n=c->sorted_entries;
         while (n > 1) {
            // invariant: sc[x] <= code < sc[x+n]
            int m = x + (n >> 1);
            if (c->sorted_codewords[m] <= code) {
               x = m;
               n -= (n>>1);
            } else {
               n >>= 1;
            }
         }
         assert(c->sorted_codewords[x] == code);
         if (c->sparse) {
            c->sorted_values[x] = values[i];
            c->codeword_lengths[x] = huff_len;
         } else {
            c->sorted_values[x] = i;
         }
      }
   }
}

// only run while parsing the header (3 times)
static int vorbis_validate(uint8 *data)
{
   static uint8 vorbis[6] = { 'v', 'o', 'r', 'b', 'i', 's' };
   return memcmp(data, vorbis, 6) == 0;
}

// called from setup only, once per code book
// (formula implied by specification)
static int lookup1_values(int entries, int dim)
{
   int r = (int) floor(exp((float) log((float) entries) / dim));
   if ((int) floor(pow((float) r+1, dim)) <= entries)   // (int) cast for MinGW warning;
      ++r;                                              // floor() to avoid _ftol() when non-CRT
   if (pow((float) r+1, dim) <= entries)
      return -1;
   if ((int) floor(pow((float) r, dim)) > entries)
      return -1;
   return r;
}

// called twice per file
static void compute_twiddle_factors(int n, float *A, float *B, float *C)
{
   int n4 = n >> 2, n8 = n >> 3;
   int k,k2;

   for (k=k2=0; k < n4; ++k,k2+=2) {
      A[k2  ] = (float)  cos(4*k*M_PI/n);
      A[k2+1] = (float) -sin(4*k*M_PI/n);
      B[k2  ] = (float)  cos((k2+1)*M_PI/n/2) * 0.5f;
      B[k2+1] = (float)  sin((k2+1)*M_PI/n/2) * 0.5f;
   }
   for (k=k2=0; k < n8; ++k,k2+=2) {
      C[k2  ] = (float)  cos(2*(k2+1)*M_PI/n);
      C[k2+1] = (float) -sin(2*(k2+1)*M_PI/n);
   }
}

static void compute_window(int n, float *window)
{
   int n2 = n >> 1, i;
   for (i=0; i < n2; ++i)
      window[i] = (float) sin(0.5 * M_PI * square((float) sin((i - 0 + 0.5) / n2 * 0.5 * M_PI)));
}

static void compute_bitreverse(int n, uint16 *rev)
{
   int ld = ilog(n) - 1; // ilog is off-by-one from normal definitions
   int i, n8 = n >> 3;
   for (i=0; i < n8; ++i)
      rev[i] = (bit_reverse(i) >> (32-ld+3)) << 2;
}

static int init_blocksize(vorb *f, int b, int n)
{
   int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3;
   f->A[b] = (float *) setup_malloc(f, sizeof(float) * n2);
   f->B[b] = (float *) setup_malloc(f, sizeof(float) * n2);
   f->C[b] = (float *) setup_malloc(f, sizeof(float) * n4);
   if (!f->A[b] || !f->B[b] || !f->C[b]) return error(f, VORBIS_outofmem);
   compute_twiddle_factors(n, f->A[b], f->B[b], f->C[b]);
   f->window[b] = (float *) setup_malloc(f, sizeof(float) * n2);
   if (!f->window[b]) return error(f, VORBIS_outofmem);
   compute_window(n, f->window[b]);
   f->bit_reverse[b] = (uint16 *) setup_malloc(f, sizeof(uint16) * n8);
   if (!f->bit_reverse[b]) return error(f, VORBIS_outofmem);
   compute_bitreverse(n, f->bit_reverse[b]);
   return TRUE;
}

static void neighbors(uint16 *x, int n, int *plow, int *phigh)
{
   int low = -1;
   int high = 65536;
   int i;
   for (i=0; i < n; ++i) {
      if (x[i] > low  && x[i] < x[n]) { *plow  = i; low = x[i]; }
      if (x[i] < high && x[i] > x[n]) { *phigh = i; high = x[i]; }
   }
}

// this has been repurposed so y is now the original index instead of y
typedef struct
{
   uint16 x,id;
} stbv__floor_ordering;

static int STBV_CDECL point_compare(const void *p, const void *q)
{
   stbv__floor_ordering *a = (stbv__floor_ordering *) p;
   stbv__floor_ordering *b = (stbv__floor_ordering *) q;
   return a->x < b->x ? -1 : a->x > b->x;
}

//
/////////////////////// END LEAF SETUP FUNCTIONS //////////////////////////


#if defined(STB_VORBIS_NO_STDIO)
   #define USE_MEMORY(z)    TRUE
#else
   #define USE_MEMORY(z)    ((z)->stream)
#endif

static uint8 get8(vorb *z)
{
   if (USE_MEMORY(z)) {
      if (z->stream >= z->stream_end) { z->eof = TRUE; return 0; }
      return *z->stream++;
   }

   #ifndef STB_VORBIS_NO_STDIO
   {
   int c = fgetc(z->f);
   if (c == EOF) { z->eof = TRUE; return 0; }
   return c;
   }
   #endif
}

static uint32 get32(vorb *f)
{
   uint32 x;
   x = get8(f);
   x += get8(f) << 8;
   x += get8(f) << 16;
   x += (uint32) get8(f) << 24;
   return x;
}

static int getn(vorb *z, uint8 *data, int n)
{
   if (USE_MEMORY(z)) {
      if (z->stream+n > z->stream_end) { z->eof = 1; return 0; }
      memcpy(data, z->stream, n);
      z->stream += n;
      return 1;
   }

   #ifndef STB_VORBIS_NO_STDIO
   if (fread(data, n, 1, z->f) == 1)
      return 1;
   else {
      z->eof = 1;
      return 0;
   }
   #endif
}

static void skip(vorb *z, int n)
{
   if (USE_MEMORY(z)) {
      z->stream += n;
      if (z->stream >= z->stream_end) z->eof = 1;
      return;
   }
   #ifndef STB_VORBIS_NO_STDIO
   {
      long x = ftell(z->f);
      fseek(z->f, x+n, SEEK_SET);
   }
   #endif
}

static int set_file_offset(stb_vorbis *f, unsigned int loc)
{
   #ifndef STB_VORBIS_NO_PUSHDATA_API
   if (f->push_mode) return 0;
   #endif
   f->eof = 0;
   if (USE_MEMORY(f)) {
      if (f->stream_start + loc >= f->stream_end || f->stream_start + loc < f->stream_start) {
         f->stream = f->stream_end;
         f->eof = 1;
         return 0;
      } else {
         f->stream = f->stream_start + loc;
         return 1;
      }
   }
   #ifndef STB_VORBIS_NO_STDIO
   if (loc + f->f_start < loc || loc >= 0x80000000) {
      loc = 0x7fffffff;
      f->eof = 1;
   } else {
      loc += f->f_start;
   }
   if (!fseek(f->f, loc, SEEK_SET))
      return 1;
   f->eof = 1;
   fseek(f->f, f->f_start, SEEK_END);
   return 0;
   #endif
}


static uint8 ogg_page_header[4] = { 0x4f, 0x67, 0x67, 0x53 };

static int capture_pattern(vorb *f)
{
   if (0x4f != get8(f)) return FALSE;
   if (0x67 != get8(f)) return FALSE;
   if (0x67 != get8(f)) return FALSE;
   if (0x53 != get8(f)) return FALSE;
   return TRUE;
}

#define PAGEFLAG_continued_packet   1
#define PAGEFLAG_first_page         2
#define PAGEFLAG_last_page          4

static int start_page_no_capturepattern(vorb *f)
{
   uint32 loc0,loc1,n;
   if (f->first_decode && !IS_PUSH_MODE(f)) {
      f->p_first.page_start = stb_vorbis_get_file_offset(f) - 4;
   }
   // stream structure version
   if (0 != get8(f)) return error(f, VORBIS_invalid_stream_structure_version);
   // header flag
   f->page_flag = get8(f);
   // absolute granule position
   loc0 = get32(f);
   loc1 = get32(f);
   // @TODO: validate loc0,loc1 as valid positions?
   // stream serial number -- vorbis doesn't interleave, so discard
   get32(f);
   //if (f->serial != get32(f)) return error(f, VORBIS_incorrect_stream_serial_number);
   // page sequence number
   n = get32(f);
   f->last_page = n;
   // CRC32
   get32(f);
   // page_segments
   f->segment_count = get8(f);
   if (!getn(f, f->segments, f->segment_count))
      return error(f, VORBIS_unexpected_eof);
   // assume we _don't_ know any the sample position of any segments
   f->end_seg_with_known_loc = -2;
   if (loc0 != ~0U || loc1 != ~0U) {
      int i;
      // determine which packet is the last one that will complete
      for (i=f->segment_count-1; i >= 0; --i)
         if (f->segments[i] < 255)
            break;
      // 'i' is now the index of the _last_ segment of a packet that ends
      if (i >= 0) {
         f->end_seg_with_known_loc = i;
         f->known_loc_for_packet   = loc0;
      }
   }
   if (f->first_decode) {
      int i,len;
      len = 0;
      for (i=0; i < f->segment_count; ++i)
         len += f->segments[i];
      len += 27 + f->segment_count;
      f->p_first.page_end = f->p_first.page_start + len;
      f->p_first.last_decoded_sample = loc0;
   }
   f->next_seg = 0;
   return TRUE;
}

static int start_page(vorb *f)
{
   if (!capture_pattern(f)) return error(f, VORBIS_missing_capture_pattern);
   return start_page_no_capturepattern(f);
}

static int start_packet(vorb *f)
{
   while (f->next_seg == -1) {
      if (!start_page(f)) return FALSE;
      if (f->page_flag & PAGEFLAG_continued_packet)
         return error(f, VORBIS_continued_packet_flag_invalid);
   }
   f->last_seg = FALSE;
   f->valid_bits = 0;
   f->packet_bytes = 0;
   f->bytes_in_seg = 0;
   // f->next_seg is now valid
   return TRUE;
}

static int maybe_start_packet(vorb *f)
{
   if (f->next_seg == -1) {
      int x = get8(f);
      if (f->eof) return FALSE; // EOF at page boundary is not an error!
      if (0x4f != x      ) return error(f, VORBIS_missing_capture_pattern);
      if (0x67 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
      if (0x67 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
      if (0x53 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
      if (!start_page_no_capturepattern(f)) return FALSE;
      if (f->page_flag & PAGEFLAG_continued_packet) {
         // set up enough state that we can read this packet if we want,
         // e.g. during recovery
         f->last_seg = FALSE;
         f->bytes_in_seg = 0;
         return error(f, VORBIS_continued_packet_flag_invalid);
      }
   }
   return start_packet(f);
}

static int next_segment(vorb *f)
{
   int len;
   if (f->last_seg) return 0;
   if (f->next_seg == -1) {
      f->last_seg_which = f->segment_count-1; // in case start_page fails
      if (!start_page(f)) { f->last_seg = 1; return 0; }
      if (!(f->page_flag & PAGEFLAG_continued_packet)) return error(f, VORBIS_continued_packet_flag_invalid);
   }
   len = f->segments[f->next_seg++];
   if (len < 255) {
      f->last_seg = TRUE;
      f->last_seg_which = f->next_seg-1;
   }
   if (f->next_seg >= f->segment_count)
      f->next_seg = -1;
   assert(f->bytes_in_seg == 0);
   f->bytes_in_seg = len;
   return len;
}

#define EOP    (-1)
#define INVALID_BITS  (-1)

static int get8_packet_raw(vorb *f)
{
   if (!f->bytes_in_seg) {  // CLANG!
      if (f->last_seg) return EOP;
      else if (!next_segment(f)) return EOP;
   }
   assert(f->bytes_in_seg > 0);
   --f->bytes_in_seg;
   ++f->packet_bytes;
   return get8(f);
}

static int get8_packet(vorb *f)
{
   int x = get8_packet_raw(f);
   f->valid_bits = 0;
   return x;
}

static int get32_packet(vorb *f)
{
   uint32 x;
   x = get8_packet(f);
   x += get8_packet(f) << 8;
   x += get8_packet(f) << 16;
   x += (uint32) get8_packet(f) << 24;
   return x;
}

static void flush_packet(vorb *f)
{
   while (get8_packet_raw(f) != EOP);
}

// @OPTIMIZE: this is the secondary bit decoder, so it's probably not as important
// as the huffman decoder?
static uint32 get_bits(vorb *f, int n)
{
   uint32 z;

   if (f->valid_bits < 0) return 0;
   if (f->valid_bits < n) {
      if (n > 24) {
         // the accumulator technique below would not work correctly in this case
         z = get_bits(f, 24);
         z += get_bits(f, n-24) << 24;
         return z;
      }
      if (f->valid_bits == 0) f->acc = 0;
      while (f->valid_bits < n) {
         int z = get8_packet_raw(f);
         if (z == EOP) {
            f->valid_bits = INVALID_BITS;
            return 0;
         }
         f->acc += z << f->valid_bits;
         f->valid_bits += 8;
      }
   }

   assert(f->valid_bits >= n);
   z = f->acc & ((1 << n)-1);
   f->acc >>= n;
   f->valid_bits -= n;
   return z;
}

// @OPTIMIZE: primary accumulator for huffman
// expand the buffer to as many bits as possible without reading off end of packet
// it might be nice to allow f->valid_bits and f->acc to be stored in registers,
// e.g. cache them locally and decode locally
static __forceinline void prep_huffman(vorb *f)
{
   if (f->valid_bits <= 24) {
      if (f->valid_bits == 0) f->acc = 0;
      do {
         int z;
         if (f->last_seg && !f->bytes_in_seg) return;
         z = get8_packet_raw(f);
         if (z == EOP) return;
         f->acc += (unsigned) z << f->valid_bits;
         f->valid_bits += 8;
      } while (f->valid_bits <= 24);
   }
}

enum
{
   VORBIS_packet_id = 1,
   VORBIS_packet_comment = 3,
   VORBIS_packet_setup = 5
};

static int codebook_decode_scalar_raw(vorb *f, Codebook *c)
{
   int i;
   prep_huffman(f);

   if (c->codewords == NULL && c->sorted_codewords == NULL)
      return -1;

   // cases to use binary search: sorted_codewords && !c->codewords
   //                             sorted_codewords && c->entries > 8
   if (c->entries > 8 ? c->sorted_codewords!=NULL : !c->codewords) {
      // binary search
      uint32 code = bit_reverse(f->acc);
      int x=0, n=c->sorted_entries, len;

      while (n > 1) {
         // invariant: sc[x] <= code < sc[x+n]
         int m = x + (n >> 1);
         if (c->sorted_codewords[m] <= code) {
            x = m;
            n -= (n>>1);
         } else {
            n >>= 1;
         }
      }
      // x is now the sorted index
      if (!c->sparse) x = c->sorted_values[x];
      // x is now sorted index if sparse, or symbol otherwise
      len = c->codeword_lengths[x];
      if (f->valid_bits >= len) {
         f->acc >>= len;
         f->valid_bits -= len;
         return x;
      }

      f->valid_bits = 0;
      return -1;
   }

   // if small, linear search
   assert(!c->sparse);
   for (i=0; i < c->entries; ++i) {
      if (c->codeword_lengths[i] == NO_CODE) continue;
      if (c->codewords[i] == (f->acc & ((1 << c->codeword_lengths[i])-1))) {
         if (f->valid_bits >= c->codeword_lengths[i]) {
            f->acc >>= c->codeword_lengths[i];
            f->valid_bits -= c->codeword_lengths[i];
            return i;
         }
         f->valid_bits = 0;
         return -1;
      }
   }

   error(f, VORBIS_invalid_stream);
   f->valid_bits = 0;
   return -1;
}

#ifndef STB_VORBIS_NO_INLINE_DECODE

#define DECODE_RAW(var, f,c)                                  \
   if (f->valid_bits < STB_VORBIS_FAST_HUFFMAN_LENGTH)        \
      prep_huffman(f);                                        \
   var = f->acc & FAST_HUFFMAN_TABLE_MASK;                    \
   var = c->fast_huffman[var];                                \
   if (var >= 0) {                                            \
      int n = c->codeword_lengths[var];                       \
      f->acc >>= n;                                           \
      f->valid_bits -= n;                                     \
      if (f->valid_bits < 0) { f->valid_bits = 0; var = -1; } \
   } else {                                                   \
      var = codebook_decode_scalar_raw(f,c);                  \
   }

#else

static int codebook_decode_scalar(vorb *f, Codebook *c)
{
   int i;
   if (f->valid_bits < STB_VORBIS_FAST_HUFFMAN_LENGTH)
      prep_huffman(f);
   // fast huffman table lookup
   i = f->acc & FAST_HUFFMAN_TABLE_MASK;
   i = c->fast_huffman[i];
   if (i >= 0) {
      f->acc >>= c->codeword_lengths[i];
      f->valid_bits -= c->codeword_lengths[i];
      if (f->valid_bits < 0) { f->valid_bits = 0; return -1; }
      return i;
   }
   return codebook_decode_scalar_raw(f,c);
}

#define DECODE_RAW(var,f,c)    var = codebook_decode_scalar(f,c);

#endif

#define DECODE(var,f,c)                                       \
   DECODE_RAW(var,f,c)                                        \
   if (c->sparse) var = c->sorted_values[var];

#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
  #define DECODE_VQ(var,f,c)   DECODE_RAW(var,f,c)
#else
  #define DECODE_VQ(var,f,c)   DECODE(var,f,c)
#endif






// CODEBOOK_ELEMENT_FAST is an optimization for the CODEBOOK_FLOATS case
// where we avoid one addition
#define CODEBOOK_ELEMENT(c,off)          (c->multiplicands[off])
#define CODEBOOK_ELEMENT_FAST(c,off)     (c->multiplicands[off])
#define CODEBOOK_ELEMENT_BASE(c)         (0)

static int codebook_decode_start(vorb *f, Codebook *c)
{
   int z = -1;

   // type 0 is only legal in a scalar context
   if (c->lookup_type == 0)
      error(f, VORBIS_invalid_stream);
   else {
      DECODE_VQ(z,f,c);
      if (c->sparse) assert(z < c->sorted_entries);
      if (z < 0) {  // check for EOP
         if (!f->bytes_in_seg)
            if (f->last_seg)
               return z;
         error(f, VORBIS_invalid_stream);
      }
   }
   return z;
}

static int codebook_decode(vorb *f, Codebook *c, float *output, int len)
{
   int i,z = codebook_decode_start(f,c);
   if (z < 0) return FALSE;
   if (len > c->dimensions) len = c->dimensions;

#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
   if (c->lookup_type == 1) {
      float last = CODEBOOK_ELEMENT_BASE(c);
      int div = 1;
      for (i=0; i < len; ++i) {
         int off = (z / div) % c->lookup_values;
         float val = CODEBOOK_ELEMENT_FAST(c,off) + last;
         output[i] += val;
         if (c->sequence_p) last = val + c->minimum_value;
         div *= c->lookup_values;
      }
      return TRUE;
   }
#endif

   z *= c->dimensions;
   if (c->sequence_p) {
      float last = CODEBOOK_ELEMENT_BASE(c);
      for (i=0; i < len; ++i) {
         float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last;
         output[i] += val;
         last = val + c->minimum_value;
      }
   } else {
      float last = CODEBOOK_ELEMENT_BASE(c);
      for (i=0; i < len; ++i) {
         output[i] += CODEBOOK_ELEMENT_FAST(c,z+i) + last;
      }
   }

   return TRUE;
}

static int codebook_decode_step(vorb *f, Codebook *c, float *output, int len, int step)
{
   int i,z = codebook_decode_start(f,c);
   float last = CODEBOOK_ELEMENT_BASE(c);
   if (z < 0) return FALSE;
   if (len > c->dimensions) len = c->dimensions;

#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
   if (c->lookup_type == 1) {
      int div = 1;
      for (i=0; i < len; ++i) {
         int off = (z / div) % c->lookup_values;
         float val = CODEBOOK_ELEMENT_FAST(c,off) + last;
         output[i*step] += val;
         if (c->sequence_p) last = val;
         div *= c->lookup_values;
      }
      return TRUE;
   }
#endif

   z *= c->dimensions;
   for (i=0; i < len; ++i) {
      float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last;
      output[i*step] += val;
      if (c->sequence_p) last = val;
   }

   return TRUE;
}

static int codebook_decode_deinterleave_repeat(vorb *f, Codebook *c, float **outputs, int ch, int *c_inter_p, int *p_inter_p, int len, int total_decode)
{
   int c_inter = *c_inter_p;
   int p_inter = *p_inter_p;
   int i,z, effective = c->dimensions;

   // type 0 is only legal in a scalar context
   if (c->lookup_type == 0)   return error(f, VORBIS_invalid_stream);

   while (total_decode > 0) {
      float last = CODEBOOK_ELEMENT_BASE(c);
      DECODE_VQ(z,f,c);
      #ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
      assert(!c->sparse || z < c->sorted_entries);
      #endif
      if (z < 0) {
         if (!f->bytes_in_seg)
            if (f->last_seg) return FALSE;
         return error(f, VORBIS_invalid_stream);
      }

      // if this will take us off the end of the buffers, stop short!
      // we check by computing the length of the virtual interleaved
      // buffer (len*ch), our current offset within it (p_inter*ch)+(c_inter),
      // and the length we'll be using (effective)
      if (c_inter + p_inter*ch + effective > len * ch) {
         effective = len*ch - (p_inter*ch - c_inter);
      }

   #ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
      if (c->lookup_type == 1) {
         int div = 1;
         for (i=0; i < effective; ++i) {
            int off = (z / div) % c->lookup_values;
            float val = CODEBOOK_ELEMENT_FAST(c,off) + last;
            if (outputs[c_inter])
               outputs[c_inter][p_inter] += val;
            if (++c_inter == ch) { c_inter = 0; ++p_inter; }
            if (c->sequence_p) last = val;
            div *= c->lookup_values;
         }
      } else
   #endif
      {
         z *= c->dimensions;
         if (c->sequence_p) {
            for (i=0; i < effective; ++i) {
               float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last;
               if (outputs[c_inter])
                  outputs[c_inter][p_inter] += val;
               if (++c_inter == ch) { c_inter = 0; ++p_inter; }
               last = val;
            }
         } else {
            for (i=0; i < effective; ++i) {
               float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last;
               if (outputs[c_inter])
                  outputs[c_inter][p_inter] += val;
               if (++c_inter == ch) { c_inter = 0; ++p_inter; }
            }
         }
      }

      total_decode -= effective;
   }
   *c_inter_p = c_inter;
   *p_inter_p = p_inter;
   return TRUE;
}

static int predict_point(int x, int x0, int x1, int y0, int y1)
{
   int dy = y1 - y0;
   int adx = x1 - x0;
   // @OPTIMIZE: force int division to round in the right direction... is this necessary on x86?
   int err = abs(dy) * (x - x0);
   int off = err / adx;
   return dy < 0 ? y0 - off : y0 + off;
}

// the following table is block-copied from the specification
static float inverse_db_table[256] =
{
  1.0649863e-07f, 1.1341951e-07f, 1.2079015e-07f, 1.2863978e-07f,
  1.3699951e-07f, 1.4590251e-07f, 1.5538408e-07f, 1.6548181e-07f,
  1.7623575e-07f, 1.8768855e-07f, 1.9988561e-07f, 2.1287530e-07f,
  2.2670913e-07f, 2.4144197e-07f, 2.5713223e-07f, 2.7384213e-07f,
  2.9163793e-07f, 3.1059021e-07f, 3.3077411e-07f, 3.5226968e-07f,
  3.7516214e-07f, 3.9954229e-07f, 4.2550680e-07f, 4.5315863e-07f,
  4.8260743e-07f, 5.1396998e-07f, 5.4737065e-07f, 5.8294187e-07f,
  6.2082472e-07f, 6.6116941e-07f, 7.0413592e-07f, 7.4989464e-07f,
  7.9862701e-07f, 8.5052630e-07f, 9.0579828e-07f, 9.6466216e-07f,
  1.0273513e-06f, 1.0941144e-06f, 1.1652161e-06f, 1.2409384e-06f,
  1.3215816e-06f, 1.4074654e-06f, 1.4989305e-06f, 1.5963394e-06f,
  1.7000785e-06f, 1.8105592e-06f, 1.9282195e-06f, 2.0535261e-06f,
  2.1869758e-06f, 2.3290978e-06f, 2.4804557e-06f, 2.6416497e-06f,
  2.8133190e-06f, 2.9961443e-06f, 3.1908506e-06f, 3.3982101e-06f,
  3.6190449e-06f, 3.8542308e-06f, 4.1047004e-06f, 4.3714470e-06f,
  4.6555282e-06f, 4.9580707e-06f, 5.2802740e-06f, 5.6234160e-06f,
  5.9888572e-06f, 6.3780469e-06f, 6.7925283e-06f, 7.2339451e-06f,
  7.7040476e-06f, 8.2047000e-06f, 8.7378876e-06f, 9.3057248e-06f,
  9.9104632e-06f, 1.0554501e-05f, 1.1240392e-05f, 1.1970856e-05f,
  1.2748789e-05f, 1.3577278e-05f, 1.4459606e-05f, 1.5399272e-05f,
  1.6400004e-05f, 1.7465768e-05f, 1.8600792e-05f, 1.9809576e-05f,
  2.1096914e-05f, 2.2467911e-05f, 2.3928002e-05f, 2.5482978e-05f,
  2.7139006e-05f, 2.8902651e-05f, 3.0780908e-05f, 3.2781225e-05f,
  3.4911534e-05f, 3.7180282e-05f, 3.9596466e-05f, 4.2169667e-05f,
  4.4910090e-05f, 4.7828601e-05f, 5.0936773e-05f, 5.4246931e-05f,
  5.7772202e-05f, 6.1526565e-05f, 6.5524908e-05f, 6.9783085e-05f,
  7.4317983e-05f, 7.9147585e-05f, 8.4291040e-05f, 8.9768747e-05f,
  9.5602426e-05f, 0.00010181521f, 0.00010843174f, 0.00011547824f,
  0.00012298267f, 0.00013097477f, 0.00013948625f, 0.00014855085f,
  0.00015820453f, 0.00016848555f, 0.00017943469f, 0.00019109536f,
  0.00020351382f, 0.00021673929f, 0.00023082423f, 0.00024582449f,
  0.00026179955f, 0.00027881276f, 0.00029693158f, 0.00031622787f,
  0.00033677814f, 0.00035866388f, 0.00038197188f, 0.00040679456f,
  0.00043323036f, 0.00046138411f, 0.00049136745f, 0.00052329927f,
  0.00055730621f, 0.00059352311f, 0.00063209358f, 0.00067317058f,
  0.00071691700f, 0.00076350630f, 0.00081312324f, 0.00086596457f,
  0.00092223983f, 0.00098217216f, 0.0010459992f,  0.0011139742f,
  0.0011863665f,  0.0012634633f,  0.0013455702f,  0.0014330129f,
  0.0015261382f,  0.0016253153f,  0.0017309374f,  0.0018434235f,
  0.0019632195f,  0.0020908006f,  0.0022266726f,  0.0023713743f,
  0.0025254795f,  0.0026895994f,  0.0028643847f,  0.0030505286f,
  0.0032487691f,  0.0034598925f,  0.0036847358f,  0.0039241906f,
  0.0041792066f,  0.0044507950f,  0.0047400328f,  0.0050480668f,
  0.0053761186f,  0.0057254891f,  0.0060975636f,  0.0064938176f,
  0.0069158225f,  0.0073652516f,  0.0078438871f,  0.0083536271f,
  0.0088964928f,  0.009474637f,   0.010090352f,   0.010746080f,
  0.011444421f,   0.012188144f,   0.012980198f,   0.013823725f,
  0.014722068f,   0.015678791f,   0.016697687f,   0.017782797f,
  0.018938423f,   0.020169149f,   0.021479854f,   0.022875735f,
  0.024362330f,   0.025945531f,   0.027631618f,   0.029427276f,
  0.031339626f,   0.033376252f,   0.035545228f,   0.037855157f,
  0.040315199f,   0.042935108f,   0.045725273f,   0.048696758f,
  0.051861348f,   0.055231591f,   0.058820850f,   0.062643361f,
  0.066714279f,   0.071049749f,   0.075666962f,   0.080584227f,
  0.085821044f,   0.091398179f,   0.097337747f,   0.10366330f,
  0.11039993f,    0.11757434f,    0.12521498f,    0.13335215f,
  0.14201813f,    0.15124727f,    0.16107617f,    0.17154380f,
  0.18269168f,    0.19456402f,    0.20720788f,    0.22067342f,
  0.23501402f,    0.25028656f,    0.26655159f,    0.28387361f,
  0.30232132f,    0.32196786f,    0.34289114f,    0.36517414f,
  0.38890521f,    0.41417847f,    0.44109412f,    0.46975890f,
  0.50028648f,    0.53279791f,    0.56742212f,    0.60429640f,
  0.64356699f,    0.68538959f,    0.72993007f,    0.77736504f,
  0.82788260f,    0.88168307f,    0.9389798f,     1.0f
};


// @OPTIMIZE: if you want to replace this bresenham line-drawing routine,
// note that you must produce bit-identical output to decode correctly;
// this specific sequence of operations is specified in the spec (it's
// drawing integer-quantized frequency-space lines that the encoder
// expects to be exactly the same)
//     ... also, isn't the whole point of Bresenham's algorithm to NOT
// have to divide in the setup? sigh.
#ifndef STB_VORBIS_NO_DEFER_FLOOR
#define LINE_OP(a,b)   a *= b
#else
#define LINE_OP(a,b)   a = b
#endif

#ifdef STB_VORBIS_DIVIDE_TABLE
#define DIVTAB_NUMER   32
#define DIVTAB_DENOM   64
int8 integer_divide_table[DIVTAB_NUMER][DIVTAB_DENOM]; // 2KB
#endif

static __forceinline void draw_line(float *output, int x0, int y0, int x1, int y1, int n)
{
   int dy = y1 - y0;
   int adx = x1 - x0;
   int ady = abs(dy);
   int base;
   int x=x0,y=y0;
   int err = 0;
   int sy;

#ifdef STB_VORBIS_DIVIDE_TABLE
   if (adx < DIVTAB_DENOM && ady < DIVTAB_NUMER) {
      if (dy < 0) {
         base = -integer_divide_table[ady][adx];
         sy = base-1;
      } else {
         base =  integer_divide_table[ady][adx];
         sy = base+1;
      }
   } else {
      base = dy / adx;
      if (dy < 0)
         sy = base - 1;
      else
         sy = base+1;
   }
#else
   base = dy / adx;
   if (dy < 0)
      sy = base - 1;
   else
      sy = base+1;
#endif
   ady -= abs(base) * adx;
   if (x1 > n) x1 = n;
   if (x < x1) {
      LINE_OP(output[x], inverse_db_table[y&255]);
      for (++x; x < x1; ++x) {
         err += ady;
         if (err >= adx) {
            err -= adx;
            y += sy;
         } else
            y += base;
         LINE_OP(output[x], inverse_db_table[y&255]);
      }
   }
}

static int residue_decode(vorb *f, Codebook *book, float *target, int offset, int n, int rtype)
{
   int k;
   if (rtype == 0) {
      int step = n / book->dimensions;
      for (k=0; k < step; ++k)
         if (!codebook_decode_step(f, book, target+offset+k, n-offset-k, step))
            return FALSE;
   } else {
      for (k=0; k < n; ) {
         if (!codebook_decode(f, book, target+offset, n-k))
            return FALSE;
         k += book->dimensions;
         offset += book->dimensions;
      }
   }
   return TRUE;
}

// n is 1/2 of the blocksize --
// specification: "Correct per-vector decode length is [n]/2"
static void decode_residue(vorb *f, float *residue_buffers[], int ch, int n, int rn, uint8 *do_not_decode)
{
   int i,j,pass;
   Residue *r = f->residue_config + rn;
   int rtype = f->residue_types[rn];
   int c = r->classbook;
   int classwords = f->codebooks[c].dimensions;
   unsigned int actual_size = rtype == 2 ? n*2 : n;
   unsigned int limit_r_begin = (r->begin < actual_size ? r->begin : actual_size);
   unsigned int limit_r_end   = (r->end   < actual_size ? r->end   : actual_size);
   int n_read = limit_r_end - limit_r_begin;
   int part_read = n_read / r->part_size;
   int temp_alloc_point = temp_alloc_save(f);
   #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
   uint8 ***part_classdata = (uint8 ***) temp_block_array(f,f->channels, part_read * sizeof(**part_classdata));
   #else
   int **classifications = (int **) temp_block_array(f,f->channels, part_read * sizeof(**classifications));
   #endif

   CHECK(f);

   for (i=0; i < ch; ++i)
      if (!do_not_decode[i])
         memset(residue_buffers[i], 0, sizeof(float) * n);

   if (rtype == 2 && ch != 1) {
      for (j=0; j < ch; ++j)
         if (!do_not_decode[j])
            break;
      if (j == ch)
         goto done;

      for (pass=0; pass < 8; ++pass) {
         int pcount = 0, class_set = 0;
         if (ch == 2) {
            while (pcount < part_read) {
               int z = r->begin + pcount*r->part_size;
               int c_inter = (z & 1), p_inter = z>>1;
               if (pass == 0) {
                  Codebook *c = f->codebooks+r->classbook;
                  int q;
                  DECODE(q,f,c);
                  if (q == EOP) goto done;
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  part_classdata[0][class_set] = r->classdata[q];
                  #else
                  for (i=classwords-1; i >= 0; --i) {
                     classifications[0][i+pcount] = q % r->classifications;
                     q /= r->classifications;
                  }
                  #endif
               }
               for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) {
                  int z = r->begin + pcount*r->part_size;
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  int c = part_classdata[0][class_set][i];
                  #else
                  int c = classifications[0][pcount];
                  #endif
                  int b = r->residue_books[c][pass];
                  if (b >= 0) {
                     Codebook *book = f->codebooks + b;
                     #ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
                     if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                        goto done;
                     #else
                     // saves 1%
                     if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                        goto done;
                     #endif
                  } else {
                     z += r->part_size;
                     c_inter = z & 1;
                     p_inter = z >> 1;
                  }
               }
               #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
               ++class_set;
               #endif
            }
         } else if (ch > 2) {
            while (pcount < part_read) {
               int z = r->begin + pcount*r->part_size;
               int c_inter = z % ch, p_inter = z/ch;
               if (pass == 0) {
                  Codebook *c = f->codebooks+r->classbook;
                  int q;
                  DECODE(q,f,c);
                  if (q == EOP) goto done;
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  part_classdata[0][class_set] = r->classdata[q];
                  #else
                  for (i=classwords-1; i >= 0; --i) {
                     classifications[0][i+pcount] = q % r->classifications;
                     q /= r->classifications;
                  }
                  #endif
               }
               for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) {
                  int z = r->begin + pcount*r->part_size;
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  int c = part_classdata[0][class_set][i];
                  #else
                  int c = classifications[0][pcount];
                  #endif
                  int b = r->residue_books[c][pass];
                  if (b >= 0) {
                     Codebook *book = f->codebooks + b;
                     if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                        goto done;
                  } else {
                     z += r->part_size;
                     c_inter = z % ch;
                     p_inter = z / ch;
                  }
               }
               #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
               ++class_set;
               #endif
            }
         }
      }
      goto done;
   }
   CHECK(f);

   for (pass=0; pass < 8; ++pass) {
      int pcount = 0, class_set=0;
      while (pcount < part_read) {
         if (pass == 0) {
            for (j=0; j < ch; ++j) {
               if (!do_not_decode[j]) {
                  Codebook *c = f->codebooks+r->classbook;
                  int temp;
                  DECODE(temp,f,c);
                  if (temp == EOP) goto done;
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  part_classdata[j][class_set] = r->classdata[temp];
                  #else
                  for (i=classwords-1; i >= 0; --i) {
                     classifications[j][i+pcount] = temp % r->classifications;
                     temp /= r->classifications;
                  }
                  #endif
               }
            }
         }
         for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) {
            for (j=0; j < ch; ++j) {
               if (!do_not_decode[j]) {
                  #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
                  int c = part_classdata[j][class_set][i];
                  #else
                  int c = classifications[j][pcount];
                  #endif
                  int b = r->residue_books[c][pass];
                  if (b >= 0) {
                     float *target = residue_buffers[j];
                     int offset = r->begin + pcount * r->part_size;
                     int n = r->part_size;
                     Codebook *book = f->codebooks + b;
                     if (!residue_decode(f, book, target, offset, n, rtype))
                        goto done;
                  }
               }
            }
         }
         #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
         ++class_set;
         #endif
      }
   }
  done:
   CHECK(f);
   #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
   temp_free(f,part_classdata);
   #else
   temp_free(f,classifications);
   #endif
   temp_alloc_restore(f,temp_alloc_point);
}


#if 0
// slow way for debugging
void inverse_mdct_slow(float *buffer, int n)
{
   int i,j;
   int n2 = n >> 1;
   float *x = (float *) malloc(sizeof(*x) * n2);
   memcpy(x, buffer, sizeof(*x) * n2);
   for (i=0; i < n; ++i) {
      float acc = 0;
      for (j=0; j < n2; ++j)
         // formula from paper:
         //acc += n/4.0f * x[j] * (float) cos(M_PI / 2 / n * (2 * i + 1 + n/2.0)*(2*j+1));
         // formula from wikipedia
         //acc += 2.0f / n2 * x[j] * (float) cos(M_PI/n2 * (i + 0.5 + n2/2)*(j + 0.5));
         // these are equivalent, except the formula from the paper inverts the multiplier!
         // however, what actually works is NO MULTIPLIER!?!
         //acc += 64 * 2.0f / n2 * x[j] * (float) cos(M_PI/n2 * (i + 0.5 + n2/2)*(j + 0.5));
         acc += x[j] * (float) cos(M_PI / 2 / n * (2 * i + 1 + n/2.0)*(2*j+1));
      buffer[i] = acc;
   }
   free(x);
}
#elif 0
// same as above, but just barely able to run in real time on modern machines
void inverse_mdct_slow(float *buffer, int n, vorb *f, int blocktype)
{
   float mcos[16384];
   int i,j;
   int n2 = n >> 1, nmask = (n << 2) -1;
   float *x = (float *) malloc(sizeof(*x) * n2);
   memcpy(x, buffer, sizeof(*x) * n2);
   for (i=0; i < 4*n; ++i)
      mcos[i] = (float) cos(M_PI / 2 * i / n);

   for (i=0; i < n; ++i) {
      float acc = 0;
      for (j=0; j < n2; ++j)
         acc += x[j] * mcos[(2 * i + 1 + n2)*(2*j+1) & nmask];
      buffer[i] = acc;
   }
   free(x);
}
#elif 0
// transform to use a slow dct-iv; this is STILL basically trivial,
// but only requires half as many ops
void dct_iv_slow(float *buffer, int n)
{
   float mcos[16384];
   float x[2048];
   int i,j;
   int n2 = n >> 1, nmask = (n << 3) - 1;
   memcpy(x, buffer, sizeof(*x) * n);
   for (i=0; i < 8*n; ++i)
      mcos[i] = (float) cos(M_PI / 4 * i / n);
   for (i=0; i < n; ++i) {
      float acc = 0;
      for (j=0; j < n; ++j)
         acc += x[j] * mcos[((2 * i + 1)*(2*j+1)) & nmask];
      buffer[i] = acc;
   }
}

void inverse_mdct_slow(float *buffer, int n, vorb *f, int blocktype)
{
   int i, n4 = n >> 2, n2 = n >> 1, n3_4 = n - n4;
   float temp[4096];

   memcpy(temp, buffer, n2 * sizeof(float));
   dct_iv_slow(temp, n2);  // returns -c'-d, a-b'

   for (i=0; i < n4  ; ++i) buffer[i] = temp[i+n4];            // a-b'
   for (   ; i < n3_4; ++i) buffer[i] = -temp[n3_4 - i - 1];   // b-a', c+d'
   for (   ; i < n   ; ++i) buffer[i] = -temp[i - n3_4];       // c'+d
}
#endif

#ifndef LIBVORBIS_MDCT
#define LIBVORBIS_MDCT 0
#endif

#if LIBVORBIS_MDCT
// directly call the vorbis MDCT using an interface documented
// by Jeff Roberts... useful for performance comparison
typedef struct
{
  int n;
  int log2n;

  float *trig;
  int   *bitrev;

  float scale;
} mdct_lookup;

extern void mdct_init(mdct_lookup *lookup, int n);
extern void mdct_clear(mdct_lookup *l);
extern void mdct_backward(mdct_lookup *init, float *in, float *out);

mdct_lookup M1,M2;

void inverse_mdct(float *buffer, int n, vorb *f, int blocktype)
{
   mdct_lookup *M;
   if (M1.n == n) M = &M1;
   else if (M2.n == n) M = &M2;
   else if (M1.n == 0) { mdct_init(&M1, n); M = &M1; }
   else {
      if (M2.n) __asm int 3;
      mdct_init(&M2, n);
      M = &M2;
   }

   mdct_backward(M, buffer, buffer);
}
#endif


// the following were split out into separate functions while optimizing;
// they could be pushed back up but eh. __forceinline showed no change;
// they're probably already being inlined.
static void imdct_step3_iter0_loop(int n, float *e, int i_off, int k_off, float *A)
{
   float *ee0 = e + i_off;
   float *ee2 = ee0 + k_off;
   int i;

   assert((n & 3) == 0);
   for (i=(n>>2); i > 0; --i) {
      float k00_20, k01_21;
      k00_20  = ee0[ 0] - ee2[ 0];
      k01_21  = ee0[-1] - ee2[-1];
      ee0[ 0] += ee2[ 0];//ee0[ 0] = ee0[ 0] + ee2[ 0];
      ee0[-1] += ee2[-1];//ee0[-1] = ee0[-1] + ee2[-1];
      ee2[ 0] = k00_20 * A[0] - k01_21 * A[1];
      ee2[-1] = k01_21 * A[0] + k00_20 * A[1];
      A += 8;

      k00_20  = ee0[-2] - ee2[-2];
      k01_21  = ee0[-3] - ee2[-3];
      ee0[-2] += ee2[-2];//ee0[-2] = ee0[-2] + ee2[-2];
      ee0[-3] += ee2[-3];//ee0[-3] = ee0[-3] + ee2[-3];
      ee2[-2] = k00_20 * A[0] - k01_21 * A[1];
      ee2[-3] = k01_21 * A[0] + k00_20 * A[1];
      A += 8;

      k00_20  = ee0[-4] - ee2[-4];
      k01_21  = ee0[-5] - ee2[-5];
      ee0[-4] += ee2[-4];//ee0[-4] = ee0[-4] + ee2[-4];
      ee0[-5] += ee2[-5];//ee0[-5] = ee0[-5] + ee2[-5];
      ee2[-4] = k00_20 * A[0] - k01_21 * A[1];
      ee2[-5] = k01_21 * A[0] + k00_20 * A[1];
      A += 8;

      k00_20  = ee0[-6] - ee2[-6];
      k01_21  = ee0[-7] - ee2[-7];
      ee0[-6] += ee2[-6];//ee0[-6] = ee0[-6] + ee2[-6];
      ee0[-7] += ee2[-7];//ee0[-7] = ee0[-7] + ee2[-7];
      ee2[-6] = k00_20 * A[0] - k01_21 * A[1];
      ee2[-7] = k01_21 * A[0] + k00_20 * A[1];
      A += 8;
      ee0 -= 8;
      ee2 -= 8;
   }
}

static void imdct_step3_inner_r_loop(int lim, float *e, int d0, int k_off, float *A, int k1)
{
   int i;
   float k00_20, k01_21;

   float *e0 = e + d0;
   float *e2 = e0 + k_off;

   for (i=lim >> 2; i > 0; --i) {
      k00_20 = e0[-0] - e2[-0];
      k01_21 = e0[-1] - e2[-1];
      e0[-0] += e2[-0];//e0[-0] = e0[-0] + e2[-0];
      e0[-1] += e2[-1];//e0[-1] = e0[-1] + e2[-1];
      e2[-0] = (k00_20)*A[0] - (k01_21) * A[1];
      e2[-1] = (k01_21)*A[0] + (k00_20) * A[1];

      A += k1;

      k00_20 = e0[-2] - e2[-2];
      k01_21 = e0[-3] - e2[-3];
      e0[-2] += e2[-2];//e0[-2] = e0[-2] + e2[-2];
      e0[-3] += e2[-3];//e0[-3] = e0[-3] + e2[-3];
      e2[-2] = (k00_20)*A[0] - (k01_21) * A[1];
      e2[-3] = (k01_21)*A[0] + (k00_20) * A[1];

      A += k1;

      k00_20 = e0[-4] - e2[-4];
      k01_21 = e0[-5] - e2[-5];
      e0[-4] += e2[-4];//e0[-4] = e0[-4] + e2[-4];
      e0[-5] += e2[-5];//e0[-5] = e0[-5] + e2[-5];
      e2[-4] = (k00_20)*A[0] - (k01_21) * A[1];
      e2[-5] = (k01_21)*A[0] + (k00_20) * A[1];

      A += k1;

      k00_20 = e0[-6] - e2[-6];
      k01_21 = e0[-7] - e2[-7];
      e0[-6] += e2[-6];//e0[-6] = e0[-6] + e2[-6];
      e0[-7] += e2[-7];//e0[-7] = e0[-7] + e2[-7];
      e2[-6] = (k00_20)*A[0] - (k01_21) * A[1];
      e2[-7] = (k01_21)*A[0] + (k00_20) * A[1];

      e0 -= 8;
      e2 -= 8;

      A += k1;
   }
}

static void imdct_step3_inner_s_loop(int n, float *e, int i_off, int k_off, float *A, int a_off, int k0)
{
   int i;
   float A0 = A[0];
   float A1 = A[0+1];
   float A2 = A[0+a_off];
   float A3 = A[0+a_off+1];
   float A4 = A[0+a_off*2+0];
   float A5 = A[0+a_off*2+1];
   float A6 = A[0+a_off*3+0];
   float A7 = A[0+a_off*3+1];

   float k00,k11;

   float *ee0 = e  +i_off;
   float *ee2 = ee0+k_off;

   for (i=n; i > 0; --i) {
      k00     = ee0[ 0] - ee2[ 0];
      k11     = ee0[-1] - ee2[-1];
      ee0[ 0] =  ee0[ 0] + ee2[ 0];
      ee0[-1] =  ee0[-1] + ee2[-1];
      ee2[ 0] = (k00) * A0 - (k11) * A1;
      ee2[-1] = (k11) * A0 + (k00) * A1;

      k00     = ee0[-2] - ee2[-2];
      k11     = ee0[-3] - ee2[-3];
      ee0[-2] =  ee0[-2] + ee2[-2];
      ee0[-3] =  ee0[-3] + ee2[-3];
      ee2[-2] = (k00) * A2 - (k11) * A3;
      ee2[-3] = (k11) * A2 + (k00) * A3;

      k00     = ee0[-4] - ee2[-4];
      k11     = ee0[-5] - ee2[-5];
      ee0[-4] =  ee0[-4] + ee2[-4];
      ee0[-5] =  ee0[-5] + ee2[-5];
      ee2[-4] = (k00) * A4 - (k11) * A5;
      ee2[-5] = (k11) * A4 + (k00) * A5;

      k00     = ee0[-6] - ee2[-6];
      k11     = ee0[-7] - ee2[-7];
      ee0[-6] =  ee0[-6] + ee2[-6];
      ee0[-7] =  ee0[-7] + ee2[-7];
      ee2[-6] = (k00) * A6 - (k11) * A7;
      ee2[-7] = (k11) * A6 + (k00) * A7;

      ee0 -= k0;
      ee2 -= k0;
   }
}

static __forceinline void iter_54(float *z)
{
   float k00,k11,k22,k33;
   float y0,y1,y2,y3;

   k00  = z[ 0] - z[-4];
   y0   = z[ 0] + z[-4];
   y2   = z[-2] + z[-6];
   k22  = z[-2] - z[-6];

   z[-0] = y0 + y2;      // z0 + z4 + z2 + z6
   z[-2] = y0 - y2;      // z0 + z4 - z2 - z6

   // done with y0,y2

   k33  = z[-3] - z[-7];

   z[-4] = k00 + k33;    // z0 - z4 + z3 - z7
   z[-6] = k00 - k33;    // z0 - z4 - z3 + z7

   // done with k33

   k11  = z[-1] - z[-5];
   y1   = z[-1] + z[-5];
   y3   = z[-3] + z[-7];

   z[-1] = y1 + y3;      // z1 + z5 + z3 + z7
   z[-3] = y1 - y3;      // z1 + z5 - z3 - z7
   z[-5] = k11 - k22;    // z1 - z5 + z2 - z6
   z[-7] = k11 + k22;    // z1 - z5 - z2 + z6
}

static void imdct_step3_inner_s_loop_ld654(int n, float *e, int i_off, float *A, int base_n)
{
   int a_off = base_n >> 3;
   float A2 = A[0+a_off];
   float *z = e + i_off;
   float *base = z - 16 * n;

   while (z > base) {
      float k00,k11;
      float l00,l11;

      k00    = z[-0] - z[ -8];
      k11    = z[-1] - z[ -9];
      l00    = z[-2] - z[-10];
      l11    = z[-3] - z[-11];
      z[ -0] = z[-0] + z[ -8];
      z[ -1] = z[-1] + z[ -9];
      z[ -2] = z[-2] + z[-10];
      z[ -3] = z[-3] + z[-11];
      z[ -8] = k00;
      z[ -9] = k11;
      z[-10] = (l00+l11) * A2;
      z[-11] = (l11-l00) * A2;

      k00    = z[ -4] - z[-12];
      k11    = z[ -5] - z[-13];
      l00    = z[ -6] - z[-14];
      l11    = z[ -7] - z[-15];
      z[ -4] = z[ -4] + z[-12];
      z[ -5] = z[ -5] + z[-13];
      z[ -6] = z[ -6] + z[-14];
      z[ -7] = z[ -7] + z[-15];
      z[-12] = k11;
      z[-13] = -k00;
      z[-14] = (l11-l00) * A2;
      z[-15] = (l00+l11) * -A2;

      iter_54(z);
      iter_54(z-8);
      z -= 16;
   }
}

static void inverse_mdct(float *buffer, int n, vorb *f, int blocktype)
{
   int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l;
   int ld;
   // @OPTIMIZE: reduce register pressure by using fewer variables?
   int save_point = temp_alloc_save(f);
   float *buf2 = (float *) temp_alloc(f, n2 * sizeof(*buf2));
   float *u=NULL,*v=NULL;
   // twiddle factors
   float *A = f->A[blocktype];

   // IMDCT algorithm from "The use of multirate filter banks for coding of high quality digital audio"
   // See notes about bugs in that paper in less-optimal implementation 'inverse_mdct_old' after this function.

   // kernel from paper


   // merged:
   //   copy and reflect spectral data
   //   step 0

   // note that it turns out that the items added together during
   // this step are, in fact, being added to themselves (as reflected
   // by step 0). inexplicable inefficiency! this became obvious
   // once I combined the passes.

   // so there's a missing 'times 2' here (for adding X to itself).
   // this propagates through linearly to the end, where the numbers
   // are 1/2 too small, and need to be compensated for.

   {
      float *d,*e, *AA, *e_stop;
      d = &buf2[n2-2];
      AA = A;
      e = &buffer[0];
      e_stop = &buffer[n2];
      while (e != e_stop) {
         d[1] = (e[0] * AA[0] - e[2]*AA[1]);
         d[0] = (e[0] * AA[1] + e[2]*AA[0]);
         d -= 2;
         AA += 2;
         e += 4;
      }

      e = &buffer[n2-3];
      while (d >= buf2) {
         d[1] = (-e[2] * AA[0] - -e[0]*AA[1]);
         d[0] = (-e[2] * AA[1] + -e[0]*AA[0]);
         d -= 2;
         AA += 2;
         e -= 4;
      }
   }

   // now we use symbolic names for these, so that we can
   // possibly swap their meaning as we change which operations
   // are in place

   u = buffer;
   v = buf2;

   // step 2    (paper output is w, now u)
   // this could be in place, but the data ends up in the wrong
   // place... _somebody_'s got to swap it, so this is nominated
   {
      float *AA = &A[n2-8];
      float *d0,*d1, *e0, *e1;

      e0 = &v[n4];
      e1 = &v[0];

      d0 = &u[n4];
      d1 = &u[0];

      while (AA >= A) {
         float v40_20, v41_21;

         v41_21 = e0[1] - e1[1];
         v40_20 = e0[0] - e1[0];
         d0[1]  = e0[1] + e1[1];
         d0[0]  = e0[0] + e1[0];
         d1[1]  = v41_21*AA[4] - v40_20*AA[5];
         d1[0]  = v40_20*AA[4] + v41_21*AA[5];

         v41_21 = e0[3] - e1[3];
         v40_20 = e0[2] - e1[2];
         d0[3]  = e0[3] + e1[3];
         d0[2]  = e0[2] + e1[2];
         d1[3]  = v41_21*AA[0] - v40_20*AA[1];
         d1[2]  = v40_20*AA[0] + v41_21*AA[1];

         AA -= 8;

         d0 += 4;
         d1 += 4;
         e0 += 4;
         e1 += 4;
      }
   }

   // step 3
   ld = ilog(n) - 1; // ilog is off-by-one from normal definitions

   // optimized step 3:

   // the original step3 loop can be nested r inside s or s inside r;
   // it's written originally as s inside r, but this is dumb when r
   // iterates many times, and s few. So I have two copies of it and
   // switch between them halfway.

   // this is iteration 0 of step 3
   imdct_step3_iter0_loop(n >> 4, u, n2-1-n4*0, -(n >> 3), A);
   imdct_step3_iter0_loop(n >> 4, u, n2-1-n4*1, -(n >> 3), A);

   // this is iteration 1 of step 3
   imdct_step3_inner_r_loop(n >> 5, u, n2-1 - n8*0, -(n >> 4), A, 16);
   imdct_step3_inner_r_loop(n >> 5, u, n2-1 - n8*1, -(n >> 4), A, 16);
   imdct_step3_inner_r_loop(n >> 5, u, n2-1 - n8*2, -(n >> 4), A, 16);
   imdct_step3_inner_r_loop(n >> 5, u, n2-1 - n8*3, -(n >> 4), A, 16);

   l=2;
   for (; l < (ld-3)>>1; ++l) {
      int k0 = n >> (l+2), k0_2 = k0>>1;
      int lim = 1 << (l+1);
      int i;
      for (i=0; i < lim; ++i)
         imdct_step3_inner_r_loop(n >> (l+4), u, n2-1 - k0*i, -k0_2, A, 1 << (l+3));
   }

   for (; l < ld-6; ++l) {
      int k0 = n >> (l+2), k1 = 1 << (l+3), k0_2 = k0>>1;
      int rlim = n >> (l+6), r;
      int lim = 1 << (l+1);
      int i_off;
      float *A0 = A;
      i_off = n2-1;
      for (r=rlim; r > 0; --r) {
         imdct_step3_inner_s_loop(lim, u, i_off, -k0_2, A0, k1, k0);
         A0 += k1*4;
         i_off -= 8;
      }
   }

   // iterations with count:
   //   ld-6,-5,-4 all interleaved together
   //       the big win comes from getting rid of needless flops
   //         due to the constants on pass 5 & 4 being all 1 and 0;
   //       combining them to be simultaneous to improve cache made little difference
   imdct_step3_inner_s_loop_ld654(n >> 5, u, n2-1, A, n);

   // output is u

   // step 4, 5, and 6
   // cannot be in-place because of step 5
   {
      uint16 *bitrev = f->bit_reverse[blocktype];
      // weirdly, I'd have thought reading sequentially and writing
      // erratically would have been better than vice-versa, but in
      // fact that's not what my testing showed. (That is, with
      // j = bitreverse(i), do you read i and write j, or read j and write i.)

      float *d0 = &v[n4-4];
      float *d1 = &v[n2-4];
      while (d0 >= v) {
         int k4;

         k4 = bitrev[0];
         d1[3] = u[k4+0];
         d1[2] = u[k4+1];
         d0[3] = u[k4+2];
         d0[2] = u[k4+3];

         k4 = bitrev[1];
         d1[1] = u[k4+0];
         d1[0] = u[k4+1];
         d0[1] = u[k4+2];
         d0[0] = u[k4+3];

         d0 -= 4;
         d1 -= 4;
         bitrev += 2;
      }
   }
   // (paper output is u, now v)


   // data must be in buf2
   assert(v == buf2);

   // step 7   (paper output is v, now v)
   // this is now in place
   {
      float *C = f->C[blocktype];
      float *d, *e;

      d = v;
      e = v + n2 - 4;

      while (d < e) {
         float a02,a11,b0,b1,b2,b3;

         a02 = d[0] - e[2];
         a11 = d[1] + e[3];

         b0 = C[1]*a02 + C[0]*a11;
         b1 = C[1]*a11 - C[0]*a02;

         b2 = d[0] + e[ 2];
         b3 = d[1] - e[ 3];

         d[0] = b2 + b0;
         d[1] = b3 + b1;
         e[2] = b2 - b0;
         e[3] = b1 - b3;

         a02 = d[2] - e[0];
         a11 = d[3] + e[1];

         b0 = C[3]*a02 + C[2]*a11;
         b1 = C[3]*a11 - C[2]*a02;

         b2 = d[2] + e[ 0];
         b3 = d[3] - e[ 1];

         d[2] = b2 + b0;
         d[3] = b3 + b1;
         e[0] = b2 - b0;
         e[1] = b1 - b3;

         C += 4;
         d += 4;
         e -= 4;
      }
   }

   // data must be in buf2


   // step 8+decode   (paper output is X, now buffer)
   // this generates pairs of data a la 8 and pushes them directly through
   // the decode kernel (pushing rather than pulling) to avoid having
   // to make another pass later

   // this cannot POSSIBLY be in place, so we refer to the buffers directly

   {
      float *d0,*d1,*d2,*d3;

      float *B = f->B[blocktype] + n2 - 8;
      float *e = buf2 + n2 - 8;
      d0 = &buffer[0];
      d1 = &buffer[n2-4];
      d2 = &buffer[n2];
      d3 = &buffer[n-4];
      while (e >= v) {
         float p0,p1,p2,p3;

         p3 =  e[6]*B[7] - e[7]*B[6];
         p2 = -e[6]*B[6] - e[7]*B[7];

         d0[0] =   p3;
         d1[3] = - p3;
         d2[0] =   p2;
         d3[3] =   p2;

         p1 =  e[4]*B[5] - e[5]*B[4];
         p0 = -e[4]*B[4] - e[5]*B[5];

         d0[1] =   p1;
         d1[2] = - p1;
         d2[1] =   p0;
         d3[2] =   p0;

         p3 =  e[2]*B[3] - e[3]*B[2];
         p2 = -e[2]*B[2] - e[3]*B[3];

         d0[2] =   p3;
         d1[1] = - p3;
         d2[2] =   p2;
         d3[1] =   p2;

         p1 =  e[0]*B[1] - e[1]*B[0];
         p0 = -e[0]*B[0] - e[1]*B[1];

         d0[3] =   p1;
         d1[0] = - p1;
         d2[3] =   p0;
         d3[0] =   p0;

         B -= 8;
         e -= 8;
         d0 += 4;
         d2 += 4;
         d1 -= 4;
         d3 -= 4;
      }
   }

   temp_free(f,buf2);
   temp_alloc_restore(f,save_point);
}

#if 0
// this is the original version of the above code, if you want to optimize it from scratch
void inverse_mdct_naive(float *buffer, int n)
{
   float s;
   float A[1 << 12], B[1 << 12], C[1 << 11];
   int i,k,k2,k4, n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l;
   int n3_4 = n - n4, ld;
   // how can they claim this only uses N words?!
   // oh, because they're only used sparsely, whoops
   float u[1 << 13], X[1 << 13], v[1 << 13], w[1 << 13];
   // set up twiddle factors

   for (k=k2=0; k < n4; ++k,k2+=2) {
      A[k2  ] = (float)  cos(4*k*M_PI/n);
      A[k2+1] = (float) -sin(4*k*M_PI/n);
      B[k2  ] = (float)  cos((k2+1)*M_PI/n/2);
      B[k2+1] = (float)  sin((k2+1)*M_PI/n/2);
   }
   for (k=k2=0; k < n8; ++k,k2+=2) {
      C[k2  ] = (float)  cos(2*(k2+1)*M_PI/n);
      C[k2+1] = (float) -sin(2*(k2+1)*M_PI/n);
   }

   // IMDCT algorithm from "The use of multirate filter banks for coding of high quality digital audio"
   // Note there are bugs in that pseudocode, presumably due to them attempting
   // to rename the arrays nicely rather than representing the way their actual
   // implementation bounces buffers back and forth. As a result, even in the
   // "some formulars corrected" version, a direct implementation fails. These
   // are noted below as "paper bug".

   // copy and reflect spectral data
   for (k=0; k < n2; ++k) u[k] = buffer[k];
   for (   ; k < n ; ++k) u[k] = -buffer[n - k - 1];
   // kernel from paper
   // step 1
   for (k=k2=k4=0; k < n4; k+=1, k2+=2, k4+=4) {
      v[n-k4-1] = (u[k4] - u[n-k4-1]) * A[k2]   - (u[k4+2] - u[n-k4-3])*A[k2+1];
      v[n-k4-3] = (u[k4] - u[n-k4-1]) * A[k2+1] + (u[k4+2] - u[n-k4-3])*A[k2];
   }
   // step 2
   for (k=k4=0; k < n8; k+=1, k4+=4) {
      w[n2+3+k4] = v[n2+3+k4] + v[k4+3];
      w[n2+1+k4] = v[n2+1+k4] + v[k4+1];
      w[k4+3]    = (v[n2+3+k4] - v[k4+3])*A[n2-4-k4] - (v[n2+1+k4]-v[k4+1])*A[n2-3-k4];
      w[k4+1]    = (v[n2+1+k4] - v[k4+1])*A[n2-4-k4] + (v[n2+3+k4]-v[k4+3])*A[n2-3-k4];
   }
   // step 3
   ld = ilog(n) - 1; // ilog is off-by-one from normal definitions
   for (l=0; l < ld-3; ++l) {
      int k0 = n >> (l+2), k1 = 1 << (l+3);
      int rlim = n >> (l+4), r4, r;
      int s2lim = 1 << (l+2), s2;
      for (r=r4=0; r < rlim; r4+=4,++r) {
         for (s2=0; s2 < s2lim; s2+=2) {
            u[n-1-k0*s2-r4] = w[n-1-k0*s2-r4] + w[n-1-k0*(s2+1)-r4];
            u[n-3-k0*s2-r4] = w[n-3-k0*s2-r4] + w[n-3-k0*(s2+1)-r4];
            u[n-1-k0*(s2+1)-r4] = (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1]
                                - (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1+1];
            u[n-3-k0*(s2+1)-r4] = (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1]
                                + (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1+1];
         }
      }
      if (l+1 < ld-3) {
         // paper bug: ping-ponging of u&w here is omitted
         memcpy(w, u, sizeof(u));
      }
   }

   // step 4
   for (i=0; i < n8; ++i) {
      int j = bit_reverse(i) >> (32-ld+3);
      assert(j < n8);
      if (i == j) {
         // paper bug: original code probably swapped in place; if copying,
         //            need to directly copy in this case
         int i8 = i << 3;
         v[i8+1] = u[i8+1];
         v[i8+3] = u[i8+3];
         v[i8+5] = u[i8+5];
         v[i8+7] = u[i8+7];
      } else if (i < j) {
         int i8 = i << 3, j8 = j << 3;
         v[j8+1] = u[i8+1], v[i8+1] = u[j8 + 1];
         v[j8+3] = u[i8+3], v[i8+3] = u[j8 + 3];
         v[j8+5] = u[i8+5], v[i8+5] = u[j8 + 5];
         v[j8+7] = u[i8+7], v[i8+7] = u[j8 + 7];
      }
   }
   // step 5
   for (k=0; k < n2; ++k) {
      w[k] = v[k*2+1];
   }
   // step 6
   for (k=k2=k4=0; k < n8; ++k, k2 += 2, k4 += 4) {
      u[n-1-k2] = w[k4];
      u[n-2-k2] = w[k4+1];
      u[n3_4 - 1 - k2] = w[k4+2];
      u[n3_4 - 2 - k2] = w[k4+3];
   }
   // step 7
   for (k=k2=0; k < n8; ++k, k2 += 2) {
      v[n2 + k2 ] = ( u[n2 + k2] + u[n-2-k2] + C[k2+1]*(u[n2+k2]-u[n-2-k2]) + C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2;
      v[n-2 - k2] = ( u[n2 + k2] + u[n-2-k2] - C[k2+1]*(u[n2+k2]-u[n-2-k2]) - C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2;
      v[n2+1+ k2] = ( u[n2+1+k2] - u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2;
      v[n-1 - k2] = (-u[n2+1+k2] + u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2;
   }
   // step 8
   for (k=k2=0; k < n4; ++k,k2 += 2) {
      X[k]      = v[k2+n2]*B[k2  ] + v[k2+1+n2]*B[k2+1];
      X[n2-1-k] = v[k2+n2]*B[k2+1] - v[k2+1+n2]*B[k2  ];
   }

   // decode kernel to output
   // determined the following value experimentally
   // (by first figuring out what made inverse_mdct_slow work); then matching that here
   // (probably vorbis encoder premultiplies by n or n/2, to save it on the decoder?)
   s = 0.5; // theoretically would be n4

   // [[[ note! the s value of 0.5 is compensated for by the B[] in the current code,
   //     so it needs to use the "old" B values to behave correctly, or else
   //     set s to 1.0 ]]]
   for (i=0; i < n4  ; ++i) buffer[i] = s * X[i+n4];
   for (   ; i < n3_4; ++i) buffer[i] = -s * X[n3_4 - i - 1];
   for (   ; i < n   ; ++i) buffer[i] = -s * X[i - n3_4];
}
#endif

static float *get_window(vorb *f, int len)
{
   len <<= 1;
   if (len == f->blocksize_0) return f->window[0];
   if (len == f->blocksize_1) return f->window[1];
   return NULL;
}

#ifndef STB_VORBIS_NO_DEFER_FLOOR
typedef int16 YTYPE;
#else
typedef int YTYPE;
#endif
static int do_floor(vorb *f, Mapping *map, int i, int n, float *target, YTYPE *finalY, uint8 *step2_flag)
{
   int n2 = n >> 1;
   int s = map->chan[i].mux, floor;
   floor = map->submap_floor[s];
   if (f->floor_types[floor] == 0) {
      return error(f, VORBIS_invalid_stream);
   } else {
      Floor1 *g = &f->floor_config[floor].floor1;
      int j,q;
      int lx = 0, ly = finalY[0] * g->floor1_multiplier;
      for (q=1; q < g->values; ++q) {
         j = g->sorted_order[q];
         #ifndef STB_VORBIS_NO_DEFER_FLOOR
         STBV_NOTUSED(step2_flag);
         if (finalY[j] >= 0)
         #else
         if (step2_flag[j])
         #endif
         {
            int hy = finalY[j] * g->floor1_multiplier;
            int hx = g->Xlist[j];
            if (lx != hx)
               draw_line(target, lx,ly, hx,hy, n2);
            CHECK(f);
            lx = hx, ly = hy;
         }
      }
      if (lx < n2) {
         // optimization of: draw_line(target, lx,ly, n,ly, n2);
         for (j=lx; j < n2; ++j)
            LINE_OP(target[j], inverse_db_table[ly]);
         CHECK(f);
      }
   }
   return TRUE;
}

// The meaning of "left" and "right"
//
// For a given frame:
//     we compute samples from 0..n
//     window_center is n/2
//     we'll window and mix the samples from left_start to left_end with data from the previous frame
//     all of the samples from left_end to right_start can be output without mixing; however,
//        this interval is 0-length except when transitioning between short and long frames
//     all of the samples from right_start to right_end need to be mixed with the next frame,
//        which we don't have, so those get saved in a buffer
//     frame N's right_end-right_start, the number of samples to mix with the next frame,
//        has to be the same as frame N+1's left_end-left_start (which they are by
//        construction)

static int vorbis_decode_initial(vorb *f, int *p_left_start, int *p_left_end, int *p_right_start, int *p_right_end, int *mode)
{
   Mode *m;
   int i, n, prev, next, window_center;
   f->channel_buffer_start = f->channel_buffer_end = 0;

  retry:
   if (f->eof) return FALSE;
   if (!maybe_start_packet(f))
      return FALSE;
   // check packet type
   if (get_bits(f,1) != 0) {
      if (IS_PUSH_MODE(f))
         return error(f,VORBIS_bad_packet_type);
      while (EOP != get8_packet(f));
      goto retry;
   }

   if (f->alloc.alloc_buffer)
      assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);

   i = get_bits(f, ilog(f->mode_count-1));
   if (i == EOP) return FALSE;
   if (i >= f->mode_count) return FALSE;
   *mode = i;
   m = f->mode_config + i;
   if (m->blockflag) {
      n = f->blocksize_1;
      prev = get_bits(f,1);
      next = get_bits(f,1);
   } else {
      prev = next = 0;
      n = f->blocksize_0;
   }

// WINDOWING

   window_center = n >> 1;
   if (m->blockflag && !prev) {
      *p_left_start = (n - f->blocksize_0) >> 2;
      *p_left_end   = (n + f->blocksize_0) >> 2;
   } else {
      *p_left_start = 0;
      *p_left_end   = window_center;
   }
   if (m->blockflag && !next) {
      *p_right_start = (n*3 - f->blocksize_0) >> 2;
      *p_right_end   = (n*3 + f->blocksize_0) >> 2;
   } else {
      *p_right_start = window_center;
      *p_right_end   = n;
   }

   return TRUE;
}

static int vorbis_decode_packet_rest(vorb *f, int *len, Mode *m, int left_start, int left_end, int right_start, int right_end, int *p_left)
{
   Mapping *map;
   int i,j,k,n,n2;
   int zero_channel[256];
   int really_zero_channel[256];

// WINDOWING

   STBV_NOTUSED(left_end);
   n = f->blocksize[m->blockflag];
   map = &f->mapping[m->mapping];

// FLOORS
   n2 = n >> 1;

   CHECK(f);

   for (i=0; i < f->channels; ++i) {
      int s = map->chan[i].mux, floor;
      zero_channel[i] = FALSE;
      floor = map->submap_floor[s];
      if (f->floor_types[floor] == 0) {
         return error(f, VORBIS_invalid_stream);
      } else {
         Floor1 *g = &f->floor_config[floor].floor1;
         if (get_bits(f, 1)) {
            short *finalY;
            uint8 step2_flag[256];
            static int range_list[4] = { 256, 128, 86, 64 };
            int range = range_list[g->floor1_multiplier-1];
            int offset = 2;
            finalY = f->finalY[i];
            finalY[0] = get_bits(f, ilog(range)-1);
            finalY[1] = get_bits(f, ilog(range)-1);
            for (j=0; j < g->partitions; ++j) {
               int pclass = g->partition_class_list[j];
               int cdim = g->class_dimensions[pclass];
               int cbits = g->class_subclasses[pclass];
               int csub = (1 << cbits)-1;
               int cval = 0;
               if (cbits) {
                  Codebook *c = f->codebooks + g->class_masterbooks[pclass];
                  DECODE(cval,f,c);
               }
               for (k=0; k < cdim; ++k) {
                  int book = g->subclass_books[pclass][cval & csub];
                  cval = cval >> cbits;
                  if (book >= 0) {
                     int temp;
                     Codebook *c = f->codebooks + book;
                     DECODE(temp,f,c);
                     finalY[offset++] = temp;
                  } else
                     finalY[offset++] = 0;
               }
            }
            if (f->valid_bits == INVALID_BITS) goto error; // behavior according to spec
            step2_flag[0] = step2_flag[1] = 1;
            for (j=2; j < g->values; ++j) {
               int low, high, pred, highroom, lowroom, room, val;
               low = g->neighbors[j][0];
               high = g->neighbors[j][1];
               //neighbors(g->Xlist, j, &low, &high);
               pred = predict_point(g->Xlist[j], g->Xlist[low], g->Xlist[high], finalY[low], finalY[high]);
               val = finalY[j];
               highroom = range - pred;
               lowroom = pred;
               if (highroom < lowroom)
                  room = highroom * 2;
               else
                  room = lowroom * 2;
               if (val) {
                  step2_flag[low] = step2_flag[high] = 1;
                  step2_flag[j] = 1;
                  if (val >= room)
                     if (highroom > lowroom)
                        finalY[j] = val - lowroom + pred;
                     else
                        finalY[j] = pred - val + highroom - 1;
                  else
                     if (val & 1)
                        finalY[j] = pred - ((val+1)>>1);
                     else
                        finalY[j] = pred + (val>>1);
               } else {
                  step2_flag[j] = 0;
                  finalY[j] = pred;
               }
            }

#ifdef STB_VORBIS_NO_DEFER_FLOOR
            do_floor(f, map, i, n, f->floor_buffers[i], finalY, step2_flag);
#else
            // defer final floor computation until _after_ residue
            for (j=0; j < g->values; ++j) {
               if (!step2_flag[j])
                  finalY[j] = -1;
            }
#endif
         } else {
           error:
            zero_channel[i] = TRUE;
         }
         // So we just defer everything else to later

         // at this point we've decoded the floor into buffer
      }
   }
   CHECK(f);
   // at this point we've decoded all floors

   if (f->alloc.alloc_buffer)
      assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);

   // re-enable coupled channels if necessary
   memcpy(really_zero_channel, zero_channel, sizeof(really_zero_channel[0]) * f->channels);
   for (i=0; i < map->coupling_steps; ++i)
      if (!zero_channel[map->chan[i].magnitude] || !zero_channel[map->chan[i].angle]) {
         zero_channel[map->chan[i].magnitude] = zero_channel[map->chan[i].angle] = FALSE;
      }

   CHECK(f);
// RESIDUE DECODE
   for (i=0; i < map->submaps; ++i) {
      float *residue_buffers[STB_VORBIS_MAX_CHANNELS];
      int r;
      uint8 do_not_decode[256];
      int ch = 0;
      for (j=0; j < f->channels; ++j) {
         if (map->chan[j].mux == i) {
            if (zero_channel[j]) {
               do_not_decode[ch] = TRUE;
               residue_buffers[ch] = NULL;
            } else {
               do_not_decode[ch] = FALSE;
               residue_buffers[ch] = f->channel_buffers[j];
            }
            ++ch;
         }
      }
      r = map->submap_residue[i];
      decode_residue(f, residue_buffers, ch, n2, r, do_not_decode);
   }

   if (f->alloc.alloc_buffer)
      assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);
   CHECK(f);

// INVERSE COUPLING
   for (i = map->coupling_steps-1; i >= 0; --i) {
      int n2 = n >> 1;
      float *m = f->channel_buffers[map->chan[i].magnitude];
      float *a = f->channel_buffers[map->chan[i].angle    ];
      for (j=0; j < n2; ++j) {
         float a2,m2;
         if (m[j] > 0)
            if (a[j] > 0)
               m2 = m[j], a2 = m[j] - a[j];
            else
               a2 = m[j], m2 = m[j] + a[j];
         else
            if (a[j] > 0)
               m2 = m[j], a2 = m[j] + a[j];
            else
               a2 = m[j], m2 = m[j] - a[j];
         m[j] = m2;
         a[j] = a2;
      }
   }
   CHECK(f);

   // finish decoding the floors
#ifndef STB_VORBIS_NO_DEFER_FLOOR
   for (i=0; i < f->channels; ++i) {
      if (really_zero_channel[i]) {
         memset(f->channel_buffers[i], 0, sizeof(*f->channel_buffers[i]) * n2);
      } else {
         do_floor(f, map, i, n, f->channel_buffers[i], f->finalY[i], NULL);
      }
   }
#else
   for (i=0; i < f->channels; ++i) {
      if (really_zero_channel[i]) {
         memset(f->channel_buffers[i], 0, sizeof(*f->channel_buffers[i]) * n2);
      } else {
         for (j=0; j < n2; ++j)
            f->channel_buffers[i][j] *= f->floor_buffers[i][j];
      }
   }
#endif

// INVERSE MDCT
   CHECK(f);
   for (i=0; i < f->channels; ++i)
      inverse_mdct(f->channel_buffers[i], n, f, m->blockflag);
   CHECK(f);

   // this shouldn't be necessary, unless we exited on an error
   // and want to flush to get to the next packet
   flush_packet(f);

   if (f->first_decode) {
      // assume we start so first non-discarded sample is sample 0
      // this isn't to spec, but spec would require us to read ahead
      // and decode the size of all current frames--could be done,
      // but presumably it's not a commonly used feature
      f->current_loc = 0u - n2; // start of first frame is positioned for discard (NB this is an intentional unsigned overflow/wrap-around)
      // we might have to discard samples "from" the next frame too,
      // if we're lapping a large block then a small at the start?
      f->discard_samples_deferred = n - right_end;
      f->current_loc_valid = TRUE;
      f->first_decode = FALSE;
   } else if (f->discard_samples_deferred) {
      if (f->discard_samples_deferred >= right_start - left_start) {
         f->discard_samples_deferred -= (right_start - left_start);
         left_start = right_start;
         *p_left = left_start;
      } else {
         left_start += f->discard_samples_deferred;
         *p_left = left_start;
         f->discard_samples_deferred = 0;
      }
   } else if (f->previous_length == 0 && f->current_loc_valid) {
      // we're recovering from a seek... that means we're going to discard
      // the samples from this packet even though we know our position from
      // the last page header, so we need to update the position based on
      // the discarded samples here
      // but wait, the code below is going to add this in itself even
      // on a discard, so we don't need to do it here...
   }

   // check if we have ogg information about the sample # for this packet
   if (f->last_seg_which == f->end_seg_with_known_loc) {
      // if we have a valid current loc, and this is final:
      if (f->current_loc_valid && (f->page_flag & PAGEFLAG_last_page)) {
         uint32 current_end = f->known_loc_for_packet;
         // then let's infer the size of the (probably) short final frame
         if (current_end < f->current_loc + (right_end-left_start)) {
            if (current_end < f->current_loc) {
               // negative truncation, that's impossible!
               *len = 0;
            } else {
               *len = current_end - f->current_loc;
            }
            *len += left_start; // this doesn't seem right, but has no ill effect on my test files
            if (*len > right_end) *len = right_end; // this should never happen
            f->current_loc += *len;
            return TRUE;
         }
      }
      // otherwise, just set our sample loc
      // guess that the ogg granule pos refers to the _middle_ of the
      // last frame?
      // set f->current_loc to the position of left_start
      f->current_loc = f->known_loc_for_packet - (n2-left_start);
      f->current_loc_valid = TRUE;
   }
   if (f->current_loc_valid)
      f->current_loc += (right_start - left_start);

   if (f->alloc.alloc_buffer)
      assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);
   *len = right_end;  // ignore samples after the window goes to 0
   CHECK(f);

   return TRUE;
}

static int vorbis_decode_packet(vorb *f, int *len, int *p_left, int *p_right)
{
   int mode, left_end, right_end;
   if (!vorbis_decode_initial(f, p_left, &left_end, p_right, &right_end, &mode)) return 0;
   return vorbis_decode_packet_rest(f, len, f->mode_config + mode, *p_left, left_end, *p_right, right_end, p_left);
}

static int vorbis_finish_frame(stb_vorbis *f, int len, int left, int right)
{
   int prev,i,j;
   // we use right&left (the start of the right- and left-window sin()-regions)
   // to determine how much to return, rather than inferring from the rules
   // (same result, clearer code); 'left' indicates where our sin() window
   // starts, therefore where the previous window's right edge starts, and
   // therefore where to start mixing from the previous buffer. 'right'
   // indicates where our sin() ending-window starts, therefore that's where
   // we start saving, and where our returned-data ends.

   // mixin from previous window
   if (f->previous_length) {
      int i,j, n = f->previous_length;
      float *w = get_window(f, n);
      if (w == NULL) return 0;
      for (i=0; i < f->channels; ++i) {
         for (j=0; j < n; ++j)
            f->channel_buffers[i][left+j] =
               f->channel_buffers[i][left+j]*w[    j] +
               f->previous_window[i][     j]*w[n-1-j];
      }
   }

   prev = f->previous_length;

   // last half of this data becomes previous window
   f->previous_length = len - right;

   // @OPTIMIZE: could avoid this copy by double-buffering the
   // output (flipping previous_window with channel_buffers), but
   // then previous_window would have to be 2x as large, and
   // channel_buffers couldn't be temp mem (although they're NOT
   // currently temp mem, they could be (unless we want to level
   // performance by spreading out the computation))
   for (i=0; i < f->channels; ++i)
      for (j=0; right+j < len; ++j)
         f->previous_window[i][j] = f->channel_buffers[i][right+j];

   if (!prev)
      // there was no previous packet, so this data isn't valid...
      // this isn't entirely true, only the would-have-overlapped data
      // isn't valid, but this seems to be what the spec requires
      return 0;

   // truncate a short frame
   if (len < right) right = len;

   f->samples_output += right-left;

   return right - left;
}

static int vorbis_pump_first_frame(stb_vorbis *f)
{
   int len, right, left, res;
   res = vorbis_decode_packet(f, &len, &left, &right);
   if (res)
      vorbis_finish_frame(f, len, left, right);
   return res;
}

#ifndef STB_VORBIS_NO_PUSHDATA_API
static int is_whole_packet_present(stb_vorbis *f)
{
   // make sure that we have the packet available before continuing...
   // this requires a full ogg parse, but we know we can fetch from f->stream

   // instead of coding this out explicitly, we could save the current read state,
   // read the next packet with get8() until end-of-packet, check f->eof, then
   // reset the state? but that would be slower, esp. since we'd have over 256 bytes
   // of state to restore (primarily the page segment table)

   int s = f->next_seg, first = TRUE;
   uint8 *p = f->stream;

   if (s != -1) { // if we're not starting the packet with a 'continue on next page' flag
      for (; s < f->segment_count; ++s) {
         p += f->segments[s];
         if (f->segments[s] < 255)               // stop at first short segment
            break;
      }
      // either this continues, or it ends it...
      if (s == f->segment_count)
         s = -1; // set 'crosses page' flag
      if (p > f->stream_end)                     return error(f, VORBIS_need_more_data);
      first = FALSE;
   }
   for (; s == -1;) {
      uint8 *q;
      int n;

      // check that we have the page header ready
      if (p + 26 >= f->stream_end)               return error(f, VORBIS_need_more_data);
      // validate the page
      if (memcmp(p, ogg_page_header, 4))         return error(f, VORBIS_invalid_stream);
      if (p[4] != 0)                             return error(f, VORBIS_invalid_stream);
      if (first) { // the first segment must NOT have 'continued_packet', later ones MUST
         if (f->previous_length)
            if ((p[5] & PAGEFLAG_continued_packet))  return error(f, VORBIS_invalid_stream);
         // if no previous length, we're resynching, so we can come in on a continued-packet,
         // which we'll just drop
      } else {
         if (!(p[5] & PAGEFLAG_continued_packet)) return error(f, VORBIS_invalid_stream);
      }
      n = p[26]; // segment counts
      q = p+27;  // q points to segment table
      p = q + n; // advance past header
      // make sure we've read the segment table
      if (p > f->stream_end)                     return error(f, VORBIS_need_more_data);
      for (s=0; s < n; ++s) {
         p += q[s];
         if (q[s] < 255)
            break;
      }
      if (s == n)
         s = -1; // set 'crosses page' flag
      if (p > f->stream_end)                     return error(f, VORBIS_need_more_data);
      first = FALSE;
   }
   return TRUE;
}
#endif // !STB_VORBIS_NO_PUSHDATA_API

static int start_decoder(vorb *f)
{
   uint8 header[6], x,y;
   int len,i,j,k, max_submaps = 0;
   int longest_floorlist=0;

   // first page, first packet
   f->first_decode = TRUE;

   if (!start_page(f))                              return FALSE;
   // validate page flag
   if (!(f->page_flag & PAGEFLAG_first_page))       return error(f, VORBIS_invalid_first_page);
   if (f->page_flag & PAGEFLAG_last_page)           return error(f, VORBIS_invalid_first_page);
   if (f->page_flag & PAGEFLAG_continued_packet)    return error(f, VORBIS_invalid_first_page);
   // check for expected packet length
   if (f->segment_count != 1)                       return error(f, VORBIS_invalid_first_page);
   if (f->segments[0] != 30) {
      // check for the Ogg skeleton fishead identifying header to refine our error
      if (f->segments[0] == 64 &&
          getn(f, header, 6) &&
          header[0] == 'f' &&
          header[1] == 'i' &&
          header[2] == 's' &&
          header[3] == 'h' &&
          header[4] == 'e' &&
          header[5] == 'a' &&
          get8(f)   == 'd' &&
          get8(f)   == '\0')                        return error(f, VORBIS_ogg_skeleton_not_supported);
      else
                                                    return error(f, VORBIS_invalid_first_page);
   }

   // read packet
   // check packet header
   if (get8(f) != VORBIS_packet_id)                 return error(f, VORBIS_invalid_first_page);
   if (!getn(f, header, 6))                         return error(f, VORBIS_unexpected_eof);
   if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_first_page);
   // vorbis_version
   if (get32(f) != 0)                               return error(f, VORBIS_invalid_first_page);
   f->channels = get8(f); if (!f->channels)         return error(f, VORBIS_invalid_first_page);
   if (f->channels > STB_VORBIS_MAX_CHANNELS)       return error(f, VORBIS_too_many_channels);
   f->sample_rate = get32(f); if (!f->sample_rate)  return error(f, VORBIS_invalid_first_page);
   get32(f); // bitrate_maximum
   get32(f); // bitrate_nominal
   get32(f); // bitrate_minimum
   x = get8(f);
   {
      int log0,log1;
      log0 = x & 15;
      log1 = x >> 4;
      f->blocksize_0 = 1 << log0;
      f->blocksize_1 = 1 << log1;
      if (log0 < 6 || log0 > 13)                       return error(f, VORBIS_invalid_setup);
      if (log1 < 6 || log1 > 13)                       return error(f, VORBIS_invalid_setup);
      if (log0 > log1)                                 return error(f, VORBIS_invalid_setup);
   }

   // framing_flag
   x = get8(f);
   if (!(x & 1))                                    return error(f, VORBIS_invalid_first_page);

   // second packet!
   if (!start_page(f))                              return FALSE;

   if (!start_packet(f))                            return FALSE;

   if (!next_segment(f))                            return FALSE;

   if (get8_packet(f) != VORBIS_packet_comment)            return error(f, VORBIS_invalid_setup);
   for (i=0; i < 6; ++i) header[i] = get8_packet(f);
   if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_setup);
   //file vendor
   len = get32_packet(f);
   f->vendor = (char*)setup_malloc(f, sizeof(char) * (len+1));
   if (f->vendor == NULL)                           return error(f, VORBIS_outofmem);
   for(i=0; i < len; ++i) {
      f->vendor[i] = get8_packet(f);
   }
   f->vendor[len] = (char)'\0';
   //user comments
   f->comment_list_length = get32_packet(f);
   f->comment_list = NULL;
   if (f->comment_list_length > 0)
   {
      f->comment_list = (char**) setup_malloc(f, sizeof(char*) * (f->comment_list_length));
      if (f->comment_list == NULL)                  return error(f, VORBIS_outofmem);
   }

   for(i=0; i < f->comment_list_length; ++i) {
      len = get32_packet(f);
      f->comment_list[i] = (char*)setup_malloc(f, sizeof(char) * (len+1));
      if (f->comment_list[i] == NULL)               return error(f, VORBIS_outofmem);

      for(j=0; j < len; ++j) {
         f->comment_list[i][j] = get8_packet(f);
      }
      f->comment_list[i][len] = (char)'\0';
   }

   // framing_flag
   x = get8_packet(f);
   if (!(x & 1))                                    return error(f, VORBIS_invalid_setup);


   skip(f, f->bytes_in_seg);
   f->bytes_in_seg = 0;

   do {
      len = next_segment(f);
      skip(f, len);
      f->bytes_in_seg = 0;
   } while (len);

   // third packet!
   if (!start_packet(f))                            return FALSE;

   #ifndef STB_VORBIS_NO_PUSHDATA_API
   if (IS_PUSH_MODE(f)) {
      if (!is_whole_packet_present(f)) {
         // convert error in ogg header to write type
         if (f->error == VORBIS_invalid_stream)
            f->error = VORBIS_invalid_setup;
         return FALSE;
      }
   }
   #endif

   crc32_init(); // always init it, to avoid multithread race conditions

   if (get8_packet(f) != VORBIS_packet_setup)       return error(f, VORBIS_invalid_setup);
   for (i=0; i < 6; ++i) header[i] = get8_packet(f);
   if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_setup);

   // codebooks

   f->codebook_count = get_bits(f,8) + 1;
   f->codebooks = (Codebook *) setup_malloc(f, sizeof(*f->codebooks) * f->codebook_count);
   if (f->codebooks == NULL)                        return error(f, VORBIS_outofmem);
   memset(f->codebooks, 0, sizeof(*f->codebooks) * f->codebook_count);
   for (i=0; i < f->codebook_count; ++i) {
      uint32 *values;
      int ordered, sorted_count;
      int total=0;
      uint8 *lengths;
      Codebook *c = f->codebooks+i;
      CHECK(f);
      x = get_bits(f, 8); if (x != 0x42)            return error(f, VORBIS_invalid_setup);
      x = get_bits(f, 8); if (x != 0x43)            return error(f, VORBIS_invalid_setup);
      x = get_bits(f, 8); if (x != 0x56)            return error(f, VORBIS_invalid_setup);
      x = get_bits(f, 8);
      c->dimensions = (get_bits(f, 8)<<8) + x;
      x = get_bits(f, 8);
      y = get_bits(f, 8);
      c->entries = (get_bits(f, 8)<<16) + (y<<8) + x;
      ordered = get_bits(f,1);
      c->sparse = ordered ? 0 : get_bits(f,1);

      if (c->dimensions == 0 && c->entries != 0)    return error(f, VORBIS_invalid_setup);

      if (c->sparse)
         lengths = (uint8 *) setup_temp_malloc(f, c->entries);
      else
         lengths = c->codeword_lengths = (uint8 *) setup_malloc(f, c->entries);

      if (!lengths) return error(f, VORBIS_outofmem);

      if (ordered) {
         int current_entry = 0;
         int current_length = get_bits(f,5) + 1;
         while (current_entry < c->entries) {
            int limit = c->entries - current_entry;
            int n = get_bits(f, ilog(limit));
            if (current_length >= 32) return error(f, VORBIS_invalid_setup);
            if (current_entry + n > (int) c->entries) { return error(f, VORBIS_invalid_setup); }
            memset(lengths + current_entry, current_length, n);
            current_entry += n;
            ++current_length;
         }
      } else {
         for (j=0; j < c->entries; ++j) {
            int present = c->sparse ? get_bits(f,1) : 1;
            if (present) {
               lengths[j] = get_bits(f, 5) + 1;
               ++total;
               if (lengths[j] == 32)
                  return error(f, VORBIS_invalid_setup);
            } else {
               lengths[j] = NO_CODE;
            }
         }
      }

      if (c->sparse && total >= c->entries >> 2) {
         // convert sparse items to non-sparse!
         if (c->entries > (int) f->setup_temp_memory_required)
            f->setup_temp_memory_required = c->entries;

         c->codeword_lengths = (uint8 *) setup_malloc(f, c->entries);
         if (c->codeword_lengths == NULL) return error(f, VORBIS_outofmem);
         memcpy(c->codeword_lengths, lengths, c->entries);
         setup_temp_free(f, lengths, c->entries); // note this is only safe if there have been no intervening temp mallocs!
         lengths = c->codeword_lengths;
         c->sparse = 0;
      }

      // compute the size of the sorted tables
      if (c->sparse) {
         sorted_count = total;
      } else {
         sorted_count = 0;
         #ifndef STB_VORBIS_NO_HUFFMAN_BINARY_SEARCH
         for (j=0; j < c->entries; ++j)
            if (lengths[j] > STB_VORBIS_FAST_HUFFMAN_LENGTH && lengths[j] != NO_CODE)
               ++sorted_count;
         #endif
      }

      c->sorted_entries = sorted_count;
      values = NULL;

      CHECK(f);
      if (!c->sparse) {
         c->codewords = (uint32 *) setup_malloc(f, sizeof(c->codewords[0]) * c->entries);
         if (!c->codewords)                  return error(f, VORBIS_outofmem);
      } else {
         unsigned int size;
         if (c->sorted_entries) {
            c->codeword_lengths = (uint8 *) setup_malloc(f, c->sorted_entries);
            if (!c->codeword_lengths)           return error(f, VORBIS_outofmem);
            c->codewords = (uint32 *) setup_temp_malloc(f, sizeof(*c->codewords) * c->sorted_entries);
            if (!c->codewords)                  return error(f, VORBIS_outofmem);
            values = (uint32 *) setup_temp_malloc(f, sizeof(*values) * c->sorted_entries);
            if (!values)                        return error(f, VORBIS_outofmem);
         }
         size = c->entries + (sizeof(*c->codewords) + sizeof(*values)) * c->sorted_entries;
         if (size > f->setup_temp_memory_required)
            f->setup_temp_memory_required = size;
      }

      if (!compute_codewords(c, lengths, c->entries, values)) {
         if (c->sparse) setup_temp_free(f, values, 0);
         return error(f, VORBIS_invalid_setup);
      }

      if (c->sorted_entries) {
         // allocate an extra slot for sentinels
         c->sorted_codewords = (uint32 *) setup_malloc(f, sizeof(*c->sorted_codewords) * (c->sorted_entries+1));
         if (c->sorted_codewords == NULL) return error(f, VORBIS_outofmem);
         // allocate an extra slot at the front so that c->sorted_values[-1] is defined
         // so that we can catch that case without an extra if
         c->sorted_values    = ( int   *) setup_malloc(f, sizeof(*c->sorted_values   ) * (c->sorted_entries+1));
         if (c->sorted_values == NULL) return error(f, VORBIS_outofmem);
         ++c->sorted_values;
         c->sorted_values[-1] = -1;
         compute_sorted_huffman(c, lengths, values);
      }

      if (c->sparse) {
         setup_temp_free(f, values, sizeof(*values)*c->sorted_entries);
         setup_temp_free(f, c->codewords, sizeof(*c->codewords)*c->sorted_entries);
         setup_temp_free(f, lengths, c->entries);
         c->codewords = NULL;
      }

      compute_accelerated_huffman(c);

      CHECK(f);
      c->lookup_type = get_bits(f, 4);
      if (c->lookup_type > 2) return error(f, VORBIS_invalid_setup);
      if (c->lookup_type > 0) {
         uint16 *mults;
         c->minimum_value = float32_unpack(get_bits(f, 32));
         c->delta_value = float32_unpack(get_bits(f, 32));
         c->value_bits = get_bits(f, 4)+1;
         c->sequence_p = get_bits(f,1);
         if (c->lookup_type == 1) {
            int values = lookup1_values(c->entries, c->dimensions);
            if (values < 0) return error(f, VORBIS_invalid_setup);
            c->lookup_values = (uint32) values;
         } else {
            c->lookup_values = c->entries * c->dimensions;
         }
         if (c->lookup_values == 0) return error(f, VORBIS_invalid_setup);
         mults = (uint16 *) setup_temp_malloc(f, sizeof(mults[0]) * c->lookup_values);
         if (mults == NULL) return error(f, VORBIS_outofmem);
         for (j=0; j < (int) c->lookup_values; ++j) {
            int q = get_bits(f, c->value_bits);
            if (q == EOP) { setup_temp_free(f,mults,sizeof(mults[0])*c->lookup_values); return error(f, VORBIS_invalid_setup); }
            mults[j] = q;
         }

#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
         if (c->lookup_type == 1) {
            int len, sparse = c->sparse;
            float last=0;
            // pre-expand the lookup1-style multiplicands, to avoid a divide in the inner loop
            if (sparse) {
               if (c->sorted_entries == 0) goto skip;
               c->multiplicands = (codetype *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->sorted_entries * c->dimensions);
            } else
               c->multiplicands = (codetype *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->entries        * c->dimensions);
            if (c->multiplicands == NULL) { setup_temp_free(f,mults,sizeof(mults[0])*c->lookup_values); return error(f, VORBIS_outofmem); }
            len = sparse ? c->sorted_entries : c->entries;
            for (j=0; j < len; ++j) {
               unsigned int z = sparse ? c->sorted_values[j] : j;
               unsigned int div=1;
               for (k=0; k < c->dimensions; ++k) {
                  int off = (z / div) % c->lookup_values;
                  float val = mults[off]*c->delta_value + c->minimum_value + last;
                  c->multiplicands[j*c->dimensions + k] = val;
                  if (c->sequence_p)
                     last = val;
                  if (k+1 < c->dimensions) {
                     if (div > UINT_MAX / (unsigned int) c->lookup_values) {
                        setup_temp_free(f, mults,sizeof(mults[0])*c->lookup_values);
                        return error(f, VORBIS_invalid_setup);
                     }
                     div *= c->lookup_values;
                  }
               }
            }
            c->lookup_type = 2;
         }
         else
#endif
         {
            float last=0;
            CHECK(f);
            c->multiplicands = (codetype *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->lookup_values);
            if (c->multiplicands == NULL) { setup_temp_free(f, mults,sizeof(mults[0])*c->lookup_values); return error(f, VORBIS_outofmem); }
            for (j=0; j < (int) c->lookup_values; ++j) {
               float val = mults[j] * c->delta_value + c->minimum_value + last;
               c->multiplicands[j] = val;
               if (c->sequence_p)
                  last = val;
            }
         }
#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
        skip:;
#endif
         setup_temp_free(f, mults, sizeof(mults[0])*c->lookup_values);

         CHECK(f);
      }
      CHECK(f);
   }

   // time domain transfers (notused)

   x = get_bits(f, 6) + 1;
   for (i=0; i < x; ++i) {
      uint32 z = get_bits(f, 16);
      if (z != 0) return error(f, VORBIS_invalid_setup);
   }

   // Floors
   f->floor_count = get_bits(f, 6)+1;
   f->floor_config = (Floor *)  setup_malloc(f, f->floor_count * sizeof(*f->floor_config));
   if (f->floor_config == NULL) return error(f, VORBIS_outofmem);
   for (i=0; i < f->floor_count; ++i) {
      f->floor_types[i] = get_bits(f, 16);
      if (f->floor_types[i] > 1) return error(f, VORBIS_invalid_setup);
      if (f->floor_types[i] == 0) {
         Floor0 *g = &f->floor_config[i].floor0;
         g->order = get_bits(f,8);
         g->rate = get_bits(f,16);
         g->bark_map_size = get_bits(f,16);
         g->amplitude_bits = get_bits(f,6);
         g->amplitude_offset = get_bits(f,8);
         g->number_of_books = get_bits(f,4) + 1;
         for (j=0; j < g->number_of_books; ++j)
            g->book_list[j] = get_bits(f,8);
         return error(f, VORBIS_feature_not_supported);
      } else {
         stbv__floor_ordering p[31*8+2];
         Floor1 *g = &f->floor_config[i].floor1;
         int max_class = -1;
         g->partitions = get_bits(f, 5);
         for (j=0; j < g->partitions; ++j) {
            g->partition_class_list[j] = get_bits(f, 4);
            if (g->partition_class_list[j] > max_class)
               max_class = g->partition_class_list[j];
         }
         for (j=0; j <= max_class; ++j) {
            g->class_dimensions[j] = get_bits(f, 3)+1;
            g->class_subclasses[j] = get_bits(f, 2);
            if (g->class_subclasses[j]) {
               g->class_masterbooks[j] = get_bits(f, 8);
               if (g->class_masterbooks[j] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
            }
            for (k=0; k < 1 << g->class_subclasses[j]; ++k) {
               g->subclass_books[j][k] = (int16)get_bits(f,8)-1;
               if (g->subclass_books[j][k] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
            }
         }
         g->floor1_multiplier = get_bits(f,2)+1;
         g->rangebits = get_bits(f,4);
         g->Xlist[0] = 0;
         g->Xlist[1] = 1 << g->rangebits;
         g->values = 2;
         for (j=0; j < g->partitions; ++j) {
            int c = g->partition_class_list[j];
            for (k=0; k < g->class_dimensions[c]; ++k) {
               g->Xlist[g->values] = get_bits(f, g->rangebits);
               ++g->values;
            }
         }
         // precompute the sorting
         for (j=0; j < g->values; ++j) {
            p[j].x = g->Xlist[j];
            p[j].id = j;
         }
         qsort(p, g->values, sizeof(p[0]), point_compare);
         for (j=0; j < g->values-1; ++j)
            if (p[j].x == p[j+1].x)
               return error(f, VORBIS_invalid_setup);
         for (j=0; j < g->values; ++j)
            g->sorted_order[j] = (uint8) p[j].id;
         // precompute the neighbors
         for (j=2; j < g->values; ++j) {
            int low = 0,hi = 0;
            neighbors(g->Xlist, j, &low,&hi);
            g->neighbors[j][0] = low;
            g->neighbors[j][1] = hi;
         }

         if (g->values > longest_floorlist)
            longest_floorlist = g->values;
      }
   }

   // Residue
   f->residue_count = get_bits(f, 6)+1;
   f->residue_config = (Residue *) setup_malloc(f, f->residue_count * sizeof(f->residue_config[0]));
   if (f->residue_config == NULL) return error(f, VORBIS_outofmem);
   memset(f->residue_config, 0, f->residue_count * sizeof(f->residue_config[0]));
   for (i=0; i < f->residue_count; ++i) {
      uint8 residue_cascade[64];
      Residue *r = f->residue_config+i;
      f->residue_types[i] = get_bits(f, 16);
      if (f->residue_types[i] > 2) return error(f, VORBIS_invalid_setup);
      r->begin = get_bits(f, 24);
      r->end = get_bits(f, 24);
      if (r->end < r->begin) return error(f, VORBIS_invalid_setup);
      r->part_size = get_bits(f,24)+1;
      r->classifications = get_bits(f,6)+1;
      r->classbook = get_bits(f,8);
      if (r->classbook >= f->codebook_count) return error(f, VORBIS_invalid_setup);
      for (j=0; j < r->classifications; ++j) {
         uint8 high_bits=0;
         uint8 low_bits=get_bits(f,3);
         if (get_bits(f,1))
            high_bits = get_bits(f,5);
         residue_cascade[j] = high_bits*8 + low_bits;
      }
      r->residue_books = (short (*)[8]) setup_malloc(f, sizeof(r->residue_books[0]) * r->classifications);
      if (r->residue_books == NULL) return error(f, VORBIS_outofmem);
      for (j=0; j < r->classifications; ++j) {
         for (k=0; k < 8; ++k) {
            if (residue_cascade[j] & (1 << k)) {
               r->residue_books[j][k] = get_bits(f, 8);
               if (r->residue_books[j][k] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
            } else {
               r->residue_books[j][k] = -1;
            }
         }
      }
      // precompute the classifications[] array to avoid inner-loop mod/divide
      // call it 'classdata' since we already have r->classifications
      r->classdata = (uint8 **) setup_malloc(f, sizeof(*r->classdata) * f->codebooks[r->classbook].entries);
      if (!r->classdata) return error(f, VORBIS_outofmem);
      memset(r->classdata, 0, sizeof(*r->classdata) * f->codebooks[r->classbook].entries);
      for (j=0; j < f->codebooks[r->classbook].entries; ++j) {
         int classwords = f->codebooks[r->classbook].dimensions;
         int temp = j;
         r->classdata[j] = (uint8 *) setup_malloc(f, sizeof(r->classdata[j][0]) * classwords);
         if (r->classdata[j] == NULL) return error(f, VORBIS_outofmem);
         for (k=classwords-1; k >= 0; --k) {
            r->classdata[j][k] = temp % r->classifications;
            temp /= r->classifications;
         }
      }
   }

   f->mapping_count = get_bits(f,6)+1;
   f->mapping = (Mapping *) setup_malloc(f, f->mapping_count * sizeof(*f->mapping));
   if (f->mapping == NULL) return error(f, VORBIS_outofmem);
   memset(f->mapping, 0, f->mapping_count * sizeof(*f->mapping));
   for (i=0; i < f->mapping_count; ++i) {
      Mapping *m = f->mapping + i;
      int mapping_type = get_bits(f,16);
      if (mapping_type != 0) return error(f, VORBIS_invalid_setup);
      m->chan = (MappingChannel *) setup_malloc(f, f->channels * sizeof(*m->chan));
      if (m->chan == NULL) return error(f, VORBIS_outofmem);
      if (get_bits(f,1))
         m->submaps = get_bits(f,4)+1;
      else
         m->submaps = 1;
      if (m->submaps > max_submaps)
         max_submaps = m->submaps;
      if (get_bits(f,1)) {
         m->coupling_steps = get_bits(f,8)+1;
         if (m->coupling_steps > f->channels) return error(f, VORBIS_invalid_setup);
         for (k=0; k < m->coupling_steps; ++k) {
            m->chan[k].magnitude = get_bits(f, ilog(f->channels-1));
            m->chan[k].angle = get_bits(f, ilog(f->channels-1));
            if (m->chan[k].magnitude >= f->channels)        return error(f, VORBIS_invalid_setup);
            if (m->chan[k].angle     >= f->channels)        return error(f, VORBIS_invalid_setup);
            if (m->chan[k].magnitude == m->chan[k].angle)   return error(f, VORBIS_invalid_setup);
         }
      } else
         m->coupling_steps = 0;

      // reserved field
      if (get_bits(f,2)) return error(f, VORBIS_invalid_setup);
      if (m->submaps > 1) {
         for (j=0; j < f->channels; ++j) {
            m->chan[j].mux = get_bits(f, 4);
            if (m->chan[j].mux >= m->submaps)                return error(f, VORBIS_invalid_setup);
         }
      } else
         // @SPECIFICATION: this case is missing from the spec
         for (j=0; j < f->channels; ++j)
            m->chan[j].mux = 0;

      for (j=0; j < m->submaps; ++j) {
         get_bits(f,8); // discard
         m->submap_floor[j] = get_bits(f,8);
         m->submap_residue[j] = get_bits(f,8);
         if (m->submap_floor[j] >= f->floor_count)      return error(f, VORBIS_invalid_setup);
         if (m->submap_residue[j] >= f->residue_count)  return error(f, VORBIS_invalid_setup);
      }
   }

   // Modes
   f->mode_count = get_bits(f, 6)+1;
   for (i=0; i < f->mode_count; ++i) {
      Mode *m = f->mode_config+i;
      m->blockflag = get_bits(f,1);
      m->windowtype = get_bits(f,16);
      m->transformtype = get_bits(f,16);
      m->mapping = get_bits(f,8);
      if (m->windowtype != 0)                 return error(f, VORBIS_invalid_setup);
      if (m->transformtype != 0)              return error(f, VORBIS_invalid_setup);
      if (m->mapping >= f->mapping_count)     return error(f, VORBIS_invalid_setup);
   }

   flush_packet(f);

   f->previous_length = 0;

   for (i=0; i < f->channels; ++i) {
      f->channel_buffers[i] = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1);
      f->previous_window[i] = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1/2);
      f->finalY[i]          = (int16 *) setup_malloc(f, sizeof(int16) * longest_floorlist);
      if (f->channel_buffers[i] == NULL || f->previous_window[i] == NULL || f->finalY[i] == NULL) return error(f, VORBIS_outofmem);
      memset(f->channel_buffers[i], 0, sizeof(float) * f->blocksize_1);
      #ifdef STB_VORBIS_NO_DEFER_FLOOR
      f->floor_buffers[i]   = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1/2);
      if (f->floor_buffers[i] == NULL) return error(f, VORBIS_outofmem);
      #endif
   }

   if (!init_blocksize(f, 0, f->blocksize_0)) return FALSE;
   if (!init_blocksize(f, 1, f->blocksize_1)) return FALSE;
   f->blocksize[0] = f->blocksize_0;
   f->blocksize[1] = f->blocksize_1;

#ifdef STB_VORBIS_DIVIDE_TABLE
   if (integer_divide_table[1][1]==0)
      for (i=0; i < DIVTAB_NUMER; ++i)
         for (j=1; j < DIVTAB_DENOM; ++j)
            integer_divide_table[i][j] = i / j;
#endif

   // compute how much temporary memory is needed

   // 1.
   {
      uint32 imdct_mem = (f->blocksize_1 * sizeof(float) >> 1);
      uint32 classify_mem;
      int i,max_part_read=0;
      for (i=0; i < f->residue_count; ++i) {
         Residue *r = f->residue_config + i;
         unsigned int actual_size = f->blocksize_1 / 2;
         unsigned int limit_r_begin = r->begin < actual_size ? r->begin : actual_size;
         unsigned int limit_r_end   = r->end   < actual_size ? r->end   : actual_size;
         int n_read = limit_r_end - limit_r_begin;
         int part_read = n_read / r->part_size;
         if (part_read > max_part_read)
            max_part_read = part_read;
      }
      #ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
      classify_mem = f->channels * (sizeof(void*) + max_part_read * sizeof(uint8 *));
      #else
      classify_mem = f->channels * (sizeof(void*) + max_part_read * sizeof(int *));
      #endif

      // maximum reasonable partition size is f->blocksize_1

      f->temp_memory_required = classify_mem;
      if (imdct_mem > f->temp_memory_required)
         f->temp_memory_required = imdct_mem;
   }


   if (f->alloc.alloc_buffer) {
      assert(f->temp_offset == f->alloc.alloc_buffer_length_in_bytes);
      // check if there's enough temp memory so we don't error later
      if (f->setup_offset + sizeof(*f) + f->temp_memory_required > (unsigned) f->temp_offset)
         return error(f, VORBIS_outofmem);
   }

   // @TODO: stb_vorbis_seek_start expects first_audio_page_offset to point to a page
   // without PAGEFLAG_continued_packet, so this either points to the first page, or
   // the page after the end of the headers. It might be cleaner to point to a page
   // in the middle of the headers, when that's the page where the first audio packet
   // starts, but we'd have to also correctly skip the end of any continued packet in
   // stb_vorbis_seek_start.
   if (f->next_seg == -1) {
      f->first_audio_page_offset = stb_vorbis_get_file_offset(f);
   } else {
      f->first_audio_page_offset = 0;
   }

   return TRUE;
}

static void vorbis_deinit(stb_vorbis *p)
{
   int i,j;

   setup_free(p, p->vendor);
   for (i=0; i < p->comment_list_length; ++i) {
      setup_free(p, p->comment_list[i]);
   }
   setup_free(p, p->comment_list);

   if (p->residue_config) {
      for (i=0; i < p->residue_count; ++i) {
         Residue *r = p->residue_config+i;
         if (r->classdata) {
            for (j=0; j < p->codebooks[r->classbook].entries; ++j)
               setup_free(p, r->classdata[j]);
            setup_free(p, r->classdata);
         }
         setup_free(p, r->residue_books);
      }
   }

   if (p->codebooks) {
      CHECK(p);
      for (i=0; i < p->codebook_count; ++i) {
         Codebook *c = p->codebooks + i;
         setup_free(p, c->codeword_lengths);
         setup_free(p, c->multiplicands);
         setup_free(p, c->codewords);
         setup_free(p, c->sorted_codewords);
         // c->sorted_values[-1] is the first entry in the array
         setup_free(p, c->sorted_values ? c->sorted_values-1 : NULL);
      }
      setup_free(p, p->codebooks);
   }
   setup_free(p, p->floor_config);
   setup_free(p, p->residue_config);
   if (p->mapping) {
      for (i=0; i < p->mapping_count; ++i)
         setup_free(p, p->mapping[i].chan);
      setup_free(p, p->mapping);
   }
   CHECK(p);
   for (i=0; i < p->channels && i < STB_VORBIS_MAX_CHANNELS; ++i) {
      setup_free(p, p->channel_buffers[i]);
      setup_free(p, p->previous_window[i]);
      #ifdef STB_VORBIS_NO_DEFER_FLOOR
      setup_free(p, p->floor_buffers[i]);
      #endif
      setup_free(p, p->finalY[i]);
   }
   for (i=0; i < 2; ++i) {
      setup_free(p, p->A[i]);
      setup_free(p, p->B[i]);
      setup_free(p, p->C[i]);
      setup_free(p, p->window[i]);
      setup_free(p, p->bit_reverse[i]);
   }
   #ifndef STB_VORBIS_NO_STDIO
   if (p->close_on_free) fclose(p->f);
   #endif
}

void stb_vorbis_close(stb_vorbis *p)
{
   if (p == NULL) return;
   vorbis_deinit(p);
   setup_free(p,p);
}

static void vorbis_init(stb_vorbis *p, const stb_vorbis_alloc *z)
{
   memset(p, 0, sizeof(*p)); // NULL out all malloc'd pointers to start
   if (z) {
      p->alloc = *z;
      p->alloc.alloc_buffer_length_in_bytes &= ~7;
      p->temp_offset = p->alloc.alloc_buffer_length_in_bytes;
   }
   p->eof = 0;
   p->error = VORBIS__no_error;
   p->stream = NULL;
   p->codebooks = NULL;
   p->page_crc_tests = -1;
   #ifndef STB_VORBIS_NO_STDIO
   p->close_on_free = FALSE;
   p->f = NULL;
   #endif
}

int stb_vorbis_get_sample_offset(stb_vorbis *f)
{
   if (f->current_loc_valid)
      return f->current_loc;
   else
      return -1;
}

stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f)
{
   stb_vorbis_info d;
   d.channels = f->channels;
   d.sample_rate = f->sample_rate;
   d.setup_memory_required = f->setup_memory_required;
   d.setup_temp_memory_required = f->setup_temp_memory_required;
   d.temp_memory_required = f->temp_memory_required;
   d.max_frame_size = f->blocksize_1 >> 1;
   return d;
}

stb_vorbis_comment stb_vorbis_get_comment(stb_vorbis *f)
{
   stb_vorbis_comment d;
   d.vendor = f->vendor;
   d.comment_list_length = f->comment_list_length;
   d.comment_list = f->comment_list;
   return d;
}

int stb_vorbis_get_error(stb_vorbis *f)
{
   int e = f->error;
   f->error = VORBIS__no_error;
   return e;
}

static stb_vorbis * vorbis_alloc(stb_vorbis *f)
{
   stb_vorbis *p = (stb_vorbis *) setup_malloc(f, sizeof(*p));
   return p;
}

#ifndef STB_VORBIS_NO_PUSHDATA_API

void stb_vorbis_flush_pushdata(stb_vorbis *f)
{
   f->previous_length = 0;
   f->page_crc_tests  = 0;
   f->discard_samples_deferred = 0;
   f->current_loc_valid = FALSE;
   f->first_decode = FALSE;
   f->samples_output = 0;
   f->channel_buffer_start = 0;
   f->channel_buffer_end = 0;
}

static int vorbis_search_for_page_pushdata(vorb *f, uint8 *data, int data_len)
{
   int i,n;
   for (i=0; i < f->page_crc_tests; ++i)
      f->scan[i].bytes_done = 0;

   // if we have room for more scans, search for them first, because
   // they may cause us to stop early if their header is incomplete
   if (f->page_crc_tests < STB_VORBIS_PUSHDATA_CRC_COUNT) {
      if (data_len < 4) return 0;
      data_len -= 3; // need to look for 4-byte sequence, so don't miss
                     // one that straddles a boundary
      for (i=0; i < data_len; ++i) {
         if (data[i] == 0x4f) {
            if (0==memcmp(data+i, ogg_page_header, 4)) {
               int j,len;
               uint32 crc;
               // make sure we have the whole page header
               if (i+26 >= data_len || i+27+data[i+26] >= data_len) {
                  // only read up to this page start, so hopefully we'll
                  // have the whole page header start next time
                  data_len = i;
                  break;
               }
               // ok, we have it all; compute the length of the page
               len = 27 + data[i+26];
               for (j=0; j < data[i+26]; ++j)
                  len += data[i+27+j];
               // scan everything up to the embedded crc (which we must 0)
               crc = 0;
               for (j=0; j < 22; ++j)
                  crc = crc32_update(crc, data[i+j]);
               // now process 4 0-bytes
               for (   ; j < 26; ++j)
                  crc = crc32_update(crc, 0);
               // len is the total number of bytes we need to scan
               n = f->page_crc_tests++;
               f->scan[n].bytes_left = len-j;
               f->scan[n].crc_so_far = crc;
               f->scan[n].goal_crc = data[i+22] + (data[i+23] << 8) + (data[i+24]<<16) + (data[i+25]<<24);
               // if the last frame on a page is continued to the next, then
               // we can't recover the sample_loc immediately
               if (data[i+27+data[i+26]-1] == 255)
                  f->scan[n].sample_loc = ~0;
               else
                  f->scan[n].sample_loc = data[i+6] + (data[i+7] << 8) + (data[i+ 8]<<16) + (data[i+ 9]<<24);
               f->scan[n].bytes_done = i+j;
               if (f->page_crc_tests == STB_VORBIS_PUSHDATA_CRC_COUNT)
                  break;
               // keep going if we still have room for more
            }
         }
      }
   }

   for (i=0; i < f->page_crc_tests;) {
      uint32 crc;
      int j;
      int n = f->scan[i].bytes_done;
      int m = f->scan[i].bytes_left;
      if (m > data_len - n) m = data_len - n;
      // m is the bytes to scan in the current chunk
      crc = f->scan[i].crc_so_far;
      for (j=0; j < m; ++j)
         crc = crc32_update(crc, data[n+j]);
      f->scan[i].bytes_left -= m;
      f->scan[i].crc_so_far = crc;
      if (f->scan[i].bytes_left == 0) {
         // does it match?
         if (f->scan[i].crc_so_far == f->scan[i].goal_crc) {
            // Houston, we have page
            data_len = n+m; // consumption amount is wherever that scan ended
            f->page_crc_tests = -1; // drop out of page scan mode
            f->previous_length = 0; // decode-but-don't-output one frame
            f->next_seg = -1;       // start a new page
            f->current_loc = f->scan[i].sample_loc; // set the current sample location
                                    // to the amount we'd have decoded had we decoded this page
            f->current_loc_valid = f->current_loc != ~0U;
            return data_len;
         }
         // delete entry
         f->scan[i] = f->scan[--f->page_crc_tests];
      } else {
         ++i;
      }
   }

   return data_len;
}

// return value: number of bytes we used
int stb_vorbis_decode_frame_pushdata(
         stb_vorbis *f,                   // the file we're decoding
         const uint8 *data, int data_len, // the memory available for decoding
         int *channels,                   // place to write number of float * buffers
         float ***output,                 // place to write float ** array of float * buffers
         int *samples                     // place to write number of output samples
     )
{
   int i;
   int len,right,left;

   if (!IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);

   if (f->page_crc_tests >= 0) {
      *samples = 0;
      return vorbis_search_for_page_pushdata(f, (uint8 *) data, data_len);
   }

   f->stream     = (uint8 *) data;
   f->stream_end = (uint8 *) data + data_len;
   f->error      = VORBIS__no_error;

   // check that we have the entire packet in memory
   if (!is_whole_packet_present(f)) {
      *samples = 0;
      return 0;
   }

   if (!vorbis_decode_packet(f, &len, &left, &right)) {
      // save the actual error we encountered
      enum STBVorbisError error = f->error;
      if (error == VORBIS_bad_packet_type) {
         // flush and resynch
         f->error = VORBIS__no_error;
         while (get8_packet(f) != EOP)
            if (f->eof) break;
         *samples = 0;
         return (int) (f->stream - data);
      }
      if (error == VORBIS_continued_packet_flag_invalid) {
         if (f->previous_length == 0) {
            // we may be resynching, in which case it's ok to hit one
            // of these; just discard the packet
            f->error = VORBIS__no_error;
            while (get8_packet(f) != EOP)
               if (f->eof) break;
            *samples = 0;
            return (int) (f->stream - data);
         }
      }
      // if we get an error while parsing, what to do?
      // well, it DEFINITELY won't work to continue from where we are!
      stb_vorbis_flush_pushdata(f);
      // restore the error that actually made us bail
      f->error = error;
      *samples = 0;
      return 1;
   }

   // success!
   len = vorbis_finish_frame(f, len, left, right);
   for (i=0; i < f->channels; ++i)
      f->outputs[i] = f->channel_buffers[i] + left;

   if (channels) *channels = f->channels;
   *samples = len;
   *output = f->outputs;
   return (int) (f->stream - data);
}

stb_vorbis *stb_vorbis_open_pushdata(
         const unsigned char *data, int data_len, // the memory available for decoding
         int *data_used,              // only defined if result is not NULL
         int *error, const stb_vorbis_alloc *alloc)
{
   stb_vorbis *f, p;
   vorbis_init(&p, alloc);
   p.stream     = (uint8 *) data;
   p.stream_end = (uint8 *) data + data_len;
   p.push_mode  = TRUE;
   if (!start_decoder(&p)) {
      if (p.eof)
         *error = VORBIS_need_more_data;
      else
         *error = p.error;
      vorbis_deinit(&p);
      return NULL;
   }
   f = vorbis_alloc(&p);
   if (f) {
      *f = p;
      *data_used = (int) (f->stream - data);
      *error = 0;
      return f;
   } else {
      vorbis_deinit(&p);
      return NULL;
   }
}
#endif // STB_VORBIS_NO_PUSHDATA_API

unsigned int stb_vorbis_get_file_offset(stb_vorbis *f)
{
   #ifndef STB_VORBIS_NO_PUSHDATA_API
   if (f->push_mode) return 0;
   #endif
   if (USE_MEMORY(f)) return (unsigned int) (f->stream - f->stream_start);
   #ifndef STB_VORBIS_NO_STDIO
   return (unsigned int) (ftell(f->f) - f->f_start);
   #endif
}

#ifndef STB_VORBIS_NO_PULLDATA_API
//
// DATA-PULLING API
//

static uint32 vorbis_find_page(stb_vorbis *f, uint32 *end, uint32 *last)
{
   for(;;) {
      int n;
      if (f->eof) return 0;
      n = get8(f);
      if (n == 0x4f) { // page header candidate
         unsigned int retry_loc = stb_vorbis_get_file_offset(f);
         int i;
         // check if we're off the end of a file_section stream
         if (retry_loc - 25 > f->stream_len)
            return 0;
         // check the rest of the header
         for (i=1; i < 4; ++i)
            if (get8(f) != ogg_page_header[i])
               break;
         if (f->eof) return 0;
         if (i == 4) {
            uint8 header[27];
            uint32 i, crc, goal, len;
            for (i=0; i < 4; ++i)
               header[i] = ogg_page_header[i];
            for (; i < 27; ++i)
               header[i] = get8(f);
            if (f->eof) return 0;
            if (header[4] != 0) goto invalid;
            goal = header[22] + (header[23] << 8) + (header[24]<<16) + ((uint32)header[25]<<24);
            for (i=22; i < 26; ++i)
               header[i] = 0;
            crc = 0;
            for (i=0; i < 27; ++i)
               crc = crc32_update(crc, header[i]);
            len = 0;
            for (i=0; i < header[26]; ++i) {
               int s = get8(f);
               crc = crc32_update(crc, s);
               len += s;
            }
            if (len && f->eof) return 0;
            for (i=0; i < len; ++i)
               crc = crc32_update(crc, get8(f));
            // finished parsing probable page
            if (crc == goal) {
               // we could now check that it's either got the last
               // page flag set, OR it's followed by the capture
               // pattern, but I guess TECHNICALLY you could have
               // a file with garbage between each ogg page and recover
               // from it automatically? So even though that paranoia
               // might decrease the chance of an invalid decode by
               // another 2^32, not worth it since it would hose those
               // invalid-but-useful files?
               if (end)
                  *end = stb_vorbis_get_file_offset(f);
               if (last) {
                  if (header[5] & 0x04)
                     *last = 1;
                  else
                     *last = 0;
               }
               set_file_offset(f, retry_loc-1);
               return 1;
            }
         }
        invalid:
         // not a valid page, so rewind and look for next one
         set_file_offset(f, retry_loc);
      }
   }
}


#define SAMPLE_unknown  0xffffffff

// seeking is implemented with a binary search, which narrows down the range to
// 64K, before using a linear search (because finding the synchronization
// pattern can be expensive, and the chance we'd find the end page again is
// relatively high for small ranges)
//
// two initial interpolation-style probes are used at the start of the search
// to try to bound either side of the binary search sensibly, while still
// working in O(log n) time if they fail.

static int get_seek_page_info(stb_vorbis *f, ProbedPage *z)
{
   uint8 header[27], lacing[255];
   int i,len;

   // record where the page starts
   z->page_start = stb_vorbis_get_file_offset(f);

   // parse the header
   getn(f, header, 27);
   if (header[0] != 'O' || header[1] != 'g' || header[2] != 'g' || header[3] != 'S')
      return 0;
   getn(f, lacing, header[26]);

   // determine the length of the payload
   len = 0;
   for (i=0; i < header[26]; ++i)
      len += lacing[i];

   // this implies where the page ends
   z->page_end = z->page_start + 27 + header[26] + len;

   // read the last-decoded sample out of the data
   z->last_decoded_sample = header[6] + (header[7] << 8) + (header[8] << 16) + (header[9] << 24);

   // restore file state to where we were
   set_file_offset(f, z->page_start);
   return 1;
}

// rarely used function to seek back to the preceding page while finding the
// start of a packet
static int go_to_page_before(stb_vorbis *f, unsigned int limit_offset)
{
   unsigned int previous_safe, end;

   // now we want to seek back 64K from the limit
   if (limit_offset >= 65536 && limit_offset-65536 >= f->first_audio_page_offset)
      previous_safe = limit_offset - 65536;
   else
      previous_safe = f->first_audio_page_offset;

   set_file_offset(f, previous_safe);

   while (vorbis_find_page(f, &end, NULL)) {
      if (end >= limit_offset && stb_vorbis_get_file_offset(f) < limit_offset)
         return 1;
      set_file_offset(f, end);
   }

   return 0;
}

// implements the search logic for finding a page and starting decoding. if
// the function succeeds, current_loc_valid will be true and current_loc will
// be less than or equal to the provided sample number (the closer the
// better).
static int seek_to_sample_coarse(stb_vorbis *f, uint32 sample_number)
{
   ProbedPage left, right, mid;
   int i, start_seg_with_known_loc, end_pos, page_start;
   uint32 delta, stream_length, padding, last_sample_limit;
   double offset = 0.0, bytes_per_sample = 0.0;
   int probe = 0;

   // find the last page and validate the target sample
   stream_length = stb_vorbis_stream_length_in_samples(f);
   if (stream_length == 0)            return error(f, VORBIS_seek_without_length);
   if (sample_number > stream_length) return error(f, VORBIS_seek_invalid);

   // this is the maximum difference between the window-center (which is the
   // actual granule position value), and the right-start (which the spec
   // indicates should be the granule position (give or take one)).
   padding = ((f->blocksize_1 - f->blocksize_0) >> 2);
   if (sample_number < padding)
      last_sample_limit = 0;
   else
      last_sample_limit = sample_number - padding;

   left = f->p_first;
   while (left.last_decoded_sample == ~0U) {
      // (untested) the first page does not have a 'last_decoded_sample'
      set_file_offset(f, left.page_end);
      if (!get_seek_page_info(f, &left)) goto error;
   }

   right = f->p_last;
   assert(right.last_decoded_sample != ~0U);

   // starting from the start is handled differently
   if (last_sample_limit <= left.last_decoded_sample) {
      if (stb_vorbis_seek_start(f)) {
         if (f->current_loc > sample_number)
            return error(f, VORBIS_seek_failed);
         return 1;
      }
      return 0;
   }

   while (left.page_end != right.page_start) {
      assert(left.page_end < right.page_start);
      // search range in bytes
      delta = right.page_start - left.page_end;
      if (delta <= 65536) {
         // there's only 64K left to search - handle it linearly
         set_file_offset(f, left.page_end);
      } else {
         if (probe < 2) {
            if (probe == 0) {
               // first probe (interpolate)
               double data_bytes = right.page_end - left.page_start;
               bytes_per_sample = data_bytes / right.last_decoded_sample;
               offset = left.page_start + bytes_per_sample * (last_sample_limit - left.last_decoded_sample);
            } else {
               // second probe (try to bound the other side)
               double error = ((double) last_sample_limit - mid.last_decoded_sample) * bytes_per_sample;
               if (error >= 0 && error <  8000) error =  8000;
               if (error <  0 && error > -8000) error = -8000;
               offset += error * 2;
            }

            // ensure the offset is valid
            if (offset < left.page_end)
               offset = left.page_end;
            if (offset > right.page_start - 65536)
               offset = right.page_start - 65536;

            set_file_offset(f, (unsigned int) offset);
         } else {
            // binary search for large ranges (offset by 32K to ensure
            // we don't hit the right page)
            set_file_offset(f, left.page_end + (delta / 2) - 32768);
         }

         if (!vorbis_find_page(f, NULL, NULL)) goto error;
      }

      for (;;) {
         if (!get_seek_page_info(f, &mid)) goto error;
         if (mid.last_decoded_sample != ~0U) break;
         // (untested) no frames end on this page
         set_file_offset(f, mid.page_end);
         assert(mid.page_start < right.page_start);
      }

      // if we've just found the last page again then we're in a tricky file,
      // and we're close enough (if it wasn't an interpolation probe).
      if (mid.page_start == right.page_start) {
         if (probe >= 2 || delta <= 65536)
            break;
      } else {
         if (last_sample_limit < mid.last_decoded_sample)
            right = mid;
         else
            left = mid;
      }

      ++probe;
   }

   // seek back to start of the last packet
   page_start = left.page_start;
   set_file_offset(f, page_start);
   if (!start_page(f)) return error(f, VORBIS_seek_failed);
   end_pos = f->end_seg_with_known_loc;
   assert(end_pos >= 0);

   for (;;) {
      for (i = end_pos; i > 0; --i)
         if (f->segments[i-1] != 255)
            break;

      start_seg_with_known_loc = i;

      if (start_seg_with_known_loc > 0 || !(f->page_flag & PAGEFLAG_continued_packet))
         break;

      // (untested) the final packet begins on an earlier page
      if (!go_to_page_before(f, page_start))
         goto error;

      page_start = stb_vorbis_get_file_offset(f);
      if (!start_page(f)) goto error;
      end_pos = f->segment_count - 1;
   }

   // prepare to start decoding
   f->current_loc_valid = FALSE;
   f->last_seg = FALSE;
   f->valid_bits = 0;
   f->packet_bytes = 0;
   f->bytes_in_seg = 0;
   f->previous_length = 0;
   f->next_seg = start_seg_with_known_loc;

   for (i = 0; i < start_seg_with_known_loc; i++)
      skip(f, f->segments[i]);

   // start decoding (optimizable - this frame is generally discarded)
   if (!vorbis_pump_first_frame(f))
      return 0;
   if (f->current_loc > sample_number)
      return error(f, VORBIS_seek_failed);
   return 1;

error:
   // try to restore the file to a valid state
   stb_vorbis_seek_start(f);
   return error(f, VORBIS_seek_failed);
}

// the same as vorbis_decode_initial, but without advancing
static int peek_decode_initial(vorb *f, int *p_left_start, int *p_left_end, int *p_right_start, int *p_right_end, int *mode)
{
   int bits_read, bytes_read;

   if (!vorbis_decode_initial(f, p_left_start, p_left_end, p_right_start, p_right_end, mode))
      return 0;

   // either 1 or 2 bytes were read, figure out which so we can rewind
   bits_read = 1 + ilog(f->mode_count-1);
   if (f->mode_config[*mode].blockflag)
      bits_read += 2;
   bytes_read = (bits_read + 7) / 8;

   f->bytes_in_seg += bytes_read;
   f->packet_bytes -= bytes_read;
   skip(f, -bytes_read);
   if (f->next_seg == -1)
      f->next_seg = f->segment_count - 1;
   else
      f->next_seg--;
   f->valid_bits = 0;

   return 1;
}

int stb_vorbis_seek_frame(stb_vorbis *f, unsigned int sample_number)
{
   uint32 max_frame_samples;

   if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);

   // fast page-level search
   if (!seek_to_sample_coarse(f, sample_number))
      return 0;

   assert(f->current_loc_valid);
   assert(f->current_loc <= sample_number);

   // linear search for the relevant packet
   max_frame_samples = (f->blocksize_1*3 - f->blocksize_0) >> 2;
   while (f->current_loc < sample_number) {
      int left_start, left_end, right_start, right_end, mode, frame_samples;
      if (!peek_decode_initial(f, &left_start, &left_end, &right_start, &right_end, &mode))
         return error(f, VORBIS_seek_failed);
      // calculate the number of samples returned by the next frame
      frame_samples = right_start - left_start;
      if (f->current_loc + frame_samples > sample_number) {
         return 1; // the next frame will contain the sample
      } else if (f->current_loc + frame_samples + max_frame_samples > sample_number) {
         // there's a chance the frame after this could contain the sample
         vorbis_pump_first_frame(f);
      } else {
         // this frame is too early to be relevant
         f->current_loc += frame_samples;
         f->previous_length = 0;
         maybe_start_packet(f);
         flush_packet(f);
      }
   }
   // the next frame should start with the sample
   if (f->current_loc != sample_number) return error(f, VORBIS_seek_failed);
   return 1;
}

int stb_vorbis_seek(stb_vorbis *f, unsigned int sample_number)
{
   if (!stb_vorbis_seek_frame(f, sample_number))
      return 0;

   if (sample_number != f->current_loc) {
      int n;
      uint32 frame_start = f->current_loc;
      stb_vorbis_get_frame_float(f, &n, NULL);
      assert(sample_number > frame_start);
      assert(f->channel_buffer_start + (int) (sample_number-frame_start) <= f->channel_buffer_end);
      f->channel_buffer_start += (sample_number - frame_start);
   }

   return 1;
}

int stb_vorbis_seek_start(stb_vorbis *f)
{
   if (IS_PUSH_MODE(f)) { return error(f, VORBIS_invalid_api_mixing); }
   set_file_offset(f, f->first_audio_page_offset);
   f->previous_length = 0;
   f->first_decode = TRUE;
   f->next_seg = -1;
   return vorbis_pump_first_frame(f);
}

unsigned int stb_vorbis_stream_length_in_samples(stb_vorbis *f)
{
   unsigned int restore_offset, previous_safe;
   unsigned int end, last_page_loc;

   if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);
   if (!f->total_samples) {
      unsigned int last;
      uint32 lo,hi;
      char header[6];

      // first, store the current decode position so we can restore it
      restore_offset = stb_vorbis_get_file_offset(f);

      // now we want to seek back 64K from the end (the last page must
      // be at most a little less than 64K, but let's allow a little slop)
      if (f->stream_len >= 65536 && f->stream_len-65536 >= f->first_audio_page_offset)
         previous_safe = f->stream_len - 65536;
      else
         previous_safe = f->first_audio_page_offset;

      set_file_offset(f, previous_safe);
      // previous_safe is now our candidate 'earliest known place that seeking
      // to will lead to the final page'

      if (!vorbis_find_page(f, &end, &last)) {
         // if we can't find a page, we're hosed!
         f->error = VORBIS_cant_find_last_page;
         f->total_samples = 0xffffffff;
         goto done;
      }

      // check if there are more pages
      last_page_loc = stb_vorbis_get_file_offset(f);

      // stop when the last_page flag is set, not when we reach eof;
      // this allows us to stop short of a 'file_section' end without
      // explicitly checking the length of the section
      while (!last) {
         set_file_offset(f, end);
         if (!vorbis_find_page(f, &end, &last)) {
            // the last page we found didn't have the 'last page' flag
            // set. whoops!
            break;
         }
         //previous_safe = last_page_loc+1; // NOTE: not used after this point, but note for debugging
         last_page_loc = stb_vorbis_get_file_offset(f);
      }

      set_file_offset(f, last_page_loc);

      // parse the header
      getn(f, (unsigned char *)header, 6);
      // extract the absolute granule position
      lo = get32(f);
      hi = get32(f);
      if (lo == 0xffffffff && hi == 0xffffffff) {
         f->error = VORBIS_cant_find_last_page;
         f->total_samples = SAMPLE_unknown;
         goto done;
      }
      if (hi)
         lo = 0xfffffffe; // saturate
      f->total_samples = lo;

      f->p_last.page_start = last_page_loc;
      f->p_last.page_end   = end;
      f->p_last.last_decoded_sample = lo;

     done:
      set_file_offset(f, restore_offset);
   }
   return f->total_samples == SAMPLE_unknown ? 0 : f->total_samples;
}

float stb_vorbis_stream_length_in_seconds(stb_vorbis *f)
{
   return stb_vorbis_stream_length_in_samples(f) / (float) f->sample_rate;
}



int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output)
{
   int len, right,left,i;
   if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);

   if (!vorbis_decode_packet(f, &len, &left, &right)) {
      f->channel_buffer_start = f->channel_buffer_end = 0;
      return 0;
   }

   len = vorbis_finish_frame(f, len, left, right);
   for (i=0; i < f->channels; ++i)
      f->outputs[i] = f->channel_buffers[i] + left;

   f->channel_buffer_start = left;
   f->channel_buffer_end   = left+len;

   if (channels) *channels = f->channels;
   if (output)   *output = f->outputs;
   return len;
}

#ifndef STB_VORBIS_NO_STDIO

stb_vorbis * stb_vorbis_open_file_section(FILE *file, int close_on_free, int *error, const stb_vorbis_alloc *alloc, unsigned int length)
{
   stb_vorbis *f, p;
   vorbis_init(&p, alloc);
   p.f = file;
   p.f_start = (uint32) ftell(file);
   p.stream_len   = length;
   p.close_on_free = close_on_free;
   if (start_decoder(&p)) {
      f = vorbis_alloc(&p);
      if (f) {
         *f = p;
         vorbis_pump_first_frame(f);
         return f;
      }
   }
   if (error) *error = p.error;
   vorbis_deinit(&p);
   return NULL;
}

stb_vorbis * stb_vorbis_open_file(FILE *file, int close_on_free, int *error, const stb_vorbis_alloc *alloc)
{
   unsigned int len, start;
   start = (unsigned int) ftell(file);
   fseek(file, 0, SEEK_END);
   len = (unsigned int) (ftell(file) - start);
   fseek(file, start, SEEK_SET);
   return stb_vorbis_open_file_section(file, close_on_free, error, alloc, len);
}

stb_vorbis * stb_vorbis_open_filename(const char *filename, int *error, const stb_vorbis_alloc *alloc)
{
   FILE *f;
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
   if (0 != fopen_s(&f, filename, "rb"))
      f = NULL;
#else
   f = fopen(filename, "rb");
#endif
   if (f)
      return stb_vorbis_open_file(f, TRUE, error, alloc);
   if (error) *error = VORBIS_file_open_failure;
   return NULL;
}
#endif // STB_VORBIS_NO_STDIO

stb_vorbis * stb_vorbis_open_memory(const unsigned char *data, int len, int *error, const stb_vorbis_alloc *alloc)
{
   stb_vorbis *f, p;
   if (!data) {
      if (error) *error = VORBIS_unexpected_eof;
      return NULL;
   }
   vorbis_init(&p, alloc);
   p.stream = (uint8 *) data;
   p.stream_end = (uint8 *) data + len;
   p.stream_start = (uint8 *) p.stream;
   p.stream_len = len;
   p.push_mode = FALSE;
   if (start_decoder(&p)) {
      f = vorbis_alloc(&p);
      if (f) {
         *f = p;
         vorbis_pump_first_frame(f);
         if (error) *error = VORBIS__no_error;
         return f;
      }
   }
   if (error) *error = p.error;
   vorbis_deinit(&p);
   return NULL;
}

#ifndef STB_VORBIS_NO_INTEGER_CONVERSION
#define PLAYBACK_MONO     1
#define PLAYBACK_LEFT     2
#define PLAYBACK_RIGHT    4

#define L  (PLAYBACK_LEFT  | PLAYBACK_MONO)
#define C  (PLAYBACK_LEFT  | PLAYBACK_RIGHT | PLAYBACK_MONO)
#define R  (PLAYBACK_RIGHT | PLAYBACK_MONO)

static int8 channel_position[7][6] =
{
   { 0 },
   { C },
   { L, R },
   { L, C, R },
   { L, R, L, R },
   { L, C, R, L, R },
   { L, C, R, L, R, C },
};


#ifndef STB_VORBIS_NO_FAST_SCALED_FLOAT
   typedef union {
      float f;
      int i;
   } float_conv;
   typedef char stb_vorbis_float_size_test[sizeof(float)==4 && sizeof(int) == 4];
   #define FASTDEF(x) float_conv x
   // add (1<<23) to convert to int, then divide by 2^SHIFT, then add 0.5/2^SHIFT to round
   #define MAGIC(SHIFT) (1.5f * (1 << (23-SHIFT)) + 0.5f/(1 << SHIFT))
   #define ADDEND(SHIFT) (((150-SHIFT) << 23) + (1 << 22))
   #define FAST_SCALED_FLOAT_TO_INT(temp,x,s) (temp.f = (x) + MAGIC(s), temp.i - ADDEND(s))
   #define check_endianness()
#else
   #define FAST_SCALED_FLOAT_TO_INT(temp,x,s) ((int) ((x) * (1 << (s))))
   #define check_endianness()
   #define FASTDEF(x)
#endif

static void copy_samples(short *dest, float *src, int len)
{
   int i;
   check_endianness();
   for (i=0; i < len; ++i) {
      FASTDEF(temp);
      int v = FAST_SCALED_FLOAT_TO_INT(temp, src[i],15);
      if ((unsigned int) (v + 32768) > 65535)
         v = v < 0 ? -32768 : 32767;
      dest[i] = v;
   }
}

static void compute_samples(int mask, short *output, int num_c, float **data, int d_offset, int len)
{
   #define STB_BUFFER_SIZE  32
   float buffer[STB_BUFFER_SIZE];
   int i,j,o,n = STB_BUFFER_SIZE;
   check_endianness();
   for (o = 0; o < len; o += STB_BUFFER_SIZE) {
      memset(buffer, 0, sizeof(buffer));
      if (o + n > len) n = len - o;
      for (j=0; j < num_c; ++j) {
         if (channel_position[num_c][j] & mask) {
            for (i=0; i < n; ++i)
               buffer[i] += data[j][d_offset+o+i];
         }
      }
      for (i=0; i < n; ++i) {
         FASTDEF(temp);
         int v = FAST_SCALED_FLOAT_TO_INT(temp,buffer[i],15);
         if ((unsigned int) (v + 32768) > 65535)
            v = v < 0 ? -32768 : 32767;
         output[o+i] = v;
      }
   }
   #undef STB_BUFFER_SIZE
}

static void compute_stereo_samples(short *output, int num_c, float **data, int d_offset, int len)
{
   #define STB_BUFFER_SIZE  32
   float buffer[STB_BUFFER_SIZE];
   int i,j,o,n = STB_BUFFER_SIZE >> 1;
   // o is the offset in the source data
   check_endianness();
   for (o = 0; o < len; o += STB_BUFFER_SIZE >> 1) {
      // o2 is the offset in the output data
      int o2 = o << 1;
      memset(buffer, 0, sizeof(buffer));
      if (o + n > len) n = len - o;
      for (j=0; j < num_c; ++j) {
         int m = channel_position[num_c][j] & (PLAYBACK_LEFT | PLAYBACK_RIGHT);
         if (m == (PLAYBACK_LEFT | PLAYBACK_RIGHT)) {
            for (i=0; i < n; ++i) {
               buffer[i*2+0] += data[j][d_offset+o+i];
               buffer[i*2+1] += data[j][d_offset+o+i];
            }
         } else if (m == PLAYBACK_LEFT) {
            for (i=0; i < n; ++i) {
               buffer[i*2+0] += data[j][d_offset+o+i];
            }
         } else if (m == PLAYBACK_RIGHT) {
            for (i=0; i < n; ++i) {
               buffer[i*2+1] += data[j][d_offset+o+i];
            }
         }
      }
      for (i=0; i < (n<<1); ++i) {
         FASTDEF(temp);
         int v = FAST_SCALED_FLOAT_TO_INT(temp,buffer[i],15);
         if ((unsigned int) (v + 32768) > 65535)
            v = v < 0 ? -32768 : 32767;
         output[o2+i] = v;
      }
   }
   #undef STB_BUFFER_SIZE
}

static void convert_samples_short(int buf_c, short **buffer, int b_offset, int data_c, float **data, int d_offset, int samples)
{
   int i;
   if (buf_c != data_c && buf_c <= 2 && data_c <= 6) {
      static int channel_selector[3][2] = { {0}, {PLAYBACK_MONO}, {PLAYBACK_LEFT, PLAYBACK_RIGHT} };
      for (i=0; i < buf_c; ++i)
         compute_samples(channel_selector[buf_c][i], buffer[i]+b_offset, data_c, data, d_offset, samples);
   } else {
      int limit = buf_c < data_c ? buf_c : data_c;
      for (i=0; i < limit; ++i)
         copy_samples(buffer[i]+b_offset, data[i]+d_offset, samples);
      for (   ; i < buf_c; ++i)
         memset(buffer[i]+b_offset, 0, sizeof(short) * samples);
   }
}

int stb_vorbis_get_frame_short(stb_vorbis *f, int num_c, short **buffer, int num_samples)
{
   float **output = NULL;
   int len = stb_vorbis_get_frame_float(f, NULL, &output);
   if (len > num_samples) len = num_samples;
   if (len)
      convert_samples_short(num_c, buffer, 0, f->channels, output, 0, len);
   return len;
}

static void convert_channels_short_interleaved(int buf_c, short *buffer, int data_c, float **data, int d_offset, int len)
{
   int i;
   check_endianness();
   if (buf_c != data_c && buf_c <= 2 && data_c <= 6) {
      assert(buf_c == 2);
      for (i=0; i < buf_c; ++i)
         compute_stereo_samples(buffer, data_c, data, d_offset, len);
   } else {
      int limit = buf_c < data_c ? buf_c : data_c;
      int j;
      for (j=0; j < len; ++j) {
         for (i=0; i < limit; ++i) {
            FASTDEF(temp);
            float f = data[i][d_offset+j];
            int v = FAST_SCALED_FLOAT_TO_INT(temp, f,15);//data[i][d_offset+j],15);
            if ((unsigned int) (v + 32768) > 65535)
               v = v < 0 ? -32768 : 32767;
            *buffer++ = v;
         }
         for (   ; i < buf_c; ++i)
            *buffer++ = 0;
      }
   }
}

int stb_vorbis_get_frame_short_interleaved(stb_vorbis *f, int num_c, short *buffer, int num_shorts)
{
   float **output;
   int len;
   if (num_c == 1) return stb_vorbis_get_frame_short(f,num_c,&buffer, num_shorts);
   len = stb_vorbis_get_frame_float(f, NULL, &output);
   if (len) {
      if (len*num_c > num_shorts) len = num_shorts / num_c;
      convert_channels_short_interleaved(num_c, buffer, f->channels, output, 0, len);
   }
   return len;
}

int stb_vorbis_get_samples_short_interleaved(stb_vorbis *f, int channels, short *buffer, int num_shorts)
{
   float **outputs;
   int len = num_shorts / channels;
   int n=0;
   while (n < len) {
      int k = f->channel_buffer_end - f->channel_buffer_start;
      if (n+k >= len) k = len - n;
      if (k)
         convert_channels_short_interleaved(channels, buffer, f->channels, f->channel_buffers, f->channel_buffer_start, k);
      buffer += k*channels;
      n += k;
      f->channel_buffer_start += k;
      if (n == len) break;
      if (!stb_vorbis_get_frame_float(f, NULL, &outputs)) break;
   }
   return n;
}

int stb_vorbis_get_samples_short(stb_vorbis *f, int channels, short **buffer, int len)
{
   float **outputs;
   int n=0;
   while (n < len) {
      int k = f->channel_buffer_end - f->channel_buffer_start;
      if (n+k >= len) k = len - n;
      if (k)
         convert_samples_short(channels, buffer, n, f->channels, f->channel_buffers, f->channel_buffer_start, k);
      n += k;
      f->channel_buffer_start += k;
      if (n == len) break;
      if (!stb_vorbis_get_frame_float(f, NULL, &outputs)) break;
   }
   return n;
}

#ifndef STB_VORBIS_NO_STDIO
int stb_vorbis_decode_filename(const char *filename, int *channels, int *sample_rate, short **output)
{
   int data_len, offset, total, limit, error;
   short *data;
   stb_vorbis *v = stb_vorbis_open_filename(filename, &error, NULL);
   if (v == NULL) return -1;
   limit = v->channels * 4096;
   *channels = v->channels;
   if (sample_rate)
      *sample_rate = v->sample_rate;
   offset = data_len = 0;
   total = limit;
   data = (short *) malloc(total * sizeof(*data));
   if (data == NULL) {
      stb_vorbis_close(v);
      return -2;
   }
   for (;;) {
      int n = stb_vorbis_get_frame_short_interleaved(v, v->channels, data+offset, total-offset);
      if (n == 0) break;
      data_len += n;
      offset += n * v->channels;
      if (offset + limit > total) {
         short *data2;
         total *= 2;
         data2 = (short *) realloc(data, total * sizeof(*data));
         if (data2 == NULL) {
            free(data);
            stb_vorbis_close(v);
            return -2;
         }
         data = data2;
      }
   }
   *output = data;
   stb_vorbis_close(v);
   return data_len;
}
#endif // NO_STDIO

int stb_vorbis_decode_memory(const uint8 *mem, int len, int *channels, int *sample_rate, short **output)
{
   int data_len, offset, total, limit, error;
   short *data;
   stb_vorbis *v = stb_vorbis_open_memory(mem, len, &error, NULL);
   if (v == NULL) return -1;
   limit = v->channels * 4096;
   *channels = v->channels;
   if (sample_rate)
      *sample_rate = v->sample_rate;
   offset = data_len = 0;
   total = limit;
   data = (short *) malloc(total * sizeof(*data));
   if (data == NULL) {
      stb_vorbis_close(v);
      return -2;
   }
   for (;;) {
      int n = stb_vorbis_get_frame_short_interleaved(v, v->channels, data+offset, total-offset);
      if (n == 0) break;
      data_len += n;
      offset += n * v->channels;
      if (offset + limit > total) {
         short *data2;
         total *= 2;
         data2 = (short *) realloc(data, total * sizeof(*data));
         if (data2 == NULL) {
            free(data);
            stb_vorbis_close(v);
            return -2;
         }
         data = data2;
      }
   }
   *output = data;
   stb_vorbis_close(v);
   return data_len;
}
#endif // STB_VORBIS_NO_INTEGER_CONVERSION

int stb_vorbis_get_samples_float_interleaved(stb_vorbis *f, int channels, float *buffer, int num_floats)
{
   float **outputs;
   int len = num_floats / channels;
   int n=0;
   int z = f->channels;
   if (z > channels) z = channels;
   while (n < len) {
      int i,j;
      int k = f->channel_buffer_end - f->channel_buffer_start;
      if (n+k >= len) k = len - n;
      for (j=0; j < k; ++j) {
         for (i=0; i < z; ++i)
            *buffer++ = f->channel_buffers[i][f->channel_buffer_start+j];
         for (   ; i < channels; ++i)
            *buffer++ = 0;
      }
      n += k;
      f->channel_buffer_start += k;
      if (n == len)
         break;
      if (!stb_vorbis_get_frame_float(f, NULL, &outputs))
         break;
   }
   return n;
}

int stb_vorbis_get_samples_float(stb_vorbis *f, int channels, float **buffer, int num_samples)
{
   float **outputs;
   int n=0;
   int z = f->channels;
   if (z > channels) z = channels;
   while (n < num_samples) {
      int i;
      int k = f->channel_buffer_end - f->channel_buffer_start;
      if (n+k >= num_samples) k = num_samples - n;
      if (k) {
         for (i=0; i < z; ++i)
            memcpy(buffer[i]+n, f->channel_buffers[i]+f->channel_buffer_start, sizeof(float)*k);
         for (   ; i < channels; ++i)
            memset(buffer[i]+n, 0, sizeof(float) * k);
      }
      n += k;
      f->channel_buffer_start += k;
      if (n == num_samples)
         break;
      if (!stb_vorbis_get_frame_float(f, NULL, &outputs))
         break;
   }
   return n;
}
#endif // STB_VORBIS_NO_PULLDATA_API

/* Version history
    1.17    - 2019-07-08 - fix CVE-2019-13217, -13218, -13219, -13220, -13221, -13222, -13223
                           found with Mayhem by ForAllSecure
    1.16    - 2019-03-04 - fix warnings
    1.15    - 2019-02-07 - explicit failure if Ogg Skeleton data is found
    1.14    - 2018-02-11 - delete bogus dealloca usage
    1.13    - 2018-01-29 - fix truncation of last frame (hopefully)
    1.12    - 2017-11-21 - limit residue begin/end to blocksize/2 to avoid large temp allocs in bad/corrupt files
    1.11    - 2017-07-23 - fix MinGW compilation
    1.10    - 2017-03-03 - more robust seeking; fix negative ilog(); clear error in open_memory
    1.09    - 2016-04-04 - back out 'avoid discarding last frame' fix from previous version
    1.08    - 2016-04-02 - fixed multiple warnings; fix setup memory leaks;
                           avoid discarding last frame of audio data
    1.07    - 2015-01-16 - fixed some warnings, fix mingw, const-correct API
                           some more crash fixes when out of memory or with corrupt files
    1.06    - 2015-08-31 - full, correct support for seeking API (Dougall Johnson)
                           some crash fixes when out of memory or with corrupt files
    1.05    - 2015-04-19 - don't define __forceinline if it's redundant
    1.04    - 2014-08-27 - fix missing const-correct case in API
    1.03    - 2014-08-07 - Warning fixes
    1.02    - 2014-07-09 - Declare qsort compare function _cdecl on windows
    1.01    - 2014-06-18 - fix stb_vorbis_get_samples_float
    1.0     - 2014-05-26 - fix memory leaks; fix warnings; fix bugs in multichannel
                           (API change) report sample rate for decode-full-file funcs
    0.99996 - bracket #include <malloc.h> for macintosh compilation by Laurent Gomila
    0.99995 - use union instead of pointer-cast for fast-float-to-int to avoid alias-optimization problem
    0.99994 - change fast-float-to-int to work in single-precision FPU mode, remove endian-dependence
    0.99993 - remove assert that fired on legal files with empty tables
    0.99992 - rewind-to-start
    0.99991 - bugfix to stb_vorbis_get_samples_short by Bernhard Wodo
    0.9999 - (should have been 0.99990) fix no-CRT support, compiling as C++
    0.9998 - add a full-decode function with a memory source
    0.9997 - fix a bug in the read-from-FILE case in 0.9996 addition
    0.9996 - query length of vorbis stream in samples/seconds
    0.9995 - bugfix to another optimization that only happened in certain files
    0.9994 - bugfix to one of the optimizations that caused significant (but inaudible?) errors
    0.9993 - performance improvements; runs in 99% to 104% of time of reference implementation
    0.9992 - performance improvement of IMDCT; now performs close to reference implementation
    0.9991 - performance improvement of IMDCT
    0.999 - (should have been 0.9990) performance improvement of IMDCT
    0.998 - no-CRT support from Casey Muratori
    0.997 - bugfixes for bugs found by Terje Mathisen
    0.996 - bugfix: fast-huffman decode initialized incorrectly for sparse codebooks; fixing gives 10% speedup - found by Terje Mathisen
    0.995 - bugfix: fix to 'effective' overrun detection - found by Terje Mathisen
    0.994 - bugfix: garbage decode on final VQ symbol of a non-multiple - found by Terje Mathisen
    0.993 - bugfix: pushdata API required 1 extra byte for empty page (failed to consume final page if empty) - found by Terje Mathisen
    0.992 - fixes for MinGW warning
    0.991 - turn fast-float-conversion on by default
    0.990 - fix push-mode seek recovery if you seek into the headers
    0.98b - fix to bad release of 0.98
    0.98 - fix push-mode seek recovery; robustify float-to-int and support non-fast mode
    0.97 - builds under c++ (typecasting, don't use 'class' keyword)
    0.96 - somehow MY 0.95 was right, but the web one was wrong, so here's my 0.95 rereleased as 0.96, fixes a typo in the clamping code
    0.95 - clamping code for 16-bit functions
    0.94 - not publically released
    0.93 - fixed all-zero-floor case (was decoding garbage)
    0.92 - fixed a memory leak
    0.91 - conditional compiles to omit parts of the API and the infrastructure to support them: STB_VORBIS_NO_PULLDATA_API, STB_VORBIS_NO_PUSHDATA_API, STB_VORBIS_NO_STDIO, STB_VORBIS_NO_INTEGER_CONVERSION
    0.90 - first public release
*/

#endif // STB_VORBIS_HEADER_ONLY


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
//FILE_END

#endif // POKI_IMPL || PK_AUDIO_IMPL

//FILE_START:sokol_audio.h
#if defined(SOKOL_IMPL) && !defined(SOKOL_AUDIO_IMPL)
#define SOKOL_AUDIO_IMPL
#endif
#ifndef SOKOL_AUDIO_INCLUDED
/*
    sokol_audio.h -- cross-platform audio-streaming API

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL or
        #define SOKOL_AUDIO_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following defines with your own implementations:

    SOKOL_DUMMY_BACKEND - use a dummy backend
    SOKOL_ASSERT(c)     - your own assert macro (default: assert(c))
    SOKOL_AUDIO_API_DECL- public function declaration prefix (default: extern)
    SOKOL_API_DECL      - same as SOKOL_AUDIO_API_DECL
    SOKOL_API_IMPL      - public function implementation prefix (default: -)

    SAUDIO_RING_MAX_SLOTS           - max number of slots in the push-audio ring buffer (default 1024)
    SAUDIO_OSX_USE_SYSTEM_HEADERS   - define this to force inclusion of system headers on
                                      macOS instead of using embedded CoreAudio declarations

    If sokol_audio.h is compiled as a DLL, define the following before
    including the declaration or implementation:

    SOKOL_DLL

    On Windows, SOKOL_DLL will define SOKOL_AUDIO_API_DECL as __declspec(dllexport)
    or __declspec(dllimport) as needed.

    Link with the following libraries:

    - on macOS: AudioToolbox
    - on iOS: AudioToolbox, AVFoundation
    - on FreeBSD: asound
    - on Linux: asound
    - on Android: aaudio
    - on Windows with MSVC or Clang toolchain: no action needed, libs are defined in-source via pragma-comment-lib
    - on Windows with MINGW/MSYS2 gcc: compile with '-mwin32' and link with -lole32

    FEATURE OVERVIEW
    ================
    You provide a mono- or stereo-stream of 32-bit float samples, which
    Sokol Audio feeds into platform-specific audio backends:

    - Windows: WASAPI
    - Linux: ALSA
    - FreeBSD: ALSA
    - macOS: CoreAudio
    - iOS: CoreAudio+AVAudioSession
    - emscripten: WebAudio with ScriptProcessorNode
    - Android: AAudio

    Sokol Audio will not do any buffer mixing or volume control, if you have
    multiple independent input streams of sample data you need to perform the
    mixing yourself before forwarding the data to Sokol Audio.

    There are two mutually exclusive ways to provide the sample data:

    1. Callback model: You provide a callback function, which will be called
       when Sokol Audio needs new samples. On all platforms except emscripten,
       this function is called from a separate thread.
    2. Push model: Your code pushes small blocks of sample data from your
       main loop or a thread you created. The pushed data is stored in
       a ring buffer where it is pulled by the backend code when
       needed.

    The callback model is preferred because it is the most direct way to
    feed sample data into the audio backends and also has less moving parts
    (there is no ring buffer between your code and the audio backend).

    Sometimes it is not possible to generate the audio stream directly in a
    callback function running in a separate thread, for such cases Sokol Audio
    provides the push-model as a convenience.

    SOKOL AUDIO, SOLOUD AND MINIAUDIO
    =================================
    The WASAPI, ALSA and CoreAudio backend code has been taken from the
    SoLoud library (with some modifications, so any bugs in there are most
    likely my fault). If you need a more fully-featured audio solution, check
    out SoLoud, it's excellent:

        https://github.com/jarikomppa/soloud

    Another alternative which feature-wise is somewhere inbetween SoLoud and
    sokol-audio might be MiniAudio:

        https://github.com/mackron/miniaudio

    GLOSSARY
    ========
    - stream buffer:
        The internal audio data buffer, usually provided by the backend API. The
        size of the stream buffer defines the base latency, smaller buffers have
        lower latency but may cause audio glitches. Bigger buffers reduce or
        eliminate glitches, but have a higher base latency.

    - stream callback:
        Optional callback function which is called by Sokol Audio when it
        needs new samples. On Windows, macOS/iOS and Linux, this is called in
        a separate thread, on WebAudio, this is called per-frame in the
        browser thread.

    - channel:
        A discrete track of audio data, currently 1-channel (mono) and
        2-channel (stereo) is supported and tested.

    - sample:
        The magnitude of an audio signal on one channel at a given time. In
        Sokol Audio, samples are 32-bit float numbers in the range -1.0 to
        +1.0.

    - frame:
        The tightly packed set of samples for all channels at a given time.
        For mono 1 frame is 1 sample. For stereo, 1 frame is 2 samples.

    - packet:
        In Sokol Audio, a small chunk of audio data that is moved from the
        main thread to the audio streaming thread in order to decouple the
        rate at which the main thread provides new audio data, and the
        streaming thread consuming audio data.

    WORKING WITH SOKOL AUDIO
    ========================
    First call saudio_setup() with your preferred audio playback options.
    In most cases you can stick with the default values, these provide
    a good balance between low-latency and glitch-free playback
    on all audio backends.

    You should always provide a logging callback to be aware of any
    warnings and errors. The easiest way is to use sokol_log.h for this:

        #include "sokol_log.h"
        // ...
        saudio_setup(&(saudio_desc){
            .logger = {
                .func = slog_func,
            }
        });

    If you want to use the callback-model, you need to provide a stream
    callback function either in saudio_desc.stream_cb or saudio_desc.stream_userdata_cb,
    otherwise keep both function pointers zero-initialized.

    Use push model and default playback parameters:

        saudio_setup(&(saudio_desc){ .logger.func = slog_func });

    Use stream callback model and default playback parameters:

        saudio_setup(&(saudio_desc){
            .stream_cb = my_stream_callback
            .logger.func = slog_func,
        });

    The standard stream callback doesn't have a user data argument, if you want
    that, use the alternative stream_userdata_cb and also set the user_data pointer:

        saudio_setup(&(saudio_desc){
            .stream_userdata_cb = my_stream_callback,
            .user_data = &my_data
            .logger.func = slog_func,
        });

    The following playback parameters can be provided through the
    saudio_desc struct:

    General parameters (both for stream-callback and push-model):

        int sample_rate     -- the sample rate in Hz, default: 44100
        int num_channels    -- number of channels, default: 1 (mono)
        int buffer_frames   -- number of frames in streaming buffer, default: 2048

    The stream callback prototype (either with or without userdata):

        void (*stream_cb)(float* buffer, int num_frames, int num_channels)
        void (*stream_userdata_cb)(float* buffer, int num_frames, int num_channels, void* user_data)
            Function pointer to the user-provide stream callback.

    Push-model parameters:

        int packet_frames   -- number of frames in a packet, default: 128
        int num_packets     -- number of packets in ring buffer, default: 64

    The sample_rate and num_channels parameters are only hints for the audio
    backend, it isn't guaranteed that those are the values used for actual
    playback.

    To get the actual parameters, call the following functions after
    saudio_setup():

        int saudio_sample_rate(void)
        int saudio_channels(void);

    It's unlikely that the number of channels will be different than requested,
    but a different sample rate isn't uncommon.

    (NOTE: there's an yet unsolved issue when an audio backend might switch
    to a different sample rate when switching output devices, for instance
    plugging in a bluetooth headset, this case is currently not handled in
    Sokol Audio).

    You can check if audio initialization was successful with
    saudio_isvalid(). If backend initialization failed for some reason
    (for instance when there's no audio device in the machine), this
    will return false. Not checking for success won't do any harm, all
    Sokol Audio function will silently fail when called after initialization
    has failed, so apart from missing audio output, nothing bad will happen.

    Before your application exits, you should call

        saudio_shutdown();

    This stops the audio thread (on Linux, Windows and macOS/iOS) and
    properly shuts down the audio backend.

    THE STREAM CALLBACK MODEL
    =========================
    To use Sokol Audio in stream-callback-mode, provide a callback function
    like this in the saudio_desc struct when calling saudio_setup():

    void stream_cb(float* buffer, int num_frames, int num_channels) {
        ...
    }

    Or the alternative version with a user-data argument:

    void stream_userdata_cb(float* buffer, int num_frames, int num_channels, void* user_data) {
        my_data_t* my_data = (my_data_t*) user_data;
        ...
    }

    The job of the callback function is to fill the *buffer* with 32-bit
    float sample values.

    To output silence, fill the buffer with zeros:

        void stream_cb(float* buffer, int num_frames, int num_channels) {
            const int num_samples = num_frames * num_channels;
            for (int i = 0; i < num_samples; i++) {
                buffer[i] = 0.0f;
            }
        }

    For stereo output (num_channels == 2), the samples for the left
    and right channel are interleaved:

        void stream_cb(float* buffer, int num_frames, int num_channels) {
            assert(2 == num_channels);
            for (int i = 0; i < num_frames; i++) {
                buffer[2*i + 0] = ...;  // left channel
                buffer[2*i + 1] = ...;  // right channel
            }
        }

    Please keep in mind that the stream callback function is running in a
    separate thread, if you need to share data with the main thread you need
    to take care yourself to make the access to the shared data thread-safe!

    THE PUSH MODEL
    ==============
    To use the push-model for providing audio data, simply don't set (keep
    zero-initialized) the stream_cb field in the saudio_desc struct when
    calling saudio_setup().

    To provide sample data with the push model, call the saudio_push()
    function at regular intervals (for instance once per frame). You can
    call the saudio_expect() function to ask Sokol Audio how much room is
    in the ring buffer, but if you provide a continuous stream of data
    at the right sample rate, saudio_expect() isn't required (it's a simple
    way to sync/throttle your sample generation code with the playback
    rate though).

    With saudio_push() you may need to maintain your own intermediate sample
    buffer, since pushing individual sample values isn't very efficient.
    The following example is from the MOD player sample in
    sokol-samples (https://github.com/floooh/sokol-samples):

        const int num_frames = saudio_expect();
        if (num_frames > 0) {
            const int num_samples = num_frames * saudio_channels();
            read_samples(flt_buf, num_samples);
            saudio_push(flt_buf, num_frames);
        }

    Another option is to ignore saudio_expect(), and just push samples as they
    are generated in small batches. In this case you *need* to generate the
    samples at the right sample rate:

    The following example is taken from the Tiny Emulators project
    (https://github.com/floooh/chips-test), this is for mono playback,
    so (num_samples == num_frames):

        // tick the sound generator
        if (ay38910_tick(&sys->psg)) {
            // new sample is ready
            sys->sample_buffer[sys->sample_pos++] = sys->psg.sample;
            if (sys->sample_pos == sys->num_samples) {
                // new sample packet is ready
                saudio_push(sys->sample_buffer, sys->num_samples);
                sys->sample_pos = 0;
            }
        }

    THE WEBAUDIO BACKEND
    ====================
    The WebAudio backend is currently using a ScriptProcessorNode callback to
    feed the sample data into WebAudio. ScriptProcessorNode has been
    deprecated for a while because it is running from the main thread, with
    the default initialization parameters it works 'pretty well' though.
    Ultimately Sokol Audio will use Audio Worklets, but this requires a few
    more things to fall into place (Audio Worklets implemented everywhere,
    SharedArrayBuffers enabled again, and I need to figure out a 'low-cost'
    solution in terms of implementation effort, since Audio Worklets are
    a lot more complex than ScriptProcessorNode if the audio data needs to come
    from the main thread).

    The WebAudio backend is automatically selected when compiling for
    emscripten (__EMSCRIPTEN__ define exists).

    https://developers.google.com/web/updates/2017/12/audio-worklet
    https://developers.google.com/web/updates/2018/06/audio-worklet-design-pattern

    "Blob URLs": https://www.html5rocks.com/en/tutorials/workers/basics/

    Also see: https://blog.paul.cx/post/a-wait-free-spsc-ringbuffer-for-the-web/

    THE COREAUDIO BACKEND
    =====================
    The CoreAudio backend is selected on macOS and iOS (__APPLE__ is defined).
    Since the CoreAudio API is implemented in C (not Objective-C) on macOS the
    implementation part of Sokol Audio can be included into a C source file.

    However on iOS, Sokol Audio must be compiled as Objective-C due to it's
    reliance on the AVAudioSession object. The iOS code path support both
    being compiled with or without ARC (Automatic Reference Counting).

    For thread synchronisation, the CoreAudio backend will use the
    pthread_mutex_* functions.

    The incoming floating point samples will be directly forwarded to
    CoreAudio without further conversion.

    macOS and iOS applications that use Sokol Audio need to link with
    the AudioToolbox framework.

    THE WASAPI BACKEND
    ==================
    The WASAPI backend is automatically selected when compiling on Windows
    (_WIN32 is defined).

    For thread synchronisation a Win32 critical section is used.

    WASAPI may use a different size for its own streaming buffer then requested,
    so the base latency may be slightly bigger. The current backend implementation
    converts the incoming floating point sample values to signed 16-bit
    integers.

    The required Windows system DLLs are linked with #pragma comment(lib, ...),
    so you shouldn't need to add additional linker libs in the build process
    (otherwise this is a bug which should be fixed in sokol_audio.h).

    THE ALSA BACKEND
    ================
    The ALSA backend is automatically selected when compiling on Linux
    ('linux' is defined).

    For thread synchronisation, the pthread_mutex_* functions are used.

    Samples are directly forwarded to ALSA in 32-bit float format, no
    further conversion is taking place.

    You need to link with the 'asound' library, and the <alsa/asoundlib.h>
    header must be present (usually both are installed with some sort
    of ALSA development package).


    MEMORY ALLOCATION OVERRIDE
    ==========================
    You can override the memory allocation functions at initialization time
    like this:

        void* my_alloc(size_t size, void* user_data) {
            return malloc(size);
        }

        void my_free(void* ptr, void* user_data) {
            free(ptr);
        }

        ...
            saudio_setup(&(saudio_desc){
                // ...
                .allocator = {
                    .alloc_fn = my_alloc,
                    .free_fn = my_free,
                    .user_data = ...,
                }
            });
        ...

    If no overrides are provided, malloc and free will be used.

    This only affects memory allocation calls done by sokol_audio.h
    itself though, not any allocations in OS libraries.

    Memory allocation will only happen on the same thread where saudio_setup()
    was called, so you don't need to worry about thread-safety.


    ERROR REPORTING AND LOGGING
    ===========================
    To get any logging information at all you need to provide a logging callback in the setup call
    the easiest way is to use sokol_log.h:

        #include "sokol_log.h"

        saudio_setup(&(saudio_desc){ .logger.func = slog_func });

    To override logging with your own callback, first write a logging function like this:

        void my_log(const char* tag,                // e.g. 'saudio'
                    uint32_t log_level,             // 0=panic, 1=error, 2=warn, 3=info
                    uint32_t log_item_id,           // SAUDIO_LOGITEM_*
                    const char* message_or_null,    // a message string, may be nullptr in release mode
                    uint32_t line_nr,               // line number in sokol_audio.h
                    const char* filename_or_null,   // source filename, may be nullptr in release mode
                    void* user_data)
        {
            ...
        }

    ...and then setup sokol-audio like this:

        saudio_setup(&(saudio_desc){
            .logger = {
                .func = my_log,
                .user_data = my_user_data,
            }
        });

    The provided logging function must be reentrant (e.g. be callable from
    different threads).

    If you don't want to provide your own custom logger it is highly recommended to use
    the standard logger in sokol_log.h instead, otherwise you won't see any warnings or
    errors.


    LICENSE
    =======

    zlib/libpng license

    Copyright (c) 2018 Andre Weissflog

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.
*/
#define SOKOL_AUDIO_INCLUDED (1)
#include <stddef.h> // size_t
#include <stdint.h>
#include <stdbool.h>

#if defined(SOKOL_API_DECL) && !defined(SOKOL_AUDIO_API_DECL)
#define SOKOL_AUDIO_API_DECL SOKOL_API_DECL
#endif
#ifndef SOKOL_AUDIO_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_AUDIO_IMPL)
#define SOKOL_AUDIO_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_AUDIO_API_DECL __declspec(dllimport)
#else
#define SOKOL_AUDIO_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
    saudio_log_item

    Log items are defined via X-Macros, and expanded to an
    enum 'saudio_log_item', and in debug mode only,
    corresponding strings.

    Used as parameter in the logging callback.
*/
#define _SAUDIO_LOG_ITEMS \
    _SAUDIO_LOGITEM_XMACRO(OK, "Ok") \
    _SAUDIO_LOGITEM_XMACRO(MALLOC_FAILED, "memory allocation failed") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_SND_PCM_OPEN_FAILED, "snd_pcm_open() failed") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_FLOAT_SAMPLES_NOT_SUPPORTED, "floating point sample format not supported") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_REQUESTED_BUFFER_SIZE_NOT_SUPPORTED, "requested buffer size not supported") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_REQUESTED_CHANNEL_COUNT_NOT_SUPPORTED, "requested channel count not supported") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_SND_PCM_HW_PARAMS_SET_RATE_NEAR_FAILED, "snd_pcm_hw_params_set_rate_near() failed") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_SND_PCM_HW_PARAMS_FAILED, "snd_pcm_hw_params() failed") \
    _SAUDIO_LOGITEM_XMACRO(ALSA_PTHREAD_CREATE_FAILED, "pthread_create() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_CREATE_EVENT_FAILED, "CreateEvent() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_CREATE_DEVICE_ENUMERATOR_FAILED, "CoCreateInstance() for IMMDeviceEnumerator failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_GET_DEFAULT_AUDIO_ENDPOINT_FAILED, "IMMDeviceEnumerator.GetDefaultAudioEndpoint() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_DEVICE_ACTIVATE_FAILED, "IMMDevice.Activate() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_AUDIO_CLIENT_INITIALIZE_FAILED, "IAudioClient.Initialize() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_AUDIO_CLIENT_GET_BUFFER_SIZE_FAILED, "IAudioClient.GetBufferSize() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_AUDIO_CLIENT_GET_SERVICE_FAILED, "IAudioClient.GetService() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_AUDIO_CLIENT_SET_EVENT_HANDLE_FAILED, "IAudioClient.SetEventHandle() failed") \
    _SAUDIO_LOGITEM_XMACRO(WASAPI_CREATE_THREAD_FAILED, "CreateThread() failed") \
    _SAUDIO_LOGITEM_XMACRO(AAUDIO_STREAMBUILDER_OPEN_STREAM_FAILED, "AAudioStreamBuilder_openStream() failed") \
    _SAUDIO_LOGITEM_XMACRO(AAUDIO_PTHREAD_CREATE_FAILED, "pthread_create() failed after AAUDIO_ERROR_DISCONNECTED") \
    _SAUDIO_LOGITEM_XMACRO(AAUDIO_RESTARTING_STREAM_AFTER_ERROR, "restarting AAudio stream after error") \
    _SAUDIO_LOGITEM_XMACRO(USING_AAUDIO_BACKEND, "using AAudio backend") \
    _SAUDIO_LOGITEM_XMACRO(AAUDIO_CREATE_STREAMBUILDER_FAILED, "AAudio_createStreamBuilder() failed") \
    _SAUDIO_LOGITEM_XMACRO(COREAUDIO_NEW_OUTPUT_FAILED, "AudioQueueNewOutput() failed") \
    _SAUDIO_LOGITEM_XMACRO(COREAUDIO_ALLOCATE_BUFFER_FAILED, "AudioQueueAllocateBuffer() failed") \
    _SAUDIO_LOGITEM_XMACRO(COREAUDIO_START_FAILED, "AudioQueueStart() failed") \
    _SAUDIO_LOGITEM_XMACRO(BACKEND_BUFFER_SIZE_ISNT_MULTIPLE_OF_PACKET_SIZE, "backend buffer size isn't multiple of packet size") \

#define _SAUDIO_LOGITEM_XMACRO(item,msg) SAUDIO_LOGITEM_##item,
typedef enum saudio_log_item {
    _SAUDIO_LOG_ITEMS
} saudio_log_item;
#undef _SAUDIO_LOGITEM_XMACRO

/*
    saudio_logger

    Used in saudio_desc to provide a custom logging and error reporting
    callback to sokol-audio.
*/
typedef struct saudio_logger {
    void (*func)(
        const char* tag,                // always "saudio"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SAUDIO_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_audio.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data);
    void* user_data;
} saudio_logger;

/*
    saudio_allocator

    Used in saudio_desc to provide custom memory-alloc and -free functions
    to sokol_audio.h. If memory management should be overridden, both the
    alloc_fn and free_fn function must be provided (e.g. it's not valid to
    override one function but not the other).
*/
typedef struct saudio_allocator {
    void* (*alloc_fn)(size_t size, void* user_data);
    void (*free_fn)(void* ptr, void* user_data);
    void* user_data;
} saudio_allocator;

typedef struct saudio_desc {
    int sample_rate;        // requested sample rate
    int num_channels;       // number of channels, default: 1 (mono)
    int buffer_frames;      // number of frames in streaming buffer
    int packet_frames;      // number of frames in a packet
    int num_packets;        // number of packets in packet queue
    void (*stream_cb)(float* buffer, int num_frames, int num_channels);  // optional streaming callback (no user data)
    void (*stream_userdata_cb)(float* buffer, int num_frames, int num_channels, void* user_data); //... and with user data
    void* user_data;        // optional user data argument for stream_userdata_cb
    saudio_allocator allocator;     // optional allocation override functions
    saudio_logger logger;           // optional logging function (default: NO LOGGING!)
} saudio_desc;

/* setup sokol-audio */
SOKOL_AUDIO_API_DECL void saudio_setup(const saudio_desc* desc);
/* shutdown sokol-audio */
SOKOL_AUDIO_API_DECL void saudio_shutdown(void);
/* true after setup if audio backend was successfully initialized */
SOKOL_AUDIO_API_DECL bool saudio_isvalid(void);
/* return the saudio_desc.user_data pointer */
SOKOL_AUDIO_API_DECL void* saudio_userdata(void);
/* return a copy of the original saudio_desc struct */
SOKOL_AUDIO_API_DECL saudio_desc saudio_query_desc(void);
/* actual sample rate */
SOKOL_AUDIO_API_DECL int saudio_sample_rate(void);
/* return actual backend buffer size in number of frames */
SOKOL_AUDIO_API_DECL int saudio_buffer_frames(void);
/* actual number of channels */
SOKOL_AUDIO_API_DECL int saudio_channels(void);
/* return true if audio context is currently suspended (only in WebAudio backend, all other backends return false) */
SOKOL_AUDIO_API_DECL bool saudio_suspended(void);
/* get current number of frames to fill packet queue */
SOKOL_AUDIO_API_DECL int saudio_expect(void);
/* push sample frames from main thread, returns number of frames actually pushed */
SOKOL_AUDIO_API_DECL int saudio_push(const float* frames, int num_frames);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */
inline void saudio_setup(const saudio_desc& desc) { return saudio_setup(&desc); }

#endif
#endif // SOKOL_AUDIO_INCLUDED

// ██ ███    ███ ██████  ██      ███████ ███    ███ ███████ ███    ██ ████████  █████  ████████ ██  ██████  ███    ██
// ██ ████  ████ ██   ██ ██      ██      ████  ████ ██      ████   ██    ██    ██   ██    ██    ██ ██    ██ ████   ██
// ██ ██ ████ ██ ██████  ██      █████   ██ ████ ██ █████   ██ ██  ██    ██    ███████    ██    ██ ██    ██ ██ ██  ██
// ██ ██  ██  ██ ██      ██      ██      ██  ██  ██ ██      ██  ██ ██    ██    ██   ██    ██    ██ ██    ██ ██  ██ ██
// ██ ██      ██ ██      ███████ ███████ ██      ██ ███████ ██   ████    ██    ██   ██    ██    ██  ██████  ██   ████
//
// >>implementation
#ifdef SOKOL_AUDIO_IMPL
#define SOKOL_AUDIO_IMPL_INCLUDED (1)

#if defined(SOKOL_MALLOC) || defined(SOKOL_CALLOC) || defined(SOKOL_FREE)
#error "SOKOL_MALLOC/CALLOC/FREE macros are no longer supported, please use saudio_desc.allocator to override memory allocation functions"
#endif

#include <stdlib.h> // alloc, free
#include <string.h> // memset, memcpy
#include <stddef.h> // size_t

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif

#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif

#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

// platform detection defines
#if defined(SOKOL_DUMMY_BACKEND)
    // nothing
#elif defined(__APPLE__)
    #define _SAUDIO_APPLE (1)
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #define _SAUDIO_IOS (1)
    #else
        #define _SAUDIO_MACOS (1)
    #endif
#elif defined(__EMSCRIPTEN__)
    #define _SAUDIO_EMSCRIPTEN (1)
#elif defined(_WIN32)
    #define _SAUDIO_WINDOWS (1)
    #include <winapifamily.h>
    #if (defined(WINAPI_FAMILY_PARTITION) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
        #error "sokol_audio.h no longer supports UWP"
    #endif
#elif defined(__ANDROID__)
    #define _SAUDIO_ANDROID (1)
#elif defined(__linux__) || defined(__unix__)
    #define _SAUDIO_LINUX (1)
#else
#error "sokol_audio.h: Unknown platform"
#endif

// platform-specific headers and definitions
#if defined(SOKOL_DUMMY_BACKEND)
    #define _SAUDIO_NOTHREADS (1)
#elif defined(_SAUDIO_WINDOWS)
    #define _SAUDIO_WINTHREADS (1)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
    #include <synchapi.h>
    #pragma comment (lib, "kernel32")
    #pragma comment (lib, "ole32")
    #ifndef CINTERFACE
    #define CINTERFACE
    #endif
    #ifndef COBJMACROS
    #define COBJMACROS
    #endif
    #ifndef CONST_VTABLE
    #define CONST_VTABLE
    #endif
    #include <mmdeviceapi.h>
    #include <audioclient.h>
    static const IID _saudio_IID_IAudioClient                               = { 0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2} };
    static const IID _saudio_IID_IMMDeviceEnumerator                        = { 0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6} };
    static const CLSID _saudio_CLSID_IMMDeviceEnumerator                    = { 0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e} };
    static const IID _saudio_IID_IAudioRenderClient                         = { 0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2} };
    static const IID _saudio_IID_Devinterface_Audio_Render                  = { 0xe6327cad, 0xdcec, 0x4949, {0xae, 0x8a, 0x99, 0x1e, 0x97, 0x6a, 0x79, 0xd2} };
    static const IID _saudio_IID_IActivateAudioInterface_Completion_Handler = { 0x94ea2b94, 0xe9cc, 0x49e0, {0xc0, 0xff, 0xee, 0x64, 0xca, 0x8f, 0x5b, 0x90} };
    static const GUID _saudio_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT               = { 0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };
    #if defined(__cplusplus)
    #define _SOKOL_AUDIO_WIN32COM_ID(x) (x)
    #else
    #define _SOKOL_AUDIO_WIN32COM_ID(x) (&x)
    #endif
    /* fix for Visual Studio 2015 SDKs */
    #ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
    #define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
    #endif
    #ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
    #define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
    #endif
    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable:4505)   /* unreferenced local function has been removed */
    #endif
#elif defined(_SAUDIO_APPLE)
    #define _SAUDIO_PTHREADS (1)
    #include <pthread.h>
    #if defined(_SAUDIO_IOS)
        // always use system headers on iOS (for now at least)
        #if !defined(SAUDIO_OSX_USE_SYSTEM_HEADERS)
            #define SAUDIO_OSX_USE_SYSTEM_HEADERS (1)
        #endif
        #if !defined(__cplusplus)
            #if __has_feature(objc_arc) && !__has_feature(objc_arc_fields)
                #error "sokol_audio.h on iOS requires __has_feature(objc_arc_field) if ARC is enabled (use a more recent compiler version)"
            #endif
        #endif
        #include <AudioToolbox/AudioToolbox.h>
        #include <AVFoundation/AVFoundation.h>
    #else
        #if defined(SAUDIO_OSX_USE_SYSTEM_HEADERS)
            #include <AudioToolbox/AudioToolbox.h>
        #endif
    #endif
#elif defined(_SAUDIO_ANDROID)
    #define _SAUDIO_PTHREADS (1)
    #include <pthread.h>
    #include "aaudio/AAudio.h"
#elif defined(_SAUDIO_LINUX)
    #if !defined(__FreeBSD__)
        #include <alloca.h>
    #endif
    #define _SAUDIO_PTHREADS (1)
    #include <pthread.h>
    #define ALSA_PCM_NEW_HW_PARAMS_API
    #include <alsa/asoundlib.h>
#elif defined(__EMSCRIPTEN__)
    #define _SAUDIO_NOTHREADS (1)
    #include <emscripten/emscripten.h>
#endif

#define _saudio_def(val, def) (((val) == 0) ? (def) : (val))
#define _saudio_def_flt(val, def) (((val) == 0.0f) ? (def) : (val))

#define _SAUDIO_DEFAULT_SAMPLE_RATE (44100)
#define _SAUDIO_DEFAULT_BUFFER_FRAMES (2048)
#define _SAUDIO_DEFAULT_PACKET_FRAMES (128)
#define _SAUDIO_DEFAULT_NUM_PACKETS ((_SAUDIO_DEFAULT_BUFFER_FRAMES/_SAUDIO_DEFAULT_PACKET_FRAMES)*4)

#ifndef SAUDIO_RING_MAX_SLOTS
#define SAUDIO_RING_MAX_SLOTS (1024)
#endif

// ███████ ████████ ██████  ██    ██  ██████ ████████ ███████
// ██         ██    ██   ██ ██    ██ ██         ██    ██
// ███████    ██    ██████  ██    ██ ██         ██    ███████
//      ██    ██    ██   ██ ██    ██ ██         ██         ██
// ███████    ██    ██   ██  ██████   ██████    ██    ███████
//
// >>structs
#if defined(_SAUDIO_PTHREADS)

typedef struct {
    pthread_mutex_t mutex;
} _saudio_mutex_t;

#elif defined(_SAUDIO_WINTHREADS)

typedef struct {
    CRITICAL_SECTION critsec;
} _saudio_mutex_t;

#elif defined(_SAUDIO_NOTHREADS)

typedef struct {
    int dummy_mutex;
} _saudio_mutex_t;

#endif

#if defined(SOKOL_DUMMY_BACKEND)

typedef struct {
    int dummy;
} _saudio_dummy_backend_t;

#elif defined(_SAUDIO_APPLE)

#if defined(SAUDIO_OSX_USE_SYSTEM_HEADERS)

typedef AudioQueueRef _saudio_AudioQueueRef;
typedef AudioQueueBufferRef _saudio_AudioQueueBufferRef;
typedef AudioStreamBasicDescription _saudio_AudioStreamBasicDescription;
typedef OSStatus _saudio_OSStatus;

#define _saudio_kAudioFormatLinearPCM (kAudioFormatLinearPCM)
#define _saudio_kLinearPCMFormatFlagIsFloat (kLinearPCMFormatFlagIsFloat)
#define _saudio_kAudioFormatFlagIsPacked (kAudioFormatFlagIsPacked)

#else
#ifdef __cplusplus
extern "C" {
#endif

// embedded AudioToolbox declarations
typedef uint32_t _saudio_AudioFormatID;
typedef uint32_t _saudio_AudioFormatFlags;
typedef int32_t _saudio_OSStatus;
typedef uint32_t _saudio_SMPTETimeType;
typedef uint32_t _saudio_SMPTETimeFlags;
typedef uint32_t _saudio_AudioTimeStampFlags;
typedef void* _saudio_CFRunLoopRef;
typedef void* _saudio_CFStringRef;
typedef void* _saudio_AudioQueueRef;

#define _saudio_kAudioFormatLinearPCM ('lpcm')
#define _saudio_kLinearPCMFormatFlagIsFloat (1U << 0)
#define _saudio_kAudioFormatFlagIsPacked (1U << 3)

typedef struct _saudio_AudioStreamBasicDescription {
    double mSampleRate;
    _saudio_AudioFormatID mFormatID;
    _saudio_AudioFormatFlags mFormatFlags;
    uint32_t mBytesPerPacket;
    uint32_t mFramesPerPacket;
    uint32_t mBytesPerFrame;
    uint32_t mChannelsPerFrame;
    uint32_t mBitsPerChannel;
    uint32_t mReserved;
} _saudio_AudioStreamBasicDescription;

typedef struct _saudio_AudioStreamPacketDescription {
    int64_t mStartOffset;
    uint32_t mVariableFramesInPacket;
    uint32_t mDataByteSize;
} _saudio_AudioStreamPacketDescription;

typedef struct _saudio_SMPTETime {
    int16_t mSubframes;
    int16_t mSubframeDivisor;
    uint32_t mCounter;
    _saudio_SMPTETimeType mType;
    _saudio_SMPTETimeFlags mFlags;
    int16_t mHours;
    int16_t mMinutes;
    int16_t mSeconds;
    int16_t mFrames;
} _saudio_SMPTETime;

typedef struct _saudio_AudioTimeStamp {
    double mSampleTime;
    uint64_t mHostTime;
    double mRateScalar;
    uint64_t mWordClockTime;
    _saudio_SMPTETime mSMPTETime;
    _saudio_AudioTimeStampFlags mFlags;
    uint32_t mReserved;
} _saudio_AudioTimeStamp;

typedef struct _saudio_AudioQueueBuffer {
    const uint32_t mAudioDataBytesCapacity;
    void* const mAudioData;
    uint32_t mAudioDataByteSize;
    void * mUserData;
    const uint32_t mPacketDescriptionCapacity;
    _saudio_AudioStreamPacketDescription* const mPacketDescriptions;
    uint32_t mPacketDescriptionCount;
} _saudio_AudioQueueBuffer;
typedef _saudio_AudioQueueBuffer* _saudio_AudioQueueBufferRef;

typedef void (*_saudio_AudioQueueOutputCallback)(void* user_data, _saudio_AudioQueueRef inAQ, _saudio_AudioQueueBufferRef inBuffer);

extern _saudio_OSStatus AudioQueueNewOutput(const _saudio_AudioStreamBasicDescription* inFormat, _saudio_AudioQueueOutputCallback inCallbackProc, void* inUserData, _saudio_CFRunLoopRef inCallbackRunLoop, _saudio_CFStringRef inCallbackRunLoopMode, uint32_t inFlags, _saudio_AudioQueueRef* outAQ);
extern _saudio_OSStatus AudioQueueDispose(_saudio_AudioQueueRef inAQ, bool inImmediate);
extern _saudio_OSStatus AudioQueueAllocateBuffer(_saudio_AudioQueueRef inAQ, uint32_t inBufferByteSize, _saudio_AudioQueueBufferRef* outBuffer);
extern _saudio_OSStatus AudioQueueEnqueueBuffer(_saudio_AudioQueueRef inAQ, _saudio_AudioQueueBufferRef inBuffer, uint32_t inNumPacketDescs, const _saudio_AudioStreamPacketDescription* inPacketDescs);
extern _saudio_OSStatus AudioQueueStart(_saudio_AudioQueueRef inAQ, const _saudio_AudioTimeStamp * inStartTime);
extern _saudio_OSStatus AudioQueueStop(_saudio_AudioQueueRef inAQ, bool inImmediate);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SAUDIO_OSX_USE_SYSTEM_HEADERS

typedef struct {
    _saudio_AudioQueueRef ca_audio_queue;
    #if defined(_SAUDIO_IOS)
    id ca_interruption_handler;
    #endif
} _saudio_apple_backend_t;

#elif defined(_SAUDIO_LINUX)

typedef struct {
    snd_pcm_t* device;
    float* buffer;
    int buffer_byte_size;
    int buffer_frames;
    pthread_t thread;
    bool thread_stop;
} _saudio_alsa_backend_t;

#elif defined(_SAUDIO_ANDROID)

typedef struct {
    AAudioStreamBuilder* builder;
    AAudioStream* stream;
    pthread_t thread;
    pthread_mutex_t mutex;
} _saudio_aaudio_backend_t;

#elif defined(_SAUDIO_WINDOWS)

typedef struct {
    HANDLE thread_handle;
    HANDLE buffer_end_event;
    bool stop;
    UINT32 dst_buffer_frames;
    int src_buffer_frames;
    int src_buffer_byte_size;
    int src_buffer_pos;
    float* src_buffer;
} _saudio_wasapi_thread_data_t;

typedef struct {
    IMMDeviceEnumerator* device_enumerator;
    IMMDevice* device;
    IAudioClient* audio_client;
    IAudioRenderClient* render_client;
    _saudio_wasapi_thread_data_t thread;
} _saudio_wasapi_backend_t;

#elif defined(_SAUDIO_EMSCRIPTEN)

typedef struct {
    uint8_t* buffer;
} _saudio_web_backend_t;

#else
#error "unknown platform"
#endif

#if defined(SOKOL_DUMMY_BACKEND)
typedef _saudio_dummy_backend_t _saudio_backend_t;
#elif defined(_SAUDIO_APPLE)
typedef _saudio_apple_backend_t _saudio_backend_t;
#elif defined(_SAUDIO_EMSCRIPTEN)
typedef _saudio_web_backend_t _saudio_backend_t;
#elif defined(_SAUDIO_WINDOWS)
typedef _saudio_wasapi_backend_t _saudio_backend_t;
#elif defined(_SAUDIO_ANDROID)
typedef _saudio_aaudio_backend_t _saudio_backend_t;
#elif defined(_SAUDIO_LINUX)
typedef _saudio_alsa_backend_t _saudio_backend_t;
#endif

/* a ringbuffer structure */
typedef struct {
    int head;  // next slot to write to
    int tail;  // next slot to read from
    int num;   // number of slots in queue
    int queue[SAUDIO_RING_MAX_SLOTS];
} _saudio_ring_t;

/* a packet FIFO structure */
typedef struct {
    bool valid;
    int packet_size;            /* size of a single packets in bytes(!) */
    int num_packets;            /* number of packet in fifo */
    uint8_t* base_ptr;          /* packet memory chunk base pointer (dynamically allocated) */
    int cur_packet;             /* current write-packet */
    int cur_offset;             /* current byte-offset into current write packet */
    _saudio_mutex_t mutex;      /* mutex for thread-safe access */
    _saudio_ring_t read_queue;  /* buffers with data, ready to be streamed */
    _saudio_ring_t write_queue; /* empty buffers, ready to be pushed to */
} _saudio_fifo_t;

/* sokol-audio state */
typedef struct {
    bool valid;
    bool setup_called;
    void (*stream_cb)(float* buffer, int num_frames, int num_channels);
    void (*stream_userdata_cb)(float* buffer, int num_frames, int num_channels, void* user_data);
    void* user_data;
    int sample_rate;            /* sample rate */
    int buffer_frames;          /* number of frames in streaming buffer */
    int bytes_per_frame;        /* filled by backend */
    int packet_frames;          /* number of frames in a packet */
    int num_packets;            /* number of packets in packet queue */
    int num_channels;           /* actual number of channels */
    saudio_desc desc;
    _saudio_fifo_t fifo;
    _saudio_backend_t backend;
} _saudio_state_t;

_SOKOL_PRIVATE _saudio_state_t _saudio;

_SOKOL_PRIVATE bool _saudio_has_callback(void) {
    return (_saudio.stream_cb || _saudio.stream_userdata_cb);
}

_SOKOL_PRIVATE void _saudio_stream_callback(float* buffer, int num_frames, int num_channels) {
    if (_saudio.stream_cb) {
        _saudio.stream_cb(buffer, num_frames, num_channels);
    }
    else if (_saudio.stream_userdata_cb) {
        _saudio.stream_userdata_cb(buffer, num_frames, num_channels, _saudio.user_data);
    }
}

// ██       ██████   ██████   ██████  ██ ███    ██  ██████
// ██      ██    ██ ██       ██       ██ ████   ██ ██
// ██      ██    ██ ██   ███ ██   ███ ██ ██ ██  ██ ██   ███
// ██      ██    ██ ██    ██ ██    ██ ██ ██  ██ ██ ██    ██
// ███████  ██████   ██████   ██████  ██ ██   ████  ██████
//
// >>logging
#if defined(SOKOL_DEBUG)
#define _SAUDIO_LOGITEM_XMACRO(item,msg) #item ": " msg,
static const char* _saudio_log_messages[] = {
    _SAUDIO_LOG_ITEMS
};
#undef _SAUDIO_LOGITEM_XMACRO
#endif // SOKOL_DEBUG

#define _SAUDIO_PANIC(code) _saudio_log(SAUDIO_LOGITEM_ ##code, 0, __LINE__)
#define _SAUDIO_ERROR(code) _saudio_log(SAUDIO_LOGITEM_ ##code, 1, __LINE__)
#define _SAUDIO_WARN(code) _saudio_log(SAUDIO_LOGITEM_ ##code, 2, __LINE__)
#define _SAUDIO_INFO(code) _saudio_log(SAUDIO_LOGITEM_ ##code, 3, __LINE__)

static void _saudio_log(saudio_log_item log_item, uint32_t log_level, uint32_t line_nr) {
    if (_saudio.desc.logger.func) {
        #if defined(SOKOL_DEBUG)
            const char* filename = __FILE__;
            const char* message = _saudio_log_messages[log_item];
        #else
            const char* filename = 0;
            const char* message = 0;
        #endif
        _saudio.desc.logger.func("saudio", log_level, (uint32_t)log_item, message, line_nr, filename, _saudio.desc.logger.user_data);
    }
    else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// ███    ███ ███████ ███    ███  ██████  ██████  ██    ██
// ████  ████ ██      ████  ████ ██    ██ ██   ██  ██  ██
// ██ ████ ██ █████   ██ ████ ██ ██    ██ ██████    ████
// ██  ██  ██ ██      ██  ██  ██ ██    ██ ██   ██    ██
// ██      ██ ███████ ██      ██  ██████  ██   ██    ██
//
// >>memory
_SOKOL_PRIVATE void _saudio_clear(void* ptr, size_t size) {
    SOKOL_ASSERT(ptr && (size > 0));
    memset(ptr, 0, size);
}

_SOKOL_PRIVATE void* _saudio_malloc(size_t size) {
    SOKOL_ASSERT(size > 0);
    void* ptr;
    if (_saudio.desc.allocator.alloc_fn) {
        ptr = _saudio.desc.allocator.alloc_fn(size, _saudio.desc.allocator.user_data);
    } else {
        ptr = malloc(size);
    }
    if (0 == ptr) {
        _SAUDIO_PANIC(MALLOC_FAILED);
    }
    return ptr;
}

_SOKOL_PRIVATE void* _saudio_malloc_clear(size_t size) {
    void* ptr = _saudio_malloc(size);
    _saudio_clear(ptr, size);
    return ptr;
}

_SOKOL_PRIVATE void _saudio_free(void* ptr) {
    if (_saudio.desc.allocator.free_fn) {
        _saudio.desc.allocator.free_fn(ptr, _saudio.desc.allocator.user_data);
    } else {
        free(ptr);
    }
}

// ███    ███ ██    ██ ████████ ███████ ██   ██
// ████  ████ ██    ██    ██    ██       ██ ██
// ██ ████ ██ ██    ██    ██    █████     ███
// ██  ██  ██ ██    ██    ██    ██       ██ ██
// ██      ██  ██████     ██    ███████ ██   ██
//
// >>mutex
#if defined(_SAUDIO_NOTHREADS)

_SOKOL_PRIVATE void _saudio_mutex_init(_saudio_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _saudio_mutex_destroy(_saudio_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _saudio_mutex_lock(_saudio_mutex_t* m) { (void)m; }
_SOKOL_PRIVATE void _saudio_mutex_unlock(_saudio_mutex_t* m) { (void)m; }

#elif defined(_SAUDIO_PTHREADS)

_SOKOL_PRIVATE void _saudio_mutex_init(_saudio_mutex_t* m) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&m->mutex, &attr);
}

_SOKOL_PRIVATE void _saudio_mutex_destroy(_saudio_mutex_t* m) {
    pthread_mutex_destroy(&m->mutex);
}

_SOKOL_PRIVATE void _saudio_mutex_lock(_saudio_mutex_t* m) {
    pthread_mutex_lock(&m->mutex);
}

_SOKOL_PRIVATE void _saudio_mutex_unlock(_saudio_mutex_t* m) {
    pthread_mutex_unlock(&m->mutex);
}

#elif defined(_SAUDIO_WINTHREADS)

_SOKOL_PRIVATE void _saudio_mutex_init(_saudio_mutex_t* m) {
    InitializeCriticalSection(&m->critsec);
}

_SOKOL_PRIVATE void _saudio_mutex_destroy(_saudio_mutex_t* m) {
    DeleteCriticalSection(&m->critsec);
}

_SOKOL_PRIVATE void _saudio_mutex_lock(_saudio_mutex_t* m) {
    EnterCriticalSection(&m->critsec);
}

_SOKOL_PRIVATE void _saudio_mutex_unlock(_saudio_mutex_t* m) {
    LeaveCriticalSection(&m->critsec);
}
#else
#error "sokol_audio.h: unknown platform!"
#endif

// ██████  ██ ███    ██  ██████  ██████  ██    ██ ███████ ███████ ███████ ██████
// ██   ██ ██ ████   ██ ██       ██   ██ ██    ██ ██      ██      ██      ██   ██
// ██████  ██ ██ ██  ██ ██   ███ ██████  ██    ██ █████   █████   █████   ██████
// ██   ██ ██ ██  ██ ██ ██    ██ ██   ██ ██    ██ ██      ██      ██      ██   ██
// ██   ██ ██ ██   ████  ██████  ██████   ██████  ██      ██      ███████ ██   ██
//
// >>ringbuffer
_SOKOL_PRIVATE int _saudio_ring_idx(_saudio_ring_t* ring, int i) {
    return (i % ring->num);
}

_SOKOL_PRIVATE void _saudio_ring_init(_saudio_ring_t* ring, int num_slots) {
    SOKOL_ASSERT((num_slots + 1) <= SAUDIO_RING_MAX_SLOTS);
    ring->head = 0;
    ring->tail = 0;
    /* one slot reserved to detect 'full' vs 'empty' */
    ring->num = num_slots + 1;
}

_SOKOL_PRIVATE bool _saudio_ring_full(_saudio_ring_t* ring) {
    return _saudio_ring_idx(ring, ring->head + 1) == ring->tail;
}

_SOKOL_PRIVATE bool _saudio_ring_empty(_saudio_ring_t* ring) {
    return ring->head == ring->tail;
}

_SOKOL_PRIVATE int _saudio_ring_count(_saudio_ring_t* ring) {
    int count;
    if (ring->head >= ring->tail) {
        count = ring->head - ring->tail;
    }
    else {
        count = (ring->head + ring->num) - ring->tail;
    }
    SOKOL_ASSERT(count < ring->num);
    return count;
}

_SOKOL_PRIVATE void _saudio_ring_enqueue(_saudio_ring_t* ring, int val) {
    SOKOL_ASSERT(!_saudio_ring_full(ring));
    ring->queue[ring->head] = val;
    ring->head = _saudio_ring_idx(ring, ring->head + 1);
}

_SOKOL_PRIVATE int _saudio_ring_dequeue(_saudio_ring_t* ring) {
    SOKOL_ASSERT(!_saudio_ring_empty(ring));
    int val = ring->queue[ring->tail];
    ring->tail = _saudio_ring_idx(ring, ring->tail + 1);
    return val;
}

// ███████ ██ ███████  ██████
// ██      ██ ██      ██    ██
// █████   ██ █████   ██    ██
// ██      ██ ██      ██    ██
// ██      ██ ██       ██████
//
// >>fifo
_SOKOL_PRIVATE void _saudio_fifo_init_mutex(_saudio_fifo_t* fifo) {
    /* this must be called before initializing both the backend and the fifo itself! */
    _saudio_mutex_init(&fifo->mutex);
}

_SOKOL_PRIVATE void _saudio_fifo_destroy_mutex(_saudio_fifo_t* fifo) {
    _saudio_mutex_destroy(&fifo->mutex);
}

_SOKOL_PRIVATE void _saudio_fifo_init(_saudio_fifo_t* fifo, int packet_size, int num_packets) {
    /* NOTE: there's a chicken-egg situation during the init phase where the
        streaming thread must be started before the fifo is actually initialized,
        thus the fifo init must already be protected from access by the fifo_read() func.
    */
    _saudio_mutex_lock(&fifo->mutex);
    SOKOL_ASSERT((packet_size > 0) && (num_packets > 0));
    fifo->packet_size = packet_size;
    fifo->num_packets = num_packets;
    fifo->base_ptr = (uint8_t*) _saudio_malloc((size_t)(packet_size * num_packets));
    fifo->cur_packet = -1;
    fifo->cur_offset = 0;
    _saudio_ring_init(&fifo->read_queue, num_packets);
    _saudio_ring_init(&fifo->write_queue, num_packets);
    for (int i = 0; i < num_packets; i++) {
        _saudio_ring_enqueue(&fifo->write_queue, i);
    }
    SOKOL_ASSERT(_saudio_ring_full(&fifo->write_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->write_queue) == num_packets);
    SOKOL_ASSERT(_saudio_ring_empty(&fifo->read_queue));
    SOKOL_ASSERT(_saudio_ring_count(&fifo->read_queue) == 0);
    fifo->valid = true;
    _saudio_mutex_unlock(&fifo->mutex);
}

_SOKOL_PRIVATE void _saudio_fifo_shutdown(_saudio_fifo_t* fifo) {
    SOKOL_ASSERT(fifo->base_ptr);
    _saudio_free(fifo->base_ptr);
    fifo->base_ptr = 0;
    fifo->valid = false;
}

_SOKOL_PRIVATE int _saudio_fifo_writable_bytes(_saudio_fifo_t* fifo) {
    _saudio_mutex_lock(&fifo->mutex);
    int num_bytes = (_saudio_ring_count(&fifo->write_queue) * fifo->packet_size);
    if (fifo->cur_packet != -1) {
        num_bytes += fifo->packet_size - fifo->cur_offset;
    }
    _saudio_mutex_unlock(&fifo->mutex);
    SOKOL_ASSERT((num_bytes >= 0) && (num_bytes <= (fifo->num_packets * fifo->packet_size)));
    return num_bytes;
}

/* write new data to the write queue, this is called from main thread */
_SOKOL_PRIVATE int _saudio_fifo_write(_saudio_fifo_t* fifo, const uint8_t* ptr, int num_bytes) {
    /* returns the number of bytes written, this will be smaller then requested
        if the write queue runs full
    */
    int all_to_copy = num_bytes;
    while (all_to_copy > 0) {
        /* need to grab a new packet? */
        if (fifo->cur_packet == -1) {
            _saudio_mutex_lock(&fifo->mutex);
            if (!_saudio_ring_empty(&fifo->write_queue)) {
                fifo->cur_packet = _saudio_ring_dequeue(&fifo->write_queue);
            }
            _saudio_mutex_unlock(&fifo->mutex);
            SOKOL_ASSERT(fifo->cur_offset == 0);
        }
        /* append data to current write packet */
        if (fifo->cur_packet != -1) {
            int to_copy = all_to_copy;
            const int max_copy = fifo->packet_size - fifo->cur_offset;
            if (to_copy > max_copy) {
                to_copy = max_copy;
            }
            uint8_t* dst = fifo->base_ptr + fifo->cur_packet * fifo->packet_size + fifo->cur_offset;
            memcpy(dst, ptr, (size_t)to_copy);
            ptr += to_copy;
            fifo->cur_offset += to_copy;
            all_to_copy -= to_copy;
            SOKOL_ASSERT(fifo->cur_offset <= fifo->packet_size);
            SOKOL_ASSERT(all_to_copy >= 0);
        }
        else {
            /* early out if we're starving */
            int bytes_copied = num_bytes - all_to_copy;
            SOKOL_ASSERT((bytes_copied >= 0) && (bytes_copied < num_bytes));
            return bytes_copied;
        }
        /* if write packet is full, push to read queue */
        if (fifo->cur_offset == fifo->packet_size) {
            _saudio_mutex_lock(&fifo->mutex);
            _saudio_ring_enqueue(&fifo->read_queue, fifo->cur_packet);
            _saudio_mutex_unlock(&fifo->mutex);
            fifo->cur_packet = -1;
            fifo->cur_offset = 0;
        }
    }
    SOKOL_ASSERT(all_to_copy == 0);
    return num_bytes;
}

/* read queued data, this is called form the stream callback (maybe separate thread) */
_SOKOL_PRIVATE int _saudio_fifo_read(_saudio_fifo_t* fifo, uint8_t* ptr, int num_bytes) {
    /* NOTE: fifo_read might be called before the fifo is properly initialized */
    _saudio_mutex_lock(&fifo->mutex);
    int num_bytes_copied = 0;
    if (fifo->valid) {
        SOKOL_ASSERT(0 == (num_bytes % fifo->packet_size));
        SOKOL_ASSERT(num_bytes <= (fifo->packet_size * fifo->num_packets));
        const int num_packets_needed = num_bytes / fifo->packet_size;
        uint8_t* dst = ptr;
        /* either pull a full buffer worth of data, or nothing */
        if (_saudio_ring_count(&fifo->read_queue) >= num_packets_needed) {
            for (int i = 0; i < num_packets_needed; i++) {
                int packet_index = _saudio_ring_dequeue(&fifo->read_queue);
                _saudio_ring_enqueue(&fifo->write_queue, packet_index);
                const uint8_t* src = fifo->base_ptr + packet_index * fifo->packet_size;
                memcpy(dst, src, (size_t)fifo->packet_size);
                dst += fifo->packet_size;
                num_bytes_copied += fifo->packet_size;
            }
            SOKOL_ASSERT(num_bytes == num_bytes_copied);
        }
    }
    _saudio_mutex_unlock(&fifo->mutex);
    return num_bytes_copied;
}

// ██████  ██    ██ ███    ███ ███    ███ ██    ██
// ██   ██ ██    ██ ████  ████ ████  ████  ██  ██
// ██   ██ ██    ██ ██ ████ ██ ██ ████ ██   ████
// ██   ██ ██    ██ ██  ██  ██ ██  ██  ██    ██
// ██████   ██████  ██      ██ ██      ██    ██
//
// >>dummy
#if defined(SOKOL_DUMMY_BACKEND)
_SOKOL_PRIVATE bool _saudio_dummy_backend_init(void) {
    _saudio.bytes_per_frame = _saudio.num_channels * (int)sizeof(float);
    return true;
}
_SOKOL_PRIVATE void _saudio_dummy_backend_shutdown(void) { }

//  █████  ██      ███████  █████
// ██   ██ ██      ██      ██   ██
// ███████ ██      ███████ ███████
// ██   ██ ██           ██ ██   ██
// ██   ██ ███████ ███████ ██   ██
//
// >>alsa
#elif defined(_SAUDIO_LINUX)

/* the streaming callback runs in a separate thread */
_SOKOL_PRIVATE void* _saudio_alsa_cb(void* param) {
    _SOKOL_UNUSED(param);
    while (!_saudio.backend.thread_stop) {
        /* snd_pcm_writei() will be blocking until it needs data */
        int write_res = snd_pcm_writei(_saudio.backend.device, _saudio.backend.buffer, (snd_pcm_uframes_t)_saudio.backend.buffer_frames);
        if (write_res < 0) {
            /* underrun occurred */
            snd_pcm_prepare(_saudio.backend.device);
        }
        else {
            /* fill the streaming buffer with new data */
            if (_saudio_has_callback()) {
                _saudio_stream_callback(_saudio.backend.buffer, _saudio.backend.buffer_frames, _saudio.num_channels);
            }
            else {
                if (0 == _saudio_fifo_read(&_saudio.fifo, (uint8_t*)_saudio.backend.buffer, _saudio.backend.buffer_byte_size)) {
                    /* not enough read data available, fill the entire buffer with silence */
                    _saudio_clear(_saudio.backend.buffer, (size_t)_saudio.backend.buffer_byte_size);
                }
            }
        }
    }
    return 0;
}

_SOKOL_PRIVATE bool _saudio_alsa_backend_init(void) {
    int dir; uint32_t rate;
    int rc = snd_pcm_open(&_saudio.backend.device, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        _SAUDIO_ERROR(ALSA_SND_PCM_OPEN_FAILED);
        return false;
    }

    /* configuration works by restricting the 'configuration space' step
       by step, we require all parameters except the sample rate to
       match perfectly
    */
    snd_pcm_hw_params_t* params = 0;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(_saudio.backend.device, params);
    snd_pcm_hw_params_set_access(_saudio.backend.device, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (0 > snd_pcm_hw_params_set_format(_saudio.backend.device, params, SND_PCM_FORMAT_FLOAT_LE)) {
        _SAUDIO_ERROR(ALSA_FLOAT_SAMPLES_NOT_SUPPORTED);
        goto error;
    }
    if (0 > snd_pcm_hw_params_set_buffer_size(_saudio.backend.device, params, (snd_pcm_uframes_t)_saudio.buffer_frames)) {
        _SAUDIO_ERROR(ALSA_REQUESTED_BUFFER_SIZE_NOT_SUPPORTED);
        goto error;
    }
    if (0 > snd_pcm_hw_params_set_channels(_saudio.backend.device, params, (uint32_t)_saudio.num_channels)) {
        _SAUDIO_ERROR(ALSA_REQUESTED_CHANNEL_COUNT_NOT_SUPPORTED);
        goto error;
    }
    /* let ALSA pick a nearby sampling rate */
    rate = (uint32_t) _saudio.sample_rate;
    dir = 0;
    if (0 > snd_pcm_hw_params_set_rate_near(_saudio.backend.device, params, &rate, &dir)) {
        _SAUDIO_ERROR(ALSA_SND_PCM_HW_PARAMS_SET_RATE_NEAR_FAILED);
        goto error;
    }
    if (0 > snd_pcm_hw_params(_saudio.backend.device, params)) {
        _SAUDIO_ERROR(ALSA_SND_PCM_HW_PARAMS_FAILED);
        goto error;
    }

    /* read back actual sample rate and channels */
    _saudio.sample_rate = (int)rate;
    _saudio.bytes_per_frame = _saudio.num_channels * (int)sizeof(float);

    /* allocate the streaming buffer */
    _saudio.backend.buffer_byte_size = _saudio.buffer_frames * _saudio.bytes_per_frame;
    _saudio.backend.buffer_frames = _saudio.buffer_frames;
    _saudio.backend.buffer = (float*) _saudio_malloc_clear((size_t)_saudio.backend.buffer_byte_size);

    /* create the buffer-streaming start thread */
    if (0 != pthread_create(&_saudio.backend.thread, 0, _saudio_alsa_cb, 0)) {
        _SAUDIO_ERROR(ALSA_PTHREAD_CREATE_FAILED);
        goto error;
    }

    return true;
error:
    if (_saudio.backend.device) {
        snd_pcm_close(_saudio.backend.device);
        _saudio.backend.device = 0;
    }
    return false;
}

_SOKOL_PRIVATE void _saudio_alsa_backend_shutdown(void) {
    SOKOL_ASSERT(_saudio.backend.device);
    _saudio.backend.thread_stop = true;
    pthread_join(_saudio.backend.thread, 0);
    snd_pcm_drain(_saudio.backend.device);
    snd_pcm_close(_saudio.backend.device);
    _saudio_free(_saudio.backend.buffer);
}

// ██     ██  █████  ███████  █████  ██████  ██
// ██     ██ ██   ██ ██      ██   ██ ██   ██ ██
// ██  █  ██ ███████ ███████ ███████ ██████  ██
// ██ ███ ██ ██   ██      ██ ██   ██ ██      ██
//  ███ ███  ██   ██ ███████ ██   ██ ██      ██
//
// >>wasapi
#elif defined(_SAUDIO_WINDOWS)

/* fill intermediate buffer with new data and reset buffer_pos */
_SOKOL_PRIVATE void _saudio_wasapi_fill_buffer(void) {
    if (_saudio_has_callback()) {
        _saudio_stream_callback(_saudio.backend.thread.src_buffer, _saudio.backend.thread.src_buffer_frames, _saudio.num_channels);
    }
    else {
        if (0 == _saudio_fifo_read(&_saudio.fifo, (uint8_t*)_saudio.backend.thread.src_buffer, _saudio.backend.thread.src_buffer_byte_size)) {
            /* not enough read data available, fill the entire buffer with silence */
            _saudio_clear(_saudio.backend.thread.src_buffer, (size_t)_saudio.backend.thread.src_buffer_byte_size);
        }
    }
}

_SOKOL_PRIVATE int _saudio_wasapi_min(int a, int b) {
    return (a < b) ? a : b;
}

_SOKOL_PRIVATE void _saudio_wasapi_submit_buffer(int num_frames) {
    BYTE* wasapi_buffer = 0;
    if (FAILED(IAudioRenderClient_GetBuffer(_saudio.backend.render_client, num_frames, &wasapi_buffer))) {
        return;
    }
    SOKOL_ASSERT(wasapi_buffer);

    /* copy samples to WASAPI buffer, refill source buffer if needed */
    int num_remaining_samples = num_frames * _saudio.num_channels;
    int buffer_pos = _saudio.backend.thread.src_buffer_pos;
    const int buffer_size_in_samples = _saudio.backend.thread.src_buffer_byte_size / (int)sizeof(float);
    float* dst = (float*)wasapi_buffer;
    const float* dst_end = dst + num_remaining_samples;
    _SOKOL_UNUSED(dst_end); // suppress unused warning in release mode
    const float* src = _saudio.backend.thread.src_buffer;

    while (num_remaining_samples > 0) {
        if (0 == buffer_pos) {
            _saudio_wasapi_fill_buffer();
        }
        const int samples_to_copy = _saudio_wasapi_min(num_remaining_samples, buffer_size_in_samples - buffer_pos);
        SOKOL_ASSERT((buffer_pos + samples_to_copy) <= buffer_size_in_samples);
        SOKOL_ASSERT((dst + samples_to_copy) <= dst_end);
        memcpy(dst, &src[buffer_pos], (size_t)samples_to_copy * sizeof(float));
        num_remaining_samples -= samples_to_copy;
        SOKOL_ASSERT(num_remaining_samples >= 0);
        buffer_pos += samples_to_copy;
        dst += samples_to_copy;

        SOKOL_ASSERT(buffer_pos <= buffer_size_in_samples);
        if (buffer_pos == buffer_size_in_samples) {
            buffer_pos = 0;
        }
    }
    _saudio.backend.thread.src_buffer_pos = buffer_pos;
    IAudioRenderClient_ReleaseBuffer(_saudio.backend.render_client, num_frames, 0);
}

_SOKOL_PRIVATE DWORD WINAPI _saudio_wasapi_thread_fn(LPVOID param) {
    (void)param;
    _saudio_wasapi_submit_buffer(_saudio.backend.thread.src_buffer_frames);
    IAudioClient_Start(_saudio.backend.audio_client);
    while (!_saudio.backend.thread.stop) {
        WaitForSingleObject(_saudio.backend.thread.buffer_end_event, INFINITE);
        UINT32 padding = 0;
        if (FAILED(IAudioClient_GetCurrentPadding(_saudio.backend.audio_client, &padding))) {
            continue;
        }
        SOKOL_ASSERT(_saudio.backend.thread.dst_buffer_frames >= padding);
        int num_frames = (int)_saudio.backend.thread.dst_buffer_frames - (int)padding;
        if (num_frames > 0) {
            _saudio_wasapi_submit_buffer(num_frames);
        }
    }
    return 0;
}

_SOKOL_PRIVATE void _saudio_wasapi_release(void) {
    if (_saudio.backend.thread.src_buffer) {
        _saudio_free(_saudio.backend.thread.src_buffer);
        _saudio.backend.thread.src_buffer = 0;
    }
    if (_saudio.backend.render_client) {
        IAudioRenderClient_Release(_saudio.backend.render_client);
        _saudio.backend.render_client = 0;
    }
    if (_saudio.backend.audio_client) {
        IAudioClient_Release(_saudio.backend.audio_client);
        _saudio.backend.audio_client = 0;
    }
    if (_saudio.backend.device) {
        IMMDevice_Release(_saudio.backend.device);
        _saudio.backend.device = 0;
    }
    if (_saudio.backend.device_enumerator) {
        IMMDeviceEnumerator_Release(_saudio.backend.device_enumerator);
        _saudio.backend.device_enumerator = 0;
    }
    if (0 != _saudio.backend.thread.buffer_end_event) {
        CloseHandle(_saudio.backend.thread.buffer_end_event);
        _saudio.backend.thread.buffer_end_event = 0;
    }
}

_SOKOL_PRIVATE bool _saudio_wasapi_backend_init(void) {
    REFERENCE_TIME dur;
    /* CoInitializeEx could have been called elsewhere already, in which
        case the function returns with S_FALSE (thus it does not make much
        sense to check the result)
    */
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    _SOKOL_UNUSED(hr);
    _saudio.backend.thread.buffer_end_event = CreateEvent(0, FALSE, FALSE, 0);
    if (0 == _saudio.backend.thread.buffer_end_event) {
        _SAUDIO_ERROR(WASAPI_CREATE_EVENT_FAILED);
        goto error;
    }
    if (FAILED(CoCreateInstance(_SOKOL_AUDIO_WIN32COM_ID(_saudio_CLSID_IMMDeviceEnumerator),
        0, CLSCTX_ALL,
        _SOKOL_AUDIO_WIN32COM_ID(_saudio_IID_IMMDeviceEnumerator),
        (void**)&_saudio.backend.device_enumerator)))
    {
        _SAUDIO_ERROR(WASAPI_CREATE_DEVICE_ENUMERATOR_FAILED);
        goto error;
    }
    if (FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(_saudio.backend.device_enumerator,
        eRender, eConsole,
        &_saudio.backend.device)))
    {
        _SAUDIO_ERROR(WASAPI_GET_DEFAULT_AUDIO_ENDPOINT_FAILED);
        goto error;
    }
    if (FAILED(IMMDevice_Activate(_saudio.backend.device,
        _SOKOL_AUDIO_WIN32COM_ID(_saudio_IID_IAudioClient),
        CLSCTX_ALL, 0,
        (void**)&_saudio.backend.audio_client)))
    {
        _SAUDIO_ERROR(WASAPI_DEVICE_ACTIVATE_FAILED);
        goto error;
    }

    WAVEFORMATEXTENSIBLE fmtex;
    _saudio_clear(&fmtex, sizeof(fmtex));
    fmtex.Format.nChannels = (WORD)_saudio.num_channels;
    fmtex.Format.nSamplesPerSec = (DWORD)_saudio.sample_rate;
    fmtex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    fmtex.Format.wBitsPerSample = 32;
    fmtex.Format.nBlockAlign = (fmtex.Format.nChannels * fmtex.Format.wBitsPerSample) / 8;
    fmtex.Format.nAvgBytesPerSec = fmtex.Format.nSamplesPerSec * fmtex.Format.nBlockAlign;
    fmtex.Format.cbSize = 22;   /* WORD + DWORD + GUID */
    fmtex.Samples.wValidBitsPerSample = 32;
    if (_saudio.num_channels == 1) {
        fmtex.dwChannelMask = SPEAKER_FRONT_CENTER;
    }
    else {
        fmtex.dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT;
    }
    fmtex.SubFormat = _saudio_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    dur = (REFERENCE_TIME)
        (((double)_saudio.buffer_frames) / (((double)_saudio.sample_rate) * (1.0/10000000.0)));
    if (FAILED(IAudioClient_Initialize(_saudio.backend.audio_client,
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        dur, 0, (WAVEFORMATEX*)&fmtex, 0)))
    {
        _SAUDIO_ERROR(WASAPI_AUDIO_CLIENT_INITIALIZE_FAILED);
        goto error;
    }
    if (FAILED(IAudioClient_GetBufferSize(_saudio.backend.audio_client, &_saudio.backend.thread.dst_buffer_frames))) {
        _SAUDIO_ERROR(WASAPI_AUDIO_CLIENT_GET_BUFFER_SIZE_FAILED);
        goto error;
    }
    if (FAILED(IAudioClient_GetService(_saudio.backend.audio_client,
        _SOKOL_AUDIO_WIN32COM_ID(_saudio_IID_IAudioRenderClient),
        (void**)&_saudio.backend.render_client)))
    {
        _SAUDIO_ERROR(WASAPI_AUDIO_CLIENT_GET_SERVICE_FAILED);
        goto error;
    }
    if (FAILED(IAudioClient_SetEventHandle(_saudio.backend.audio_client, _saudio.backend.thread.buffer_end_event))) {
        _SAUDIO_ERROR(WASAPI_AUDIO_CLIENT_SET_EVENT_HANDLE_FAILED);
        goto error;
    }
    _saudio.bytes_per_frame = _saudio.num_channels * (int)sizeof(float);
    _saudio.backend.thread.src_buffer_frames = _saudio.buffer_frames;
    _saudio.backend.thread.src_buffer_byte_size = _saudio.backend.thread.src_buffer_frames * _saudio.bytes_per_frame;

    /* allocate an intermediate buffer for sample format conversion */
    _saudio.backend.thread.src_buffer = (float*) _saudio_malloc((size_t)_saudio.backend.thread.src_buffer_byte_size);

    /* create streaming thread */
    _saudio.backend.thread.thread_handle = CreateThread(NULL, 0, _saudio_wasapi_thread_fn, 0, 0, 0);
    if (0 == _saudio.backend.thread.thread_handle) {
        _SAUDIO_ERROR(WASAPI_CREATE_THREAD_FAILED);
        goto error;
    }
    return true;
error:
    _saudio_wasapi_release();
    return false;
}

_SOKOL_PRIVATE void _saudio_wasapi_backend_shutdown(void) {
    if (_saudio.backend.thread.thread_handle) {
        _saudio.backend.thread.stop = true;
        SetEvent(_saudio.backend.thread.buffer_end_event);
        WaitForSingleObject(_saudio.backend.thread.thread_handle, INFINITE);
        CloseHandle(_saudio.backend.thread.thread_handle);
        _saudio.backend.thread.thread_handle = 0;
    }
    if (_saudio.backend.audio_client) {
        IAudioClient_Stop(_saudio.backend.audio_client);
    }
    _saudio_wasapi_release();
    CoUninitialize();
}

// ██     ██ ███████ ██████   █████  ██    ██ ██████  ██  ██████
// ██     ██ ██      ██   ██ ██   ██ ██    ██ ██   ██ ██ ██    ██
// ██  █  ██ █████   ██████  ███████ ██    ██ ██   ██ ██ ██    ██
// ██ ███ ██ ██      ██   ██ ██   ██ ██    ██ ██   ██ ██ ██    ██
//  ███ ███  ███████ ██████  ██   ██  ██████  ██████  ██  ██████
//
// >>webaudio
#elif defined(_SAUDIO_EMSCRIPTEN)

#ifdef __cplusplus
extern "C" {
#endif

EMSCRIPTEN_KEEPALIVE int _saudio_emsc_pull(int num_frames) {
    SOKOL_ASSERT(_saudio.backend.buffer);
    if (num_frames == _saudio.buffer_frames) {
        if (_saudio_has_callback()) {
            _saudio_stream_callback((float*)_saudio.backend.buffer, num_frames, _saudio.num_channels);
        }
        else {
            const int num_bytes = num_frames * _saudio.bytes_per_frame;
            if (0 == _saudio_fifo_read(&_saudio.fifo, _saudio.backend.buffer, num_bytes)) {
                /* not enough read data available, fill the entire buffer with silence */
                _saudio_clear(_saudio.backend.buffer, (size_t)num_bytes);
            }
        }
        int res = (int) _saudio.backend.buffer;
        return res;
    }
    else {
        return 0;
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/* setup the WebAudio context and attach a ScriptProcessorNode */
EM_JS(int, saudio_js_init, (int sample_rate, int num_channels, int buffer_size), {
    Module._saudio_context = null;
    Module._saudio_node = null;
    if (typeof AudioContext !== 'undefined') {
        Module._saudio_context = new AudioContext({
            sampleRate: sample_rate,
            latencyHint: 'interactive',
        });
    }
    else {
        Module._saudio_context = null;
        console.log('sokol_audio.h: no WebAudio support');
    }
    if (Module._saudio_context) {
        console.log('sokol_audio.h: sample rate ', Module._saudio_context.sampleRate);
        Module._saudio_node = Module._saudio_context.createScriptProcessor(buffer_size, 0, num_channels);
        Module._saudio_node.onaudioprocess = (event) => {
            const num_frames = event.outputBuffer.length;
            const ptr = __saudio_emsc_pull(num_frames);
            if (ptr) {
                const num_channels = event.outputBuffer.numberOfChannels;
                for (let chn = 0; chn < num_channels; chn++) {
                    const chan = event.outputBuffer.getChannelData(chn);
                    for (let i = 0; i < num_frames; i++) {
                        chan[i] = HEAPF32[(ptr>>2) + ((num_channels*i)+chn)]
                    }
                }
            }
        };
        Module._saudio_node.connect(Module._saudio_context.destination);

        // in some browsers, WebAudio needs to be activated on a user action
        const resume_webaudio = () => {
            if (Module._saudio_context) {
                if (Module._saudio_context.state === 'suspended') {
                    Module._saudio_context.resume();
                }
            }
        };
        document.addEventListener('click', resume_webaudio, {once:true});
        document.addEventListener('touchend', resume_webaudio, {once:true});
        document.addEventListener('keydown', resume_webaudio, {once:true});
        return 1;
    }
    else {
        return 0;
    }
})

/* shutdown the WebAudioContext and ScriptProcessorNode */
EM_JS(void, saudio_js_shutdown, (void), {
    \x2F\x2A\x2A @suppress {missingProperties} \x2A\x2F
    const ctx = Module._saudio_context;
    if (ctx !== null) {
        if (Module._saudio_node) {
            Module._saudio_node.disconnect();
        }
        ctx.close();
        Module._saudio_context = null;
        Module._saudio_node = null;
    }
})

/* get the actual sample rate back from the WebAudio context */
EM_JS(int, saudio_js_sample_rate, (void), {
    if (Module._saudio_context) {
        return Module._saudio_context.sampleRate;
    }
    else {
        return 0;
    }
})

/* get the actual buffer size in number of frames */
EM_JS(int, saudio_js_buffer_frames, (void), {
    if (Module._saudio_node) {
        return Module._saudio_node.bufferSize;
    }
    else {
        return 0;
    }
})

/* return 1 if the WebAudio context is currently suspended, else 0 */
EM_JS(int, saudio_js_suspended, (void), {
    if (Module._saudio_context) {
        if (Module._saudio_context.state === 'suspended') {
            return 1;
        }
        else {
            return 0;
        }
    }
})

_SOKOL_PRIVATE bool _saudio_webaudio_backend_init(void) {
    if (saudio_js_init(_saudio.sample_rate, _saudio.num_channels, _saudio.buffer_frames)) {
        _saudio.bytes_per_frame = (int)sizeof(float) * _saudio.num_channels;
        _saudio.sample_rate = saudio_js_sample_rate();
        _saudio.buffer_frames = saudio_js_buffer_frames();
        const size_t buf_size = (size_t) (_saudio.buffer_frames * _saudio.bytes_per_frame);
        _saudio.backend.buffer = (uint8_t*) _saudio_malloc(buf_size);
        return true;
    }
    else {
        return false;
    }
}

_SOKOL_PRIVATE void _saudio_webaudio_backend_shutdown(void) {
    saudio_js_shutdown();
    if (_saudio.backend.buffer) {
        _saudio_free(_saudio.backend.buffer);
        _saudio.backend.buffer = 0;
    }
}

//  █████   █████  ██    ██ ██████  ██  ██████
// ██   ██ ██   ██ ██    ██ ██   ██ ██ ██    ██
// ███████ ███████ ██    ██ ██   ██ ██ ██    ██
// ██   ██ ██   ██ ██    ██ ██   ██ ██ ██    ██
// ██   ██ ██   ██  ██████  ██████  ██  ██████
//
// >>aaudio
#elif defined(_SAUDIO_ANDROID)

_SOKOL_PRIVATE aaudio_data_callback_result_t _saudio_aaudio_data_callback(AAudioStream* stream, void* user_data, void* audio_data, int32_t num_frames) {
    _SOKOL_UNUSED(user_data);
    _SOKOL_UNUSED(stream);
    if (_saudio_has_callback()) {
        _saudio_stream_callback((float*)audio_data, (int)num_frames, _saudio.num_channels);
    }
    else {
        uint8_t* ptr = (uint8_t*)audio_data;
        int num_bytes = _saudio.bytes_per_frame * num_frames;
        if (0 == _saudio_fifo_read(&_saudio.fifo, ptr, num_bytes)) {
            // not enough read data available, fill the entire buffer with silence
            memset(ptr, 0, (size_t)num_bytes);
        }
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

_SOKOL_PRIVATE bool _saudio_aaudio_start_stream(void) {
    if (AAudioStreamBuilder_openStream(_saudio.backend.builder, &_saudio.backend.stream) != AAUDIO_OK) {
        _SAUDIO_ERROR(AAUDIO_STREAMBUILDER_OPEN_STREAM_FAILED);
        return false;
    }
    AAudioStream_requestStart(_saudio.backend.stream);
    return true;
}

_SOKOL_PRIVATE void _saudio_aaudio_stop_stream(void) {
    if (_saudio.backend.stream) {
        AAudioStream_requestStop(_saudio.backend.stream);
        AAudioStream_close(_saudio.backend.stream);
        _saudio.backend.stream = 0;
    }
}

_SOKOL_PRIVATE void* _saudio_aaudio_restart_stream_thread_fn(void* param) {
    _SOKOL_UNUSED(param);
    _SAUDIO_WARN(AAUDIO_RESTARTING_STREAM_AFTER_ERROR);
    pthread_mutex_lock(&_saudio.backend.mutex);
    _saudio_aaudio_stop_stream();
    _saudio_aaudio_start_stream();
    pthread_mutex_unlock(&_saudio.backend.mutex);
    return 0;
}

_SOKOL_PRIVATE void _saudio_aaudio_error_callback(AAudioStream* stream, void* user_data, aaudio_result_t error) {
    _SOKOL_UNUSED(stream);
    _SOKOL_UNUSED(user_data);
    if (error == AAUDIO_ERROR_DISCONNECTED) {
        if (0 != pthread_create(&_saudio.backend.thread, 0, _saudio_aaudio_restart_stream_thread_fn, 0)) {
            _SAUDIO_ERROR(AAUDIO_PTHREAD_CREATE_FAILED);
        }
    }
}

_SOKOL_PRIVATE void _saudio_aaudio_backend_shutdown(void) {
    pthread_mutex_lock(&_saudio.backend.mutex);
    _saudio_aaudio_stop_stream();
    pthread_mutex_unlock(&_saudio.backend.mutex);
    if (_saudio.backend.builder) {
        AAudioStreamBuilder_delete(_saudio.backend.builder);
        _saudio.backend.builder = 0;
    }
    pthread_mutex_destroy(&_saudio.backend.mutex);
}

_SOKOL_PRIVATE bool _saudio_aaudio_backend_init(void) {
    _SAUDIO_INFO(USING_AAUDIO_BACKEND);

    _saudio.bytes_per_frame = _saudio.num_channels * (int)sizeof(float);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&_saudio.backend.mutex, &attr);

    if (AAudio_createStreamBuilder(&_saudio.backend.builder) != AAUDIO_OK) {
        _SAUDIO_ERROR(AAUDIO_CREATE_STREAMBUILDER_FAILED);
        _saudio_aaudio_backend_shutdown();
        return false;
    }

    AAudioStreamBuilder_setFormat(_saudio.backend.builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setSampleRate(_saudio.backend.builder, _saudio.sample_rate);
    AAudioStreamBuilder_setChannelCount(_saudio.backend.builder, _saudio.num_channels);
    AAudioStreamBuilder_setBufferCapacityInFrames(_saudio.backend.builder, _saudio.buffer_frames * 2);
    AAudioStreamBuilder_setFramesPerDataCallback(_saudio.backend.builder, _saudio.buffer_frames);
    AAudioStreamBuilder_setDataCallback(_saudio.backend.builder, _saudio_aaudio_data_callback, 0);
    AAudioStreamBuilder_setErrorCallback(_saudio.backend.builder, _saudio_aaudio_error_callback, 0);

    if (!_saudio_aaudio_start_stream()) {
        _saudio_aaudio_backend_shutdown();
        return false;
    }

    return true;
}

//  ██████  ██████  ██████  ███████  █████  ██    ██ ██████  ██  ██████
// ██      ██    ██ ██   ██ ██      ██   ██ ██    ██ ██   ██ ██ ██    ██
// ██      ██    ██ ██████  █████   ███████ ██    ██ ██   ██ ██ ██    ██
// ██      ██    ██ ██   ██ ██      ██   ██ ██    ██ ██   ██ ██ ██    ██
//  ██████  ██████  ██   ██ ███████ ██   ██  ██████  ██████  ██  ██████
//
// >>coreaudio
#elif defined(_SAUDIO_APPLE)

#if defined(_SAUDIO_IOS)
#if __has_feature(objc_arc)
#define _SAUDIO_OBJC_RELEASE(obj) { obj = nil; }
#else
#define _SAUDIO_OBJC_RELEASE(obj) { [obj release]; obj = nil; }
#endif

@interface _saudio_interruption_handler : NSObject { }
@end

@implementation _saudio_interruption_handler
-(id)init {
    self = [super init];
    AVAudioSession* session = [AVAudioSession sharedInstance];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handle_interruption:) name:AVAudioSessionInterruptionNotification object:session];
    return self;
}

-(void)dealloc {
    [self remove_handler];
    #if !__has_feature(objc_arc)
    [super dealloc];
    #endif
}

-(void)remove_handler {
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"AVAudioSessionInterruptionNotification" object:nil];
}

-(void)handle_interruption:(NSNotification*)notification {
    AVAudioSession* session = [AVAudioSession sharedInstance];
    SOKOL_ASSERT(session);
    NSDictionary* dict = notification.userInfo;
    SOKOL_ASSERT(dict);
    NSInteger type = [[dict valueForKey:AVAudioSessionInterruptionTypeKey] integerValue];
    switch (type) {
        case AVAudioSessionInterruptionTypeBegan:
            if (_saudio.backend.ca_audio_queue) {
                AudioQueuePause(_saudio.backend.ca_audio_queue);
            }
            [session setActive:false error:nil];
            break;
        case AVAudioSessionInterruptionTypeEnded:
            [session setActive:true error:nil];
            if (_saudio.backend.ca_audio_queue) {
                AudioQueueStart(_saudio.backend.ca_audio_queue, NULL);
            }
            break;
        default:
            break;
    }
}
@end
#endif // _SAUDIO_IOS

/* NOTE: the buffer data callback is called on a separate thread! */
_SOKOL_PRIVATE void _saudio_coreaudio_callback(void* user_data, _saudio_AudioQueueRef queue, _saudio_AudioQueueBufferRef buffer) {
    _SOKOL_UNUSED(user_data);
    if (_saudio_has_callback()) {
        const int num_frames = (int)buffer->mAudioDataByteSize / _saudio.bytes_per_frame;
        const int num_channels = _saudio.num_channels;
        _saudio_stream_callback((float*)buffer->mAudioData, num_frames, num_channels);
    }
    else {
        uint8_t* ptr = (uint8_t*)buffer->mAudioData;
        int num_bytes = (int) buffer->mAudioDataByteSize;
        if (0 == _saudio_fifo_read(&_saudio.fifo, ptr, num_bytes)) {
            /* not enough read data available, fill the entire buffer with silence */
            _saudio_clear(ptr, (size_t)num_bytes);
        }
    }
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

_SOKOL_PRIVATE void _saudio_coreaudio_backend_shutdown(void) {
    if (_saudio.backend.ca_audio_queue) {
        AudioQueueStop(_saudio.backend.ca_audio_queue, true);
        AudioQueueDispose(_saudio.backend.ca_audio_queue, false);
        _saudio.backend.ca_audio_queue = 0;
    }
    #if defined(_SAUDIO_IOS)
        /* remove interruption handler */
        if (_saudio.backend.ca_interruption_handler != nil) {
            [_saudio.backend.ca_interruption_handler remove_handler];
            _SAUDIO_OBJC_RELEASE(_saudio.backend.ca_interruption_handler);
        }
        /* deactivate audio session */
        AVAudioSession* session = [AVAudioSession sharedInstance];
        SOKOL_ASSERT(session);
        [session setActive:false error:nil];;
    #endif // _SAUDIO_IOS
}

_SOKOL_PRIVATE bool _saudio_coreaudio_backend_init(void) {
    SOKOL_ASSERT(0 == _saudio.backend.ca_audio_queue);

    #if defined(_SAUDIO_IOS)
        /* activate audio session */
        AVAudioSession* session = [AVAudioSession sharedInstance];
        SOKOL_ASSERT(session != nil);
        [session setCategory: AVAudioSessionCategoryPlayback error:nil];
        [session setActive:true error:nil];

        /* create interruption handler */
        _saudio.backend.ca_interruption_handler = [[_saudio_interruption_handler alloc] init];
    #endif

    /* create an audio queue with fp32 samples */
    _saudio_AudioStreamBasicDescription fmt;
    _saudio_clear(&fmt, sizeof(fmt));
    fmt.mSampleRate = (double) _saudio.sample_rate;
    fmt.mFormatID = _saudio_kAudioFormatLinearPCM;
    fmt.mFormatFlags = _saudio_kLinearPCMFormatFlagIsFloat | _saudio_kAudioFormatFlagIsPacked;
    fmt.mFramesPerPacket = 1;
    fmt.mChannelsPerFrame = (uint32_t) _saudio.num_channels;
    fmt.mBytesPerFrame = (uint32_t)sizeof(float) * (uint32_t)_saudio.num_channels;
    fmt.mBytesPerPacket = fmt.mBytesPerFrame;
    fmt.mBitsPerChannel = 32;
    _saudio_OSStatus res = AudioQueueNewOutput(&fmt, _saudio_coreaudio_callback, 0, NULL, NULL, 0, &_saudio.backend.ca_audio_queue);
    if (0 != res) {
        _SAUDIO_ERROR(COREAUDIO_NEW_OUTPUT_FAILED);
        return false;
    }
    SOKOL_ASSERT(_saudio.backend.ca_audio_queue);

    /* create 2 audio buffers */
    for (int i = 0; i < 2; i++) {
        _saudio_AudioQueueBufferRef buf = NULL;
        const uint32_t buf_byte_size = (uint32_t)_saudio.buffer_frames * fmt.mBytesPerFrame;
        res = AudioQueueAllocateBuffer(_saudio.backend.ca_audio_queue, buf_byte_size, &buf);
        if (0 != res) {
            _SAUDIO_ERROR(COREAUDIO_ALLOCATE_BUFFER_FAILED);
            _saudio_coreaudio_backend_shutdown();
            return false;
        }
        buf->mAudioDataByteSize = buf_byte_size;
        _saudio_clear(buf->mAudioData, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(_saudio.backend.ca_audio_queue, buf, 0, NULL);
    }

    /* init or modify actual playback parameters */
    _saudio.bytes_per_frame = (int)fmt.mBytesPerFrame;

    /* ...and start playback */
    res = AudioQueueStart(_saudio.backend.ca_audio_queue, NULL);
    if (0 != res) {
        _SAUDIO_ERROR(COREAUDIO_START_FAILED);
        _saudio_coreaudio_backend_shutdown();
        return false;
    }
    return true;
}

#else
#error "unsupported platform"
#endif

bool _saudio_backend_init(void) {
    #if defined(SOKOL_DUMMY_BACKEND)
        return _saudio_dummy_backend_init();
    #elif defined(_SAUDIO_LINUX)
        return _saudio_alsa_backend_init();
    #elif defined(_SAUDIO_WINDOWS)
        return _saudio_wasapi_backend_init();
    #elif defined(_SAUDIO_EMSCRIPTEN)
        return _saudio_webaudio_backend_init();
    #elif defined(_SAUDIO_ANDROID)
        return _saudio_aaudio_backend_init();
    #elif defined(_SAUDIO_APPLE)
        return _saudio_coreaudio_backend_init();
    #else
    #error "unknown platform"
    #endif
}

void _saudio_backend_shutdown(void) {
    #if defined(SOKOL_DUMMY_BACKEND)
        _saudio_dummy_backend_shutdown();
    #elif defined(_SAUDIO_LINUX)
        _saudio_alsa_backend_shutdown();
    #elif defined(_SAUDIO_WINDOWS)
        _saudio_wasapi_backend_shutdown();
    #elif defined(_SAUDIO_EMSCRIPTEN)
        _saudio_webaudio_backend_shutdown();
    #elif defined(_SAUDIO_ANDROID)
        _saudio_aaudio_backend_shutdown();
    #elif defined(_SAUDIO_APPLE)
        _saudio_coreaudio_backend_shutdown();
    #else
    #error "unknown platform"
    #endif
}

// ██████  ██    ██ ██████  ██      ██  ██████
// ██   ██ ██    ██ ██   ██ ██      ██ ██
// ██████  ██    ██ ██████  ██      ██ ██
// ██      ██    ██ ██   ██ ██      ██ ██
// ██       ██████  ██████  ███████ ██  ██████
//
// >>public
SOKOL_API_IMPL void saudio_setup(const saudio_desc* desc) {
    SOKOL_ASSERT(!_saudio.valid);
    SOKOL_ASSERT(!_saudio.setup_called);
    SOKOL_ASSERT(desc);
    SOKOL_ASSERT((desc->allocator.alloc_fn && desc->allocator.free_fn) || (!desc->allocator.alloc_fn && !desc->allocator.free_fn));
    _saudio_clear(&_saudio, sizeof(_saudio));
    _saudio.setup_called = true;
    _saudio.desc = *desc;
    _saudio.stream_cb = desc->stream_cb;
    _saudio.stream_userdata_cb = desc->stream_userdata_cb;
    _saudio.user_data = desc->user_data;
    _saudio.sample_rate = _saudio_def(_saudio.desc.sample_rate, _SAUDIO_DEFAULT_SAMPLE_RATE);
    _saudio.buffer_frames = _saudio_def(_saudio.desc.buffer_frames, _SAUDIO_DEFAULT_BUFFER_FRAMES);
    _saudio.packet_frames = _saudio_def(_saudio.desc.packet_frames, _SAUDIO_DEFAULT_PACKET_FRAMES);
    _saudio.num_packets = _saudio_def(_saudio.desc.num_packets, _SAUDIO_DEFAULT_NUM_PACKETS);
    _saudio.num_channels = _saudio_def(_saudio.desc.num_channels, 1);
    _saudio_fifo_init_mutex(&_saudio.fifo);
    if (_saudio_backend_init()) {
        /* the backend might not support the requested exact buffer size,
           make sure the actual buffer size is still a multiple of
           the requested packet size
        */
        if (0 != (_saudio.buffer_frames % _saudio.packet_frames)) {
            _SAUDIO_ERROR(BACKEND_BUFFER_SIZE_ISNT_MULTIPLE_OF_PACKET_SIZE);
            _saudio_backend_shutdown();
            return;
        }
        SOKOL_ASSERT(_saudio.bytes_per_frame > 0);
        _saudio_fifo_init(&_saudio.fifo, _saudio.packet_frames * _saudio.bytes_per_frame, _saudio.num_packets);
        _saudio.valid = true;
    }
    else {
        _saudio_fifo_destroy_mutex(&_saudio.fifo);
    }
}

SOKOL_API_IMPL void saudio_shutdown(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    _saudio.setup_called = false;
    if (_saudio.valid) {
        _saudio_backend_shutdown();
        _saudio_fifo_shutdown(&_saudio.fifo);
        _saudio_fifo_destroy_mutex(&_saudio.fifo);
        _saudio.valid = false;
    }
}

SOKOL_API_IMPL bool saudio_isvalid(void) {
    return _saudio.valid;
}

SOKOL_API_IMPL void* saudio_userdata(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    return _saudio.desc.user_data;
}

SOKOL_API_IMPL saudio_desc saudio_query_desc(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    return _saudio.desc;
}

SOKOL_API_IMPL int saudio_sample_rate(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    return _saudio.sample_rate;
}

SOKOL_API_IMPL int saudio_buffer_frames(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    return _saudio.buffer_frames;
}

SOKOL_API_IMPL int saudio_channels(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    return _saudio.num_channels;
}

SOKOL_API_IMPL bool saudio_suspended(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    #if defined(_SAUDIO_EMSCRIPTEN)
        if (_saudio.valid) {
            return 1 == saudio_js_suspended();
        }
        else {
            return false;
        }
    #else
        return false;
    #endif
}

SOKOL_API_IMPL int saudio_expect(void) {
    SOKOL_ASSERT(_saudio.setup_called);
    if (_saudio.valid) {
        const int num_frames = _saudio_fifo_writable_bytes(&_saudio.fifo) / _saudio.bytes_per_frame;
        return num_frames;
    }
    else {
        return 0;
    }
}

SOKOL_API_IMPL int saudio_push(const float* frames, int num_frames) {
    SOKOL_ASSERT(_saudio.setup_called);
    SOKOL_ASSERT(frames && (num_frames > 0));
    if (_saudio.valid) {
        const int num_bytes = num_frames * _saudio.bytes_per_frame;
        const int num_written = _saudio_fifo_write(&_saudio.fifo, (const uint8_t*)frames, num_bytes);
        return num_written / _saudio.bytes_per_frame;
    }
    else {
        return 0;
    }
}

#undef _saudio_def
#undef _saudio_def_flt

#if defined(_SAUDIO_WINDOWS)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

#endif /* SOKOL_AUDIO_IMPL */
//FILE_END

//FILE_START:thread.h
/*
minithread - minimal cross platform threading

Copyright (c) Arne Koenig 2025
Redistribution and use in source and binary forms, with or without modification, are permitted.
THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED WARRANTY. IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES ARISING FROM THE USE OF THIS SOFTWARE.
*/

#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef _WIN32
typedef HANDLE mt_thread;
typedef CRITICAL_SECTION mt_mutex;
typedef volatile LONG mt_atomic_int32;
#else
typedef pthread_t mt_thread;
typedef pthread_mutex_t mt_mutex;
typedef volatile int32_t mt_atomic_int32;
#endif

typedef void* (*mt_thread_func)(void*);


static inline int mt_thread_create(mt_thread* thread, mt_thread_func func, void* arg) {
#ifdef _WIN32
    DWORD thread_id;
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, &thread_id);
    return *thread != NULL ? 0 : -1;
#else
    return pthread_create(thread, NULL, func, arg);
#endif
}

static inline int mt_thread_join(mt_thread thread) {
#ifdef _WIN32
    DWORD result = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return result == WAIT_OBJECT_0 ? 0 : -1;
#else
    return pthread_join(thread, NULL);
#endif
}

static inline void mt_thread_sleep_ms(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}


static inline int mt_mutex_init(mt_mutex* mutex) {
#ifdef _WIN32
    InitializeCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_init(mutex, NULL);
#endif
}

static inline void mt_mutex_destroy(mt_mutex* mutex) {
#ifdef _WIN32
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(mutex);
#endif
}

static inline void mt_mutex_lock(mt_mutex* mutex) {
#ifdef _WIN32
    EnterCriticalSection(mutex);
#else
    pthread_mutex_lock(mutex);
#endif
}

static inline void mt_mutex_unlock(mt_mutex* mutex) {
#ifdef _WIN32
    LeaveCriticalSection(mutex);
#else
    pthread_mutex_unlock(mutex);
#endif
}

static inline void mt_atomic_init(mt_atomic_int32* a, int32_t value) {
    *a = value;
}

static inline int32_t mt_atomic_load(const mt_atomic_int32* a) {
#ifdef _WIN32
    return InterlockedCompareExchange((volatile LONG*)a, 0, 0);
#elif defined(__clang__) || defined(__GNUC__)
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#else
    return __sync_fetch_and_add((volatile int32_t*)a, 0);
#endif
}

static inline void mt_atomic_store(mt_atomic_int32* a, int32_t value) {
#ifdef _WIN32
    InterlockedExchange((volatile LONG*)a, value);
#elif defined(__clang__) || defined(__GNUC__)
    __atomic_store_n(a, value, __ATOMIC_SEQ_CST);
#else
    do {
        int32_t old = __sync_fetch_and_add(a, 0);
    } while (!__sync_bool_compare_and_swap(a, old, value));
#endif
}

static inline int32_t mt_atomic_increment(mt_atomic_int32* a) {
#ifdef _WIN32
    return InterlockedIncrement((volatile LONG*)a);
#else
    return __sync_add_and_fetch(a, 1);
#endif
}

static inline int32_t mt_atomic_decrement(mt_atomic_int32* a) {
#ifdef _WIN32
    return InterlockedDecrement((volatile LONG*)a);
#else
    return __sync_sub_and_fetch(a, 1);
#endif
}

static inline int32_t mt_atomic_add(mt_atomic_int32* a, int32_t x) {
#ifdef _WIN32
    return InterlockedAdd((volatile LONG*)a, x);
#else
    return __sync_add_and_fetch(a, x);
#endif
}

#ifdef __cplusplus
}
#endif
//FILE_END
//FILE_START:tmixer.h
/*
tmixer - tiny audio mixer

This is a C-language rewrite of the original C++ library tinymixer
(https://github.com/mendsley/tinymixer), originally authored by
Mathew Endsley, licensed under the BSD 2-Clause License.

All credit goes to the original author.

-------------------------------------------------------------------------------

Original copyright:

Copyright (c) 2011-2013, Mathew Endsley
All rights reserved.

C rewrite and modifications:

Copyright (c) 2025, Arne König
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TINY_MIXER_H
#define TINY_MIXER_H

#include <stdint.h>

typedef struct tm_buffer tm_buffer;

typedef struct tm_channel {
	int index;
} tm_channel;

typedef struct tm_callbacks {
	void* udata;
	void* (*allocate)(void* opaque, int bytes);
	void  (*free)(void* opaque, void* pointer);
	void (*pre_effects)(void* opaque, float* samples, int nsamples, float gain);
	void (*channel_complete)(void* opaque, void* channel_opaque, tm_channel channel);
} tm_callbacks;

typedef struct tm_buffer_callbacks {
	void (*on_destroy)(void* buffer_opqaue);
	void* (*start_source)(void* buffer_opaque);
	void (*end_source)(void* buffer_opaque, void* source_opaque);
	int (*request_samples)(void* buffer_opaque, void* source_opaque, const float** left, const float** right, int nsamples);
} tm_buffer_callbacks;

typedef struct tm_resampler {
	float ideal_rate;
	float prev_samples[2];
} tm_resampler;

typedef struct tm_lowpass_filter {
	float cutoff_frequency; // 1250.0 is a decent starting point
	float sample_rate;
	float channel_history[2];
} tm_lowpass_filter;

void tm_init(tm_callbacks callbacks, int sample_rate);
void tm_shutdown(void);
void tm_getsamples(float* samples, int nsamples);
void tm_set_mastergain(float gain);

void tm_create_buffer_interleaved_s16le(int channels, const int16_t* pcm_data, int pcm_data_size, const tm_buffer** handle);
void tm_create_buffer_interleaved_float(int channels, const float* pcm_data, int pcm_data_size, const tm_buffer** handle);
void tm_create_buffer_vorbis_stream(const void* data, int ndata, void* opaque, void (*closed)(void*), const tm_buffer** handle);
void tm_create_buffer_custom_stream(void* opaque, tm_buffer_callbacks callbacks, const tm_buffer** buffer);
int tm_get_buffer_size(const tm_buffer* handle);
void tm_release_buffer(const tm_buffer* handle);

void tm_update_listener(const float* position);
void tm_set_base_gain(int index, float gain);
void tm_set_callback_gain(float gain);
void tm_effects_compressor(const float thresholds[2], const float multipliers[2], float attack_seconds, float release_seconds);

bool tm_add(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel);
bool tm_add_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel);
bool tm_add_spatial(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel);
bool tm_add_spatial_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel);

void tm_channel_set_opaque(tm_channel channel, void* opaque);
void tm_channel_stop(tm_channel channel);
void tm_channel_set_position(tm_channel channel, const float* position);
void tm_channel_fadeout(tm_channel channel, float seconds);
void tm_channel_set_gain(tm_channel channel, float gain);
void tm_channel_set_frequency(tm_channel channel, float frequency);

float tm_channel_get_gain(tm_channel channel);
void tm_stop_all_sources();

static inline bool tm_channel_isvalid(tm_channel channel) { return channel.index != 0; }
static inline bool tm_channel_equals(tm_channel lhs, tm_channel rhs) { return lhs.index == rhs.index; }

void tm_resampler_init(tm_resampler* resampler, int input_sample_rate, int output_sample_rate);
void tm_resampler_init_rate(tm_resampler* resampler, float ideal_rate);
int tm_resampler_calculate_input_samples(const tm_resampler* resampler, int output_samples);
int tm_resampler_calculate_output_samples(const tm_resampler* resampler, int input_samples);
void tm_resample_stereo(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples);
void tm_resample_mono(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples);

void tm_lowpass_filter_init(tm_lowpass_filter* filter, float cutoff_frequency, float sample_rate);
void tm_lowpass_filter_apply(tm_lowpass_filter* filter, float* output, float* input, int num_samples, int num_channels);

#endif //TINY_MIXER_H

#ifdef TMIXER_IMPL

//--IMPL--------------------------------------------------------------------------------------------------

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PK_AUDIO_SINGLE_HEADER
#include "vorbis.c"
#include "thread.h"
#endif

enum {
    TM_SOURCEFLAG_PLAYING = 1 << 0,
    TM_SOURCEFLAG_POSITIONAL = 1 << 1,
    TM_SOURCEFLAG_LOOPING = 1 << 2,
    TM_SOURCEFLAG_FADEOUT = 1 << 3,
    TM_SOURCEFLAG_FREQUENCY = 1 << 4,
};

typedef struct buffer_t buffer_t;
typedef struct source_t source_t;

typedef struct buffer_functions_t {
	void (*on_destroy)(buffer_t* buffer);
	void (*start_source)(source_t* source);
	void (*end_source)(source_t* source);
	int (*request_samples)(source_t* source, const float** left, const float** right, int nsamples);
	int (*get_buffer_size)(const buffer_t* buffer);
} buffer_functions_t;

typedef struct buffer_t {
	const buffer_functions_t* funcs;
	mt_atomic_int32 refcnt;
} buffer_t;

typedef struct static_source_data_t {
	int32_t sample_pos;
} static_source_data_t;

typedef struct vorbis_stream_data_t {
	stb_vorbis* v;
	const float* outputs[2];
	int noutputs;
} vorbis_stream_data_t;

typedef struct custom_stream_data_t {
	void* opaque;
} custom_stream_data_t;

typedef struct source_t {
	const buffer_t* buffer;
	union {
		static_source_data_t static_source;
		vorbis_stream_data_t vorbis_stream;
		custom_stream_data_t custom_stream;
	} instance_data;
	float position[3];
	float fadeout_per_sample;
	float gain_base;
	float distance_min;
	float distance_difference;
	float frequency;
	uint16_t frame_age;
	uint8_t flags;
	uint8_t gain_base_index;
	tm_resampler resampler;
	void* opaque;
} source_t;

#define N_GAINTYPES 8
#define N_SOURCES 32
#define N_SAMPLES 2048
#define N_SAMPLESF (float)N_SAMPLES
#define SPEAKER_DIST 0.17677669529663688110021109052621f // 1/(4 *sqrtf(2))

static struct {
	mt_mutex lock;
	tm_callbacks callbacks;
	float position[3];
	float gain_master;
	float gain_base[N_GAINTYPES];
	float gain_callback;
	int32_t sample_rate;
	float compressor_last_samples[2];
	float compressor_thresholds[2];
	float compressor_multipliers[2];
	float compressor_factor;
	float compressor_attack_per1ksamples;
	float compressor_release_per1ksamples;
	int32_t samples_remaining;
	source_t sources[N_SOURCES];
	float buffer[2*N_SAMPLES];
	float scratch[2*N_SAMPLES];
} tm;


static inline float _tm_clamp(float v, float min, float max) { return min > v ? min : (v > max ? max : v); }
static inline int _tm_min(int a, int b) { return (a < b) ? a : b; }
static inline float _tm_dist(const float* a, const float* b) {
	const float distsq = (a[0] - b[0])*(a[0] - b[0]) + (a[1] - b[1])*(a[1] - b[1]) + (a[2] - b[2])*(a[2] - b[2]);
	return sqrtf(distsq);
}
static inline void _tm_vcopy(float* v, const float* a) { v[0] = a[0], v[1] = a[1], v[2] = a[2]; }

static void _tm_addref(buffer_t* buffer) {
	++buffer->refcnt;
}

static void _tm_decref(buffer_t* buffer) {
	if (1 == mt_atomic_decrement(&buffer->refcnt)) {
		buffer->funcs->on_destroy(buffer);
		tm.callbacks.free(tm.callbacks.udata, buffer);
	}
}

static void kill_source(source_t* source) {

	if (tm.callbacks.channel_complete) {
		tm_channel channel;
		channel.index = (int)(source - tm.sources) + 1;

		// unlock the mutex while in the user callback
        mt_mutex_unlock(&tm.lock);
		tm.callbacks.channel_complete(tm.callbacks.udata, source->opaque, channel);
        mt_mutex_lock(&tm.lock);
	}

	if (source->buffer) {
		source->buffer->funcs->end_source(source);
		_tm_decref((buffer_t*)source->buffer);
	}

	source->buffer = 0;
	source->flags = 0;
}

static source_t* find_source(void) {
	source_t* best_source = NULL;
	uint16_t best_age = 0;

	for (int ii = 0; ii < N_SOURCES; ++ii) {
		source_t* source = &tm.sources[ii];
		if (!source->buffer)
			return source;

		if (0 == (source->flags & TM_SOURCEFLAG_LOOPING)) {
			const uint16_t age = source->frame_age;
			if (age >= best_age) {
				best_source = source;
				best_age = age;
			}
		}
	}

	if (NULL != best_source) {
		if (best_source->buffer) {
			kill_source(best_source);
		}
	}
	return best_source;
}

static void render(source_t* source, float* buffer, const float gain[2]) {

	float* left = buffer;
	float* right = buffer + N_SAMPLES;

	int remaining = N_SAMPLES;
	while (remaining > 0) {
		int samples_read = remaining;
		int samples_written = samples_read;

		const float* srcleft;
		const float* srcright;

		// source has a non-1.0f frequency shift
		if (source->flags & TM_SOURCEFLAG_FREQUENCY) {
			samples_read = tm_resampler_calculate_input_samples(&source->resampler, samples_read);
			samples_read = source->buffer->funcs->request_samples(source, &srcleft, &srcright, samples_read);
			if (samples_read == 0) {
				// source is no longer playing
				source->flags &= ~TM_SOURCEFLAG_PLAYING;
				return;
			}

			samples_written = tm_resampler_calculate_output_samples(&source->resampler, samples_read);
			tm_resample_mono(&source->resampler, srcleft, samples_read, tm.scratch, samples_written);

			if (srcleft == srcright) {
				srcleft = tm.scratch;
				srcright = tm.scratch;
			} else {
				// resmample the second streo channel
				tm_resample_mono(&source->resampler, srcright, samples_read, tm.scratch + samples_written, samples_written);

				srcleft = tm.scratch;
				srcright = tm.scratch + samples_written;
			}
		} else {
			samples_read = source->buffer->funcs->request_samples(source, &srcleft, &srcright, samples_read);
			if (samples_read == 0) {
				// source is no longer playing
				source->flags &= ~TM_SOURCEFLAG_PLAYING;
				return;
			}
			samples_written = samples_read;
		}

		// render the source to the output mix
		for (int ii = 0; ii < samples_written; ++ii)
			*left++ += gain[0] * srcleft[ii];
		for (int ii = 0; ii < samples_written; ++ii)
			*right++ += gain[1] * srcright[ii];

		remaining -= samples_written;
	}
}

static void render_effects(float* buffer) {
	float compressor_factor = tm.compressor_factor;

	// get maximum absolute power level from the rendered buffer, and adjust the compressor factor
	float max_power = 0;
	for (int ii = 0; ii < N_SAMPLES; ++ii) {
		const float power = fabsf(buffer[ii]);
		if (power > max_power)
			max_power = power;
	}

	float target_compressor_factor = 1.0f;
	if (max_power > tm.compressor_thresholds[1])
		target_compressor_factor = tm.compressor_multipliers[1];
	else if (max_power > tm.compressor_thresholds[0])
		target_compressor_factor = tm.compressor_multipliers[0];

	float attack_release = 1.0f;
	if (target_compressor_factor < compressor_factor)
		attack_release = tm.compressor_attack_per1ksamples;
	else if (target_compressor_factor > compressor_factor)
		attack_release = tm.compressor_release_per1ksamples;

	// linearly interp compressor_factor toward the target compressor value
	const float interp = attack_release * N_SAMPLESF;
	compressor_factor = compressor_factor + interp*(target_compressor_factor - compressor_factor);
	compressor_factor = _tm_clamp(compressor_factor, tm.compressor_multipliers[1], 1.0f);

    // 2-pass compressor to limit dynamic range of audio clipping levels
	if (compressor_factor < 1.0f) {
		for (int cc = 0; cc < 2; ++cc) {
			float prev_sample = tm.compressor_last_samples[cc];

			float sample = 0;
			float* channel = buffer + cc*N_SAMPLES;
			for (int ii = 0; ii < N_SAMPLES; ++ii) {
				float sample = channel[ii];

				// uhhh... linear space? really??
				float diff = sample - prev_sample;
				sample = prev_sample + compressor_factor*diff;
				channel[ii] = sample;
			}

			tm.compressor_last_samples[cc] = sample;
		}
	}

	tm.compressor_factor = compressor_factor;
}

static void mix(float* buffer) {
	int nplaying = 0;
	int playing[N_SOURCES];
	float gain[N_SOURCES][2];

	// build active sounds
	for (int ii = 0; ii < N_SOURCES; ++ii) {
		const source_t* source = &tm.sources[ii];
		if (source->flags & TM_SOURCEFLAG_PLAYING) {
			playing[nplaying] = ii;
			++nplaying;
		}
	}

	// Update source gains
	for (int ii = 0; ii < nplaying; ++ii) {
		const source_t* source = &tm.sources[playing[ii]];

		const float gain_base = tm.gain_master * tm.gain_base[source->gain_base_index] * source->gain_base;
		gain[ii][0] = gain_base;
		gain[ii][1] = gain_base;

		if (source->flags & TM_SOURCEFLAG_POSITIONAL) {
			const float dist = _tm_dist(tm.position, source->position);
			if (dist > 1.0e-8f) {
				// attenuation factor due to distance
				const float gain_distance = 1.0f - (dist - source->distance_min) / source->distance_difference;

				// attenuation factor due to panning (position audio)
				const float gain_panning = (source->position[0] - tm.position[0]) / dist;

				gain[ii][0] *= gain_distance * (1.0f + gain_panning * -SPEAKER_DIST);
				gain[ii][1] *= gain_distance * (1.0f + gain_panning * +SPEAKER_DIST);
			}
		}

		// clamp gains
		gain[ii][0] = _tm_clamp(gain[ii][0], 0.0f, 1.0f);
		gain[ii][1] = _tm_clamp(gain[ii][1], 0.0f, 1.0f);
	}

	memset(buffer, 0, sizeof(float)*2*N_SAMPLES);

	// render playing sources
	for (int ii = 0; ii < nplaying; ++ii) {
		source_t* source = &tm.sources[playing[ii]];
		render(source, buffer, gain[ii]);
	}

	// allow application to apply a premixed track (such as music)
	if (tm.callbacks.pre_effects)
		(tm.callbacks.pre_effects)(tm.callbacks.udata, buffer, N_SAMPLES, tm.gain_callback);

	// render effects
	render_effects(buffer);

	// perform source-level post processing
	for (int ii = 0; ii < nplaying; ++ii) {
		source_t* source = &tm.sources[playing[ii]];
		++source->frame_age;

		// handle fadeout->stop
		if (source->flags & TM_SOURCEFLAG_FADEOUT) {
			source->gain_base -= source->fadeout_per_sample * N_SAMPLES;
			if (source->gain_base <= 0.0f) {
				source->flags = 0;
			}
		}
	}

	// cleanup dead sources
	for (int ii = 0; ii < nplaying; ++ii) {
		source_t* source = &tm.sources[playing[ii]];
		if (0 == (source->flags & TM_SOURCEFLAG_PLAYING)) {
			kill_source(source);
		}
	}
}

void tm_getsamples(float* samples, int nsamples) {
	mt_mutex_lock(&tm.lock);

	// was data leftover after the previous call to getsamples? Copy that out here
	while (nsamples && tm.samples_remaining) {
		const int samples_to_mix = _tm_min(nsamples, tm.samples_remaining);
		const int offset = N_SAMPLES - tm.samples_remaining;

		// clip and interleave
		for (int cc = 0; cc < 2; ++cc)
			for (int ii = 0; ii < samples_to_mix; ++ii)
				samples[cc + 2*ii] = _tm_clamp(tm.buffer[cc*N_SAMPLES + offset + ii], -1.0f, 1.0f);

		tm.samples_remaining -= samples_to_mix;
		samples += (2*samples_to_mix);
		nsamples -= samples_to_mix;
        mt_mutex_unlock(&tm.lock);
	}

	// Copy out samples
	while (nsamples) {
		const int samples_to_mix = _tm_min(nsamples, N_SAMPLES);
		mix(tm.buffer);
		tm.samples_remaining = N_SAMPLES;

		// clip and interleave
		for (int cc = 0; cc < 2; ++cc)
			for (int ii = 0; ii < samples_to_mix; ++ii)
				samples[cc + 2*ii] = _tm_clamp(tm.buffer[cc*N_SAMPLES + ii], -1.0f, 1.0f);


		tm.samples_remaining -= samples_to_mix;
		samples += (2*samples_to_mix);
		nsamples -= samples_to_mix;
	}

	mt_mutex_unlock(&tm.lock);
}

void tm_set_mastergain(float gain) {
	mt_mutex_lock(&tm.lock);
	tm.gain_master = gain;
	mt_mutex_unlock(&tm.lock);
}

static source_t* add(const tm_buffer* handle, int gain_index, float gain, float pitch) {
	source_t* source = find_source();
	if (!source)
		return 0;

	source->buffer = (const buffer_t*)handle;
	source->gain_base = gain;
	source->gain_base_index = (uint8_t)gain_index;
	source->frame_age = 0;

	const float diff = pitch - 1.0f;
	if (diff*diff < 1.0e-8f) {
		source->flags &= ~TM_SOURCEFLAG_FREQUENCY;
	} else {
		source->flags |= TM_SOURCEFLAG_FREQUENCY;
		tm_resampler_init_rate(&source->resampler, pitch);
	}

	source->buffer->funcs->start_source(source);

	_tm_addref((buffer_t*)handle);
	return source;
}

static void play(source_t* source) {
	source->flags |= TM_SOURCEFLAG_PLAYING;
}

typedef struct {
	buffer_t buffer;
	int32_t nsamples;
	uint8_t nchannels;
	// float smaples[nsamples*nschannels];
} static_sample_buffer;

static void static_sample_buffer_on_destroy(buffer_t* buffer) {
}

static void static_sample_buffer_start_source(source_t* source) {
	source->instance_data.static_source.sample_pos = 0;
}

static void static_sample_buffer_end_source(source_t*source) {
}

static int static_sample_buffer_request_samples(source_t* source, const float** left, const float** right, int nsamples) {
	const static_sample_buffer* buffer = (const static_sample_buffer*)source->buffer;

	int sample_pos = source->instance_data.static_source.sample_pos;

	// handle looping sources
	if (sample_pos == buffer->nsamples) {
		if (source->flags & TM_SOURCEFLAG_LOOPING) {
			sample_pos = 0;
		} else {
			return 0;
		}
	}

	nsamples = _tm_min(buffer->nsamples - sample_pos, nsamples);
	const float* srcleft = (float*)(buffer + 1) + sample_pos;

	*left = srcleft;
	if (buffer->nchannels == 1)
		*right = srcleft;
	else {
		*right = srcleft + buffer->nsamples;
	}

	source->instance_data.static_source.sample_pos = sample_pos + nsamples;
	return nsamples;
}

static int static_sample_buffer_get_buffer_size(const buffer_t* buffer) {
	const static_sample_buffer* sbuffer = (const static_sample_buffer*)buffer;
	return sizeof(static_sample_buffer) + sizeof(float)*sbuffer->nchannels*sbuffer->nsamples;
}

static buffer_functions_t static_sample_functions = {
	static_sample_buffer_on_destroy,
	static_sample_buffer_start_source,
	static_sample_buffer_end_source,
	static_sample_buffer_request_samples,
	static_sample_buffer_get_buffer_size,
};

void tm_create_buffer_interleaved_s16le(int channels, const int16_t* pcm_data, int pcm_data_size, const tm_buffer** handle) {
	const int nsamples = pcm_data_size/sizeof(uint16_t)/channels;

	static_sample_buffer* buffer = (static_sample_buffer*)tm.callbacks.allocate(tm.callbacks.udata, sizeof(buffer_t) + nsamples*channels*sizeof(float));
	buffer->buffer.funcs = &static_sample_functions;
	buffer->buffer.refcnt = 1;
	buffer->nchannels = (uint8_t)channels;
	buffer->nsamples = nsamples;

	// copy samples
	const int16_t* source = (const int16_t*)pcm_data;
	float* dest = (float*)(buffer + 1);
	for (int cc = 0; cc < channels; ++cc) {
		for (int ii = 0; ii < nsamples; ++ii)
			*dest++ = (float)source[channels*ii + cc] / (float)0x8000;
	}

	*handle = (tm_buffer*)buffer;
}

void tm_create_buffer_interleaved_float(int channels, const float* pcm_data, int pcm_data_size, const tm_buffer** handle) {
	const int nsamples = pcm_data_size/sizeof(float)/channels;

	static_sample_buffer* buffer = (static_sample_buffer*)tm.callbacks.allocate(tm.callbacks.udata, sizeof(buffer_t) + nsamples*channels*sizeof(float));
	buffer->buffer.funcs = &static_sample_functions;
	buffer->buffer.refcnt = 1;
	buffer->nchannels = (uint8_t)channels;
	buffer->nsamples = nsamples;

	// copy samples
	const float* source = (const float*)pcm_data;
	float* dest = (float*)(buffer + 1);
	for (int cc = 0; cc < channels; ++cc) {
		for (int ii = 0; ii < nsamples; ++ii)
			*dest++ = source[channels*ii + cc];//(int16_t)_tm_clamp((int32_t)(source[channels*ii + cc] * (float)0x8000), (int16_t)0x8000, 0x7fff);
	}

	*handle = (tm_buffer*)buffer;
}

typedef struct {
	buffer_t buffer;
	void* opaque;
	void (*closed)(void*);
	int ndata;
	// uint8_t vorbis_data[ndata]
} vorbis_stream_buffer;

static void vorbis_stream_on_destroy(buffer_t* buffer) {
	vorbis_stream_buffer* vbuffer = (vorbis_stream_buffer*)buffer;
	if (vbuffer->closed)
		vbuffer->closed(vbuffer->opaque);
}

static void vorbis_stream_start_source(source_t* source) {
	const vorbis_stream_buffer* vbuffer = (const vorbis_stream_buffer*)source->buffer;
	vorbis_stream_data_t* vsd = &source->instance_data.vorbis_stream;

	// open the vorbis stream
	unsigned char* data = (unsigned char*)(vbuffer + 1);
	vsd->v = stb_vorbis_open_memory(data, vbuffer->ndata, NULL, NULL);
	vsd->noutputs = 0;
}

static void vorbis_stream_end_source(source_t* source) {
	stb_vorbis* v = source->instance_data.vorbis_stream.v;
	if (v)
		stb_vorbis_close(v);

	source->instance_data.vorbis_stream.v = 0;
}

static int vorbis_stream_request_samples(source_t* source, const float** left, const float** right, int nsamples) {
	vorbis_stream_data_t* vsd = &source->instance_data.vorbis_stream;

	// no steam?
	if (!vsd->v)
		return 0;

	if (vsd->noutputs == 0) {
		int channels;
		float** outputs;
		vsd->noutputs = stb_vorbis_get_frame_float(vsd->v, &channels, &outputs);

		// if we're looping and have reached the end, seek to the start and try again
		if (vsd->noutputs == 0 && source->flags & TM_SOURCEFLAG_LOOPING) {
			stb_vorbis_seek_start(vsd->v);
			vsd->noutputs = stb_vorbis_get_frame_float(vsd->v, &channels, &outputs);
		}

		// handle mono streams
		if (vsd->noutputs) {
			vsd->outputs[0] = outputs[0];
			vsd->outputs[1] = (channels == 1) ? outputs[0] : outputs[1];
		}
	}

	nsamples = _tm_min(nsamples, vsd->noutputs);

	*left = vsd->outputs[0];
	*right = vsd->outputs[1];

	vsd->noutputs -= nsamples;
	vsd->outputs[0] += nsamples;
	vsd->outputs[1] += nsamples;
	return nsamples;
}

static int vorbis_stream_get_buffer_size(const buffer_t* buffer) {
	const vorbis_stream_buffer* vbuffer = (const vorbis_stream_buffer*)buffer;
	return sizeof(vorbis_stream_buffer) + vbuffer->ndata;
}

static buffer_functions_t vorbis_stream_buffer_funcs = {
	vorbis_stream_on_destroy,
	vorbis_stream_start_source,
	vorbis_stream_end_source,
	vorbis_stream_request_samples,
	vorbis_stream_get_buffer_size,
};

void tm_create_buffer_vorbis_stream(const void* data, int ndata, void* opaque, void (*closed)(void*), const tm_buffer** handle) {
    mt_mutex_lock(&tm.lock);
	vorbis_stream_buffer* buffer = (vorbis_stream_buffer*)tm.callbacks.allocate(tm.callbacks.udata, sizeof(vorbis_stream_buffer) + ndata);
	buffer->buffer.funcs = &vorbis_stream_buffer_funcs;
	buffer->buffer.refcnt = 1;
	buffer->opaque = opaque;
	buffer->closed = closed;
	buffer->ndata = ndata;

	// copy vorbis data
	memcpy(buffer + 1, data, ndata);
	*handle = (tm_buffer*)buffer;
    mt_mutex_unlock(&tm.lock);
}

typedef struct {
	buffer_t buffer;
	void* opaque;
	tm_buffer_callbacks callbacks;
} custom_stream_buffer;

static void custom_stream_on_destroy(buffer_t* buffer) {
	custom_stream_buffer* cbuffer = (custom_stream_buffer*)buffer;
	cbuffer->callbacks.on_destroy(cbuffer->opaque);
}

static void custom_stream_start_source(source_t* source) {
	custom_stream_buffer* cbuffer = (custom_stream_buffer*)source->buffer;
	source->instance_data.custom_stream.opaque = cbuffer->callbacks.start_source(cbuffer->opaque);
}

static void custom_stream_end_source(source_t* source) {
	custom_stream_buffer* cbuffer = (custom_stream_buffer*)source->buffer;
	cbuffer->callbacks.end_source(cbuffer->opaque, source->instance_data.custom_stream.opaque);
}

static int custom_stream_request_samples(source_t* source, const float** left, const float** right, int nsamples) {
	custom_stream_buffer* cbuffer = (custom_stream_buffer*)source->buffer;
	return cbuffer->callbacks.request_samples(cbuffer->opaque, source->instance_data.custom_stream.opaque, left, right, nsamples);
}

static int custom_stream_get_buffer_size(const buffer_t* buffer) {
	return sizeof(custom_stream_buffer);
}

static buffer_functions_t custom_stream_buffer_funcs = {
	custom_stream_on_destroy,
	custom_stream_start_source,
	custom_stream_end_source,
	custom_stream_request_samples,
	custom_stream_get_buffer_size,
};

void tm_create_buffer_custom_stream(void* opaque, tm_buffer_callbacks callbacks, const tm_buffer** handle) {
    mt_mutex_lock(&tm.lock);

	custom_stream_buffer* buffer = (custom_stream_buffer*)tm.callbacks.allocate(tm.callbacks.udata, sizeof(custom_stream_buffer));
	buffer->buffer.funcs = &custom_stream_buffer_funcs;
	buffer->buffer.refcnt = 1;
	buffer->opaque = opaque;
	buffer->callbacks = callbacks;

	*handle = (tm_buffer*)buffer;
    mt_mutex_unlock(&tm.lock);
}

int tm_get_buffer_size(const tm_buffer* handle) {
    mt_mutex_lock(&tm.lock);
	const buffer_t* buffer = (const buffer_t*)handle;
    mt_mutex_unlock(&tm.lock);
	return buffer->funcs->get_buffer_size(buffer);
}

void tm_release_buffer(const tm_buffer* handle) {
    mt_mutex_lock(&tm.lock);
	_tm_decref((buffer_t*)handle);
    mt_mutex_unlock(&tm.lock);
}

bool tm_add(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel) {
    mt_mutex_lock(&tm.lock);
    source_t* source = add(handle, gain_index, gain, pitch);
    if (source) {
        play(source);
        channel->index = (int)(source - tm.sources) + 1;
        mt_mutex_unlock(&tm.lock);
        return true;
    }
    mt_mutex_unlock(&tm.lock);
    return false;
}

bool tm_add_spatial(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel) {
    mt_mutex_lock(&tm.lock);
	source_t *source = add(handle, gain_index, gain, pitch);
	if (source) {
		_tm_vcopy(source->position, position);
		source->flags |= TM_SOURCEFLAG_POSITIONAL;
		source->distance_min = distance_min;
		source->distance_difference = (distance_max - distance_min);
		play(source);
		channel->index = (int)(source - tm.sources) + 1;

        mt_mutex_unlock(&tm.lock);
		return true;
	}

    mt_mutex_unlock(&tm.lock);
	return false;
}


static source_t* _add_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel) {
	source_t* source = add(handle, gain_index, gain, pitch);
	if (!source) {
		channel->index = 0;
		return 0;
	}

	source->flags |= TM_SOURCEFLAG_LOOPING;
	channel->index = (int)(source - tm.sources) + 1;
	return source;
}

bool tm_add_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel) {
    mt_mutex_lock(&tm.lock);
    source_t* source = _add_loop(handle, gain_index, gain, pitch, channel);
    if (source) {
        play(source);

        mt_mutex_unlock(&tm.lock);
        return true;
    }

    mt_mutex_unlock(&tm.lock);
    return false;
}

bool tm_add_spatial_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel) {
    mt_mutex_lock(&tm.lock);
	source_t* source = _add_loop(handle, gain_index, gain, pitch, channel);
	if (source) {
		_tm_vcopy(source->position, position);
		source->flags |= TM_SOURCEFLAG_POSITIONAL;
		source->distance_min = distance_min;
		source->distance_difference = (distance_max - distance_min);

		play(source);

        mt_mutex_unlock(&tm.lock);
		return true;
	}

    mt_mutex_unlock(&tm.lock);
	return false;
}

void tm_channel_set_opaque(tm_channel channel, void* opaque) {
    mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	source->opaque = opaque;
    mt_mutex_unlock(&tm.lock);
}

void tm_channel_stop(tm_channel channel) {
    mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	kill_source(source);
    mt_mutex_unlock(&tm.lock);
}

void tm_channel_set_position(tm_channel channel, const float* position) {
    mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	_tm_vcopy(source->position, position);
    mt_mutex_unlock(&tm.lock);
}

void tm_channel_fadeout(tm_channel channel, float seconds) {
	mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	source->fadeout_per_sample = 1.0f / (seconds * tm.sample_rate);
	source->flags |= TM_SOURCEFLAG_FADEOUT;
}

void tm_channel_set_gain(tm_channel channel, float gain) {
    mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	source->gain_base = gain;
	source->flags &= ~TM_SOURCEFLAG_FADEOUT;
    mt_mutex_unlock(&tm.lock);
}

float tm_channel_get_gain(tm_channel channel) {
	mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];
	mt_mutex_unlock(&tm.lock);
	return source->gain_base;
}

void tm_channel_set_frequency(tm_channel channel, float frequency) {
	mt_mutex_lock(&tm.lock);
	source_t* source = &tm.sources[channel.index - 1];

	// clear frequency shift if ~0.0f
	const float diff = frequency - 1.0f;
	if (diff*diff < 1.0e-8f) {
		source->flags &= ~TM_SOURCEFLAG_FREQUENCY;
	} else {
		source->flags |= TM_SOURCEFLAG_FREQUENCY;
		source->resampler.ideal_rate = frequency;
	}
	mt_mutex_unlock(&tm.lock);
}

static void* _tm_default_allocate(void* opaque, int bytes) {
    return malloc(bytes);
}

static void _tm_default_free(void* opaque, void* ptr) {
    free(ptr);
}

void tm_init(tm_callbacks callbacks, int sample_rate) {
	// setup default callbacks where needed
	if (!callbacks.allocate) {
		callbacks.allocate = _tm_default_allocate;
	}
	if (!callbacks.free) {
		callbacks.free = _tm_default_free;
	}

    mt_mutex_init(&tm.lock);

	tm.gain_master = 1.0f;
	for (int ii = 0; ii < N_GAINTYPES; ++ii)
		tm.gain_base[ii] = 1.0f;

	tm.sample_rate = sample_rate;
	tm.callbacks = callbacks;
	tm.samples_remaining = 0;

	const float default_thresholds[2] = {1.0f, 1.0f};
	const float default_multipliers[2] = {1.0f, 1.0f};
	const float default_attack = 0.0f;
	const float default_release = 0.0f;
	tm_effects_compressor(default_thresholds, default_multipliers, default_attack, default_release);
	tm.compressor_factor = 1.0f;
	tm.compressor_last_samples[0] = tm.compressor_last_samples[1] = 0;
}

void tm_shutdown() {
    mt_mutex_destroy(&tm.lock);
}

void tm_update_listener(const float* position) {
	mt_mutex_lock(&tm.lock);
	_tm_vcopy(tm.position, position);
	mt_mutex_unlock(&tm.lock);
}

void tm_set_base_gain(int index, float gain) {
	mt_mutex_lock(&tm.lock);
	tm.gain_base[index] = gain;
	mt_mutex_unlock(&tm.lock);
}

void tm_set_callback_gain(float gain) {
	mt_mutex_lock(&tm.lock);
	tm.gain_callback = gain;
	mt_mutex_unlock(&tm.lock);
}

void tm_effects_compressor(const float thresholds[2], const float multipliers[2], float attack_seconds, float release_seconds) {
	mt_mutex_lock(&tm.lock);
	tm.compressor_thresholds[0] = _tm_clamp(thresholds[0], 0.0f, 1.0f);
	tm.compressor_thresholds[1] = _tm_clamp(thresholds[1], 0.0f, 1.0f);
	tm.compressor_multipliers[0] = _tm_clamp(multipliers[0], 0.0f, 1.0f);
	tm.compressor_multipliers[1] = _tm_clamp(multipliers[1], 0.0f, 1.0f);

    float attackSampleRate = (attack_seconds * (float)tm.sample_rate);
	tm.compressor_attack_per1ksamples = (attackSampleRate > 0.0f) ? (1.0f / attackSampleRate) : 1.0f;

    float releaseSampleRate = (release_seconds * (float)tm.sample_rate);
	tm.compressor_release_per1ksamples = (releaseSampleRate > 0.0f) ? (1.0f / releaseSampleRate) : 1.0f;
	mt_mutex_unlock(&tm.lock);
}

void tm_stop_all_sources() {
	mt_mutex_lock(&tm.lock);
	for (int ii = 0; ii < N_SOURCES; ++ii) {
		source_t* source = &tm.sources[ii];
		if (source->buffer) {
			kill_source(source);
		}
	}
	mt_mutex_unlock(&tm.lock);
}

void tm_resampler_init(tm_resampler* resampler, int input_sample_rate, int output_sample_rate) {
	const float ideal_rate = (float)input_sample_rate / (float)output_sample_rate;
	tm_resampler_init_rate(resampler, ideal_rate);
}

void tm_resampler_init_rate(tm_resampler* resampler, float ideal_rate) {
	resampler->ideal_rate = ideal_rate;
	resampler->prev_samples[0] = 0.0f;
	resampler->prev_samples[1] = 0.0f;
}

int tm_resampler_calculate_input_samples(const tm_resampler* resampler, int output_samples) {
	const float input_samples = ceilf(resampler->ideal_rate * (float)output_samples);
	return (int)input_samples;
}

int tm_resampler_calculate_output_samples(const tm_resampler* resampler, int input_samples) {
	const float output_samples = ceilf((float)input_samples / resampler->ideal_rate);
	return (int)output_samples;
}

void tm_resample_stereo(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples) {
	float* output_end = output + (2 * num_output_samples) - 2;

	float pos = resampler->ideal_rate;
	while (pos < 1.0f) {
		output[0] = resampler->prev_samples[0] + pos * (input[0] - resampler->prev_samples[0]);
		output[1] = resampler->prev_samples[1] + pos * (input[1] - resampler->prev_samples[1]);

		output += 2;
		pos += resampler->ideal_rate;
	}

	while (output < output_end) {
		const float pos_floor = floorf(pos);
		const int index = 2 * (int)pos_floor;
		output[0] = input[index - 2] + (input[index + 0] - input[index - 2]) * (pos - pos_floor);
		output[1] = input[index - 1] + (input[index + 1] - input[index - 1]) * (pos - pos_floor);

		output += 2;
		pos += resampler->ideal_rate;
	}

	output[0] = input[2 * num_input_samples - 2];
	output[1] = input[2 * num_input_samples - 1];

	resampler->prev_samples[0] = output[0];
	resampler->prev_samples[1] = output[1];
}

void tm_resample_mono(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples) {
	float* output_end = output + num_output_samples - 1;

	float pos = resampler->ideal_rate;
	while (pos < 1.0f) {
		output[0] = resampler->prev_samples[0] + pos * (input[0] - resampler->prev_samples[0]);

		++output;
		pos += resampler->ideal_rate;
	}

	while (output < output_end) {
		const float pos_floor = floorf(pos);
		const int index = (int)pos_floor;
		output[0] = input[index - 1] + (input[index] - input[index - 1]) * (pos - pos_floor);

		++output;
		pos += resampler->ideal_rate;
	}

	output[0] = input[num_input_samples - 1];

	resampler->prev_samples[0] = output[0];
}

void* tm_vorbis_malloc(size_t sz) {
	return tm.callbacks.allocate(tm.callbacks.udata, (int)sz);
}

void tm_vorbis_free(void* ptr) {
	tm.callbacks.free(tm.callbacks.udata, ptr);
}

void* tm_vorbis_temp_malloc(size_t sz) {
	return tm_vorbis_malloc((int)sz);
}

void tm_vorbis_temp_free(void* ptr) {
	tm_vorbis_free(ptr);
}

void tm_lowpass_filter_init(tm_lowpass_filter* filter, float cutoff_frequency, float sample_rate) {
	filter->cutoff_frequency = cutoff_frequency;
	filter->sample_rate = sample_rate;

	memset(filter->channel_history, 0, sizeof(filter->channel_history));
}

void tm_lowpass_filter_apply(tm_lowpass_filter* filter, float* output, float* input, int num_samples, int num_channels) {

	float yk[2] = {
		  filter->channel_history[0]
		, filter->channel_history[1]
	};

	const float alpha = filter->cutoff_frequency / filter->sample_rate;

	for (int ii = 0; ii != num_samples; ++ii) {
		for (int channel = 0; channel != num_channels; ++channel) {
			yk[channel] += alpha * (input[ii*num_channels + channel] - yk[channel]);
			output[ii*num_channels + channel] = yk[channel];
		}
	}

	filter->channel_history[0] = yk[0];
	filter->channel_history[1] = yk[1];
}

#endif //TMIXER_IMPL
//FILE_END

//FILE_START:pk_audio.h
#ifndef PK_AUDIO_H
#define PK_AUDIO_H

#ifndef PK_AUDIO_SINGLE_HEADER
#include "../poki.h"
#include "sokol_audio.h"
#include "tmixer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pk_audio_desc {
    saudio_desc saudio;
    tm_callbacks mixer_callbacks;
} pk_audio_desc;

void pk_audio_setup(const pk_audio_desc* desc);
void pk_audio_shutdown(void);

/*
Options for playing a sound.
Filling in the node field will make the sound spatial.
In this case, you should also fill in range_min and range_max.
*/
typedef struct pk_sound_channel_desc {
    const tm_buffer* buffer;
    float range_min;
    float range_max;
    pk_node* node;
    bool loop;
} pk_sound_channel_desc;

typedef struct pk_sound {
    tm_channel channel;
    pk_node* node;
} pk_sound;

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc);
void pk_update_sound(pk_sound* sound);
void pk_stop_sound(pk_sound* sound);

typedef struct pk_sound_listener {
    HMM_Vec3 position;
    float smoothing; // Around 2 is a good starting point.
} pk_sound_listener;

void pk_update_sound_listener(pk_sound_listener* listener, HMM_Vec3 new_pos, float dt);

//--loading------------------------------------------------------------

typedef void(*pk_sound_buffer_loaded_callback)(const tm_buffer* buffer);

typedef struct pk_sound_buffer_request {
    const char* path;
    sfetch_range_t buffer;
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} pk_sound_buffer_request;

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req);
//since the header is included, just use tm_release_buffer(...) here.



#ifdef __cplusplus
}
#endif
#endif //PK_AUDIO_H


//FILE_END

#if defined(POKI_IMPL) || defined(PK_AUDIO_IMPL)

//FILE_START:pk_audio.c
#ifndef PK_AUDIO_SINGLE_HEADER
#include "pk_audio.h"
#include "../poki.h"
#include <string.h>
#endif

#ifdef PK_SINGLE_HEADER
#ifndef POKI_H
#error "please include poki.h before pk_audio.h"
#endif
#endif

static void _pk_stream_cb(float* buffer, int num_frames, int num_channels, void* udata) {
    (void)num_channels; (void)udata;
    tm_getsamples(buffer, num_frames);
}

void pk_audio_setup(const pk_audio_desc* desc) {
    saudio_desc ad = { 0 };
    memcpy(&ad, &desc->saudio, sizeof(saudio_desc));
    if (!ad.stream_cb && !ad.stream_userdata_cb) {
        ad.stream_userdata_cb = _pk_stream_cb;
    }
    saudio_setup(&ad);
    tm_init(desc->mixer_callbacks, saudio_sample_rate());
}

void pk_audio_shutdown() {
    if (saudio_isvalid()) {
        tm_shutdown();
        saudio_shutdown();
    }
}

void pk_play_sound(pk_sound* sound, const pk_sound_channel_desc* desc) {
    pk_assert(desc->buffer);
    sound->channel = (tm_channel){ 0 };
    sound->node = desc->node;
    // check if we need positional audio
    if (desc->node != NULL) {
        if (desc->loop) {
            tm_add_spatial_loop(
                desc->buffer, 0, 0.75f, 1.0f,
                (const float*)&desc->node->position,
                desc->range_min, desc->range_max,
                &sound->channel
            );
        }
        else tm_add_spatial(
            desc->buffer, 0, 0.75f, 1.0f,
            (const float*)&desc->node->position,
            desc->range_min, desc->range_max,
            &sound->channel
        );
    }
    else {
        if (desc->loop) {
            tm_add_loop(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
        }
        else tm_add(desc->buffer, 0, 0.75f, 1.0f, &sound->channel);
    }
}

void pk_update_sound(pk_sound* sound) {
    if (sound->node) {
        tm_channel_set_position(sound->channel, sound->node->position.Elements);
    }
}

void pk_stop_sound(pk_sound* sound) {
    pk_assert(sound);
    tm_channel_stop(sound->channel);
}

void pk_update_sound_listener(pk_sound_listener* li, HMM_Vec3 pos, float dt) {
    const float smoothing = 1.0f - expf(-dt * li->smoothing);
    li->position = HMM_LerpV3(li->position, smoothing, pos);
    tm_update_listener((const float*)li->position.Elements);
}


typedef struct {
    pk_sound_buffer_loaded_callback loaded_cb;
    pk_fail_callback fail_cb;
} sound_request_data;

static void _sound_fetch_callback(const sfetch_response_t* response) {
    sound_request_data data = *(sound_request_data*)response->user_data;

    if (response->fetched) {
        const tm_buffer* buffer = NULL;
        tm_create_buffer_vorbis_stream(
            response->buffer.ptr,
            (int)response->buffer.size,
            NULL, NULL,
            &buffer
        );
        if (data.loaded_cb) {
            data.loaded_cb(buffer);
        }
    }
    if (response->failed) {
        switch (response->error_code) {
        case SFETCH_ERROR_FILE_NOT_FOUND: pk_printf("Sound file not found: %s\n", response->path); break;
        case SFETCH_ERROR_BUFFER_TOO_SMALL: pk_printf("Sound buffer too small: %s\n", response->path); break;
        default: break;
        }
        if (data.fail_cb) {
            data.fail_cb(response);
        }
    }
}

sfetch_handle_t pk_load_sound_buffer(const pk_sound_buffer_request* req) {
    sound_request_data data = {
        .loaded_cb = req->loaded_cb,
        .fail_cb = req->fail_cb,
    };

    return sfetch_send(&(sfetch_request_t) {
        .path = req->path,
        .buffer = req->buffer,
        .callback = _sound_fetch_callback,
        .user_data = SFETCH_RANGE(data),
    });
}

//FILE_END

#endif // POKI_IMPL || PK_AUDIO_IMPL

