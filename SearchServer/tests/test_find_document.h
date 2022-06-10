#pragma once
#include "generator.h"

template <typename ExecutionPolicy>
void Test(const string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& mode) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(mode, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << " : total_relevance" << endl;
}

#define TEST(mode) Test(#mode, search_server, queries, execution::mode)

void TestFindTopDocument() {
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries(generator, dictionary, 100, 70);
    
    cout << "______________Find top document___________________" << endl;
    TEST(seq);
    TEST(par);
}