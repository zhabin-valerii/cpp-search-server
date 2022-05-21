#include "document.h"

using namespace std::string_literals;


Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<<(std::ostream& out, const Document docs) {
    out << "{ "s << "document_id = "s << docs.id
        << ", relevance = "s << docs.relevance
        << ", rating = "s << docs.rating << " }"s;
    return out;
}
