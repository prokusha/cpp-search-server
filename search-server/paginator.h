#include <iostream>
#include <cmath>
#include <vector>
#include <string>

#include "document.h"

template <typename It>
class IteratorRange {
public:
    IteratorRange(It begin_, It end_) : it_begin_(begin_), it_end_(end_) {}
    auto begin() const {
        return it_begin_;
    }
    auto end() const {
        return it_end_;
    }
    size_t size() {
        return distance(it_begin_, it_end_);
    }
private:
    It it_begin_;
    It it_end_;
};

template <typename It>
class Paginator {
public:
    Paginator(It begin_, It end_, size_t size_) {
        size_t count = 0;
        It begin = begin_;
        for (auto i = begin_; i != end_; ++i) {
            if(count == size_) {
                pages.push_back({begin, i-1});
                begin = i;
                count = 0;
            } else {
                count++;
            }
        }
        pages.push_back({begin, end_-1});
    }
    auto begin() const {
        return pages.begin();
    }
    auto end() const {
        return pages.end();
    }
    size_t size() {
        return distance(pages.begin(), pages.end());
    }
private:
    std::vector<IteratorRange<It>> pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }";
    return out;
}

template<typename It>
std::ostream& operator<<(std::ostream& out, IteratorRange<It> doc) {
    for (auto it = doc.begin(); it != doc.end() + 1; it++)
    {
        out << *it;
    }
    return out;
}
