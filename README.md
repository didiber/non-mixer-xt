Non-Mixer-XT
============

Screenshot
----------

![screenshot](https://raw.github.com/Stazed/non-mixer-xt/main/mixer/doc/non-mixer-xt-1.0.0.png "Non-Mixer-XT Release 1.0.0")

Non-Mixer-XT is a reboot of original Non-Mixer with eXTended LV2 support, CLAP, VST(2) support and VST3* support. LV2 support includes X11, ShowInterface and External custom UI support. In addition, MIDI support with JACK timebase support and much more. The generic parameter editor has been redesigned to accommodate larger LV2 plugins, preset support and state save and restore. With version 1.1.0, CLAP support was added. With version 1.2.0, VST3 support was added. With version 1.3.0, VST(2) support was added using vestige.h by Javier Serrano Polo. Special thanks to Filipe Coelho from the Carla project, David Robillard from Jalv project, and Rui Nuno Capela from the Qtractor project.

Beginning with Release 2.0.0, the default build will use FLTK instead of NTK libraries.

Non-Mixer-XT build instructions:
--------------------------------

Dependencies :

For fltk build:
* fltk
* fltk-dev
* fltk-static.<br/>

Please note that the FLTK build uses static linking. Some distributions have a separate package for the fltk static linking. <br/>

For example:<br/>
    OpenSuse static package is named fltk-devel-static, and Fedora is named fltk-static.<br/>
    Other distributions like Arch, Debian and Ubuntu will install the static library with the development package.

For NTK build:
* ntk

Other Dependencies:
* lilv        (Optional LV2 support)
* suil        (Optional LV2 support)
* liblo
* liblo-dev
* lv2         (Optional LV2 support)
* lv2-dev     (Optional LV2 support)
* ladspa      (Optional LADSPA support)
* liblrdf     (Optional LADSPA support)
* jack2       (Need development packages also)
* zix-0       (Optional LV2 support)
* clap        (Optional CLAP support)
* pangocairo  (optional needed by some plugins)
* xfixes      (Need development packages also)
* xinerama    (Need development packages also)
* xcursor     (Need development packages also)
* jpeg        (Need development packages also)

Getting submodules (nonlib and FL):
---------------

```bash
    git submodule update --init
```

Getting ZIX
-----------

If your distribution does not have ZIX available, you can get it at:

```bash
    git clone https://github.com/drobilla/zix.git
```

Getting NTK:
------------

Your distribution may have NTK available. If not, you can get an "unofficial" updated NTK at:

```bash
    git clone https://github.com/Stazed/ntk-unofficial.git
```

Getting CLAP:
-------------

If your distribution does not have CLAP available, you can get it at:

```bash
    git clone https://github.com/free-audio/clap.git
```

Build Non-Mixer-XT:
-------------------

For cmake build:

```bash
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
```

To uninstall:

```bash
    sudo make uninstall
```

To build with NTK:

```bash
    mkdir build
    cd build
    cmake -DEnableNTK=ON ..
    make
    sudo make install
```

For package maintainers, if you are building generic binary packages to be used on different architectures,
then NativeOptimizations must be disabled:

```bash
    cmake -DNativeOptimizations=OFF ..
```
To disable VST2 support:

```bash
    cmake -DEnableVST2Support=OFF ..
```

To disable VST3 support:

```bash
    cmake -DEnableVST3Support=OFF ..
```

To disable CLAP support:

```bash
    cmake -DEnableCLAPSupport=OFF ..
```

To disable LV2 support:

```bash
    cmake -DEnableLV2Support=OFF ..
```

To disable LADSPA support:

```bash
    cmake -DEnableLADSPASupport=OFF ..
```

Controlling Non-Mixer-XT with OSC:
-------------

See [OSC.md](OSC.md)

*VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.

## SAST Tools

[PVS-Studio](https://pvs-studio.com/en/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.
