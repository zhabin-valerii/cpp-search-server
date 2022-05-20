#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server) :
    search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto find_documents = search_server_.FindTopDocuments(raw_query, status);
    AddResult(find_documents);
    return find_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto find_documents = search_server_.FindTopDocuments(raw_query);
    AddResult(find_documents);
    return find_documents;
}

int RequestQueue::GetNoResultRequests() const {
    int count = 0;
    for (auto s : requests_) {
        if (!s.valid_) {
            ++count;
        }
    }
    return count;
}

void RequestQueue::AddResult(std::vector<Document> docs) {
    QueryResult result;
    result.find_documents_ = docs;
    if (docs.size() > 0) {
        result.valid_ = true;
    }
    else {
        result.valid_ = false;
    }
    requests_.push_back(result);
    if (requests_.size() > min_in_day_) {
        requests_.pop_front();
    }
}