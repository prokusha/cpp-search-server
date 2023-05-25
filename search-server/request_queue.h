#pragma once

#include "search_server.h"
#include <stack>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
        if (requests_.size() == min_in_day_) {
            requests_.pop_front();
        }
        requests_.push_back({raw_query, result});
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::string query;
        std::vector<Document> result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    // возможно, здесь вам понадобится что-то ещё
};
