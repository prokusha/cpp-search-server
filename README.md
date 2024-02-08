# Search Server
#### Document search system by keywords.

## Features:
- Ranking of search results by TF-IDF statistical measure
- Stop word processing (not taken into account by the search engine and not affecting the search results)
- Minus-words processing (documents containing minus-words will not be included in the search results)
- Creation and processing of the query queue
- Removal of duplicate documents
- Page-by-page separation of search results
- Multi-threaded mode

## Build
Example of building a cmake project on a Linux:

    mkdir ./build
    cd ./build
    cmake ../
    cmake --build .

