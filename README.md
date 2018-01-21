# Foxy

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

### Example

This is small example of what the server will someday be
capable of doing and can currently do. Note, the
namespaces used in the example correspond to shortened
namespace paths in Asio.

```cpp
// define a Qi rule that matches on a signed integral type
// this time, we're not interested in the value of the int
// in the URI
auto const int_rule = qi::rule<char const*>{"/" >> qi::int_};

// create another rule that matches on any alphabetic string
// this time, we actually want to use the value from the URI
// and we store it as a std::string type
auto const name_rule = qi::rule<char const*, std::string()>{"/" >> +qi::alpha};

// create the request handler for the integral rule we
// defined above
// (does not include proper error handling)
// connection = shared_ptr to HTTP connection object
// this route handler accepts a string body that the
// user can play around with
auto const int_handler =
  [](
    error_code const ec,
    http::request<http::string_body> request,
    auto connection) -> void
  {
    // target = URI from request
    auto const target = request.target();

    auto res = http::response<http::string_body>{http::status::ok, 11};
    res.body() =
      "Received the following request-target: " +
      std::string{target.begin(), target.end()};

    res.prepare_payload();

    http::write(connection->get_socket(), res);

    // required to end the session
    // it is the user's responsiblity to manage when
    // this is called
    connection->run(ec);
  };

// create another handler for the string rule defined above
auto const name_handler =
  [](
    error_code const ec,
    http::request<http::empty_body> request,
    auto connection,
    std::string const name) -> void
  {
    auto res = http::response<http::string_body>{http::status::ok, 11};
    res.body() = name;
    res.prepare_payload();
    http::write(connection->get_socket(), res);
    connection->run(ec);
  };

// create our route list structure
// note, this is currently a tuple-like structure which
// is what allows Foxy to have its static type checking
auto routes = foxy::make_routes(
  foxy::make_route<http::string_body>(int_rule, route_handler),
  foxy::make_route<http::empty_body>(name_rule, name_handler));

auto const endpoint = tcp::endpoint{ip::make_address_v4(addr), port};

// finally create the server and begin running it
std::make_shared<foxy::listener<decltype(routes)>>(
  ioc, endpoint, routes
)->run();
```

Hitting this server with some sample `curl` requests using
Windows PowerShell yields:
```
PS C:\Users\...> curl 127.0.0.1:1337/1337

StatusCode        : 200
StatusDescription : OK
Content           : {82, 101, 99, 101...}
RawContent        : HTTP/1.1 200 OK
                    Content-Length: 44

                    Received the following request-target: /1337
Headers           : {[Content-Length, 44]}
RawContentLength  : 44



PS C:\Users\...> curl 127.0.0.1:1337/himom

StatusCode        : 200
StatusDescription : OK
Content           : {104, 105, 109, 111...}
RawContent        : HTTP/1.1 200 OK
                    Content-Length: 5

                    himom
Headers           : {[Content-Length, 5]}
RawContentLength  : 5
```