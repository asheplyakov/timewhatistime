=============================================
Computer timekeeping devices and benchmarking
=============================================

Synopsis
========

* `Physical principles`_
* `x86 timers and counters`_
* `OS interfaces`_
* `C++: std::chrono`_
* `Java`_
* `Picking a right timer for benchmarks`_


Physical principles
===================

Mechanical resonance of a vibrating crystal of `piezoelectric`_ material
(typically quartz) creates an electrical signal with a precise frequency.

* Common frequencies: 30 kHz -- 200 MHz
* Accuracy: :math:`10^{-5} \ldots 10^{-12}`

.. _piezoelectric: https://en.wikipedia.org/wiki/Piezoelectricity


Parameters
----------

* Resolution: minimal time between two ticks
* Latency: the time it takes to read the timer value
* Accuracy: possible deviation of measured value from the true one
* Precision: the numer of digits/bits in the measured value


x86 timers and counters
=======================

* Real Time Clock (`RTC`_), resolution = 1 second
* `8254 PIT`_ (programmable interval timer), frequency 105/88 = 1.19 MHz
* `APIC timer`, resolution ~ 1 microsecond
* `ACPI PM timer`_
* `HPET`_ (high precision event timer)
* `TSC`_ (time stamp counter)
* `Virtualization and timers`_


RTC
---

* Keeps the system date and time
* Has an alternative power source (lithium battery or a supercapacitor)
* Frequency: 32768 Hz
* Precision: 1 second
* Typical accuracy: 5 parts per million (a second in 2.3 days)


----


8254 PIT
--------

3 16-bit counters (also called channels)

* channel 0 is assigned to IRQ-0 (used as default time source)
* channel 1 is assigned to DRAM refresh (at least in early models)
* channel 2 is assigned to PC speaker

Each counter can operate in one of six modes

* reads and writes require communication through an IO port
* programming the timer takes several cycles
* device communicates the event via an interrupt
* the device might be in use by the OS scheduler

Introduced with Intel 8086 CPUs (in 1978).


----

APIC timer
----------

In 1994 Intel releassed the MultiProcessor Specification (`MPS`) to allow
construction of multi-processor i386 systems. Early x86 multi-processor
systems were APIC_ (advanced programmable interrupt controller) architectures.

* Local APIC timer is hardwared to each CPU core
* Ticks at (some of) local CPU frequency
* 2 modes: periodic and one-shot

* Software has no reliable way to discover the timer frequency (even in the kernel mode)
* Timer frequency is not constant
* Timers of different CPU cores are not synchronized
* Device communicates the result via an interrupt
* Sometimes buggy at the silicon level
* Tricky discovery via MPS tables or ACPI

.. _APIC: https://en.wikipedia.org/wiki/Advanced_Programmable_Interrupt_Controller


ACPI PM timer
-------------

Advanced Configuration and Power Interface (ACPI) has been designed to
replace `MPS`, `APM`, and `PnP BIOS` specifications. First released in 1996.

ACPI specification requires `Power Management Timer`: a timer with a stable
frequency (not affected by CPU frequency, power saving, etc)

* Typical frequency: 3.759545 MHz
* 32-bit counter

However

* Does not generate periodic interrupts
* Does not generate one-shot interrupts
* Access method: IO ports
* Tricky discovery via ACPI


----

HPET
----

To resolve APIC timer problems Intel and Microsoft designed a `High Precision Event Timer`
specification circa 2002. It has been incorporated into PS chipsets since 2005.

* 64-bit up-counter and (at least) 3 comparators
* comparator can generate an interrupt when the least significant bits equal
  to the corresponding bits of the main counter (64-bit) value
* comparator can be either periodic or one-shot
* specification requires frequency >= 10 MHz
* programmed via memory mapped I/O


* Typical frequency: 14.318 MHz (12x the standard 8254 frequency)
* Typical counter latency: 1 -- 2 microsecond
* Missed interrupt if the target time has passed when writing to the chip register
  (comparators tests for equality rather than "greater or equal")
* Requires ring 0 privileges to read/program the device
* Tricky detection via ACPI


TSC
---

All x86 CPUs since Pentium (1993) are equipped with `Time Stamp Counter`.
Initially TSC was a 64 bit counter auto-incremented on each CPU cycle.
The value can be read with a single non-privileged instruction `rdtsc`.

Multi-processor systems and power management features made TSC tricky:

