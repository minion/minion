Variable Types
==============

Minion’s input language is purposefully designed to map exactly to
Minion’s internals. Unlike most other constraint solvers, Minion does
not internally add extra variables and decompose large complex
constraints into small parts. This provides complete control over how
problems are implemented inside Minion, but also requires understanding
how Minion works to get the best results.

For those who, quite reasonably, do not wish to get involved in such
details, ’Tailor’ abstracts away from these details, and also internally
implements a number of optimisations.

One of the most immediately confusing features of Minion are the
variable types. Rather than try to provide a "one-size-fits-all"
variable implementation, Minion provides four different ones; ``BOOL``,
``DISCRETE``, ``BOUND`` and ``SPARSEBOUND``. First we shall provide a
brief discussion of both what these variables are, and a brief
discussion of how they are implemented currently.

``BOOL``
   Variables with domain :math:`\{0,1\}`. Uses special optimised data
   structure.

``DISCRETE``
   Variables whose domain is a range of integers. Memory usage and the
   worst-case performance of most operations is O(domain size). Allows
   any subset of the domain to be represented.

``BOUND``
   Variable whose domain is a range of integers. Memory usage and the
   worst-case performance of all operations is O(1). During search, the
   domain can only be reduced by changing one of the bounds.

``SPARSEBOUND``
   Variable whose domain is an arbitrary range of integers. Otherwise
   identical to BOUND.

It appears one obvious variable implementation, ``SPARSEDISCRETE``, is
missing. This did exist in some very early versions of Minion but due to
bugs and lack of use was removed.

Some of the differences between the variable types only effect
performance, whereas some others can effect search size. We provide
these here.

#. In any problem, changing a ``BOOL`` variable to a ``DISCRETE``,
   ``BOUND`` or ``SPARSEBOUND`` variable with domain :math:`\{0,1\}`
   should not change the size of the resulting search. ``BOOL`` should
   always be fastest, followed by ``DISCRETE``, ``BOUND`` and
   ``SPARSEBOUND``.
#. A ``BOUND`` variable will in general produce a search with more nodes
   per second, but more nodes overall, than a ``DISCRETE`` variable.
#. Using ``SPARSEBOUND`` or ``BOUND`` variables with a unary constraint
   imposing the sparse domain should produce identical searches, except
   the ``SPARSEBOUND`` will be faster if the domain is sparse.

As a basic rule of thumb, Always use ``BOOL`` for Boolean domains,
``DISCRETE`` for domains of size up to around 100, and the ``BOUND``.
With ``DISCRETE`` domains, use the ``w-inset`` constraint to limit the
domain. When to use ``SPARSEBOUND`` over ``BOUND`` is harder, but
usually the choice is clear, as either the domain will be a range, or a
set like :math:`\{1,10,100,100\}`.
