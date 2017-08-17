/* Copyright 2017 Columbia University, SLD Group */

#ifndef __CACHE_UTILS_HPP__
#define __CACHE_UTILS_HPP__

#include <iostream>
#include <utility>
#include "utils/esp_systemc.hpp"
#include "utils/esp_utils.hpp"
#include "cache_consts.hpp"
#include "cache_types.hpp"

#define CACHE_REPORT_INFO(text)						\
    cerr << "Info:  " << sc_object::basename() << ".\t " << text << endl;

#define CACHE_REPORT_ERROR(text, var)					\
    cerr << "ERROR: " << sc_object::basename() << ".\t " << text << " : " \
    << var << endl;

#define CACHE_REPORT_TIME(time, text)					\
    cerr << "Info:  " << sc_object::basename() << ".\t @" << time << " : " \
    << text << endl;

#define CACHE_REPORT_VAR(time, text, var)				\
    cerr << "Info:  " << sc_object::basename() << ".\t @" << time << " : " \
    << text << " : " << var << endl;

#define CACHE_REPORT_DEBUG(time, text, var)				\
    cerr << "Debug:  " << sc_object::basename() << ".\t @" << time << " : " \
    << text << " : " << var << endl;

inline void write_word(line_t &line, word_t word, word_offset_t w_off, byte_offset_t b_off, hsize_t hsize)
{
    uint32_t size, b_off_tmp;
    
    if (hsize == BYTE) {
	b_off_tmp = BYTES_PER_WORD - 1 - b_off;
	size = 8;
    } else if (hsize == HALFWORD) {
	b_off_tmp = BYTES_PER_WORD - 2 - b_off;
	size = BITS_PER_HALFWORD;
    } else if (hsize == WORD) {
	b_off_tmp = b_off;
	size = BITS_PER_WORD;
    }

    uint32_t w_off_bits = BITS_PER_WORD * w_off;
    uint32_t b_off_bits = 8 * b_off_tmp;
    uint32_t off_bits = w_off_bits + b_off_bits;

    line.range(off_bits + size - 1, off_bits) = word.range(b_off_bits + size - 1, b_off_bits);
}

inline word_t read_word(line_t line, word_offset_t w_off)
{
    word_t word;
    word = (sc_uint<BITS_PER_WORD>) line.range(BITS_PER_WORD*w_off+BITS_PER_WORD-1, BITS_PER_WORD*w_off);

    return word;
}

#endif /* __CACHE_UTILS_HPP__ */