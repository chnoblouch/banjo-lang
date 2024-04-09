==========
Attributes
==========

.. highlight:: banjo

exposed
=======

``exposed`` turns the symbol into a global symbol so it can be accessed from other code at link time.

link_name
=========

``link_name`` replaces the mangled function name in the compiled file with a custom symbol.
This can be used to disable name mangling for interop with other languages.

::

    @[link_name=glClearColor]
    native func clear_color(r: f32, g: f32, b: f32, a: f32);


dllexport
=========

``dllexport`` exports the symbol when building a shared library. This makes the symbol visible to applications using the library.