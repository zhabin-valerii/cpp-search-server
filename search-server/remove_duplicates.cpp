#include "remove_duplicates.h"

using namespace std::string_literals;

void RemoveDuplicates(SearchServer& search_server) {
	std::map<int, std::set<std::string>> id_words;
	for (const int id_doc : search_server) {
		const auto& words = search_server.GetWordFrequencies(id_doc);
		std::set<std::string> set_words;
		for (auto& word : words) {
			set_words.insert(word.first);
		}
		id_words.emplace(id_doc,set_words);
	}
	std::set<int> ids_delited;
	for (auto i = id_words.begin(); i != id_words.end(); ++i) {
		for (auto j = id_words.begin(); j != id_words.end();) {
			++j;
			if (j != id_words.end()&&i->first<j->first) {
				if (i->second == j->second) {
					search_server.RemoveDocument(j->first);
					ids_delited.insert(j->first);
				}
			}
		}
	}
	for (auto id : ids_delited) {
		std::cout << "Found duplicate document id "s << id << std::endl;
	}
}