- March 19, 2024 : Minion 2

While the major version number has increased due to significant internal changes, there are few user-visible updates in this release. The internal changes have both cleaned up a lot of old code, and made Minion usable as a library (but this work is not yet read for general usage). Some small issues were fixed which stopped Minion building on the latest versions of operating systems and compilers.

The most significant change is a overhaul of the documentation, which is now hosted at https://minion-solver.readthedocs.org/

- ** April 20, 2021 ** Minion 1.9 has been released

After a very long pause, Minion has had another release. Minion has also been moved to github: https://www.github.com/minion/minion . The license of Minion has also changed to MPL 2.0, with the agreement of all Minion authors.

Many small changes and improvements have been made since the last release. The most significant user-visible change is the introduction of parallelisation. This is designed to be most useful for very long-running problems, and will not typically speed up problems that take only a second or two. Also, some problems (such as optimisation problems), are not supported yet.

Longer notes:

- Re-implemented true Windows support
- Improved modulo when 2nd and 3rd arguments are fixed.
- Removed variable locking code.
- Added initial sphinx documentation.
- Fixed -restarts timeout and windows line endings.
- Improved propagation on w-inintervalset and notinset on bound variables.
- Added support for bound variables in shortstr2 family.
- Added -jsonsolsout command.
- Added lex minimising/maximising feature.
- Removed neighbourhoods from the system.
- Improved dumptreejson and dumptree, and included solutions in the dump tree.
- Added rust-based tester
- Changed random number generation to modern C++.
- Made build system reproducible and improved empty table constraints.
- Improved -restarts and added -restarts-multiplier and -no-restarts-bias.
- Added -valorder flag and simulated annealing support.
- Improved propagation in x*y=z, with x a boolean.
- Added more efficient x*y=z when x is a bool and made reify(gaceq) efficient for booleans.
- Added more incremental frameupdate and improved and tested frameupdate.
- Added static option to configure script --  disables -mdynamic-no-pic on MacOS.
- Added better div and dependency checking.
- Added _limit variants of SAC and speeded up SAC and SACBounds through galloping.
- Added more efficient SAC-bounds.
- Added watchelement_one_undefzero.



-   ** July 22, 2015 ** Minion 1.8 has been released

This is a small update to Minion, which mainly cleans up internal code.

Minion now uses less memory, does not the boost library any more, and is more stable on Windows.

With this release we also offically move Minion to bitbucket:

New code repository:

https://bitbucket.org/stacs_cp/minion

New website:

http://constraintmodelling.org/tools/minion/

-   **July 1, 2014** Minion 1.7 has been released.
    Changelog:

-   Improvements:
    -   element_undefzero constraint
    -   alldiff_matrix constraint
    -   Minion now defaults to supporting 64-bit instead of 32-bit
        domains
    -   Handle some cases where wdeg caused minion to crash
    -   Update to support windows 8, latest visual studio and Mac OS X
        mavericks
    -   Remove dependancy on boost (instead require a C++11 compiler)

-   Bugs which could result in incorrect answers

-   -   Correctly reject domains like {1..0}.
    -   reified haggisgac_stable would produce incorrect answers
        occasionally when
    -   reified watchelement_undefzero would produce incorrect answers
        occasionally

-   Removed functionality

-   -   Minion will now no longer take gz and bz2 files and
        auto-uncompress them. You can emulate this functionality by
        doing \'gunzip -c file.gz \| minion \--\'

```{=html}
<!-- -->
```
-   **October 27, 2013.** Minion 1.6.1, a minor bugfix release, has been
    released.
    Changelog:

-   Improvements:
    -   Fixed a slowdown in reification of equality constraints
    -   The windows binary now runs on all versions of Windows back to
        32-bit Windows XP.

-   **September 30, 2013.** Minion 1.6 has been released. We have had no
    \"wrong answer\" bugs in over a year, and Minion has been stable for
    a number of years. Therefore we decided to leap-frog 1.0 and go
    directly to 1.6 (instead of 0.16).

    Changelog:

    -   Bugfixes:
        -   Fix an occasional crash when minion exits (this only
            happened after all output was produced.)
    -   Improvements:
        -   Added an implementation of the shortstr2 propagator (as
            published in IJCAI 2013), and SHORTTUPLES, which is a
            compressed form of tuples used by ShortSTR2 and some other
            propagators.
        -   Added the command line option \'-map-long-short\' to
            automatically compress standard tuples into short tuples,
            for use with shortstr2 and haggisgac (also published in
            IJCAI 2013)
        -   Added an implementation of the mddc, negativemddc and
            str2plus propagators.
        -   All constraints now work with negative values, in particular
            pow, div and mod.
        -   Minion can now support 64-bit domains on both 32-bit and
            64-bit processors. Enable with the DOMAINS64 compile time
            option to CMake.
    -   Other changes:
        -   Many old pieces of code were cleaned up or deleted.

