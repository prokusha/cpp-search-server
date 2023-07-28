#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> duplicat;
    set<set<string_view>> documents_words_freq_;
    for (const int document_id : search_server) {
        const auto& words_freq = search_server.GetWordFrequencies(document_id);
        set<string_view> words;
        for (const auto& [word, freq] : words_freq) {
            words.insert(word);
        }
        if (documents_words_freq_.count(words)) {
            duplicat.insert(document_id);
        } else {
            documents_words_freq_.insert(words);
        }
    }

    for (const auto& id : duplicat) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id " << id << endl;
    }
}

