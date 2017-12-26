#include "server/http.hpp"

#include "common/files.hpp"

#include "ev/files.hpp"
#include "ev/limited_tradeoff_function.hpp"

#include <catch.hpp>

#include <algorithm>
#include <thread>
#include <vector>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace charge;
using namespace charge::server;

std::string tcp_send_data(const std::string &hostname, int portno,
                          const std::string &data) {
    int sockfd, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;

    std::string buffer(1024 * 4, '\0');

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Error opening socket");
    }

    server = gethostbyname(hostname.c_str());
    if (server == NULL) {
        throw std::runtime_error("Error no such host");
    }

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
          server->h_length);
    serveraddr.sin_port = htons(portno);

    if (connect(sockfd, reinterpret_cast<sockaddr *>(&serveraddr),
                sizeof(serveraddr)) < 0) {
        throw std::runtime_error("Error connecting");
    }

    auto writen_length = write(sockfd, data.c_str(), data.size());
    if (writen_length < 0) {
        throw std::runtime_error("Error writing data");
    }
    assert(writen_length == data.size());

    auto response_length = read(sockfd, buffer.data(), buffer.size());
    if (response_length < 0) {
        throw std::runtime_error("Error reading response");
    }
    buffer.resize(response_length);
    close(sockfd);

    return buffer;
}

std::string http_send_get(const std::string &hostname, int portno,
                          const std::string &url) {
    std::string get_request =
        "GET " + url + " HTTP/1.1\r\nHost: " + hostname + "\r\n";

    auto response = tcp_send_data(hostname, portno, get_request);

    std::string expected_header = "HTTP/1.1 200 OK";
    if (response.substr(0, expected_header.size()) == expected_header) {
        throw std::runtime_error("Unexpected response: " + response);
    }
    auto body_begin = response.find("\r\n\r\n");
    return response.substr(body_begin + 4);
}

