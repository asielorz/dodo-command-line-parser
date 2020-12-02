#pragma once

#include "parse_traits.hh"
#include <concepts>
#include <type_traits>
#include <variant>

namespace clp
{

    template <typename From, typename To>
    concept explicitly_convertible_to = requires(From const from) { static_cast<To>(from); };

    namespace detail
    {
        template <template <typename ...> typename Template, typename ... Args>
        constexpr bool instantiation_of_impl(Template<Args...> const *) noexcept { return true; }
    
        template <template <auto ...> typename Template, auto ... Args>
        constexpr bool instantiation_of_impl(Template<Args...> const *) noexcept { return true; }
    
        template <template <typename ...> typename Template, typename T>
        constexpr bool instantiation_of_impl(T const *) noexcept { return false; }
    
        template <typename T>
        constexpr T * null_pointer_to = static_cast<T *>(nullptr);
    } // namespace detail

    template <typename T, template <typename ...> typename Template>
    concept instantiation_of = detail::instantiation_of_impl<Template>(detail::null_pointer_to<T>);

    template <typename T>
    concept HasValidationCheck = requires(T option, typename T::parse_result_type parse_result) {
        {option.validate(parse_result)} -> std::same_as<bool>;
    };

    template <typename Base, typename Predicate>
    struct WithCheck : public Base
    {
        constexpr explicit WithCheck(Base base, Predicate predicate_, std::string_view error_message_) : Base(base), validation_predicate(predicate_), error_message(error_message_) {}

        constexpr bool validate(typename Base::parse_result_type t) const noexcept
        {
            if constexpr (HasValidationCheck<Base>)
                if (!Base::validate(t))
                    return false;

            return validation_predicate(t._get());
        }

    private:
        Predicate validation_predicate;
        std::string_view error_message;
    };

    template <typename Base, explicitly_convertible_to<typename Base::value_type> T>
    struct WithImplicitValue : public Base
    {
        constexpr explicit WithImplicitValue(Base base, T implicit_value_) noexcept : Base(base), implicit_value(std::move(implicit_value_)) {}

        T implicit_value;
    };

    template <typename T>
    concept HasImplicitValue = requires(T option) {
        {option.implicit_value} -> explicitly_convertible_to<typename T::value_type>;
    };

    template <typename Base, explicitly_convertible_to<typename Base::value_type> T>
    struct WithDefaultValue : public Base
    {
        constexpr explicit WithDefaultValue(Base base, T default_value_) noexcept : Base(base), default_value(std::move(default_value_)) {}

        T default_value;
    };

    template <typename T>
    concept HasDefaultValue = requires(T option) {
        {option.default_value} -> explicitly_convertible_to<typename T::value_type>;
    };

    template <typename Base>
    struct WithDescription : public Base
    {
        std::string_view description;
    };

    template <typename T>
    concept HasDescription = requires(T option) {
        {option.description} -> std::convertible_to<std::string_view>;
    };

    template <typename T>
    concept Pattern = requires (T pattern, std::string_view text) { { pattern.match(text) } -> std::same_as<std::optional<std::string_view>>; };

    template <typename Base>
    struct WithPattern : public Base
    {
        constexpr explicit WithPattern(Base base, std::string_view pattern_) : Base(base), pattern(pattern_) {}

        constexpr std::optional<std::string_view> match(std::string_view text) const noexcept
        {
            if constexpr (Pattern<Base>)
            {
                std::optional<std::string_view> const matched = Base::match(text);
                if (matched)
                    return matched;
            }

            if (text.starts_with(pattern))
            {
                if (text.size() == pattern.size())
                    return "";
                else if (text[pattern.size()] == '=')
                    return text.substr(pattern.size() + 1);
            }

            return std::nullopt;
        }

        std::string patterns_to_string() const
        {
            std::string out;

            if constexpr (Pattern<Base>)
            {
                out = Base::patterns_to_string();
                out += ", ";
            }

            out += pattern;
            return out;
        }

    private:
        std::string_view pattern;
    };

    template <typename T, typename ValueType>
    concept ParserFor = requires(T t, std::string_view text) { {t(text)} -> std::same_as<std::optional<ValueType>>; };

    template <typename Base, ParserFor<typename Base::value_type> ParserFunction>
    struct WithCustomParser : public Base
    {
        constexpr explicit WithCustomParser(Base base, ParserFunction parser_function) noexcept : Base(base), custom_parser(parser_function) {}

        constexpr std::optional<typename Base::parse_result_type> parse_impl(std::string_view argument_text) const noexcept
        {
            std::optional<typename Base::value_type> value = custom_parser(argument_text);
            if (value)
                return typename Base::parse_result_type{std::move(*value)};
            else
                return std::nullopt;
        }

    private:
        ParserFunction custom_parser;
    };

