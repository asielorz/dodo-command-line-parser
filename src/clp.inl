namespace clp
{

    template <typename ParseResultType, typename T>
    constexpr ParseResultType make_parse_result(T t) noexcept
    {
        using ValueType = typename ParseResultType::value_type;
        return ParseResultType{ValueType(t)};
    }

    template <typename Base>
    constexpr auto OptionInterface<Base>::parse(int argc, char const * const argv[]) const noexcept -> std::optional<typename Base::parse_result_type>
    {
        for (int i = 0; i < argc; ++i)
        {
            std::optional<std::string_view> const matched = this->match(argv[i]);
            if (matched)
            {
                if constexpr (HasImplicitValue<Base>)
                    if (matched->empty())
                        return make_parse_result<typename Base::parse_result_type>(this->implicit_value);

                auto parse_result = this->parse_impl(*matched);

                if constexpr (HasValidationCheck<Base>)
                    if (parse_result && !this->validate(*parse_result))
                        return std::nullopt;

                return parse_result;
            }
        }

        if constexpr (HasDefaultValue<Base>)
            return make_parse_result<typename Base::parse_result_type>(this->default_value);
        else
            return std::nullopt;
    }

    #undef clp_Opt
    #define clp_Opt(type, var) clp::OptionInterface(clp::Option<std::remove_pointer_t<decltype([](){        \
        struct OptionTypeImpl                                                                               \
        {                                                                                                   \
            using value_type = type;                                                                        \
            type var;                                                                                       \
            constexpr type const & _get() const noexcept { return var; }                                    \
        };                                                                                                  \
        return static_cast<OptionTypeImpl *>(nullptr);                                                      \
    }())>>(#type))

    #undef clp_Flag
    #define clp_Flag(var) clp_Opt(bool, var).default_to(false).implicitly(true)

    template <SingleOption ... Options>
    constexpr auto Compound<Options...>::parse(int argc, char const * const argv[]) const noexcept
    {
        using parse_result_type = typename Compound<Options...>::parse_result_type;

        return [...opts = this->access_option<Options>().parse(argc, argv)]() mutable noexcept -> std::optional<parse_result_type>
        {
            if (!(opts && ...))
                return std::nullopt;
            else
                return parse_result_type{std::move(*opts)...};
        }();
    }

    template <SingleOption A, SingleOption B>
    constexpr Compound<A, B> operator | (A a, B b) noexcept
    {
        return Compound<A, B>(a, b);
    }

    template <SingleOption ... A, SingleOption B>
    constexpr Compound<A..., B> operator | (Compound<A...> a, B b) noexcept
    {
        return Compound<A..., B>(a.template access_option<A>()..., b);
    }

    template <SingleOption A, SingleOption ... B>
    constexpr Compound<A, B...> operator | (A a, Compound<B...> b) noexcept
    {
        return Compound<A, B...>(a, b.template access_option<B>()...);
    }

    template <SingleOption ... A, SingleOption ... B>
    constexpr Compound<A..., B...> operator | (Compound<A...> a, Compound<B...> b) noexcept
    {
        return Compound<A..., B...>(
            a.template access_option<A>()...,
            b.template access_option<B>()...);
    }

    namespace detail
    {
        template <CommandType Next, CommandType ... Rest, CommandType ... Commands>
        constexpr auto parse_impl(CommandSelector<Commands...> const & commands, int argc, char const * const argv[]) noexcept
            -> std::optional<typename CommandSelector<Commands...>::parse_result_type>
        {
            Next const & next = commands.access_command<Next>();
            if (next.match(argv[0]))
            {
                auto result = next.parse(argc, argv);
                if (!result)
                    return std::nullopt;
                else
                    return std::move(*result);
            }
            else
            {
                if constexpr (sizeof...(Rest) > 0)
                    return clp::detail::parse_impl<Rest...>(commands, argc, argv);
                else
                    return std::nullopt;
            }
        }
    }

    template <CommandType ... Commands>
    constexpr auto CommandSelector<Commands...>::parse(int argc, char const * const argv[]) const noexcept -> std::optional<parse_result_type>
    {
        if (argc <= 0)
            return std::nullopt;

        return clp::detail::parse_impl<Commands...>(*this, argc, argv);
    }

    template <Parser P>
    constexpr auto Command<P>::parse(int argc, char const * const argv[]) const noexcept
    { 
        return parser.parse(argc - 1, argv + 1);
    }

    template <CommandType A, CommandType B>
    constexpr CommandSelector<A, B> operator | (A a, B b) noexcept
    {
        return CommandSelector<A, B>(a, b);
    }

    template <CommandType ... A, CommandType B>
    constexpr CommandSelector<A..., B> operator | (CommandSelector<A...> a, B b) noexcept
    {
        return CommandSelector<A..., B>(a.template access_command<A>()..., b);
    }

    template <CommandType A, CommandType ... B>
    constexpr CommandSelector<A, B...> operator | (A a, CommandSelector<B...> b) noexcept
    {
        return CommandSelector<A, B...>(a, b.template access_command<B>()...);
    }

    template <CommandType ... A, CommandType ... B>
    constexpr CommandSelector<A..., B...> operator | (CommandSelector<A...> a, CommandSelector<B...> b) noexcept
    {
        return CommandSelector<A..., B...>(a.template access_command<A>()..., b.template access_command<B>()...);
    }

    template <typename Base>
    std::string OptionInterface<Base>::to_string() const requires HasDescription<Base>
    {
        constexpr int column_width = 40;

        std::string out = this->patterns_to_string();
        out += " <";
        out += this->hint_text();
        out += ">";
        while (out.size() < column_width) out.push_back(' ');
        out += this->description;

        if constexpr (HasDefaultValue<Base>)
        {
            out += '\n';
            for (int i = 0; i < column_width; ++i) out.push_back(' ');
            out += "By default: ";
            out += ::to_string(this->default_value);
        }

        if constexpr (HasImplicitValue<Base>)
        {
            out += '\n';
            for (int i = 0; i < column_width; ++i) out.push_back(' ');
            out += "Implicitly: ";
            out += ::to_string(this->implicit_value);
        }

        out += '\n';
        return out;
    }

    template <SingleOption ... Options>
    std::string Compound<Options...>::to_string() const
    {
        return (this->template access_option<Options>().to_string() + ...);
    }

} // namespace clp

template <typename T, size_t N>
struct parse_traits<clp::constant_range<T, N>>
{
    static std::string to_string(clp::constant_range<T, N> const & r)
    {
        std::string result;

        for (T const & t : r.array)
        {
            result += parse_traits<T>::to_string(t);
            result += ' ';
        }

        // Remove last space.
        if (!result.empty())
            result.pop_back();

        return result;
    }
};
