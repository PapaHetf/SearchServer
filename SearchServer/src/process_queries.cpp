#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries){
	std::vector<std::vector<Document>> result(queries.size());
	std::transform(execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const string query) {
		return search_server.FindTopDocuments(query);
		});
	return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries){
	std::vector<Document> result;
	for (auto& vec_docs : ProcessQueries(search_server, queries)) {
		std::transform(vec_docs.begin(), vec_docs.end(), back_inserter(result), [](Document& doc) {
			return doc;
		});
	}
	return result;
}
