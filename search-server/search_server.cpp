#include "search_server.h"
#include "string_processing.h"

#include <utility>
#include <iostream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <execution>

using namespace std::string_literals;


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}
SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0)) {
        throw std::invalid_argument("invalid id: id<0"s);
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("invalid id: id already exists"s);
    }
    buffer_[document_id] = document;
    const auto words = SplitIntoWordsNoStop(buffer_[document_id]);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        std::execution::seq,
        raw_query,
        [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word_to_document_freqs_.find(word)->first);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy, const std::string_view raw_query, int document_id) const {
    using namespace std;
    if (!documents_.count(document_id)) {
        throw out_of_range("No document with id "s + to_string(document_id));
    }
    const Query_vec query = ParseQueryVec(raw_query);
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](auto& word) {
            auto it = document_to_word_freqs_.find(document_id);
            return it != document_to_word_freqs_.end() && it->second.count(word) != 0; })) {
        return { {}, documents_.at(document_id).status };
    }
    std::vector<std::string_view> matched_words(query.plus_words.size());
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(), [this, document_id](auto word) {
            auto it = word_to_document_freqs_.find(word);
            return it != word_to_document_freqs_.end() && it->second.count(document_id) != 0;
        });
    //std::sort(matched_words.begin(), matched_words.end());
    //auto last = std::unique(matched_words.begin(), matched_words.end());
    //matched_words.erase(last - 1, matched_words.end());
    //������ ���������
    size_t index = matched_words.size();
    std::set<std::string_view> s;
    for (unsigned i = 0; i < index; ++i) {
        if (matched_words[i] != "") {
            s.insert(matched_words[i]);
        }
    }
    matched_words.resize(s.size());
    index = 0;
    for (auto w : s) {
        matched_words[index] = w; ++index;
    }
    return { matched_words, documents_.at(document_id).status };
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> dummy;
    if (count(document_ids_.begin(), document_ids_.end(), document_id) == 0) {
        return dummy;
    }
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto& [str, id] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_[str].erase(document_id);
    }
    documents_.erase(documents_.find(document_id));
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
    if (document_ids_.count(document_id)) {
        std::vector<std::string_view> words(document_to_word_freqs_.at(document_id).size());
        std::transform(document_to_word_freqs_.at(document_id).begin(),
            document_to_word_freqs_.at(document_id).end(), words.begin(),
            [](auto& word) {
                return word.first;
            });

        std::for_each(std::execution::par, words.begin(), words.end(), [this, document_id](auto& word) {
            word_to_document_freqs_.find(word)->second.erase(document_id);
            });
        documents_.erase(document_id);
        document_to_word_freqs_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

//private

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid symbols"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("query empty"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("invalid symbols in query"s);
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query_vec SearchServer::ParseQueryVec(const std::string_view text) const {
    Query_vec result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