* In early multi-processor systems TSCs of different CPUs are not synchornized at all
* CPU frequency scaling made the timer frequency non-constant
* TSC might stop when CPU enters a low-energy state
* TSC might be reset when CPU exits a low-energy state

Most of these problems have been solved and modern x86 CPUs (Intel: Core 2, Xeon,
Atom and newer, AMD: Barcelona/Phenom and newer) feature a constant rate TSC (typically
driven by memory interconnect bus, such as QPI or HyperTransport). However

* Older CPUs are still around
* The frequency of TSC differs from the CPU maximal frequency
* `rdtsc` can be executed speculatively just like any other instruction

----


Virtualization and timers
-------------------------

* All timers except TSC are simulated by the hypervisor
  (thus the hypervisor thread simulating timers can be pre-emptied by the host OS)
* Paranoid hypervisor block `rdtsc` instruction in guest mode
* VM can be suspended, (live) migrated -- even TSC is not stable any more

Basically no luck with microsecond measurements


OS interfaces
=============

Linux: clocksource
------------------

During the boot the kernel detects available timekeeping devices and selects
the best one to use. On x86 the preferred clock source is the TSC, if it is
not available (or not reliable) the HPET is the 2nd best option.

.. code:: bash

   $ cat /sys/devices/system/clocksource/clocksource0/current_clocksource
   tsc
   $ cat /sys/devices/system/clocksource/clocksource0/available_clocksource
   tsc hpet acpi_pm


C API
------

UNIX like OSes hide the platform pecularites and provide

.. code:: C

   struct timespec {
       time_t tv_sec;
       long   tv_nsec;
   };

   int clock_gettime(clockid_t clk_id, struct timespec *tp);

Note: just because the structure stores the fractional part as nanoseconds
**DOES NOT** mean the API guarantees the nanosecond resolution and/or accuracy.
To find out the clock resolution use

.. code:: C

   int clock_getres(clockid_t clk_id, struct timespec *tp);

(and take the result with a grain of salt).

* ``CLOCK_REALTIME`` -- system-wide clock, measures the real time.
  Can be set by administrator, subject to NTP adjustments.
* ``CLOCK_MONOTONIC`` -- monotonic time since some unspecified point.
  **Can't** be set, subject to NTP adjustments.
* ``CLOCK_MONOTONIC_RAW`` -- monotonic time since some unspecified point.
  **Can't** be set, **NOT** affected by NTP.
* ``CLOCK_THREAD_CPUTIME_ID`` -- CPU time consumed by the calling thread
* ``CLOCK_PROCESS_CPUTIME_ID`` -- CPU time consumed by all threads of the process

.. code:: C

   #define _GNU_SOURCE
   #include <time.h>
   #include <sys/types.h>
   #include <errno.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <stdint.h>
   #include <inttypes.h>

   int64_t timespec_diff_usec(struct timespec const* start, struct timespec const* end) {
        int64_t ret = 0;
        ret = end->tv_sec - start->tv_sec;
        ret *= 1000000;
        ret += (end->tv_nsec - start->tv_nsec)/1000;
        return ret;
   }

   void realloc_benchmark(unsigned L) {
        unsigned int *v = NULL;
        struct timespec start, end;
        int64_t elapsed;
        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
            perror("clock_gettime");
            exit(1);
        }
        for (unsigned i = 0; i < L; i++) {
             v = realloc(v, sizeof(i)*(i+1));
             if (!v) {
                perror("realloc");
                exit(1);
             }
             v[i] = i;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
            perror("clock_gettime");
            exit(1);
        }
        elapsed = timespec_diff_usec(&start, &end);
        printf("%u reallocs in %" PRId64 " usec\n", L, elapsed);
   }

   int main(int argc, char** argv) {
       unsigned L = 0;
       struct timespec res;
       if (argc >= 2) {
          L = atoi(argv[1]);
       }
       if (0 == L) {
           L = 1U << 20;
       }
       if (clock_getres(CLOCK_MONOTONIC, &res) < 0) {
           perror("clock_getres");
           exit(1);
       }
       printf("Using CLOCK_MONOTONIC, resolution: %ld nsec\n", res.tv_nsec);
       realloc_benchmark(L);
       return 0;
   }


C++: std::chrono
================

* `std::chrono::system_clock` -- system wall clock
* `std::chrono::steady_clock` -- monotonic clock, constant interval between ticks
* `std::chrono::high_resolution_clock` -- clock with smallest tick period provided by implementation

