#ifndef CHARGE_SERVER_HANDLERS_ALGORITHM_HANDLER_HPP
#define CHARGE_SERVER_HANDLERS_ALGORITHM_HANDLER_HPP

#include "server/route_result.hpp"

namespace charge::server::handlers
{

class AlgorithmHandler {
   public:
    virtual std::vector<RouteResult> route(std::uint32_t start, std::uint32_t target, bool search_space) const = 0;
};

}

#endif
