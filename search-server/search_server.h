#pragma once
#include "document.h"
#include "string_processing.h"


#include <algorithm>
#include <string>
#include <vector>
#include <tuple>
#include <set>
#include <map>
#include <stdexcept>
#include <execution>

const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr auto INACCURACY = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy, const std::string_view raw_query, int document_id) const;

    auto begin() {
        return document_ids_.begin();
    }

    auto end() {
        return document_ids_.end();
    }

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string data;
    };
    const std::set<std::string,std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    template <typename StringContainer>
    std::set<std::string,std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings);

    static bool IsValidWord(const std::string_view word);


    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    struct Query_vec {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query_vec ParseQueryVec(const std::string_view text) const;
    Query ParseQuery(const std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < INACCURACY) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename StringContainer>
std::set<std::string, std::less<>> SearchServer::MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    using namespace std::string_literals;
    std::set<std::string,std::less<>> non_empty_strings;
    for (const std::string_view str : strings) {
        if (!IsValidWord(str)) {
            std::string error = "invalid symbols in stop words : "s + std::string(str);
            throw std::invalid_argument(error);
        }
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}