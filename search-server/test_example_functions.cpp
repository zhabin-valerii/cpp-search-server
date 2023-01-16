#include "test_example_functions.h"
#include "document.h"

using namespace std::string_literals;

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::exception& e) {
        std::cout << "Error adding document: "s << document_id << ": "s << e.what() << std::endl;
    }
}
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Search results for: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            std::cout << document;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Search error: "s << e.what() << std::endl;
    }
}