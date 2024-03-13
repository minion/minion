Variable Types
==============

Minion’s input language is purposefully designed to map exactly to
Minion’s internals. Unlike most other constraint solvers, Minion does
not internally add extra variables and decompose large complex
constraints into small parts. This provides complete control over how
problems are implemented inside Minion, but also requires understanding
how Minion works to get the best results.

One of the most immediately confusing features of Minion are the
variable types. Rather than try to provide a "one-size-fits-all"
variable implementation, Minion provides four different ones; ``BOOL``,
``DISCRETE``, ``BOUND`` and ``SPARSEBOUND``. All variables have domains which are a finite of of integers. First we shall provide a
brief discussion of both what these variables are, and a brief
discussion of why their advantages and disadvantages.

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
missing. This turns out not to be useful in practice.

When we want a ``DISCRETE`` variable whose domain is not a complete range of integers, in most cases, rather than use a ``SPARSEBOUND``, it is better to instead just remove the values using the `w-inintervalset` constraint (discussed later, in the constraints).

Some of the differences between the variable types only effect
performance, whereas some others can effect search size. 

#. In any problem, changing a ``BOOL`` variable to a ``DISCRETE``,
   ``BOUND`` or ``SPARSEBOUND`` variable with domain :math:`\{0,1\}`
   will not change the size of the resulting search. ``BOOL`` should
   always be fastest, followed by ``DISCRETE``, ``BOUND`` and
   ``SPARSEBOUND``.
#. A ``BOUND`` variable will in general produce a search which explores nodes faster, but may have more search nodes, than a ``DISCRETE`` variable.
#. Using ``SPARSEBOUND`` or ``BOUND`` variables with a unary constraint
   imposing the sparse domain should produce identical searches, except
   the ``SPARSEBOUND`` will be faster if the domain is very sparse.

As a basic rule of thumb, Always use ``BOOL`` for Boolean domains,
``DISCRETE`` for domains of size up to around 1000, and then ``BOUND``.
With ``DISCRETE`` domains, use the ``w-inintervalset`` constraint to limit the
domain. When to use ``SPARSEBOUND`` over ``BOUND`` is harder, but
usually the choice is clear, as either the domain will be a range, or a very sparse list, like :math:`\{1,100,1000,10000\}`.