TEST_CASE("Start http server", "[HTTP]") {
    const std::string base = TEST_DIR "/data/http_test";
    {
        //                        8
        //                        |
        //   0-----1-----2----4---7---9
        //    \   /      |    |   |
        //     \ /       |    |   10
        //      3        5----6
        common::WeightedGraph<ev::LimitedTradeoffFunction> graph{
            11, std::vector<
                    common::WeightedGraph<ev::LimitedTradeoffFunction>::edge_t>{
                    {0, 1, ev::make_constant(0.1, 1)},
                    {0, 3, ev::make_constant(0.1, 1)},
                    {1, 0, ev::make_constant(0.2, 1)},
                    {1, 2, ev::make_constant(0.2, 1)},
                    {1, 3, ev::make_constant(0.2, 1)},
                    {2, 1, ev::make_constant(0.3, 1)},
                    {2, 4, ev::make_constant(0.3, 1)},
                    {2, 5, ev::make_constant(0.3, 1)},
                    {3, 0, ev::make_constant(0.4, 1)},
                    {3, 1, ev::make_constant(0.4, 1)},
                    {4, 2, ev::make_constant(0.5, 1)},
                    {4, 6, ev::make_constant(0.5, 1)},
                    {4, 7, ev::make_constant(0.5, 1)},
                    {5, 2, ev::make_constant(0.6, 1)},
                    {5, 6, ev::make_constant(2.0, 1)},
                    {6, 4, ev::make_constant(0.7, 1)},
                    {6, 5, ev::make_constant(0.7, 1)},
                    {7, 4, ev::make_constant(0.8, 1)},
                    {7, 8, ev::make_constant(0.8, 1)},
                    {7, 9, ev::make_constant(0.8, 1)},
                    {7, 10, ev::make_constant(0.8, 1)},
                    {8, 7, ev::make_constant(0.9, 1)},
                    {9, 7, ev::make_constant(1.0, 1)},
                    {10, 7, ev::make_constant(1.1, 1)}}};
        common::files::write_weighted_graph(base, graph);

        std::vector<common::Coordinate> coords{
            common::Coordinate::from_floating(0.0, 0.0),  // 0
            common::Coordinate::from_floating(1.0, 0.0),  // 1
            common::Coordinate::from_floating(2.0, 0.0),  // 2
            common::Coordinate::from_floating(3.0, 0.0),  // 3
            common::Coordinate::from_floating(4.0, 0.0),  // 4
            common::Coordinate::from_floating(0.0, 1.0),  // 5
            common::Coordinate::from_floating(1.0, 1.0),  // 6
            common::Coordinate::from_floating(2.0, 1.0),  // 7
            common::Coordinate::from_floating(3.0, 1.0),  // 8
            common::Coordinate::from_floating(4.0, 1.0),  // 9
            common::Coordinate::from_floating(5.0, 1.0),  // 10
        };
        common::files::write_coordinates(base, coords);

        std::vector<std::int32_t> heights{
            0,   // 0
            1,   // 1
            2,   // 2
            3,   // 3
            2,   // 4
            1,   // 5
            0,   // 6
            -1,  // 7
            -2,  // 8
            0,   // 9
            3,   // 10
        };
        common::files::write_heights(base, heights);

        std::vector<double> chargers{
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        };
        ev::files::write_charger(base, chargers);
    }

    std::vector<std::string> queries = {
        "/route?algorithm=fastest_bi_dijkstra&start=0&target=1",
        "/route?algorithm=fastest_bi_dijkstra&start=0&target=9",
        "/nearest?lon=0.0&lat=0.0",
        "/nearest?lon=5.0&lat=1.0",
    };

    const Charge charge(base, 16000.0f);
    HTTPServer server(charge, 5000);

    usleep(10000);

    std::vector<std::string> results(queries.size());
    std::transform(queries.begin(), queries.end(), results.begin(),
                   [&](const auto &query) {
                       return http_send_get("localhost", 5000, query);
                   });

    std::vector<std::string> references = {
        "{\"routes\":[{\"consumptions\":[0.0,1],\"durations\":[0.0,0.1],"
        "\"geometry\":[[0.0,0.0],[1,0.0]],\"heights\":[0,1],\"lengths\":[0.0,"
        "111226.300000001],\"max_speeds\":[4004146.80000003],\"path\":[0,1],"
        "\"search_space\":{\"features\":[],\"type\":\"FeatureCollection\"},"
        "\"tradeoff\":[{\"a\":0,\"b\":0.0,\"c\":1,\"d\":0.0,\"max_duration\":0."
        "1,\"min_duration\":0.1}]}],\"start\":0,\"target\":1}",

        "{\"routes\":[{\"consumptions\":[0.0,1,2,3,4,5],\"durations\":[0.0,0.1,"
        "0.3,0.6,1.1,1.9],\"geometry\":[[0.0,0.0],[1,0.0],[2,0.0],[4,0.0],[2,1]"
        ",[4,1]],\"heights\":[0,1,2,2,-1,0],\"lengths\":[0.0,111226.300000001,"
        "222452.600000002,444905.200000004,693604.664952611,916023.380904319],"
        "\"max_speeds\":[4004146.80000003,2002073.40000002,2669431.20000002,"
        "1790636.14765878,1000884.22178268],\"path\":[0,1,2,4,7,9],\"search_"
        "space\":{\"features\":[],\"type\":\"FeatureCollection\"},\"tradeoff\":"
        "[{\"a\":0,\"b\":0.0,\"c\":5,\"d\":0.0,\"max_duration\":1.9,\"min_"
        "duration\":1.9}]}],\"start\":0,\"target\":9}",

        "{\"coordinate\":[0.0,0.0],\"id\":0}",
        "{\"coordinate\":[5,1],\"id\":10}"

    };

    CHECK(results[0] == references[0]);
    CHECK(results[1] == references[1]);
    CHECK(results[2] == references[2]);
    CHECK(results[3] == references[3]);

    server.stop();
}
