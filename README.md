# cpp-search-server
Основа поискового сервера осуществляет хранение документов, поиск по ключевым словам и ранжирование результатов согласно статистической мере TF-IDF.
Имеет поддержку функциональности минус-слов и стоп-слов, создание и обработку очереди запросов, удаление дубликатов документов, вывод результатов постранично и возможность работы в многопоточном режиме.

## Работа с ядром поискового сервера и его основные возможности
##### SearchServer:
При создании экземпляра класса SearchServer в конструктор передаётся сртока со стоп-словами, разделёнными пробелами. Также предусмотрема возможность передавать в конструктор контейнер стоп-слов (с последовательным доступом к элементам с возможностью использования в for-range цикле).

AddDocument - добавляет документ для поиска. Аргументы метода: id документа, статус, рейтинг, документ в формате строки.

FindTopDocuments - сновной метод пооиска документов. Возвращает вектор документов, согласно соответсвию переданным ключевым словам. Результаты поиска отсортированы согласно статистической мере TF-IDF. Предусмотрена фильтрация документов по id, сатусу и рейтингу. Реальзован в однопоточной и многопоточной версии.

```CPP
std::vector<std::string> stop_words{"и"s, "но"s, "с"s};
// создаём экземпляр поискового сервера со списком стоп слов
SearchServer server(stop_words);
// добавляем документы в сервер
server.AddDocument(0, "белый кот и пушистый хвост"sv);
server.AddDocument(1, "черный пёс но желтый хвост"sv);
server.AddDocument(3, "красный попугай с длинными крыльями"sv);
std::cout << "Кол-во документов : "sv << server.GetDocumentCount() << std::endl;

// ищем документы с ключевыми словами
auto result = server.FindTopDocuments("красный попугай"sv);
std::cout << "Документы с ключевыми словами \"красный попугай\" : "sv << std::endl;
for (auto &doc : result) {
    std::cout << doc << std::endl;
}

// добавляем копию другого документа
server.AddDocument(4, "красный попугай с длинными крыльями"sv);
result = server.FindTopDocuments("черный дракон"sv);
std::cout << "Документы с ключевыми словами \"красный попугай\" : "sv << std::endl;
for (auto &doc : result) {
    std::cout << doc << std::endl;
}
// проверяем сервер на копии и удаляем их
RemoveDuplicates(server);
result = server.FindTopDocuments("красный попугай"sv);
std::cout << "Документы с ключевыми словами \"красный попугай\" после удаления копий : "sv << std::endl;
for (auto &doc : result) {
    std::cout << doc << std::endl;
}
```
##### Paginator:
Обеспечивает выдачу документов постранично.
Для взаимодействия с классом Paginator используется шаблонная функция Paginate принимающая в качестве аргументов контейнер документов и размер необходимой страницы и возвращает экземпляр класса Paginator который поддеживает возможность прохождения по содежимому  в for-range цикле.
```CPP
SearchServer server("и но или"s);
// добавляем документы в сервер
server.AddDocument(0, "первый документ"sv);
server.AddDocument(1, "второй документ"sv);
server.AddDocument(2, "третий документ"sv);
server.AddDocument(3, "четвертый документ"sv);
server.AddDocument(4, "пятый документ"sv);
        
const auto search_results = server.FindTopDocuments("документ");

// разбиваем результат поиска на страницы
size_t page_size = 2;
const auto pages = Paginate(search_results, page_size);
for (auto page : pages) {
    std::cout << page << std::endl;
    std::cout << "Page break"s << std::endl;
}
```
##### ProcessQueries и ProcessQueriesJoined:
Эти методы обеспечивают параллельное исполнение нескольких запросов к поисковой системе.
```CPP
SearchServer search_server("and with"s);

int id = 0;
for (const std::string& text : {
        "funny pet and nasty rat"s,
        "funny pet with curly hair"s,
        "funny pet and not very nasty rat"s,
        "pet with rat and rat and rat"s,
        "nasty rat with curly hair"s,}) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }

const std::vector<std::string> queries = {
    "nasty rat -not"s,
    "not very funny nasty pet"s,
    "curly hair"s
};

id = 0;
for (const auto& documents : ProcessQueries(search_server, queries)) {
    std::cout << documents.size() << " documents for query ["s << queries[static_cast<size_t>(id++)] << "]"s << std::endl;
}

cout << endl << endl;
for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
    std::cout << "Document "s << document.id << " matched with relevance "s << document.relevance << std::endl;
}
```
## Системные требования
Компилятор С++ с поддержкой стандарта C++17 или новее.
Для сборки многопоточных версий методов необходим Intel TBB.
