# The dodo command line parsing library

![Dodo](https://upload.wikimedia.org/wikipedia/commons/thumb/9/94/De_Alice%27s_Abenteuer_im_Wunderland_Carroll_pic_10.jpg/800px-De_Alice%27s_Abenteuer_im_Wunderland_Carroll_pic_10.jpg)

Dodo is a C++20 command line parsing library, with support for positional arguments, named options and git style commands. The library allows for declaratively expressing a command line parser function. This is a function that maps (argc, argv) to an anonymous structure declared by the library based on the parser function declaration that contains the variables read from the command line. It supports optional arguments with default values, conditions to check if an option has a correct value, customization points for parsing user defined types, and declaring subparsers for commands like git does (git commit, git push...).

The name just follows the trend of giving animal names to software projects. And I like dodo birds. They are funny.

The design is heavily inspired by the library [Clara](https://github.com/catchorg/Clara) by Phil Nash, with modifications of my own to make it more functional.

## Compliance

The library in implemented in standard C++20 and currently (as of 12-12-2020) only builds on latest Visual Studio 2019 (Version 16.8). GCC and clang both reject the code. Clang because lambdas in unevaluated contexts have not been implemented yet. GCC because it doesn't allow declaring new types inside a decltype expression, which is necessary for the interface of the library.

This library may abuse macros and template metaprogramming to be able to offer its interface. Since compiler implementation slightly varies at corner cases of the language accross different compilers and versions, I would not advise using dodo for a commercial product where stability over a long period of time and a wide variety of platforms is a priority. If that is your case, please do consider looking for something else. [Lyra](https://github.com/bfgroup/Lyra) is a great command line parsing library in C++11 that supports the needs of most programs and is more likely to perform reliably. Consider dodo more like an exercise on new features of C++ and on pushing the expressive power of the language.

## License

Dodo is distributed under the MIT software licence. (See accompanying file LICENSE.txt)

## Features

- Header only with no external dependencies (except the std library).

- Define your interface once to get parsing, type conversions and usage strings with no redundancy.

- Composing. Each option or argument is an independent parser. Combine these to produce a compound parser.

- Define structures that will hold the parsed arguments implicitly from the parser.

- No intermediate maps of strings. No possible type errors or string typos at runtime on the user side. The data ends directly in variables of the correct type that are accessed statically.

- Typed arguments. Customization of conversion to and from string through traits and custom parsers.

- `dodo::expected<value, error>` type for error propagation.

- Validation functions for arguments that limit the values they can take.

- Automatic generation of `--help` text from the parser function.

- Command parsers, defined as a variant of the arguments taken by the parser of each of the commands.

## Unsupported features that are common in command line parsing libraries

- Grouping of short arguments.  
E.g. having `-rf` mean the same as `-r -f`

- Option name and value in different arguments.  
E.g. having `--path C://Users/foo/Desktop/` mean the same as `--path=C://Users/foo/Desktop/`

- Mixing positional arguments and named options. All positional arguments must go before all named options.  
Correct: `program.exe foo bar --opt1=6 --opt2=8`  
Wrong: `program.exe foo --opt1=6 bar --opt2=8`

## Using

Dodo is a single header library. To use the library, just `#include "dodo.hh"`

A parser for a single option can be created like this:

```cpp
constexpr auto cli = 
	dodo_Opt(int, width)
		["-w"]["--width"]
		("Width of the window in pixels.");
```

The parser can be used directly like this:

```cpp
auto const result = cli.parse(dodo::Args(argc, argv));
if (result)
	do_stuff_with(result->width);
else
	std::cerr << result.error() << '\n';
```

Parsers can be combined with `operator |`, like this:

```cpp
constexpr auto cli
	= dodo_Opt(int, width)
		["-w"]["--width"]
		("Width of the window in pixels.")
	| dodo_Opt(int, height)
		["-h"]["--height"]
		("Height of the window in pixels.")
	| dodo_Opt(bool, fullscreen)
		["--fullscreen"]
		("Start the application in fullscreen mode.")
	| dodo_Opt(std::string, username)
		["-n"]["--name"]
		("Username used to log in to the server.");
```

In this case the structure returned by parse will contain the members `width`, `height`, `fullscreen` and `username`.

```cpp
auto const args = cli.parse(dodo::Args(argc, argv));
if (!args)
{
	std::cerr << args.error() << '\n';
	exit(1);
}
auto const window_handle = open_window(args->width, args->height, args->fullscreen);
auto const connection_handle = start_session(server_url, args->username);
```

`dodo_Opt` specifies named options that start with `-`. So the program above could be invoked with the command line `-w=1920 -h=1080 -n=foobar --fullscreen=true` for example. The order of the arguments is not important since they are found by name. The user must input the arguments as `--name=value`. Inputing arguments separated by a space as `--name value` is **not** supported.

For positional arguments `dodo_Arg` is used. With `dodo_Arg`, the user does not need to type the name of the option. However, positional arguments must be given in order and before named options. For example:

```cpp
constexpr auto cli
	= dodo_Arg(int, width, "width")
		("Width of the window in pixels.")
	| dodo_Arg(int, height, "height")
		("Height of the window in pixels.")
	| dodo_Arg(std::string, username, "username")
		("Username used to log in to the server.");
```

The parser above would succesfully parse the command line `1920 1980 foobar`, however the line `1920 foobar 1080` would fail because the program expects to find the height in second position, and there is no way of converting `"foobar"` to an integer.

### Default value

By default, if a named argument is not provided by the user, the function will fail to parse. However, it is possible to provide a default value to an option. If one is provided and the user does not input an option, parsing will succeed and the option will take the default value. A default value is added to an option through the `by_default` method.

```cpp
constexpr auto cli
	= dodo_Opt(int, width)
		["-w"]["--width"]
		("Width of the window in pixels.")
		.by_default(1920)
	| dodo_Opt(int, height)
		["-h"]["--height"]
		("Height of the window in pixels.")
		.by_default(1080)
	| dodo_Opt(bool, fullscreen)
		["--fullscreen"]
		("Start the application in fullscreen mode.")
		.by_default(false)
	| dodo_Opt(std::string, username)
		["-n"]["--name"]
		("Username used to log in to the server.")
		.by_default("anonymous"sv);
```

It is important to notice that the default value does not need to be of the exact same type as the option as long as it is convertible. This allows for having constexpr parsers for string or vector types, by using types like string_view for the default value.

Positional arguments may also be given a default value. As in C++, it only makes sense for arguments with default values to be the last ones. These will take the default value if less arguments than the necessary are provided. For example, for the parser below, the line `1920 1080` would be equivalent to `1920 1080 anonymous`.

```cpp
constexpr auto cli
	= dodo_Arg(int, width, "width")
		("Width of the window in pixels.")
	| dodo_Arg(int, height, "height")
		("Height of the window in pixels.")
	| dodo_Arg(std::string, username, "username")
		("Username used to log in to the server.")
		.by_default("anonymous");
```

### Implicit value and flags

It is very common for boolean named options to be false by default, and to just name them to make them true. The user would expect `--fullscreen` to mean `--fullscreen=true` without having to type the longer version. For this, dodo allows defining an implicit value to a named option, which is the value the option will take when being named without having a value explicitly assigned as explained above. So, for the desired behavior, we would define the fullscreen option as:

```cpp
constexpr auto cli
	= dodo_Opt(bool, fullscreen)
		["--fullscreen"]
		("Start the application in fullscreen mode.")
		.by_default(false)
		.implicitly(true);
```

For the extremely common case of booleans that default to false and are implicitly true, dodo also provides the `dodo_Flag` macro to shorten.

```cpp
// Equivalent to the above
constexpr auto cli
	= dodo_Flag(fullscreen)
		["--fullscreen"]
		("Start the application in fullscreen mode.");
```

### Validation checks

It is possible to give both a named option and a positional argument a validation function that will make parsing fail if the given value does not satisfy a condition. For example:

```cpp
constexpr auto cli = 
	dodo_Opt(int, width)
		["-w"]["--width"]
		("Width of the window in pixels.")
		.check([](int width){ return width > 0 && width <= 3840; }, "Width must be between 0 and 3840 (4k).");
```

### Custom parsers

A named option and a positional argument can take a custom parser for the conversion from string to the type of the variable. A parser for the type `T` is a function that takes a `std::string_view` and returns a `std::optional<T>`. By default `dodo::parse_traits<T>::parse` is used, but each option may be given a custom parser. The example below allows for parsing booleans with `"on"` and `"off"` instead of `"true"` and `"false"`.

```cpp
constexpr auto on_off_boolean_parser = [](std::string_view text) noexcept -> std::optional<bool>
{
	if (text == "on")
		return true;
	else if (text == "off")
		return false;
	else
		return std::nullopt;
};

constexpr auto cli = dodo_Flag(some_flag)["--flag"]("Example flag.")
	.custom_parser(on_off_boolean_parser);
```

### Conversion to string, descriptions and hints.

All parsers in dodo have a to_string function that returns a helpful text that can be printed to the screen to guide the user. By convention this is done when the user requests the `help` command. This is the string that would be generated by a command. The description we have been providing to each option with the `operator ()` is used for this help text.

```cpp
constexpr auto cli
	= dodo_Opt(int, width)
		["-w"]["--width"]
		("Width of the window in pixels.")
		.by_default(1920)
	| dodo_Opt(int, height)
		["-h"]["--height"]
		("Height of the window in pixels.")
		.by_default(1080)
	| dodo_Flag(fullscreen)
		["--fullscreen"]
		("Start the application in fullscreen mode.")
	| dodo_Opt(std::string, username)
		["-n"]["--name"]
		("Username used to log in to the server.")
		.by_default("anonymous"sv);
```
```
-w, --width  <int>                     Width of the window in pixels.
                                       By default: 1920
-h, --height <int>                     Height of the window in pixels.
                                       By default: 1080
--fullscreen <bool>                    Start the application in fullscreen mode.
                                       By default: false
                                       Implicitly: true
-n, --name <std::string>               Username used to log in to the server.
                                       By default: anonymous
```

By default, dodo uses the type of the option as a hint in order to explain the user what kind of values the option can take. However, sometimes the name of a type is not helpful enough. For these situations, dodo allows for customizing the hint text of the variable.

```cpp
constexpr auto cli
	= dodo_Opt(std::string, config_file_path)
		["--config"]
		("File to load configuration from.")
		.by_default("./config.json"sv)
		.hint("path");
```
```
--config <path>                        File to load configuration from.
                                       By default: ./config.json
```

### Parse traits

`dodo::parse_traits` is a traits class that defines how a type is constructed from a string. It also defines how the type is converted to string for displaying in the help text. The library offers a set of specialization, and the user may add specializations of their own for their types. A good use case for this is being able to parse enums.

A specialization of parse traits for the type `T` must have two static functions, `parse` and `to_string`, with the following form:

```cpp
template <>
struct parse_traits<T>
{
	static constexpr std::optional<T> parse(std::string_view text) noexcept;
	static std::string to_string(T x) noexcept;
};
```

The library provides parse traits for the following types:

- `bool`
- `int16_t`
- `int32_t`
- `int64_t`
- `uint16_t`
- `uint32_t`
- `uint64_t`
- `float`
- `double`
- `long double`
- `std::string`
- `std::string_view` (Note that in this case a reference to the string in argv is kept, so ensure that the original string outlives the string_view. For command line arguments, they live for the whole program.)
- `std::vector<T, A>`

### Options of vector types

An option of a vector type takes a string with space separated list of arguments. Each of the space separated substrings is parsed as a separate element of the vector.

```cpp
enum struct Platform { windows, linux, server, ps4, xboxone, nintendo_switch };
// Assuming dodo::parse_traits<Platform> exist and do the obvious thing.

constexpr auto cli
	= dodo_Opt(std::vector<Platform>, platforms)
		["--platforms"]
		("Platforms for which to build.")
		.by_default_range(Platform::windows, Platform::ps4);
```

The above can parse a string of the form `--platforms="windows linux xboxone"` and return a vector containing `{Platform::windows, Platform::linux, Platform::xboxone}`. `by_default_range` defines the set of values the vector will contain if nothing is provided. An equivalent `implicitly_range` also exists. These functions allow the parser to be constexpr (by using a `dodo::constant_range` under the hood), which would be impossible if it had to contain a vector.

### Commands

A very common pattern for command line programs is to have a single executable that can perform more than one action. For example, the same git executable is used to pull, push, commit, branch... Git achieves this through commands. An invocation of git first selects the command and then provides the arguments for that command. Different commands take different arguments. Dodo models a command selector as a set of pairs of name and parser, which in turn returns a variant containing the result of the chosen command's parser.

A command selector is declared by joining with the `operator |` two or more `dodo::Command` objects. A single command is not a parser, since it makes no sense to select between a single command.

```cpp
constexpr auto cli = 
	dodo::Command("open-window", "Open a test window",
		dodo_Opt(int, width)["-w"]["--width"]
			("Width of the screen") |
		dodo_Opt(int, height)["-h"]["--height"]
			("Height of the screen")
	)
	| dodo::Command("fetch-url", "Fetch a URL and print the HTTP response.",
		dodo_Opt(std::string, url)["--url"]
			("Url to fetch") |
		dodo_Opt(int, max_attempts)["--max-attempts"]
			("Maximum number of attempts before failing") |
		dodo_Opt(float, timeout)["--timeout"]
			("Time to wait for response before failing the attempt")
			.by_default(10.0f)
	);
```

In order to access the results, the best option is to visit the variant. `dodo::overload` is a very convenient way of defining a visitor inline from several lambdas.

```cpp
auto const args = cli.parse(dodo::Args(argc, argv));
if (!args)
{
	std::cerr << args.error() << '\n';
	exit(1);
}

std::visit(dodo::overload
(
	[](dodo_command_type(cli, 0) const & window_args) 
	{
		open_window(window_args->width, window_args->height); 
	},
	[](dodo_command_type(cli, 1) const & url_args) 
	{
		fetch_url(url_args->url, url_args->max_attempts, url_args->timeout);
	}
), *args);
```

### Commands with shared options

Sometimes, all of the commands in a program share some arguments, even if they also take their own custom arguments. This can be achieved through the `dodo::SharedOptions` class, that can be combined with a command selector to form a command selector with shared options. The result of parsing will then contain two members. A struct with the shared arguments called `shared_arguments` and a variant with the command called `command`.

```cpp
constexpr auto cli = 
	dodo::SharedOptions(
		dodo_Opt(std::string, root_path)["--root-path"]
			("Path in which to operate.")
			.by_default("."sv)
		| dodo_Flag(dry_run) ["--dry-run"]
			("Do not perform any change. Just print what the changes would be.")
	) 
	| dodo::Command("open-window", "Open a test window",
		dodo_Opt(int, width)["-w"]["--width"]
			("Width of the screen")
		| dodo_Opt(int, height)["-h"]["--height"]
			("Height of the screen")
	)
	| dodo::Command("fetch-url", "Fetch a URL and print the HTTP response.",
		dodo_Opt(std::string, url)["--url"]
			("Url to fetch")
		| dodo_Opt(int, max_attempts)["--max-attempts"]
			("Maximum number of attempts before failing")
		| dodo_Opt(float, timeout)["--timeout"]
			("Time to wait for response before failing the attempt")
			.by_default(10.0f)
	);
	
auto const args = cli.parse(dodo::Args(argc, argv));
if (!args)
{
	std::cerr << args.error() << '\n';
	exit(1);
}

if (args->shared_options.dry_run)
{
	// ...
}

std::visit(some_visitor, args->command);
```

### The implicit command

Sometimes a parser may have a command that is assumed if no command is provided. The most common example for this is help. Let us imagine as an example a program that can either print its help text, print its version, or actually do the work it is supposed to do. There would be three possible ways of invoking this program:

- `program.exe help`
- `program.exe version`
- `program.exe --some-option=5 --other-option=www.url.com`

The third command does not have a command name. It is assumed that it is selected because none of the other commands is named. That is the implicit command. The most frequent use case is for when the name of the program already describes what the implicit command does.

```cpp
constexpr auto cli 
	= dodo::Command("help", "Print help to the screen", dodo::noop_parser<struct Help>)
	| dodo::Command("version", "Print the program's version to the screen", dodo::noop_parser<struct Version>)
	| dodo_Opt(std::string, url)["--url"]
			("Url to fetch")
	| dodo_Opt(int, max_attempts)["--max-attempts"]
		("Maximum number of attempts before failing")
	| dodo_Opt(float, timeout)["--timeout"]
		("Time to wait for response before failing the attempt")
		.by_default(10.0f);
	
auto const args = cli.parse(dodo::Args(argc, argv));
if (!args)
{
	std::cerr << args.error() << '\n';
	exit(1);
}

std::visit(dodo::overload
(
	[](dodo_command_type(cli, 0) const &) 
	{
		std::cout << cli.implicit_command.to_string() << '\n';
	},
	[](dodo_command_type(cli, 1) const &) 
	{
		std::cout << "fetch-url version 1.6.34" << '\n';
	},
	[](dodo_command_type(cli, 2) const & url_args) 
	{
		fetch_url(url_args->url, url_args->max_attempts, url_args->timeout);
	}
), *args);

std::visit(some_visitor, args->command);
```

`dodo::noop_parser` is an empty parser that always succeeds and returns an empty struct. The tag type given as template parameter allows several noop parsers to have different types as result type in order to have several of them in a variant.

### Parsing a command line string

It is possible to construct a dodo::Args object from a single string of space separated arguments. This is useful for people implementing their own editors where the user can type a command in order to invoke it. For example, Unreal Engine has a terminal that can be opened with the `~` key, where the user can type a command to have the engine execute it. This way, the user can use dodo not only for the arguments that are input to main, but also for any command inputed in string form. It supports Linux style escaping with backslash `\`, 'single quotes' and "double quotes".

There are two versions of the function, `dodo::Args::from_command_line` and `dodo::Args::from_command_line_skip_program_name`. The second will skip the first argument.

```cpp
dodo::Args const args = dodo::Args::from_command_line("some-command foo bar 'En un lugar de la Mancha' --some-value=25");
// args = {"some-command", "foo", "bar", "En un lugar de la Mancha", "--some-value=25"};

dodo::Args const args2 = dodo::Args::from_command_line_skip_program_name("some-command foo bar 'En un lugar de la Mancha' --some-value=25");
// args = {"foo", "bar", "En un lugar de la Mancha", "--some-value=25"};
```