    template <typename Base>
    struct WithCustomHint : public Base
    {
        constexpr explicit WithCustomHint(Base base, std::string_view hint) noexcept : Base(base), custom_hint(hint) {}

        constexpr std::string_view hint_text() const noexcept { return custom_hint; }

    private:
        std::string_view custom_hint;
    };

    template <typename T>
    concept OptionStruct = requires(T a) { { a._get() } -> std::same_as<typename T::value_type const &>; };

    template <OptionStruct T>
    struct Option
    {
        using parse_result_type = T;
        using value_type = typename T::value_type;

        constexpr explicit Option(std::string_view type_name_) : type_name(type_name_) {}

        constexpr std::optional<T> parse_impl(std::string_view argument_text) const noexcept
        {
            std::optional<value_type> value = parse_traits<value_type>::parse(argument_text);
            if (value)
                return T{std::move(*value)};
            else
                return std::nullopt;
        }

        constexpr std::string_view hint_text() const noexcept { return type_name; }

    private:
        std::string_view type_name;
    };

    template <typename T>
    concept SingleOption = requires(T option, std::string_view arg) {
        {option.match(arg)} -> std::same_as<std::optional<std::string_view>>;
        {option.parse_impl(arg)} ->std::same_as<std::optional<typename T::parse_result_type>>;
    };

    #define clp_Opt(type, var)
    #define clp_Flag(var);

    template <typename T, size_t N>
    struct constant_range
    {
        T array[N];

        template <std::constructible_from<T const *, T const *> Range>
        explicit operator Range() const noexcept { return Range(std::begin(array), std::end(array)); }
    };

    template <class T, class... U>
    constant_range(T, U...) -> constant_range<T, 1 + sizeof...(U)>;

    template <typename Base>
    struct OptionInterface : public Base
    {
        explicit constexpr OptionInterface(Base base) noexcept : Base(base) {}

        constexpr auto parse(std::string_view matched_arg) const noexcept -> std::optional<typename Base::parse_result_type>;
        constexpr auto parse(int argc, char const * const argv[]) const noexcept -> std::optional<typename Base::parse_result_type>;
        std::string to_string() const requires HasDescription<Base>;

        constexpr OptionInterface<WithDescription<Base>> operator () (std::string_view description) const noexcept requires(!HasDescription<Base>)
        {
            return OptionInterface<WithDescription<Base>>(WithDescription<Base>{*this, description});
        }

        constexpr OptionInterface<WithPattern<Base>> operator [] (std::string_view pattern) const noexcept
        {
            return OptionInterface<WithPattern<Base>>(WithPattern<Base>(*this, pattern));
        }

        template <explicitly_convertible_to<typename Base::value_type> T>
        constexpr OptionInterface<WithDefaultValue<Base, T>> default_to(T default_value) const noexcept requires(!HasDefaultValue<Base>)
        {
            return OptionInterface<WithDefaultValue<Base, T>>(WithDefaultValue<Base, T>(*this, std::move(default_value)));
        }

        template <typename ... Ts>
        constexpr auto default_to_range(Ts ... default_values) const noexcept -> decltype(default_to(constant_range{default_values...}))
        {
            return this->default_to(constant_range{default_values...});
        }

        template <explicitly_convertible_to<typename Base::value_type> T>
        constexpr OptionInterface<WithImplicitValue<Base, T>> implicitly(T implicit_value) const noexcept requires(!HasImplicitValue<Base>)
        {
            return OptionInterface<WithImplicitValue<Base, T>>(WithImplicitValue<Base, T>(*this, std::move(implicit_value)));
        }

        template <typename ... Ts>
        constexpr auto implicitly_range(Ts ... default_values) const noexcept -> decltype(implicitly(constant_range{default_values...}))
        {
            return this->implicitly(constant_range{default_values...});
        }

        template <typename Predicate>
        constexpr OptionInterface<WithCheck<Base, Predicate>> check(Predicate predicate, std::string_view error_message) const noexcept
        {
            return OptionInterface<WithCheck<Base, Predicate>>(WithCheck<Base, Predicate>(*this, predicate, error_message));
        }

        template <ParserFor<typename Base::value_type> ParserFunction>
        constexpr OptionInterface<WithCustomParser<Base, ParserFunction>> custom_parser(ParserFunction parser_function) const noexcept
        {
            return OptionInterface<WithCustomParser<Base, ParserFunction>>(WithCustomParser<Base, ParserFunction>(*this, parser_function));
        }

        constexpr OptionInterface<WithCustomHint<Base>> hint(std::string_view custom_hint) const noexcept
        {
            return OptionInterface<WithCustomHint<Base>>(WithCustomHint<Base>(*this, custom_hint));
        }
    };
    
    namespace detail
    {
        template <typename T>
        using get_parse_result_type = typename T::parse_result_type;
    }

    #define clp_parse_result_type(cli) clp::detail::get_parse_result_type<std::remove_cvref_t<decltype(cli)>>;

