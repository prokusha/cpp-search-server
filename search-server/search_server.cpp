#include "search_server.h"
#include "paginator.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
    if(!IsValidWord(stop_words_text)) {
        throw invalid_argument("This string contains forbidden characters");
    }
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
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
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_words_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    SearchServer::Query query = SearchServer::ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string, double> empty;
    if (!document_words_freqs_.count(document_id)) {
        return empty;
    }
    return document_words_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    document_id_.erase(document_id);
    documents_.erase(document_id);
    for (auto [key, value] : word_to_document_freqs_) {
        word_to_document_freqs_[key].erase(document_id);
    }
    if (document_words_freqs_.count(document_id)) {
        document_words_freqs_.erase(document_id);
    }
}

bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= static_cast<char>(0) && c < static_cast<char>(32);
    });
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
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
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
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

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    if(!IsValidWord(text)) {
        throw invalid_argument("This string contains forbidden characters");
    }
    SearchServer::Query query;
    for (const string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}
// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
