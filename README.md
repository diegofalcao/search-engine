Description
=============

The program is a search engine that uses the vector model to retrieve the relevant documents based on the user's query.

The program asks you to enter a search query and then returns all documents matching the query in decreasing order of cosine similarity according to the vector space model.

Formulas:
- Formula for document tf(term frequency) is (1 + log([tf]));
- Formula for term idf is log([number of documents in the collection] / [total number of documents that contains the term]).

To generate the cossene, the program is not using the query magnitude to normalize the cossene values between 0 and 1. So, if you look in the screenshot section, the values in the 'Relevance' column are out of this range.

The document collection consists of 23155 documents which are product descriptions of dresses. This collection can be found in the 'dataset' folder.

How to compile
=============

search-engine depends on libxml2 to parse the input XML file. So, in order to compile the program you should set the path to this dependency in the gcc as follow:

gcc search-engine.c -I[path_to_libxml2_in_your_OS] -lxml2 -o search-engine

For image searching, it also depends on the img-histogram-gen project available at https://github.com/diegofalcao/img-histogram-gen. So, clone this repo in the same level of the search-engine project.

Screenshot
=============

In the picture below you can see the program running and its output for an specific query.

![search-engine-running-screenshot](https://cloud.githubusercontent.com/assets/9353351/4945300/56b4e85a-6606-11e4-8c75-bf78d701d2a3.png)

