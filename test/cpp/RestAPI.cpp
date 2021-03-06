#include "TestFrameworks.h"

#include "Json.h"
#include "RestApi.h"
#include "Storage.h"

using namespace fix;
using namespace std::string_literals;

class StorageMock : public Storage {
public:
  MAKE_CONST_MOCK0(selectMaxIssueID, unsigned(), override);
  MAKE_MOCK1(insertIssueIncreasedID, Json(Json const& requestedIssue), override);
  MAKE_CONST_MOCK0(allIssues, std::vector<Json>(), override);
  MAKE_CONST_MOCK1(issue, Json(unsigned id), override);
};

TEST_CASE( "Bad URIs give 404" ) {
  StorageMock storage;
  RestApi api{storage};

  auto response = api.process("/some/bad/uri", "POST", "");

  REQUIRE(response.content == Json{});
  REQUIRE(response.httpCode == 404);
}


TEST_CASE( "Creating an issue...", "[issue]" ) {
  StorageMock storage;
  RestApi api{storage};

  std::string uri = "/issue/new";
  std::string method = "POST";
  auto requestedIssue = Json {
      { "data", {
        { "summary", "New Issue" },
        { "description", "Some lengthy text here to describe the issue" }
      }}
  };
  auto request = requestedIssue.dump();


  SECTION("... inserts the issue into storage and returns it with its data if the input is correct.") {
    auto expectedIssue = requestedIssue;

    expectedIssue["data"]["ID"] = 1;
    REQUIRE_CALL(storage, insertIssueIncreasedID(requestedIssue)).RETURN(expectedIssue);
    auto response = api.process(uri, method, request);

    REQUIRE(response.content == expectedIssue);
    REQUIRE(response.httpCode == 200);
  }


  SECTION("... returns status 400 with corresponding error message if...") {
    RestApi::Response response = {Json{}, 0};

    SECTION("... the request can not be parsed.") {
      response = api.process(uri, method, "{ some non-json string");
    }

    SECTION("... the request does not contain parseable issue data.") {
      response = api.process(uri, method, "{}");
    }

    CHECK(response.content == Json{});
    CHECK(response.httpCode == 400);
  }

  SECTION("... returns status 405 if the method is wrong.") {
    auto response = api.process(uri, "GET", "");
    CHECK(response.httpCode == 405);
    CHECK(response.content == Json{});
  }
}

TEST_CASE( "Listing all issues...", "[issue]" ) {
  StorageMock storage;
  RestApi api{storage};

  std::string uri = "/issue/list";
  std::string method = "GET";

  Json expectedResponse = Json{
      { "data", {
          {"issues", Json::array()}
      }}
  };

  SECTION( "... returns an empty list if no issues are in the storage" ) {
    REQUIRE_CALL(storage, allIssues()).RETURN(std::vector<Json>{});
    auto response = api.process(uri, method, "");
    CHECK(response.httpCode == 200);
    CHECK(response.content == expectedResponse);
  }

  SECTION( "... returns a list with all issues present in the storage" ) {
    Json issue1{
        { "data", {
            { "ID", 1 },
            { "summary", "New Issue" },
            { "description", "Some lengthy text here to describe the issue" }
        }}
    };
    Json issue2{
        { "data", {
            { "ID", 2 },
            { "summary", "Another Issue" },
            { "description", "That's it then" }
        }}
    };

    auto expected_issues = Json::array({
      { { "ID", 1 },
        { "summary", "New Issue" }
      },
      { { "ID", 2 },
        { "summary", "Another Issue" }
      }
    });

    expectedResponse["data"]["issues"] = expected_issues;

    REQUIRE_CALL(storage, allIssues()).RETURN(std::vector<Json>{issue1, issue2});
    auto response = api.process(uri, method, "");
    CHECK(response.httpCode == 200);
    CHECK(response.content == expectedResponse);

  }


  SECTION("... returns status 405 if the method is wrong.") {
    auto response = api.process(uri, "POST", "");
    CHECK(response.httpCode == 405);
    CHECK(response.content == Json{});
  }
}

TEST_CASE( "Querying an issue by ID...", "[issue]" ) {
  StorageMock storage;
  RestApi api{storage};

  std::string uri = "/issue/32";
  std::string method = "GET";

  SECTION( "... returns the JSON object describing the issue if it is in the storage" ) {
    auto requestedIssue = Json {
        { "data", {
                      { "ID", 32 },
                      { "summary", "New Issue" },
                      { "description", "Some lengthy text here to describe the issue" }
                  }}
    };
    REQUIRE_CALL(storage, issue(32)).RETURN(requestedIssue);
    auto response = api.process(uri, method, "");
    CHECK(response.httpCode == 200);
    CHECK(response.content == requestedIssue);
  }

  SECTION( "... returns a 404 error if no issue with the ID is in the storage" ) {
    uri = "/issue/12";
    REQUIRE_CALL(storage, issue(12)).RETURN(Json{});
    auto response = api.process(uri, method, "");
    CHECK(response.httpCode == 404);
    CHECK(response.content == Json{});
  }

  SECTION("... returns status 405 if the method is wrong.") {
    auto response = api.process(uri, "POST", "");
    CHECK(response.httpCode == 405);
    CHECK(response.content == Json{});
  }
}
