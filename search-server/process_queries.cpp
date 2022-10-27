#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> documents_lists(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), documents_lists.begin(),
        [&search_server](const std::string& str) {
            return search_server.FindTopDocuments(str);
        });

    return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    std::list<Document> documents_lists;
    for (auto& docs : ProcessQueries(search_server, queries)) {
        for (auto& doc : docs) {
            documents_lists.push_back(doc);
        }
    }
    return documents_lists;
}
/*const int space_count = transform_reduce(
        execution::par,  // для демонстрации, можно и убрать
        s.begin(), s.end(),  // входной диапазон
        0,  // начальное значение
        plus<>{},  // reduce-операция (группирующая функция)
        [](char c) { return c == ' '; }  // map-операция
    );*/