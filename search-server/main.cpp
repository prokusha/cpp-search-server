
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words dont work");
    }
}

void TestSearchesWithTheMinusWord() {
    const int doc_id = 1;
    const string content = "what are you doing in my swamp";
    const vector<int> rating = {1, 3, 3};
    const int doc_id_0 = 2;
    const string content_0 = "never gonna give you up";
    const vector<int> rating_0 = {1, 4, 3, 7};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
        server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, rating_0);
        const auto found_docs = server.FindTopDocuments("you -swamp"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_0);
    }
}

void TestMatchDocuments() {
    const int doc_id = 69;
    const string content = "this string not make sense";
    const vector<int> rating = {1, 3, 3, 7};
    const int doc_id_0 = 70;
    const string content_0 = "this string now make sense";
    const vector<int> rating_0 = {1, 3, 3, 7};
    const vector<string> test = {"make", "sense"};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
        server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, rating_0);
        const auto [words, status] = server.MatchDocument("make sense -not"s, 69);
        const auto [words_0, status_0] = server.MatchDocument("make sense -not"s, 70);
        ASSERT_HINT(words.empty(), "Minus word got on the list");
        ASSERT_EQUAL_HINT(words_0.size(), 2, "MatchDocument returned an unexpected number of words");
        ASSERT_EQUAL_HINT(words_0, test, "The document does not match the request");
    }
}

void TestRelevanceSorting() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& response = server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL_HINT(response.size(), 3, "The number of results does not meet expectation");
        ASSERT_EQUAL_HINT(response[0].id, 1, "The order of the documents does not match the expectation");
        ASSERT_EQUAL_HINT(response[1].id, 0, "The order of the documents does not match the expectation");
        ASSERT_EQUAL_HINT(response[2].id, 2, "The order of the documents does not match the expectation");
    }
}

void TestCorrectnessOfRelevanceCalculation() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& response = server.FindTopDocuments("пушистый ухоженный кот"s);
        const double eps = 1e-6;
        ASSERT_EQUAL_HINT(response.size(), 3, "The number of results does not meet expectation");
        ASSERT_HINT(response[0].relevance - 0.866434 < eps, "Relevance does not match the expectation");
        ASSERT_HINT(response[1].relevance - 0.173287 < eps, "Relevance does not match the expectation");
        ASSERT_HINT(response[2].relevance - 0.173287 < eps, "Relevance does not match the expectation");

    }
}

void TestCorrectnessOfRatingCalculation() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& response = server.FindTopDocuments("пушистый ухоженный кот"s);
        ASSERT_EQUAL_HINT(response.size(), 3, "The number of results does not meet expectation");
        ASSERT_EQUAL_HINT(response[0].rating, (7 + 7 + 2)/3, "Rating does not match the expectation");
        ASSERT_EQUAL_HINT(response[1].rating, (8 - 3)/2, "Rating does not match the expectation");
        ASSERT_EQUAL_HINT(response[2].rating, (5 + 2 + 1 - 12)/4, "Rating does not match the expectation");
    }
}

void TestDocumentSearchByStatus() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& response_empty = server.FindTopDocuments("пушистый кот", DocumentStatus::IRRELEVANT);
        ASSERT(response_empty.empty());
    }
    {
        const auto& response = server.FindTopDocuments("пушистый кот", DocumentStatus::REMOVED);
        ASSERT_EQUAL(response.size(), 1);
        ASSERT_EQUAL(response[0].id, 1);
    }
}

void TestDocumentSearchByPredicat() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& predicat_rating = server.FindTopDocuments("пушистый ухоженный кот", [](int document_id, DocumentStatus status, int rating){return rating == 9;});
        ASSERT_EQUAL(predicat_rating.size(), 1);
        ASSERT_EQUAL(predicat_rating[0].id, 3);
    }
    {
        const auto& predicat = server.FindTopDocuments("пушистый ухоженный кот", [](int document_id, DocumentStatus status, int rating){return document_id == 2;});
        ASSERT_EQUAL(predicat.size(), 1);
        ASSERT_EQUAL(predicat[0].id, 2);
    }

}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestSearchesWithTheMinusWord);
    RUN_TEST(TestMatchDocuments);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestCorrectnessOfRelevanceCalculation);
    RUN_TEST(TestCorrectnessOfRatingCalculation);
    RUN_TEST(TestDocumentSearchByStatus);
    RUN_TEST(TestDocumentSearchByPredicat);
}
