/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * Portions created by Alan Antonuk are Copyright (c) 2013-2014 Alan Antonuk.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef AMQP_TIMER_H
#define AMQP_TIMER_H

#include <stdint.h>

#ifdef _WIN32
# ifndef WINVER
#  define WINVER 0x0502
# endif
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Winsock2.h>
#else
# include <sys/time.h>
#endif

#define AMQP_MS_PER_S  1000
#define AMQP_US_PER_MS 1000
#define AMQP_NS_PER_S  1000000000
#define AMQP_NS_PER_MS 1000000
#define AMQP_NS_PER_US 1000

typedef struct amqp_timer_t_ {
  uint64_t expiration_ns;
} amqp_timer_t;

/* Gets a monotonic timestamp in ns */
uint64_t
amqp_get_monotonic_timestamp(void);

/* Start an amqp_timer_t set to expire timeout from now. */
int amqp_timer_start(amqp_timer_t *timer, struct timeval *timeout);

amqp_timer_t amqp_timer_start_immediate(void);

amqp_timer_t amqp_timer_start_infinite(void);

/* Get the (positive) number of ms left on the timer, or -1 for an infinite
 * timer, or AMQP_STATUS_TIMEOUT on time out, or AMQP_STATUS_TIMER_FAILURE on
 * failure. */
int amqp_timer_ms_left(amqp_timer_t timer);

#endif /* AMQP_TIMER_H */
