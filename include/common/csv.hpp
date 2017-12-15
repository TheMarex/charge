#ifndef CHARGE_COMMON_CSV_PARSER_HPP
#define CHARGE_COMMON_CSV_PARSER_HPP

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace charge::common {

namespace csv {
struct skip final {
    bool operator==(const skip &) const { return true; }
};
inline skip ignored;
}

namespace detail {
template <std::size_t Column, class... Types>
void parse_column(std::vector<std::string> &columns, std::tuple<Types...> &tuple) {
    // clang-format off
    if constexpr(std::is_same_v<double, typename std::tuple_element<Column, std::tuple<Types...>>::type>) {
        std::get<Column>(tuple) = std::stof(columns[Column]);
    } else if constexpr(std::is_same_v<int, typename std::tuple_element<Column, std::tuple<Types...>>::type>) {
        std::get<Column>(tuple) = std::stoi(columns[Column]);
    } else if constexpr(std::is_same_v<unsigned, typename std::tuple_element<Column, std::tuple<Types...>>::type>) {
        std::get<Column>(tuple) = std::stoi(columns[Column]);
    } else if constexpr(std::is_same_v<std::string, typename std::tuple_element<Column, std::tuple<Types...>>::type>) {
        std::get<Column>(tuple).swap(columns[Column]);
    } else if constexpr(std::is_same_v<csv::skip, typename std::tuple_element<Column, std::tuple<Types...>>::type>) {
        std::get<Column>(tuple) = csv::ignored;
    } else {
        throw std::runtime_error("Can't parse type.");
    }
    // clang-format on

    // clang-format off
    if constexpr(Column + 1 < std::tuple_size<std::tuple<Types...>>::value) {
        parse_column<Column + 1>(columns, tuple);
    }
    // clang-format on
}

template <class... Types> std::tuple<Types...> parse_columns(std::vector<std::string> &columns) {
    std::tuple<Types...> tuple;

    // resursively fills the tuple
    parse_column<0>(columns, tuple);

    return tuple;
}

template<typename T>
auto to_csv_column(const T& value);

template <typename T> struct stream_printer {
    friend auto &operator<<(std::ostream &ss, const stream_printer& printer) {
        ss << printer.value;
        return ss;
    }

    T value;
};

template <typename T1, typename T2> struct stream_printer<std::tuple<T1, T2>> {
    friend auto &operator<<(std::ostream &ss, const stream_printer& other) {
        ss << "(" << std::get<0>(other.value) << " " <<std::get<1>(other.value) << ")";
        return ss;
    }

    std::tuple<T1, T2> value;
};

template <typename T> struct stream_printer<std::vector<T>> {
    friend auto &operator<<(std::ostream &ss, const stream_printer& other) {
        if (other.value.empty())
            return ss;

        for (auto iter = other.value.begin(); iter != std::prev(other.value.end()); ++iter)
            ss << to_csv_column(*iter) << ",";
        ss << to_csv_column(*std::prev(other.value.end()));
        return ss;
    }

    std::vector<T> value;
};

template <> struct stream_printer<csv::skip> {
    friend auto &operator<<(std::ostream &ss, const stream_printer&) { return ss; }
};

template<typename T>
auto to_csv_column(const T& value) { return stream_printer<typename std::remove_reference<T>::type>{value}; }

template <std::size_t Column, class... Types>
void append_column(std::stringstream &ss, const std::tuple<Types...> &tuple,
                   const char *delimiter) {
    // clang-format off
    if constexpr(Column != 0) {
        ss << delimiter;
    }
    // clang-format on

    ss << to_csv_column(std::get<Column>(tuple));

    // clang-format off
    if constexpr(Column + 1 < std::tuple_size<std::tuple<Types...>>::value) {
        append_column<Column + 1>(ss, tuple, delimiter);
    }
    // clang-format on
}

template <class... Types>
std::string join_tuple(const std::tuple<Types...> &tuple, const char *delimiter) {
    std::stringstream ss;

    append_column<0>(ss, tuple, delimiter);

    return ss.str();
}
}

template <class... Types> class CSVReader {
  public:
    using output_t = std::tuple<Types...>;

    CSVReader(const std::string &path, const char *delimiter = ",")
        : stream(path), delimiter(delimiter) {
        stream.exceptions(std::ifstream::badbit);
    }

    bool read_header(std::vector<std::string> &header) {
        std::string line;
        if (std::getline(stream, line)) {
            boost::split(header, line, boost::is_any_of(delimiter));
            return true;
        }

        return false;
    }

    bool read(output_t &value) {
        std::string line;
        if (std::getline(stream, line)) {
            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_any_of(delimiter));

            // ignore empty lines
            if (tokens.size() == 0)
                return true;

            if (tokens.size() != std::tuple_size<output_t>::value) {
                throw std::runtime_error("Could not parse line: \"" + line +
                                         "\". Number of columns is " +
                                         std::to_string(tokens.size()));
            }

            value = detail::parse_columns<Types...>(tokens);
            return true;
        }

        return false;
    }

  private:
    const char *delimiter;
    std::ifstream stream;
};

template <class... Types> class CSVWriter {
  public:
    using input_t = std::tuple<Types...>;

    CSVWriter(const std::string &path, const char *delimiter = ",")
        : stream(path), delimiter(delimiter) {
        stream.exceptions(std::ifstream::badbit);
    }

    void write_header(const std::vector<std::string> &header) {
        stream << boost::algorithm::join(header, delimiter) << std::endl;
        ;
    }

    void write(const input_t &value) {
        stream << detail::join_tuple<Types...>(value, delimiter) << std::endl;
    }

  private:
    const char *delimiter;
    std::ofstream stream;
};
}

#endif
