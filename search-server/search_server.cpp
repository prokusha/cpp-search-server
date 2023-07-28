#include "search_server.h"
#include "paginator.h"
#include "string_processing.h"
#include <algorithm>
#include <exception>
#include <iterator>
#include <numeric>
#include <utility>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
    if(!IsValidWord(stop_words_text)) {
        throw invalid_argument("This string contains forbidden characters");
    }
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
                     const vector<int>& ratings) {
    if(document_id < 0) {
        throw invalid_argument("id < 0");
    }
    if(documents_.count(document_id)) {
        throw invalid_argument("This id is already occupied");
    }
    if(!IsValidWord(document)) {
        throw invalid_argument("This string contains forbidden characters");
    }
    document_id_.insert(document_id);
    const vector<string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (string_view word : words) {
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        document_words_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    auto predicate = [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        };
    return FindTopDocuments(raw_query, predicate);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    vector<string_view> matched_words;
    for (string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            return {matched_words, documents_.at(document_id).status};
        }
    }
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, string_view raw_query, int document_id) const {
    Query query = ParseQueryPar(raw_query);

    const auto& documents = document_words_freqs_.at(document_id);

    if(any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [&documents](auto word){
        return documents.count(word);
    })) {
        return {{}, documents_.at(document_id).status};
    }

    vector<string_view> matched_words(query.plus_words.size());

    vector<string_view>::iterator it = copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&documents](auto word){
        return documents.count(word);
    });

    sort(execution::par, matched_words.begin(), it);
    matched_words.erase(unique(execution::par, matched_words.begin(), it), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> empty;
    if (!document_words_freqs_.count(document_id)) {
        return empty;
    }
    return document_words_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    const auto it = find(begin(), end(), document_id);
    if (it != end()) {
        document_id_.erase(document_id);
        documents_.erase(document_id);

        vector<string_view> document_words_freqs_delete_;

        transform(document_words_freqs_[document_id].begin(), document_words_freqs_[document_id].end(), back_inserter(document_words_freqs_delete_), [](const auto& words){
            const auto& [key, value] = words;
            return key;});

        for_each(execution::seq, document_words_freqs_delete_.begin(), document_words_freqs_delete_.end(), [this, document_id](string_view key){
            word_to_document_freqs_[string(key)].erase(document_id);});
        document_words_freqs_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}
void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
    const auto it = find(begin(), end(), document_id);
    if (it != end()) {
        document_id_.erase(document_id);
        documents_.erase(document_id);

        vector<string_view> document_words_freqs_delete_;

        transform(document_words_freqs_[document_id].begin(), document_words_freqs_[document_id].end(), back_inserter(document_words_freqs_delete_), [](const auto& words){
            const auto& [key, value] = words;
            return key;});

        for_each(execution::par, document_words_freqs_delete_.begin(), document_words_freqs_delete_.end(), [this, document_id](string_view key){
            word_to_document_freqs_[string(key)].erase(document_id);});
        document_words_freqs_.erase(document_id);
    }
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= static_cast<char>(0) && c < static_cast<char>(32);
    });
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (string_view word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    bool is_minus = false;
    bool first_minus = text[0] == '-';
    bool only_minus = first_minus && text.size() == 1;
    bool two_minus = first_minus && text[1] == '-';
    // Word shouldn't be empty
    if(only_minus || two_minus) {
        throw invalid_argument("Minus words error");
    }
    if (first_minus) {
        is_minus = true;
        text = text.substr(1);
    }
    SearchServer::QueryWord result = {text, is_minus, IsStopWord(text)};
    return result;
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    if(!IsValidWord(text)) {
        throw invalid_argument("This string contains forbidden characters");
    }
    Query query;
    for (string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    sort(query.minus_words.begin(), query.minus_words.end());
    sort(query.plus_words.begin(), query.plus_words.end());

    query.minus_words.erase(unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    query.plus_words.erase(unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());

    return query;
}

SearchServer::Query SearchServer::ParseQueryPar(string_view text) const {
    if(!IsValidWord(text)) {
        throw invalid_argument("This string contains forbidden characters");
    }
    Query query;
    for (string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}
