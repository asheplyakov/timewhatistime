=============================================
Computer timekeeping devices and benchmarking
=============================================

Synopsis
========

* `Physical principles`_
* `x86 timers and counters`_
* `OS interfaces`_
* `Picking a right timer for benchmarks`


Physical principles
===================

Mechanical resonance of a vibrating crystal of `piezoelectric`_ material
(typically quartz) creates an electrical signal with a precise frequency.

* Common frequencies: 30 kHz -- 200 MHz
* Frequency drift: 10**-5 -- 10**-12

.. _piezoelectric: https://en.wikipedia.org/wiki/Piezoelectricity


x86 timers and counters
=======================

* Real Time Clock (RTC), keeps the date and time, 1 second precision
* `8254 PIT`_ (programmable interval timer), frequency 105/88 = 1.19 MHz
* `APIC timer`, resolution ~ 1 microsecond
* `ACPI PM timer`_
* `HPET`_ (high precision event timer)
* `TSC`_ (time stamp counter)
* `PTP`_ clock in network interface card(s)

---

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

---

APIC timer
----------

* Local APIC timer is hardwared to each CPU core
* Ticks at (some of) local CPU frequency
* 2 modes: periodic and one-shot

* Timer frequency is not constant
* Timers of different CPU cores are not synchronized
* Device communicates the result via an interrupt
* Sometimes buggy at the silicon level
* Tricky discovery via ACPI

---

HPET
----

* 64-bit up-counter and (at least) 3 comparators
* comparator can generate an interrupt when the least significant bits equal
  to the corresponding bits of the main counter (64-bit) value
* comparator can be either periodic or one-shot
* specification requires frequency >= 10 MHz
* programmed via memory mapped I/O


* Typical implementations run the counter at about 18 MHz and require 1 -- 2 microseconds to read the value
* Comparators tests for equality rather than "greater or equal" so it's easy to miss an interrupt
* Requires ring 0 privileges to read/program the device
* Tricky detection via ACPI

Not suitable as (microsecond precision) timestamp source for benchmarks


TSC
---

* A 64-bit counter auto-incremented on each CPU cycle...
* The value can be read with a single non-privileged instruction `rdtsc`

However

* Early CPUs (mid 2000th) always incremented TSC every clock cycle, thus the timer frequency varied due to power management features
* TSC might stop when CPU enters a low-energy state
* TSC might be reset when CPU exits a low-energy state
* `rdtsc` instruction can be executed speculatively

Modern x86 CPUs (Intel: Core 2, Xeon, Atom and newer, AMD: Barcelona/Phenom
and newer) feature a constant rate TSC (typically driven by memory interconnect
bus, such as QPI or HyperTransport)

----

OS interfaces
=============

UNIX hides the platform pecularites and provides

.. code:: C

   int clock_gettime(clockid_t clk_id, struct timespec *tp);

* ``CLOCK_REALTIME`` -- system-wide clock, measures the real time.
  Can be set by administrator, subject to NTP adjustments.
* ``CLOCK_MONOTONIC`` -- monotonic time since some unspecified point.
  **Can't** be set, subject to NTP adjustments.
* ``CLOCK_MONOTONIC_RAW`` -- monotonic time since some unspecified point.
  **Can't** be set, **NOT** affected by NTP.


Picking a right timer for benchmarks
====================================

Benchmarks are about measuring time it takes to do something.

Millisecond precision is good enough
------------------------------------

- Make sure to NOT set clock during the benchmark
- (temporarily) disable the time synchronization (NTP, PTP, etc)


Acheiving microsecond accuracy
------------------------------

* The only timer which **MIGHT** be suitable is `TSC` (on x86 platform)

* However avoid using ``rdtsc`` directly for finding out if TSC of
  a given CPU/hypervisor is good enough is *difficult*

* Also avoid using ``rdtscp`` due to a high overhead

* Use the ``CLOCK_MONOTONIC_RAW`` clock. Linux picks TSC as a source
  if TSC is good enough

* ``clock_gettime(CLOCK_MONOTONIC_RAW, ...)`` does **NOT** involve
  a system call when kernel picks TSC as a time source


