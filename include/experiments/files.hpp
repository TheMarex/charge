#ifndef CHARGE_EXPERIMENTS_FILES_HPP
#define CHARGE_EXPERIMENTS_FILES_HPP

#include "common/serialization.hpp"
#include "experiments/query.hpp"

namespace charge {
namespace experiments {
namespace files {

inline auto read_queries(const std::string &path) {
    common::CSVReader<unsigned, int, int, double, double, common::csv::skip> reader(path);
    std::vector<std::string> header;
    reader.read_header(header);

    std::vector<Query> queries;
    if (header.back() == "rank") {
        common::CSVReader<unsigned, int, int, double, double, unsigned> reader(path);
        reader.read_header(header);
        assert(header.size() == 6);
        assert(header[0] == "id");
        assert(header[1] == "start");
        assert(header[2] == "target");
        assert(header[3] == "min_consumption");
        assert(header[4] == "max_consumption");
        assert(header[5] == "rank");
        std::vector<decltype(reader)::output_t> lines;
        common::serialization::read(reader, lines);
        queries.resize(lines.size());
        std::transform(lines.begin(), lines.end(), queries.begin(), [](const auto &l) {
            auto[id, source, target, min_c, max_c, rank] = l;
            return Query{source, target, min_c, max_c, id, rank};
        });
    } else {
        assert(header.size() == 5);
        assert(header[0] == "id");
        assert(header[1] == "start");
        assert(header[2] == "target");
        assert(header[3] == "min_consumption");
        assert(header[4] == "max_consumption");
        std::vector<decltype(reader)::output_t> lines;
        common::serialization::read(reader, lines);
        queries.resize(lines.size());
        std::transform(lines.begin(), lines.end(), queries.begin(), [](const auto &l) {
            auto[id, source, target, min_c, max_c, other] = l;
            return Query{source, target, min_c, max_c, id, 0};
        });
    }

    return queries;
}

inline void write_queries(const std::string &path, const std::vector<Query> &queries) {
    common::CSVWriter<unsigned, int, int, double, double, unsigned> writer(path);
    std::vector<decltype(writer)::input_t> lines(queries.size());
    std::transform(queries.begin(), queries.end(), lines.begin(), [](const auto &q) {
        return decltype(writer)::input_t{
            q.id, q.start, q.target, q.min_consumption, q.max_consumption, q.rank};
    });

    std::vector<std::string> header{"id",  "start", "target", "min_consumption", "max_consumption",
                                    "rank"};
    writer.write_header(header);
    common::serialization::write(writer, lines);
}

} // namespace files
} // namespace experiments
} // namespace charge

#endif
