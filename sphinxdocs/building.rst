Installing Minion
=================

The main Minion website is http://github.com/minion/minion/, and this
contains links to the download page. Currently, executables with and
without debug information are provided for Mac, Linux, and Windows.

Installation instructions for Windows
-------------------------------------

Download the Windows archive ``minion-x.y.z-windows.tar.gz`` and unpack,
you should find Minion executable ``minion.exe``. The executable should
work from the Windows command shell ``cmd.exe``.

Installation instructions for Mac
---------------------------------

Download the Mac archive ``minion-x.y.z-mac.tar.gz`` and unpack.

Installation instructions for Linux
-----------------------------------

Download the Linux archive ``minion-x.y.z-linux.tar.gz`` and unpack.

Compilation instructions
------------------------

If there is no executable which works on your computer, you can use the
source package (named ``minion-x.y.z-source.tar.gz``).

Minion requires a reasonably up-to-date C++ compiler, and Python. Python
is only used during the building process.

To compile, create a new directory for the build, and issue the
following commands:

.. code-block:: bash

   <path/to/source>/configure.py
   make

where ``<path/to/minion>`` is the path to the Minion distribution (the
directory which contains ``configure.py``).

The ``configure.py`` script takes a variety of options, which are listed
with ``--help``. We give the most important ones here:

-  ``--quick``: Make the *compiling* faster, at the cost of a slower
   executable
-  ``--debug``: Enable many debugging related options (this will make
   Minion much slower, but adds many extra internal checks)
-  ``--compiler``: Set the name of the compiler to use (this should be
   the name of the executable)

Once the ``minion`` executable is created, you can copy it anywhere
you like.


On all platforms, Minion needs to be run from a command shell so that
the output can be seen. If you go to the Minion directory in a shell and
run the executable, it should output version information and a help
message.
