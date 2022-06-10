#pragma once
#include "generator.h"

template <typename QueriesProcessor>
void Test(string_view mark, QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
    LOG_DURATION(mark);
    const auto documents = processor(search_server, queries);
    cout << documents.size() << " : count documents" << endl;
}

#define TEST(processor) Test(#processor, processor, search_server, queries)

//распараллеливание обработки нескольких запросов
void TestProcessQueriesJoined() {
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
            cout << "_________Test process queries joined_____________" << endl;
        for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
            cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
        }
    }

    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 10000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 100'000, 10);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    const auto queries = GenerateQueries(generator, dictionary, 10'000, 7);
    TEST(ProcessQueriesJoined);
}

template <typename ExecutionPolicy>
void Test(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    const int document_count = search_server.GetDocumentCount();

    cout << document_count << ": count documents" << endl;
    
    for (int id = 0; id < document_count; ++id) {
        search_server.RemoveDocument(policy, id);
    }

    cout << search_server.GetDocumentCount() << ": document count after delete" << endl;
}

#define TEST1(mode) Test(#mode, search_server, execution::mode)

void TestRemoveDoc() {
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
            ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
        }

        const string query = "curly and funny"s;

        cout << "___________Remove document_______________" << endl;
        auto report = [&search_server, &query] {
            cout << search_server.GetDocumentCount() << " documents total, "s
                << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
        };

        report();
        // однопоточная версия
        search_server.RemoveDocument(5);
        report();
        // однопоточная версия
        search_server.RemoveDocument(execution::seq, 1);
        report();
        // многопоточная версия
        search_server.RemoveDocument(execution::par, 2);
        report();

        search_server.RemoveDocument(execution::par, 3);
        report();
    }
    mt19937 generator;

    const auto dictionary = GenerateDictionary(generator, 10000, 25);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
    }

    TEST1(seq);
    TEST1(par);
}
