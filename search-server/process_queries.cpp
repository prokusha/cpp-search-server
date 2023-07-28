#include "process_queries.h"
#include "document.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <execution>

using namespace std;

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    vector<Document> result;
    vector<vector<Document>> documents = ProcessQueries(search_server, queries);
    for (auto& document : documents) {
        result.insert(result.end(), document.begin(), document.end());
    }
    return result;
}

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.cbegin(), queries.cend(), result.begin(), [&search_server](string query){return search_server.FindTopDocuments(query);});
    return result;
}
