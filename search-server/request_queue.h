#pragma once
#include "search_server.h"

#include <string>
#include <vector>
#include <deque>


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        const auto find_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
        AddResult(find_documents);
        return find_documents;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> find_documents_;
        bool valid_;
    };
    const SearchServer& search_server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    void AddResult(std::vector<Document> docs);
};