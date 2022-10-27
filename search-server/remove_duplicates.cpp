#include "remove_duplicates.h"

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
	std::map<std::set<std::string_view>,int> id_words;
	std::set<int> ids_delited;
	for (const int id_doc : search_server) {
		const auto& words = search_server.GetWordFrequencies(id_doc);
		std::set<std::string_view> set_words;
		for (auto& [str,freq] : words) {
			set_words.insert(str);
		}
		if (!id_words.count(set_words)) {
			id_words.emplace(set_words, id_doc);
		}
		else {
			ids_delited.insert(id_doc);
		}
	}
	for (auto i = ids_delited.begin(); i != ids_delited.end(); ++i) {
		search_server.RemoveDocument(*i);
		std::cout << "Found duplicate document id "s << *i << std::endl;
	}
}