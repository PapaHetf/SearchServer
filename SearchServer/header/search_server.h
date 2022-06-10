#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <string_view>
#include <future>

#include "string_processing.h"
#include "document.h"
#include "read_input_functions.h"
#include <execution>
#include "concurrent_map.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const vector<string_view>& words, DocumentStatus status);
   
class SearchServer {
public:
    
    template <typename StringContainer>
    explicit SearchServer(StringContainer stop_words); 
    explicit SearchServer(const string_view stop_words_text);                                                            
    explicit SearchServer(const string& stop_words_text);

    void AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings);

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string_view raw_query, DocumentPredicate document_predicate) const; 
    vector<Document> FindTopDocuments(const string_view raw_query, DocumentStatus status) const;  
    vector<Document> FindTopDocuments(const string_view raw_query) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> FindTopDocuments(ExecutionPolicy policy, const string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy policy, const string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy policy, const string_view raw_query) const;

    int GetDocumentCount() const;

    set<int>::const_iterator begin() const; 

    set<int>::const_iterator end() const; 

    map<string_view, double> GetWordFrequencies(int document_id) const;  

    void RemoveDocument(int document_id);  
    void RemoveDocument(std::execution::sequenced_policy, int document_id);    
    void RemoveDocument(std::execution::parallel_policy, int document_id);

    tuple<vector<string_view>, DocumentStatus> MatchDocument(const string_view, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy, const string_view raw_query, int document_id) const;  //новый метод
    tuple<vector<string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy, const string_view raw_query, int document_id) const;   //новый метод


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        set<string_view> plus_words;
        set<string_view> minus_words;
    };

    set<string, less<>> all_words_;
    set<string_view> stop_words_;
    map<string_view, map<int, double>> word_to_document_freqs_;  //<word, <id, freq>>
    map<int, map<string_view, double>> doc_id_word_freq_; //<id, <word, freq>> для метода GetWordFrequencies
    map<int, DocumentData> documents_;  
   
    set<int> document_ids_;

    bool IsStopWord(const string_view word) const;

    static bool IsValidWord(const string_view word);

    vector<string_view> SplitIntoWordsNoStop(const string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    QueryWord ParseQueryWord(const string_view text) const;
    
    Query ParseQuery(const string_view text) const;
    
    double ComputeWordInverseDocumentFreq(const string_view word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const;
};
    template <typename StringContainer>
    SearchServer::SearchServer(StringContainer stop_words) {
        if (!all_of(stop_words.begin(), stop_words.end(), IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
        for (std::string_view word : stop_words) {
            if (!word.empty()) {
                auto it_word = all_words_.insert(string(word));
                stop_words_.insert(*it_word.first);
            }
        }       
    }

    template <typename DocumentPredicate>
    vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(execution::seq, raw_query, document_predicate);
    }

    template <typename ExecutionPolicy, typename DocumentPredicate>
    vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy seq_par, const string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);     
        auto matched_documents = FindAllDocuments(seq_par, query, document_predicate);
        
        sort(seq_par, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    template <typename ExecutionPolicy>
    vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy seq_par, const string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(seq_par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }

    template <typename ExecutionPolicy>
    vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy seq_par, const string_view raw_query) const {
        return FindTopDocuments(seq_par, raw_query, DocumentStatus::ACTUAL);
    }

    template <typename DocumentPredicate>
    vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
            }
        }
       
        for (const string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }

    template <typename DocumentPredicate>
    vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy, const Query& query, DocumentPredicate document_predicate) const {
        return FindAllDocuments(query, document_predicate);
    }
    
    template <typename DocumentPredicate>
    vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const {
        ConcurrentMap<int, double> document_to_relevance_mutex(word_to_document_freqs_.size());
        for (const string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for_each(execution::par, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](auto& element) {
                const auto& document_data = documents_.at(element.first);
                if (document_predicate(element.first, document_data.status, document_data.rating)) {
                    document_to_relevance_mutex[element.first].ref_to_value += element.second * inverse_document_freq;
                }
            });
        }
        
        auto document_to_relevance = document_to_relevance_mutex.BuildOrdinaryMap();
        for (const string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for_each(execution::par, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [&](auto& element) {  
                document_to_relevance.erase(element.first); });
        }  

        vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance.size());
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
        }
        return matched_documents;
    }

    void AddDocument(SearchServer& search_server, int document_id, const string_view document, DocumentStatus status,
        const vector<int>& ratings);

    void FindTopDocuments(const SearchServer& search_server, const string_view raw_query);

    void MatchDocuments(const SearchServer& search_server, const string_view query);