.. code:: c++

   #include <type_traits>
   #include <vector>
   #include <chrono>
   #include <iostream>
   #include <cstdlib>
   
   typedef std::conditional<std::chrono::high_resolution_clock::is_steady,
   	                 std::chrono::high_resolution_clock,
   			 std::chrono::steady_clock>::type benchmark_clock;
   
   void push_back_benchmark(unsigned L) {
       auto start = benchmark_clock::now();
       std::vector<unsigned> v;
       for (unsigned i = 0; i < L; i++) {
            v.push_back(i);
       }
       auto end = benchmark_clock::now();
       auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
       std::cout << L << " .push_back() in " << elapsed << " usec" << std::endl;
   }
   
   int main(int argc, char** argv) {
       unsigned L = 0;
       if (argc >= 2) {
           L = std::atoi(argv[1]);
       }
       if (0 == L) {
           L = 1U << 20;
       }
       std::cout << "Using " <<
           (std::chrono::high_resolution_clock::is_steady ? "high_resolution_clock" : "steady_clock")
           << ", resolution: "
           << benchmark_clock::period::num << '/' << benchmark_clock::period::den
           << " sec" << std::endl;
       push_back_benchmark(L);
       return 0;
   }


Beware
------

`steady_clock` as implemented in GCC C++ runtime uses ``CLOCK_MONOTONIC``,
which is affected by `adjtime`. Thus interval between ticks is not constant
if time synchronization software is running.


Java
====


System.currentTimeMillis()
--------------------------

* Number of milliseconds since UNIX epoch (January 1, 1970 UTC)
* Actual resolution may be more coarse


System.nanoTime()
-----------------

* Number of nanoseconds since some arbitrary origin (may be in the future)
* Not necessarily nanosecond resosution
* The resolution is at least as good as that of `currentTimeMillis`


Pitfals
-------

OpenJDK implements `nanoTime` with `clock_gettime(CLOCK_MONOTONIC, ...)`,
which is a subject of NTP adjustments


Picking a right timer for benchmarks
====================================

Benchmarks are about measuring time it takes to do something.

Millisecond resolution is good enough
-------------------------------------

- Make sure to NOT set clock during the benchmark
- (temporarily) disable the time synchronization (NTP, PTP, etc)


Acheiving microsecond resolution 
--------------------------------

* The only timer which **MIGHT** be suitable is `TSC` (on x86 platform)

* However avoid using ``rdtsc`` directly for finding out if TSC of
  a given CPU/hypervisor is good enough is *difficult*

* Also avoid using ``rdtscp`` due to a high overhead

* Use the ``CLOCK_MONOTONIC`` clock. Linux picks TSC as a source
  if TSC is good enough

* ``clock_gettime(CLOCK_MONOTONIC, ...)`` does **NOT** involve
  a system call when kernel picks TSC as a time source


Clock sanity checklist
~~~~~~~~~~~~~~~~~~~~~~

#. Verify that current clocksource is TSC

   .. code:: bash

      $ cat /sys/devices/system/clocksource/clocksource0/current_clocksource
      tsc

#. Check if the kernel indicates the following flags in `/proc/cpuinfo`

   - `constant_tsc`
   - `tsc_known_freq`
   - `nonstop_tsc`

#. Check if ``clock_gettime(CLOCK_MONOTONIC, ..)`` does **NOT** make system calls.
   Compile the following program and run it with `strace`_

   .. code:: c

      #define _GNU_SOURCE
      #include <time.h>
      #include <stdlib.h>

      int main(int argc, char** argv) {
          struct timespec ts;
          for (int i = 0; i < 1000000; i++) {
              asm volatile("": : :"memory");
              if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
                  exit(1);
              }
          }
          return 0;
      }

   .. code:: bash

      $ gcc -std=gnu99 -O2 -g -Wall -o clocksanity clocksanity.c
      $ strace -e clock_gettime ./clocksanity
        +++ exited with 0 +++

    If `strace` prints lots of lines like::

      clock_gettime(CLOCK_MONOTONIC, {tv_sec=1775767, tv_nsec=426776012}) = 0

    than `clock_gettime` is a system call (and thus the overhead of timing
    is way too high and timing itself might become a bottleneck)

.. _strace: http://man7.org/linux/man-pages/man1/strace.1.html
