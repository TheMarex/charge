#include "common/csv.hpp"
#include "common/binary.hpp"
#include "common/serialization.hpp"

#include <catch.hpp>

using namespace charge;
using namespace charge::common;

TEST_CASE("Simple reader test", "csv") {
    const std::string test_path(TEST_DIR "/data/csv_reader_test_file.csv");
    {
        BinaryWriter writer(test_path);
        const std::string data(
            "foo,bar,1337,4.0\n"
            "bla,blub,42,0.0\n");
        writer.write(*data.data(), data.size());
    }

    {
        CSVReader<std::string, std::string, int, double> csv_reader(test_path);
        std::vector<std::tuple<std::string, std::string, int, double>> lines;
        serialization::read(csv_reader, lines);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] == std::make_tuple("foo", "bar", 1337, 4.0));
        REQUIRE(lines[1] == std::make_tuple("bla", "blub", 42, 0.0));
    }

    {
        CSVReader<csv::skip, csv::skip, int, double> csv_reader(test_path);
        std::vector<std::tuple<csv::skip, csv::skip, int, double>> lines;
        serialization::read(csv_reader, lines);

        REQUIRE(lines.size() == 2);
        REQUIRE(lines[0] ==
                std::make_tuple(csv::ignored, csv::ignored, 1337, 4.0));
        REQUIRE(lines[1] ==
                std::make_tuple(csv::ignored, csv::ignored, 42, 0.0));
    }
}

TEST_CASE("Simple writer test", "csv") {
    const std::string test_path(TEST_DIR "/data/csv_writer_test_file.csv");
    {
        CSVWriter<std::string, std::string, int, double, std::tuple<std::int32_t, std::int32_t>> csv_writer(test_path);
        std::vector<std::tuple<std::string, std::string, int, double, std::tuple<std::int32_t, std::int32_t>>> lines{
            std::make_tuple("foo", "bar", 1337, 4.0, std::make_tuple(1, 2)),
            std::make_tuple("bla", "blub", 42, 0.0, std::make_tuple(3, 4))};
        serialization::write(csv_writer, lines);
    }

    {
        const std::string data_reference(
            "foo,bar,1337,4,(1 2)\n"
            "bla,blub,42,0,(3 4)\n");

        BinaryReader reader(test_path);
        std::string data(data_reference.size(), '\0');
        reader.read(*data.data(), data.size());

        REQUIRE(data == data_reference);
    }
}
