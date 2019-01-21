
#include "seasocks/Response.h"
#include "internal/ConcreteResponse.h"
#include <catch2/catch.hpp>

using namespace seasocks;
using namespace Catch::Matchers;

TEST_CASE("unhandled response returns nullptr", "[ResponseTests]") {
    const auto resp = Response::unhandled();
    CHECK(resp == nullptr);
}

TEST_CASE("not found response", "[ResponseTests]") {
    const auto resp = std::dynamic_pointer_cast<ConcreteResponse>(Response::notFound());

    CHECK(resp->responseCode() == ResponseCode::NotFound);
    CHECK_THAT(resp->payload(), Equals("Not found"));
    CHECK_THAT(resp->contentType(), Equals("text/plain"));
    CHECK(resp->getAdditionalHeaders().empty() == true);
    CHECK(resp->keepConnectionAlive() == false);
}

TEST_CASE("error response", "[ResponseTests]") {
    const std::string msg("no reason");
    const auto resp = std::dynamic_pointer_cast<ConcreteResponse>(Response::error(ResponseCode::Forbidden, msg));

    CHECK(resp->responseCode() == ResponseCode::Forbidden);
    CHECK_THAT(resp->payload(), Equals(msg));
    CHECK_THAT(resp->contentType(), Equals("text/plain"));
    CHECK(resp->getAdditionalHeaders().empty() == true);
    CHECK(resp->keepConnectionAlive() == false);
}

TEST_CASE("text response", "[ResponseTests]") {
    const std::string msg("abc");
    const auto resp = std::dynamic_pointer_cast<ConcreteResponse>(Response::textResponse(msg));

    CHECK(resp->responseCode() == ResponseCode::Ok);
    CHECK_THAT(resp->payload(), Equals(msg));
    CHECK_THAT(resp->contentType(), Equals("text/plain"));
    CHECK(resp->getAdditionalHeaders().empty() == true);
    CHECK(resp->keepConnectionAlive() == true);
}

TEST_CASE("json response", "[ResponseTests]") {
    const std::string msg("abc");
    const auto resp = std::dynamic_pointer_cast<ConcreteResponse>(Response::jsonResponse(msg));

    CHECK(resp->responseCode() == ResponseCode::Ok);
    CHECK_THAT(resp->payload(), Equals(msg));
    CHECK_THAT(resp->contentType(), Equals("application/json"));
    CHECK(resp->getAdditionalHeaders().empty() == true);
    CHECK(resp->keepConnectionAlive() == true);
}

TEST_CASE("html response", "[ResponseTests]") {
    const std::string msg("abc");
    const auto resp = std::dynamic_pointer_cast<ConcreteResponse>(Response::htmlResponse(msg));

    CHECK(resp->responseCode() == ResponseCode::Ok);
    CHECK_THAT(resp->payload(), Equals(msg));
    CHECK_THAT(resp->contentType(), Equals("text/html"));
    CHECK(resp->getAdditionalHeaders().empty() == true);
    CHECK(resp->keepConnectionAlive() == true);
}