-   **December 17, 2012.** Minion 0.15 has been released. Merry
    Christmas!
    Changelog:

-   Improvements:
    -   The Mac version of Minion is now much faster (up to 30%), due to
        a change of compiler switches.
    -   The \'HaggisGAC\' constraint has been added. This constraint is
        allows users to specify their own constraints, using \"don\'t
        care\" values.
    -   The \'-outputCompressed\' flag has been added, which makes
        Minion try to compress an input file by removing implied
        constraints. In some situations this can lead to big speed
        increases, in others a small slowdown. We are particularly
        interested in any feedback on this option.

-   **July 27, 2012.** Minion 0.14 has been released.

    Changelog:

    -   Bugfixes:
        -   When Minion was asked to find all solutions to a problem
            that could be solved without search, it returned the one
            solution then segfaulted. This was corrected, it does not
            change the set of solutions but does now exit correctly.
    -   Improvements:
        -   Minion is now able to split the remaining search to allow
            parallel search. See the manual for further details. This
            introduces the -split command-line switch.
    -   Other changes:
        -   The -resume-file commandline switch has been removed because
            its function is no longer necessary as resume files are now
            self-contained. See the manual.

-   **May 1, 2012.** Minion 0.13 has been released.

    Changelog:

    -   Bugfixes:
        -   Two \'incorrect answer\' bugs were fixed, involving
            Booleans. These are both cases of invalid input being
            accidentally accepted.
        -   According to the manual reify(C,b) would only accept a
            BOOLEAN variable, but the code would accept other variables,
            and then produce a wrong answer. The code now works
            correctly.
        -   The \'!\' operator would accept non-boolean variables, and
            then produce incorrect results. The input is now rejected.
        -   SAC did not propagate as much as it should. This never
            produced incorrect answers, only a slightly larger search in
            some cases.
        -   Minion could produce incorrect answers when too large inputs
            were given (around 2\^31). Now sensible error message are
            produced.
        -   Previously when using a \'MINIMISING\' condition, Minion
            would print out the negative of the optimal value found.
    -   Improvements:
        -   Previously variables left out of the variable ordering were
            not branched on. While this was expected behaviour, it could
            lead to Minion printing out invalid values if these
            variables were not assigned by propagation. Now Minion
            always branches on all variables.
        -   Add flag \'-printonlyoptimal\' which prints only the optimal
            solution for optimisation problems, not the intermediate
            solutions.
        -   All constraints can now be reified.
        -   Big speed improvements to some cases of element
        -   -varorder=random now uses a dynamic random order, rather
            than a static one.
        -   Make memory allocation lazy. This is more efficent when we
            don\'t need much of it.
        -   Produce better error messages for invalid input.

-   **March 21, 2011.** Minion 0.12 has been released.

    Changelog:

    -   Bugfixes:
        -   fix visualiser to work with more recent versions of Haskell
        -   fix memory reporting on Mac OS X
        -   fix some numerical issues with CPU time reporting
        -   fixes to watched-or and watched-and constraints
    -   Misc improvements:
        -   more robust and more thorough testing
        -   better error messages when variables are used with
            incompatible constraints

-   **February 12, 2011.** Minion 0.11 has been released.

    Changelog:

    -   New features:
        -   added negation of diseq constraint
        -   added negation of watched-and constraint
        -   enable watched-or to work with bound variables
    -   Bugfixes:
        -   bug fixes with nested watched-and and watched-or
        -   bug fixes in the tester
    -   Misc improvements:
        -   improvements in documentation
        -   integrate new implementations of gcc and gccweak with many
            more compile-time options
        -   added visualisation scripts
        -   added literal tightness statistics to -instancestats
        -   Minion might work on OpenSolaris now

    Never Trust a Monster to do the Work of an Evil Scientist.

