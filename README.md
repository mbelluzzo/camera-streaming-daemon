## Camera Streaming Daemon ##

### Compilation and installation ###

In order to compile you need the following packages:

  - GCC compiler
  - C and C++ standard libraries
  - GLib 2 (https://wiki.gnome.org/Projects/GLib)
  - Avahi 0.6 or newer (https://github.com/lathiat/avahi)
  - GStreamer 1.10 or newer (https://gstreamer.freedesktop.org/)

#### Build ####

Build system follows the usual configure/build/install cycle. Configuration is needed
to be done only once. A typical configuration is shown below:

    $ ./autogen.sh && ./configure CFLAGS='-g -O2' \
            --sysconfdir=/etc --localstatedir=/var --libdir=/usr/lib64 \
	    --prefix=/usr

Build:

    $ make

Install:

    $ make install
    $ # or... to another root directory:
    $ make DESTDIR=/tmp/root/dir install

### Running ###

Camera streaming daemon is composed of a single binary that can be run without any aditional parameter:

    $ camera-streaming-daemon

### Contributing ###

Pull-requests are accepted on GitHub. Make sure to check coding style with the
provided script in tools/checkpatch, check for memory leaks with valgrind and
test on real hardware.

#### Valgrind ####

In order to avoid seeing a lot of glib and gstreamer false positives memory leaks it is recommended to run valgrind using the followind command:
    $ GDEBUG=gc-friendly G_SLICE=always-malloc valgrind --suppressions=valgrind.supp --leak-check=full --track-origins=yes --show-possibly-lost=no --num-callers=20 ./camera-streaming-daemon
