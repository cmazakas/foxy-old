# Foxy

Foxy is an attempt at creating a useful HTTP router in C++
using Boost.Beast (and hence Boost.Asio) along with the Boost
library, Spirit.

Spirit is an embedded domain specific language that enables the
programmer to write human-readable rules for parsing and generation of text.
It also maintains type-safety and has many customization points for its output.

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
// Foxy is built on the back of Asio; the required executor for awaitables in
// Foxy is a simple strand type templated on the io_context's executor type
//
using strand_type = boost::asio::strand<
  boost::asio::io_context::executor_type>;

// our actual work-horse, the IO context
//
asio::io_context io;

// create a rule that'll match on something such as "/1337"
// note that this rule synthesizes an attribute
//
auto const int_rule = qi::rule<char const*, int()>("/" >> qi::int_);

// We now create a default rule that'll handle any URI we don't care about in
// particular
//
auto const not_found_rule =
  qi::rule<char const*, foxy::string_view()>(qi::raw[*qi::char_]);

// To avoid drowning in type boilerplate, we opt-in to some of Foxy's helper
// functions.
// Note, when a request handler is invoked by our router, the session's parser
// already has a complete header read into it.
// Proper semantics require reading in the rest of the message
//
auto const routes = foxy::make_routes(
  foxy::make_route(
    int_rule,
    [](
      boost::system::error_code const ec,
      std::shared_ptr<foxy::session>  session,
      int const                       user_id
    ) -> foxy::awaitable<void, strand_type>
    {
      auto& s      = *session;
      auto& buffer = s.buffer();
      auto& parser = s.parser();
      auto& stream = s.stream();

      auto token = co_await foxy::this_coro::token();

      boost::ignore_unused(
        co_await http::async_read(stream, buffer, parser, token));

      auto res = http::response<http::string_body>(http::status::ok, 11);
      res.body() = "Your user id is : " + std::to_string(user_id) + "\n";
      res.prepare_payload();

      boost::ignore_unused(co_await http::async_write(stream, res, token));

      stream.shutdown(tcp::socket::shutdown_both);
      stream.close();

      co_return;
    }
  ),
  foxy::make_route(
    not_found_rule,
    [](
      boost::system::error_code const ec,
      std::shared_ptr<foxy::session>  session,
      foxy::string_view const         target
    ) -> foxy::awaitable<void, strand_type>
    {
      auto& s      = *session;
      auto& buffer = s.buffer();
      auto& parser = s.parser();
      auto& stream = s.stream();

      auto token = co_await foxy::this_coro::token();

      boost::ignore_unused(
        co_await http::async_read(stream, buffer, parser, token));

      auto res =
        http::response<http::string_body>(http::status::not_found, 11);

      res.body() =
        "Could not find the following target : " +
        std::string(target.begin(), target.end()) +
        "\n";

      res.prepare_payload();

      boost::ignore_unused(
        co_await http::async_write(stream, res, token));

      stream.shutdown(tcp::socket::shutdown_both);
      stream.close();

      co_return;
    }
  ));

// spawn the server independently
//
foxy::co_spawn(
  io,
  [&]()
  {
    return foxy::listener(
      io,
      tcp::endpoint(asio::ip::address_v4({127, 0, 0, 1}), 1337),
      routes);
  },
  foxy::detached);

io.run();
```

Hitting this server with some sample `curl` requests using
Windows PowerShell yields:
```
PS C:\Users\cmaza> curl 127.0.0.1:1337/1337


StatusCode        : 200
StatusDescription : OK
Content           : {89, 111, 117, 114...}
RawContent        : HTTP/1.1 200 OK
                    Content-Length: 23

                    Your user id is : 1337

Headers           : {[Content-Length, 23]}
RawContentLength  : 23



PS C:\Users\cmaza> curl 127.0.0.1:1337/abasdfasdfasdfsdfewrewer
curl : Could not find the following target : /abasdfasdfasdfsdfewrewer
At line:1 char:1
+ curl 127.0.0.1:1337/abasdfasdfasdfsdfewrewer
+ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    + CategoryInfo          : InvalidOperation: (System.Net.HttpWebRequest:HttpWebRequest) [Invoke-WebRequest], WebException
    + FullyQualifiedErrorId : WebCmdletWebResponseException,Microsoft.PowerShell.Commands.InvokeWebRequestCommand
```