-   **March 17, 2010.** Minion 0.10 has been released.

    Changelog:

    -   Nauty is now included by default for automatic symmetry
        detection.
    -   New features:
        -   added watchvecexists_less constraint
        -   added quick lexless constraint
        -   added lightweight table constraint
        -   new flag -instancestats that will give various properties
            about the instance
        -   new flag -searchlimit to limit only time spent in search and
            not in preprocessing
    -   Bugfixes:
        -   time limits are now taken into account during preprocessing
            as well
        -   some of the generators were getting old and have been
            updated
        -   reification of unary constraints works properly now
        -   set constraints on empty sets now work
        -   several bugfixes for watched constraints
    -   Misc improvements:
        -   speed improvements and memory for the parser
        -   reduced memory requirements
        -   improved testing
        -   tidier reimplementation of the search manager
        -   bumped minimum Mac OSX version from 10.4 to 10.5

-   **March 12, 2010.**

    We are applying again to be a [Google Summer of
    Code](http://socghop.appspot.com) mentoring organisation. Please
    take a look at our [ideas page](gsoc.html).

-   **September 28, 2009.**

    We have added first case studies to the new
    [applications](cases.html) page.

-   **August 21, 2009.** Minion 0.9 has been released.

    Changelog:

    -   New features:
        -   added ability to abort and resume runs
        -   added lex\[rv\] (which achieves GAC with repeated variables)
            and lex\[quick\] constraints
        -   added -cpulimit flag to limit CPU rather than wall time
    -   Misc improvements:
        -   various source code cleanups
        -   converted the benchmarks to Minion 3 format
        -   speedup for input file parsing
        -   added support for Windows without Cygwin
        -   reduced memory usage

-   **August 19, 2009.** We are officially retiring the Subversion
    repository. The source code can be found in our new [Git
    repository](http://minion.git.sourceforge.net/git/gitweb.cgi?p=minion)
    from now on.

-   **April 06, 2009.** Minion 0.8.1 has been released.

    There are some changes and bugfixes, for details please see the
    [news
    item](https://sourceforge.net/forum/forum.php?forum_id=938577).

-   **March 19, 2009.** Our Google Summer of Code application has been
    rejected.

-   **March 11, 2009.** We have applied to be a [Google Summer of
    Code](http://code.google.com/soc/) mentoring organisation. Please
    take a look at our [ideas page](gsoc/index.html).

-   **February 18, 2009.** Minion 0.8 has been released.

    There are numerous changes and bugfixes, for details please see the
    [changelog](https://sourceforge.net/project/shownotes.php?group_id=159145&release_id=662152).

-   **June 30, 2008.** Release Candidate 1 of MINION 0.7.0 is available.
    There are numerous significant changes and also a greatly expanded
    manual including tutorial material. It can be downloaded from our
    download page.

-   **May 17, 2008.** MINION 0.6.0 has been released. The major changes
    are
    -   Bugs fixed:
    -   -check was always enabled. This slowed down problems which found
        many solutions (hundreds per second)
    -   **New features:**
    -   hamming, watchedvecexists_less, watchedvecexists_and,
        negativetable.
    -   Faster performance at root node.
    -   New algorithm for solving problems whose solutions for a group.
    -   Accept gzipped and bzip2ed input.
    -   Better error messages from invalid command line options or input
        files.
    -   -check now faster when activated.
    -   Documentation improvements.
    -   Many internal improvements and cleanups.

-   **February 8, 2008.** MINION 0.5.1 has been released. The major
    changes are
    -   The windows version of Minion now requires \'cygwin\'.
    -   The \'gacelement\' constraint has been renamed
        \'gacelement-deprecated\'.
    -   Several bugfixes and minor improvements.
    -   You can read more [at the detailed
        changelog.](https://sourceforge.net/project/shownotes.php?release_id=574899)

-   **January 17, 2008.** MINION 0.5.0 has been released. The major
    changes are
    -   considerable stability improvements
    -   several bugfixes
    -   new, enhanced input format

-   May 9, 2007 The current svn-version of MINION contains a
    **translator** that creates MINION instances from
    [Essence\'](http://www.cs.york.ac.uk/aig/constraints/AutoModel/)
    problem models. More information about this translator (including
    examples) can be found
    [here](http://www.cs.st-andrews.ac.uk/~andrea/translator/index.html).
