##### compiler 

* compile

* link

* compile-file

* get-subroutine

* add-include-path

* add-system-include-path

* set-library-path

* \*have-compile\*

This is 't if the run time C compiler is available (backed by the Tiny C
Compiler), this is '() if it is not available. This feature currently is not
stable.

##### math.h

        log                Compute the natural logarithm of a float
        log10              Compute the logarith of a float in base 10
        fabs               Return the absolute
        sin                Sine of an angle (in radians)
        cos                Cosine of an angle (in radians)
        tan                Tangent of an angle (in radians)
        asin               Arc Sine
        acos               Arc Cosine
        atan               Arc Tangent
        sinh               Hyperbolic Sine
        cosh               Hyperbolic Cosine
        tanh               Hyperbolic Tangent
        exp                Eulers constant raised to the power of X
        sqrt               Squareroot of a float
        ceil               Round upwards
        floor              Round downwards
        pow                Computer X raised to the Y
        modf               Split a value into integer and fractional parts

##### Additional functions

Functions added optionally in modules.

For all the mathematical functions imported from the C math library the
arguments are converted to floating point numbers before, the functions also
all return floating point values, apart from [modf][] which returns a cons of
two floating point values.

* log

Calculate the [natural logarithm][] of a floating point value. Integers are
converted to floating point values before hand.

        # (log ARITH)
        > (log e)
        1.0
        > (log 1)
        0.0
        > (log -1)
        nan
        > (log 10)
        2.302585
        > (log 3.3)
        1.193922

* log10

Calculate a [logarithm][] with base 10. If you need a logarithm of a value 'x'
to any other base, base 'b', you can use:

        logb(x) = log10(x) / log10(b)
        or
        logb(x) = log(x) / log(b)

        # (log10 ARITH)
        > (log10 e)
        0.434294
        > (log10 1)
        0.0
        > (log10 -1)
        nan
        > (log10 10)
        1.0
        > (log10 3.3)
        0.518514

* fabs

Return the [absolute value][] of a floating point number.

        # (fabs ARITH)
        > (fabs 1)
        1
        > (fabs -2)
        2
        > (fabs 5.2)
        5.2
        > (fabs -4.0)
        4.0

* sin

Calculate the [sine][] of an angle, in radians.

        # (sin ARITH)
        > (sin pi)
        0.0
        > (sin (/ pi 2))
        1.0

* cos

Calculate the [cosine][] of an angle, in radians.

        # (cos ARITH)
        > (cos pi)
        -1.0
        > (cos (/ pi 2))
        0.0
        > (cos (/ pi 3))
        0.5

* tan

Calculate the [tangent][] of an angle, in radians.

        # (tan ARITH)
        > (tan 0)
        0
        > (tan (/ pi 3))
        1.732051

* asin

Calculate the [reciprocal of sine][], or the "cosecant".

        # (asin ARITH)
        > (asin 0)
        0.0
        > (asin 1.0)
        1.570796

* acos

Calculate the [reciprocal of cosine][], or the "secant".

        # (acos ARITH)
        > (acos -1)
        3.141593
        > (acos 0.0)
        1.570796

* atan

Calculate the [reciprocal of tangent][], or the "cotangent".

        (atan ARITH)

* sinh

Calculate the [hyperbolic sine][].

        (sinh ARITH)

* cosh

Calculate the [hyperbolic cosine][].

        (cosh ARITH)

* tanh

Calculate the [hyperbolic tangent][].

        (tanh ARITH)

* exp

Calculate the [exponential function][]. Or Euler's number raised to the
floating point number provided.

        # (exp ARITH)
        > (exp 0)
        1.0
        > (exp 1)
        2.718282
        > (exp -1)
        0.367879
        > (exp 2)
        7.389056
        > (exp 0.5)
        1.648721

* sqrt

Calculate the [square root][] of a number.

        # (sqrt ARITH)
        > (sqrt 100)
        10.0
        > (sqrt 69.0)
        8.306624
        > (sqrt -4)
        -nan
        > (sqrt 4)
        2.0

* ceil

Calculate the [ceil][] of a float, or round up to the nearest integer, from
"ceiling".

        # (ceil ARITH)
        > (ceil 4)
        4
        > (ceil 4.1)
        5.0
        > (ceil -3)
        -3.0
        > (ceil -3.1)
        -3.0

* floor

Round down to the "[floor][]", or down to the nearest integer.

        # (floor ARITH)
        > (floor 4)
        4.0
        > (floor 4.9)
        4.0
        > (floor -3)
        -3.0
        > (floor -4.9)
        -5.0

* pow

Raise the first value to the power of the second value.

        # (pow ARITH ARITH)
        > (pow 2 4)
        16.0
        > (pow -2  0.5)
        1.414214
        > (pow -2 -0.5)
        0.707107
        > (pow -2 -0.5)
        nan

* [modf][]

Split a floating point value (integers are converted to floats first) into
integer and fractional parts, returning a cons of the two values.

        # (modf ARITH)
        > (modf 2)
        (2.000000 . 0.000000)
        > (modf 2.1)
        (2.000000 . 0.100000)
        > (modf -3.5)
        (-3.000000 . -0.500000)
        > (modf -0.4)
        (-0.000000 . -0.400000)

* line-editor-mode

If the line editing library is used then this function can be used to query the
state of line editor mode, 't' representing ["Vi"][] like editing mode, 'nil'
["Emacs"][] mode. The mode can be changed by passing in 't' to set it to 'Vi'
mode and 'nil' to Emacs mode.

        (line-editor-mode)
        (line-editor-mode T-or-Nil)

* history-length

This variable controls the length of the history that the line-editing library
saves.

        # (history-length INT)
        > (history-length 1) # disable history
        > (history-length 2) # remember one entry
        > (history-length 200) # remember 199 enteries.

* clear-screen

Clear the terminal screen, in interactive mode the return value (always 't') is
printed and then the prompt if that option is set.

        # (clear-screen)
        > (clear-screen)
        t

[natural logarithm]: https://en.wikipedia.org/wiki/Natural_logarithm
[logarithm]: https://en.wikipedia.org/wiki/Logarithm
[absolute value]: https://en.wikipedia.org/wiki/Absolute_value
[sine]: https://en.wikipedia.org/wiki/Sine
[cosine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent
[tangent]:https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent
[reciprocal of sine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[reciprocal of cosine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[reciprocal of tangent]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[hyperbolic sine]: https://en.wikipedia.org/wiki/Hyperbolic_function
[hyperbolic cosine]: https://en.wikipedia.org/wiki/Hyperbolic_function
[hyperbolic tangent]: https://en.wikipedia.org/wiki/Hyperbolic_function
[exponential function]: https://en.wikipedia.org/wiki/Exponential_function
[square root]: https://en.wikipedia.org/wiki/Square_root
[ceil]: http://www.cplusplus.com/reference/cmath/ceil/
[floor]: http://www.cplusplus.com/reference/cmath/floor/
[modf]: http://www.cplusplus.com/reference/cmath/modf/

