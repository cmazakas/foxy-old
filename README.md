# foxy

Foxy is a loose attempt at creating a useful HTTP router in C++
using Boost.Beast (and hence Boost.Asio) along with the Boost
library Spirit.

Spirit is an embedded domain specific language that enables the
programmer to write human-readable parsing rules.

One of the main motivations of Foxy is to make parameterized routing
easier in C++. For example, a user would be able to register a route
using:

```
"/users/" >> int_ >> "/photos/" >> int_
```

The beauty of Spirit begins to shine here. It becomes apparent that the
route will match on something such as: `/users/1337/photos/7331` and it is
left up to the user to decide what value is returned from the parsing expression
(though some library boilerplate will need to be involved to register an
arbitrary user-defined type).
