#include "search_server.h"

    void PrintDocument(const Document& document) {
        cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

    void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const string_view word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
    
    SearchServer::SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)){}
    
    SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}

    void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);

        const double inv_word_count = 1.0 / words.size();
        for (const string_view word : words) {
            auto it_word = all_words_.insert(string(word));
            word_to_document_freqs_[*it_word.first][document_id] += inv_word_count;
            doc_id_word_freq_[document_id][*it_word.first] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        document_ids_.insert(document_id); 
    }

    vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }

    vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

    set<int>::const_iterator SearchServer::begin() const {       
        return document_ids_.begin();
    }
    
    set<int>::const_iterator SearchServer::end()const {
        return document_ids_.end();
    }
   
     map<string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
        static map<string_view, double> empty = {};
        return (doc_id_word_freq_.count(document_id) ? doc_id_word_freq_.at(document_id) : empty);
    }    
    
    void SearchServer::RemoveDocument(int document_id) {
        if (!document_ids_.count(document_id)) {
            return;
        }
        documents_.erase(document_id); 
        document_ids_.erase(document_id); 
        doc_id_word_freq_.erase(document_id);
        for (auto& [word, id_freq] : word_to_document_freqs_) {
            if (id_freq.count(document_id)) {
                id_freq.erase(document_id);
            }
        }
    }

    void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {  
        return RemoveDocument(document_id);
    }

    void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
        if (!document_ids_.count(document_id)) {
            return;
        }
        std::for_each(execution::par, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&document_id](auto& element) {
            if (element.second.count(document_id)) {
                element.second.erase(document_id);
            }
            });
        documents_.erase(document_id);  
        document_ids_.erase(document_id); 
        doc_id_word_freq_.erase(document_id);
    }

    tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
        if (!document_ids_.count(document_id)) {
            throw std::out_of_range("Передан несуществующий document_id "s);
        }
        const auto query = ParseQuery(raw_query);
        vector<string_view> matched_words;
        for (const string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };     
    }

    tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, const string_view raw_query, int document_id) const{
        return MatchDocument(raw_query, document_id);
    }

    tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy, const string_view raw_query, int document_id) const {
        if (!document_ids_.count(document_id)) {
            throw std::out_of_range("Передан несуществующий document_id "s);
        }
        const auto query = ParseQuery(raw_query);
        vector<string_view> matched_words;
        matched_words.reserve(query.plus_words.size());
        std::for_each(execution::par, query.plus_words.begin(), query.plus_words.end(), [&](const auto word) {
            if (word_to_document_freqs_.count(word) != 0) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
            });
        std::for_each(execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const auto word) {
            if (word_to_document_freqs_.count(word) != 0) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    return;
                }
            }
            });
            return { matched_words, documents_.at(document_id).status };
        }

    bool SearchServer::IsStopWord(const string_view word) const {
        return stop_words_.count(word) > 0;
    }

    bool SearchServer::IsValidWord(const string_view word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
        vector<string_view> words;
        for (string_view word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                string word_{ word };
                throw invalid_argument("Word "s + word_ + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
        if (text.empty()) {
            throw invalid_argument("Query word is empty"s);
        }
        string_view word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            string word_{word};
            throw invalid_argument("Query word "s + word_ + " is invalid");
        }

        return { word, is_minus, IsStopWord(word) };
    }

    SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
        Query result;
        for (const string_view word : SplitIntoWords(text)) {
            const auto query_word = SearchServer::ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                }
                else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

    double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    void AddDocument(SearchServer& search_server, int document_id, const string_view document, DocumentStatus status,
        const vector<int>& ratings) {
        try {
            search_server.AddDocument(document_id, document, status, ratings);
        }
        catch (const invalid_argument& e) {
            cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
        }
    }

    void FindTopDocuments(const SearchServer& search_server, const string_view raw_query) {
        cout << "Результаты поиска по запросу: "s << raw_query << endl;
        try {
            for (const Document& document : search_server.FindTopDocuments(raw_query)) {
                PrintDocument(document);
            }
        }
        catch (const invalid_argument& e) {
            cout << "Ошибка поиска: "s << e.what() << endl;
        }
    }

    void MatchDocuments(const SearchServer& search_server, const string_view query) {
        try {
            cout << "Матчинг документов по запросу: "s << query << endl;
            const int document_count = search_server.GetDocumentCount();
            auto it = search_server.begin();
            for (int index = 0; index < document_count; ++index) {                
                const int document_id = *it;
                const auto [words, status] = search_server.MatchDocument(query, document_id);
                PrintMatchDocumentResult(document_id, words, status);
                ++it;
            }
        }
        catch (const invalid_argument& e) {
            cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
        }
    }
    