# RV7 - preparing since 2025

*Work is being done, we are glad your eyes earn a delight.*

## What is this?

The threat of collapse for God's creation looms more than ever before, the humans have
driven themselves away from the divine to certain evil. RV7 is a UNIX-like operating system
aimed to be easily readable, modifiable, modular, and built by/for OSMORA. Those with non
constructive disagreements and/or lack of use for what's to be used in revolution-phi may
withdraw from this repository.

## Build Instructions for the RV7 Enjoying OSMORIANs

> [!IMPORTANT]
> It is important that you have the correct dependencies installed before you attempt
> to build the system, see dependencies section.

First you must bootstrap the project by running ``./boostrap``. The project can then
be configured by running ``./configure``.

Before the system can be built, you must build yourself a toolchain by running
``make toolchain``, this will take awhile but once it is complete, the system can be built
with a single ``make``.

## Dependencies

The following is required to be installed to build the system:

- xorriso
- make
- git
- autoconf
- clang
- as
- rsync
