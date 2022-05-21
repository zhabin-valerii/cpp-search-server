#pragma once

#include <string>
#include <vector>
#include <algorithm>

template<typename ItRange>
class IteratorRange {
public:
    IteratorRange(ItRange begin, ItRange end) :
        begin_(begin),
        end_(end),
        size_(distance(begin, end))
    {
    }

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    auto size() const {
        return size_;
    }

private:
    ItRange begin_;
    ItRange end_;
    size_t size_;
};

template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            docs_.push_back({ begin, current_page_end });

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return docs_.begin();
    }

    auto end() const {
        return docs_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> docs_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename It>
std::ostream& operator<<(std::ostream& out, const IteratorRange<It>& page) {
    for (It it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}