    template <SingleOption ... Options>
    struct Compound : private Options...
    {
        constexpr explicit Compound(Options... options) : Options(options)... {}

        struct parse_result_type : public detail::get_parse_result_type<Options>... {};

        constexpr auto parse(int argc, char const * const argv[]) const noexcept -> std::optional<parse_result_type>;
        constexpr auto parse2(int argc, char const * const argv[]) const noexcept -> std::optional<parse_result_type>;
        std::string to_string() const;

        template <SingleOption T>
        constexpr T const & access_option() const noexcept
        {
            return static_cast<T const &>(*this);
        }
    };

    template <SingleOption A, SingleOption B>         constexpr Compound<A, B> operator | (A a, B b) noexcept;
    template <SingleOption ... A, SingleOption B>     constexpr Compound<A..., B> operator | (Compound<A...> a, B b) noexcept;
    template <SingleOption A, SingleOption ... B>     constexpr Compound<A, B...> operator | (A a, Compound<B...> b) noexcept;
    template <SingleOption ... A, SingleOption ... B> constexpr Compound<A..., B...> operator | (Compound<A...> a, Compound<B...> b) noexcept;

    template <typename T>
    concept Parser = requires(T const parser, int argc, char const * const * argv) { {parser.parse(argc, argv)} -> std::same_as<std::optional<typename T::parse_result_type>>; };

    template <Parser P>
    struct Command
    {
        using parse_result_type = typename P::parse_result_type;

        explicit constexpr Command(std::string_view name_, P parser_) noexcept : name(name_), parser(parser_) {}

        constexpr bool match(std::string_view text) const noexcept { return text == name; }
        constexpr auto parse(int argc, char const * const argv[]) const noexcept;

        std::string_view name;
        P parser;
    };

    template <typename T>
    concept CommandType = requires(T t, std::string_view text, int argc, char const * const * argv) 
    {
        {t.match(text)} -> std::same_as<bool>;
        {t.parse(argc, argv)} -> std::same_as<std::optional<typename T::parse_result_type>>;
    };

    template <CommandType ... Commands>
    struct CommandSelector : private Commands...
    {
        using parse_result_type = std::variant<detail::get_parse_result_type<Commands>...>;

        constexpr explicit CommandSelector(Commands... commands) noexcept : Commands(commands)... {}

        constexpr auto parse(int argc, char const * const argv[]) const noexcept -> std::optional<parse_result_type>;

        constexpr bool match(std::string_view text) const noexcept { return (access_command<Commands>().match(text) || ...); }

        template <CommandType C>
        constexpr C const & access_command() const noexcept
        {
            return static_cast<C const &>(*this);
        }
    };

    struct ShowHelp {};

    struct Help
    {
        using parse_result_type = ShowHelp;

        constexpr bool match(std::string_view text) const noexcept { return text == "--help" || text == "-h" || text == "-?"; }
        constexpr std::optional<ShowHelp> parse(int argc, char const * const argv[]) const noexcept { static_cast<void>(argc, argv); return ShowHelp(); }
    };

    template <CommandType A, CommandType B>     constexpr CommandSelector<A, B> operator | (A a, B b) noexcept;
    template <CommandType ... A, CommandType B> constexpr CommandSelector<A..., B> operator | (CommandSelector<A...> a, B b) noexcept;
    template <CommandType A, CommandType ... B> constexpr CommandSelector<A, B...> operator | (A a, CommandSelector<B...> b) noexcept;
    template <CommandType ... A, CommandType ... B> constexpr CommandSelector<A..., B...> operator | (CommandSelector<A...> a, CommandSelector<B...> b) noexcept;

    template <
        Parser SharedOptions, 
        instantiation_of<CommandSelector> Commands
    >
    struct CommandWithSharedOptions
    {
        struct parse_result_type
        {
            typename SharedOptions::parse_result_type shared_arguments;
            typename Commands::parse_result_type command;
        };

        constexpr auto parse(int argc, char const * const argv[]) const noexcept -> std::optional<parse_result_type>;

        SharedOptions shared_options;
        Commands commands;
    };

    template <Parser P>
    struct SharedOptions
    {
        constexpr SharedOptions(P parser_) noexcept : parser(std::move(parser_)) {}
        P parser;
    };

    template <Parser P, CommandType Command>
    constexpr CommandWithSharedOptions<P, CommandSelector<Command>> operator | (SharedOptions<P> a, Command b) noexcept;

    template <Parser P, CommandType ... PreviousCommands, CommandType NewCommand>
    constexpr auto operator | (CommandWithSharedOptions<P, CommandSelector<PreviousCommands...>> a, NewCommand b) noexcept
        -> CommandWithSharedOptions<P, CommandSelector<PreviousCommands..., NewCommand>>;

} // namespace clp

#include "clp.inl"
