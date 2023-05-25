#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server) {}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}
int RequestQueue::GetNoResultRequests() const {
    int no_result = 0;
    for (const auto& [query, result] : requests_) {
        if (result.empty()) {
            no_result++;
        }
    }
    return no_result;
}
