# WinAmp plugin -- complex / real (wav) audio universal quadrature modulator

It can:

* Works with Winamp (v5.666 recommended) and XMPlay (we have use v3.8.2.3)
as input plugin for our complex-valued Hilbert conjugate CWAVE audio files
as well as for any **uncompressed PCM and float WAV files** (fast high
quality transparent converter to analytical signal included).

* Execute user-defined DSP-computational graph under input data. Graph
can contain arbitrary set of spectrum shifting, harmonically phase modulations
and mixing, as well as some stereo channels and signal component operations.

* Support both real-time playback and transcoding - for both Winamp and XMPlay.

* Advanced output signal rendering to 16 or 24 bits. With right player
setup the plugin completely controls all data path from the file data samples
(complex- or real-valued) to selected in player system audio-interface.

_For effective use, please read documentation carefully._

The project has started in 2010 as "2-hour-to-complete" experimental code.
But the experiments needs had grown, and we got what we got. Here is
some grave of the work, enjoy.

**NOTE** We provide project solution for Microsoft Visual Studio 2013
(more recent versions should to work too) and Code::Blocks. Our Code::Blocks
project works just fine with versions V16.01 / MinGW gcc V4.9.2, 32 bit and
V17.12 / MinGW gcc V5.1.0, 32 bit. It does not work with more recent
Code::Blocks V20.03 due to some conflicts with our Win32 SDK patches,
which we have made for older versions. At the time we don't plan to fix
this - for some reason we still have use old version, which need our
compatibility patches.

**NOTE** From V02.02.03 we provide different build configurations for
current Windows (Vista+) and Windows XP for both supported development
tools -- Visual Studio 2013(+) and Code::Blocks. Due to some limitation
of Unicode support in system C-runtime in Windows XP, please avoid using
MinGW / Code::Blocks XP build.
