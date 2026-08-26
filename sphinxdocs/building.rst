Installing Minion
=================

Minion is available from https://github.com/minion/minion/, which has
prebuilt executables for Linux, macOS and Windows on its releases page,
and the source. On all platforms Minion is run from a command shell, so
that its output can be seen.

Compilation instructions
------------------------

Minion requires a reasonably up-to-date C++ compiler, and Python. Python
is only used during the building process.

To compile, create a new directory for the build, and issue the
following commands:

.. code-block:: bash

   <path/to/source>/configure.py
   make

where ``<path/to/source>`` is the path to the Minion distribution (the
directory which contains ``configure.py``).

The ``configure.py`` script takes a variety of options, which are listed
with ``--help``. We give the most important ones here:

-  ``--quick``: Make the *compiling* faster, at the cost of a slower
   executable
-  ``--debug``: Enable many debugging related options (this will make
   Minion much slower, but adds many extra internal checks)
-  ``--compiler``: Set the name of the compiler to use (this should be
   the name of the executable)
-  ``--assertions``: Enable internal assertion checks (implied by ``--debug``)
-  ``--domains64``: Use 64-bit integers for domain values (default is 32-bit)
-  ``--wdeg yes|no``: Enable or disable weighted degree heuristics (default yes)
-  ``--constraints <list>``: Build only the named constraints, reducing binary
   size and compile time. Takes a comma-separated list of constraint names.
-  ``--buildsystem <name>``: Set the build system. Supported values are
   ``make`` (default), ``sh`` (a plain shell script), ``tup``, and ``bat``
   (Windows batch).
-  ``--static``: Build a statically linked executable.
-  ``--extraflags <flags>``: Append extra compiler flags.
-  ``--setflags <flags>``: Override all compiler flags.
-  ``--unoptimised``: Disable compiler optimisation.
-  ``--profile``: Add profiling compiler flags.

Once the ``minion`` executable is created, you can copy it anywhere
you like.


Run it with no arguments and it prints a usage message; ``minion --help``
lists every switch, and ``minion --version`` reports the version and the
git revision it was built from.

Rust interface (minion-sys)
---------------------------

Minion also ships with `minion-sys`, a Rust crate that wraps the Minion
solver as a library, so that a Rust program can build a model and run
search in-process rather than by invoking the binary. See
``minion-sys/README.md`` in the source tree.
