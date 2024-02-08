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
Additional components need to be downloaded for the build:

- ANTLR - https://www.antlr.org/download/antlr-4.13.1-complete.jar
- ANTLR Runtime - https://www.antlr.org/download/antlr4-cpp-runtime-4.13.1-source.zip

Place these components in the project folder so that the structure looks like this:
    .
    ├── antlr-4.13.1-complete.jar
    ├── antlr4_runtime
    ├── cell.cpp
    ├── cell.h
    ├── CMakeLists.txt
    ├── ...

Example of building a cmake project on a Linux:

    mkdir ./build
    cd ./build
    cmake ../
    cmake --build .

