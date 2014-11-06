Description
=============

The program is a search engine that uses the vector model to retrieve the relevant documents based on the user's query.

The program asks you to enter a search query and then returns all documents matching the query in decreasing order of cosine similarity according to the vector space model.

Formulas:
- Formula for document tf(term frequency) is (1 + log([tf));
- Formula for term idf is log([number of documents in the collection] / [total number of documents that contains the term]).

The document collection consists of 27155 documents which are product descriptions of dresses. This collection can be found in the 'dataset' folder.

How to compile
=============

search-engine depends on libxml2 to parse the input XML file. So, in order to compile the program you should set the path to this dependency in the gcc as follow:

gcc search-engine.c -I<path_to_libxml2_in_your_os> -lxml2 -o search-engine
