// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words dont work");
    }
}

void TestMinusPlusWords() {
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
        ASSERT_HINT(!words_0.empty(), "MatchDocument returned an empty list");
        ASSERT_EQUAL_HINT(words_0, test, "The document does not match the request");
    }
}

void TestRelevanseAndRating() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    {
        const auto& respone = server.FindTopDocuments("пушистый ухоженный кот"s);
        //Проверка порядка
        ASSERT_EQUAL_HINT(respone[0].id, 1, "The order of the documents does not match the expectation");
        ASSERT_EQUAL_HINT(respone[1].id, 0, "The order of the documents does not match the expectation");
        ASSERT_EQUAL_HINT(respone[2].id, 2, "The order of the documents does not match the expectation");
        //Проверка коректности значения релевантности
        const double eps = 1e-6;
        ASSERT_HINT(respone[0].relevance - 0.866434 < eps, "Relevance does not match the expectation");
        ASSERT_HINT(respone[1].relevance - 0.173287 < eps, "Relevance does not match the expectation");
        ASSERT_HINT(respone[2].relevance - 0.173287 < eps, "Relevance does not match the expectation");
        //Проверка среднего рейтинга
        ASSERT_EQUAL_HINT(respone[0].rating, 5, "Rating does not match the expectation");
        ASSERT_EQUAL_HINT(respone[1].rating, 2, "Rating does not match the expectation");
        ASSERT_EQUAL_HINT(respone[2].rating, -1, "Rating does not match the expectation");
    }
}

void TestPeredicantAndStatus() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::REMOVED, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    //Если статус не верный
    const auto& response_empty = server.FindTopDocuments("пушистый кот", DocumentStatus::IRRELEVANT);
    ASSERT(response_empty.empty());
    //При верном статусе
    const auto& response = server.FindTopDocuments("пушистый кот", DocumentStatus::REMOVED);
    ASSERT(!response.empty());
    ASSERT_EQUAL(response[0].id, 1);
    //Проверка работы передиката с рейтингом
    const auto& peredicat_rating = server.FindTopDocuments("пушистый ухоженный кот", [](int document_id, DocumentStatus status, int rating){return rating == 9;});
    ASSERT(!peredicat_rating.empty());
    ASSERT_EQUAL(peredicat_rating.size(), 1);
    ASSERT_EQUAL(peredicat_rating[0].rating, 9);
    //ID
    const auto& peredicat = server.FindTopDocuments("пушистый ухоженный кот", [](int document_id, DocumentStatus status, int rating){return document_id == 2;});
    ASSERT(!peredicat.empty());
    ASSERT_EQUAL(peredicat.size(), 1);
    ASSERT_EQUAL(peredicat[0].id, 2);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusPlusWords);
    RUN_TEST(TestMatchDocuments);
    RUN_TEST(TestRelevanseAndRating);
    RUN_TEST(TestPeredicantAndStatus);
}

