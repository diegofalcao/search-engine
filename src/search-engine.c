#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <libxml/parser.h>
#include <ctype.h>
#include <time.h>

#include "search-engine.h"

Term *vocabulary[NUM_OF_TERMS];
Entry *entries[NUM_OF_DOCUMENTS];

unsigned int sumValues(const char string[]) {

    unsigned int sum = 0;

    while (*string != 0) { /* end of the string */
        sum += *string;

        string++;
    }

    return sum;
}

/*
 * Generate a hash based on a name
 */
unsigned int generateHash(const char termName[]) {

    return (sumValues(termName) % NUM_OF_TERMS);
}

/*
 * Generate a hash based on a fileName
 */
unsigned int generateHashFilename(const char fileName[]) {

    return (sumValues(fileName) % NUM_OF_DOCUMENTS);
}

/*
 * Generate a hash based on a id
 */
unsigned int generateHashById(const char id[]) {
    int value = atoi(id) - 1;

    if (strcmp(&id[strlen(id) - 1], "P") == 0) {
        value = 17688 + value;
    }

    return value;
}

/*
 * Converts a string to lower case
 */
void tolowerStr(char *str) {
    int i = 0;

    while (str[i]) {
        str[i] = tolower(str[i]);

        i++;
    }
}

/*
 * Delete special characters from the start and end of the string
 */
void deleteSpecialCharactersFromStartEndStr(char *str) {

    char ch[2] = {'.', ','};

    int size = strlen(str);

    int i,j = 0;

    for (i = 0; i < 2; i++) {
        if (str[0] == ch[i]) {
            str[0] = '\0';
        } else if(str[size - 1] == ch[i]) {
            str[size - 1] = '\0';
        }
    }
}

void normalizeTerm(char *str) {
    deleteSpecialCharactersFromStartEndStr(str);
    tolowerStr(str);
}

/*
 * Generate the term IDF (Inverse Document Frequency)
 */
double generateTermIDF(Term *term) {
    double result;

    if (term->totalNumOfDocuments == 0) {
        result = 0;
    } else {
        result = log((double)NUM_OF_DOCUMENTS / term->totalNumOfDocuments);
    }

    return result;
}

/*
 * Generate the document TF (Term Frequency)
 */
double getDocumentTF(Document *document) {
    double result;

    if (document->tf == 0) {
        result = 0;
    } else {
        result = 1 + log(document->tf);
    }

    return result;
}
/*
 * Generate Doc magnitude and vocabulary terms IDF for the terms of the vocabulary
 */
void generateDocMagnitudeAndVocabularyTermsIDF() {

    int j;

    for (j = 0; j < NUM_OF_DOCUMENTS; j++) {
        Entry *entryTmp = entries[j];

        if (entryTmp == NULL) {
            continue;
        }

        int i;

        for (i = 0; i < NUM_OF_TERMS; i++) {
            if (vocabulary[i] == NULL) {
                continue;
            }

            Term *term = vocabulary[i];

            while (term != NULL) {

                term->idf = generateTermIDF(term);

                Document *documentTmp = term->document;

                for (; documentTmp != NULL; documentTmp = documentTmp->next) {
                    if (strcmp(documentTmp->id, entryTmp->documentId) == 0) {

                        /* This multiplication is to improve the performance of the pow (tf-idf, 2) */
                        entryTmp->magnitude += (term->idf * getDocumentTF(documentTmp)) * (term->idf * getDocumentTF(documentTmp));

                        break;
                    }
                }

                term = term->next;
            }
        }

        entryTmp->magnitude = sqrt(entryTmp->magnitude);
    }
}

/*
 * This method index all the terms based on a hash function
 */
unsigned int indexTerm(const char documentId[], const char documentName[], char termName[]) {
    unsigned int position;

    normalizeTerm(termName);

    position = generateHash(termName);

    Term *term = (Term *)malloc(sizeof(Term));
    term->name = termName;
    term->document = NULL;
    term->next = NULL;
    term->totalNumOfOccurrences = 1;
    term->totalNumOfDocuments = 1;

    Document *document = (Document *)malloc(sizeof(Document));
    document->id = documentId;
    document->name = documentName;
    document->term = termName;
    document->tf = 1;
    document->next = NULL;

    /* Empty position, just include the new term here */
    if (vocabulary[position] == NULL) {
        term->document = document;

        vocabulary[position] = term;
    /* Same term was found! So, let's use the 'next' document attribute to add the new occurrence of
    the term in another document or increase the numOfOccurrence of the document that contains the term */
    } else {
        Term *termTmp = vocabulary[position];

        Term *firstTermTmp = (Term *) malloc(sizeof(Term));

        firstTermTmp = termTmp;

        bool hasTerm = false;

        for (; termTmp != NULL; termTmp = termTmp->next) {

            if (strcmp(termTmp->name, termName) == 0) {

                Document *documentTmp = termTmp->document;

                termTmp->totalNumOfOccurrences++;

                hasTerm = true;

                if (strcmp(documentTmp->id, documentId) == 0) {
                    documentTmp->tf++;

                    break;
                }

                Document *documentAux = termTmp->document;
                document->next = documentAux;

                termTmp->document = document;

                termTmp->totalNumOfDocuments++;
            }
        }

        /* if the term names are different but the position generated by the hash function is the same,
        an collision has occurred. So, we need to add this new term    as the first term of this position */
        if (!hasTerm) {
            term->next = firstTermTmp->next;
            term->document = document;

            firstTermTmp->next = term;
        }

    }

    return position;
}

/*
 * Index all the terms of the description attribute  based on a hash function
 */
void indexEntry(Product *product) {
    char *cpDocumentId = (char *)malloc(NUM_OF_DOCUMENTS * sizeof(char));

    strcpy(cpDocumentId, product->id);

    Entry *entry = malloc(sizeof (Entry));
    entry->documentId = malloc(NUM_OF_DOCUMENTS * sizeof(char));
    entry->documentName = malloc(DOCUMENT_NAME_SIZE * sizeof(char));

    strcpy(entry->documentId, cpDocumentId);
    strcpy(entry->documentName, product->imgFileName);

    int position = generateHashById(cpDocumentId);

    entries[position] = entry;

    char *cpDescription = (char *)malloc(500 * sizeof(char));

    strcpy(cpDescription, product->description);

    char *token = strtok(cpDescription, " ");

    char *cpToken = NULL;

    char *cpDocumentNameTmp = (char *)malloc(DOCUMENT_NAME_SIZE * sizeof(char));
    strcpy(cpDocumentNameTmp, product->imgFileName);

    char *cpDocumentIdTmp = (char *)malloc(NUM_OF_DOCUMENTS * sizeof(char));
    strcpy(cpDocumentIdTmp, cpDocumentId);

    while (token != NULL) {
        cpToken = (char *) malloc(20 * sizeof(char));

        strcpy(cpToken, token);

        indexTerm(cpDocumentIdTmp, cpDocumentNameTmp, cpToken);

        token = strtok(NULL, " ");
    }
}

/*
 * Sort entries collection by the value of cossene in descending order
 */
void sortEntriesByCosDesc(Entry *values[]) {

    int i,j;

    Entry *aux;

    for (i = 0; i < NUM_OF_DOCUMENTS; ++i) {
        for (j = i + 1; j < NUM_OF_DOCUMENTS; ++j) {
            if(values[i] == NULL || values[j] == NULL) {
                continue;
            }

            double magnitude1 = values[i]->sum / values[i]->magnitude;
            double magnitude2 = values[j]->sum / values[j]->magnitude;

            if (magnitude1 < magnitude2) {
                aux =  values[i];
                values[i] = values[j];
                values[j] = aux;
            }
        }
    }
}

/*
 * Get the term frequency of an term in the query
 */
double getQueryTF(char *query, char *term) {
    char *p = query;
    int len1 = strlen(term);

    int count = 0;

    while ((p = strstr(p, term)) != NULL && p != term) {
        count++;
        p += len1;
    }

    double result = 0;

    if (count == 0) {
        result = 0;
    } else {
        result = 1 + log(count);
    }

    return result;
}

/*
 * Search term occurrences using the inverted index
 */
void searchByVectorModel(char termName[]) {
    clock_t begin, end;
    double searchTimeSpent;

    begin = clock();

    Entry *results[NUM_OF_DOCUMENTS] = {};

    char *cpTermName = (char *) malloc(QUERY_SIZE * sizeof(char));

    normalizeTerm(termName);

    strcpy(cpTermName, termName);

    char *token = strtok(termName, " ");

    int countSearchResult = 0;

    while (token != NULL) {
        char *cpToken = (char *)malloc(sizeof(token));

        strcpy(cpToken, token);

        unsigned int position = generateHash(cpToken);

        Term *term = vocabulary[position];

        while (term != NULL && strcmp(term->name, cpToken) != 0) {
            term = term->next;
        }

        if (term == NULL) {
            printf("\nNo results for query " ANSI_BOLD_WHITE "%s\n" ANSI_COLOR_RESET, cpTermName);

            return;
        }

        Document *document = term->document;

        int i = 0;

        int queryTF = getQueryTF(cpTermName, token);

        for (; document != NULL; document = document->next) {
            int position = generateHashById(document->id);

            if (results[position] == NULL) {
                Entry *entry = entries[position];
                entry->sum = (term->idf * getDocumentTF(document)) * (term->idf * queryTF);

                results[position] = entry;

                countSearchResult++;
            } else {
                Entry *entry = results[position];

                entry->sum += (term->idf * getDocumentTF(document)) * (term->idf * queryTF);
            }
        }

        token = strtok(NULL, " ");
    }

    sortEntriesByCosDesc(results);

    end = clock();

    searchTimeSpent = (double)(end - begin) / CLOCKS_PER_SEC;

    printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------");

    printf(ANSI_COLOR_RESET "\n  List of documents for query " ANSI_BOLD_WHITE "%s" ANSI_COLOR_RESET, cpTermName);

    printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------\n");

    printf(ANSI_COLOR_RESET "\t\t\t\t\t\t\t\t\t\tAbout " ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET " results (%lf seconds)\n", countSearchResult, searchTimeSpent);

    int x;

    printf(ANSI_COLOR_RESET);

    char COLUMN_SPACE[4] = "\t\t";

    printf(ANSI_BOLD_WHITE "\n    ID%sRelevance (cos)%sName\n" ANSI_COLOR_RESET, COLUMN_SPACE, COLUMN_SPACE);

    int countResult = 0;

    for (x = 0; x < NUM_OF_DOCUMENTS && countResult < MAX_SEARCH_RESULT; x++) {
        if (results[x] == NULL) {
            continue;
        }

        Entry *entry = results[x];

        double cos = entry->sum / entry->magnitude;

        printf("    %-6s\t%-23lf\t%-12s\n", entry->documentId, cos, entry->documentName);

        countResult++;
    }

    printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------\n" ANSI_COLOR_RESET);

    printf("\t\t\t\t\t\t\t\t\t\tMaximum result size per search: " ANSI_COLOR_YELLOW "%d\n" ANSI_COLOR_RESET, MAX_SEARCH_RESULT);
}

/**
 * Remove new line character from the end of the given string
 */
void removeNewLineCharFromString(char *str) {
    int len = strlen(str) - 1; /* position before '\0' */

    /* if it's a new line remove it */
    if (str[len] == '\n')
        str[len] = '\0';
}

/*
 * Creates and populates a product struct based on a XML cursor
 */
Product *createPopulatedStructProduct(xmlNodePtr in_cur) {

    Product *newProduct = (Product *) malloc(sizeof(Product));

    while (in_cur != NULL) {
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "id")) {
            newProduct->id = (char *) in_cur->children->content;
        }

        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "descricao")) {
            newProduct->description = (char *) in_cur->children->content;
        }

        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "preco")) {
            newProduct->price = (char *) in_cur->children->content;
        }

        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "img")) {
            newProduct->imgFileName = (char *) in_cur->children->content;
        }

        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "titulo")) {
            newProduct->title = (char *) in_cur->children->content;
        }

        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "categoria")) {
            newProduct->category = (char *) in_cur->children->content;
        }

        in_cur = in_cur->next;
    }

    return newProduct;
}
/*
 * Generate the inverted index processing a XML file
 */
void processXMLData(const char datasetFileName[]) {
    xmlDocPtr doc;
    xmlNodePtr cur;

    printf("Indexing... ");

    fflush(stdout); /* Ensure that the printf above will be printed in the terminal before the indexing process */

    doc = xmlParseFile(datasetFileName);

    if(doc == NULL) {
        fprintf(stderr, "Document not parsed sucessfully! \n");
        return;
    }

    cur = xmlDocGetRootElement(doc);

    if(cur == NULL) {
        fprintf(stderr, "Empty XML document! \n");
        xmlFreeDoc(doc);
        return;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "produtos")) {
        fprintf(stderr,"Document with wrong type! (root node != produtos) \n");
        xmlFreeDoc(doc);
        return;
    }

    cur = cur->xmlChildrenNode;

    Product *currentProduct = NULL;

    int numOfDocuments = 0;

    while (cur != NULL && numOfDocuments < NUM_OF_DOCUMENTS) {

        if (!xmlStrcmp(cur->name, (const xmlChar *) "produto")) {

            currentProduct = (Product *) realloc(currentProduct, sizeof(Product));
            currentProduct = createPopulatedStructProduct(cur->xmlChildrenNode);

            indexEntry(currentProduct);

            numOfDocuments++;
        }

        cur = cur->next;
    }

    generateDocMagnitudeAndVocabularyTermsIDF();

    printf(ANSI_BOLD_WHITE "[" ANSI_COLOR_GREEN " DONE " ANSI_COLOR_RESET ANSI_BOLD_WHITE "]" ANSI_COLOR_RESET " - " ANSI_COLOR_YELLOW "%d"
           ANSI_COLOR_RESET " documents were indexed!\n" ANSI_COLOR_RESET, numOfDocuments);
}

/*
 * Print all the terms of the vocabulary
 */
void printVocabulary() {

    int i;

    for (i = 0; i < NUM_OF_TERMS; i++) {
        Term *term = vocabulary[i];

        for (; term != NULL; term = term->next) {

            printf("\nTERM: %s", term->name);

            Document *document = term->document;

            printf("\nDOCUMENTS: ");

            for (; document != NULL; document = document->next) {
                printf("\nDOCUMENT NAME: %s, DOCUMENT TERM: %s", document->name, document->term);
            }
        }
    }

    printf("\n");
}

int main(void) {
    char query[QUERY_SIZE] = { 0 };

    processXMLData("../dataset/textDescDafitiPosthaus.xml");

    while (true) {
        printf("\nPlease, input the text to search or " ANSI_COLOR_RED "!q" ANSI_COLOR_RESET " to exit: ");

        fgets(query, sizeof(query), stdin);

        removeNewLineCharFromString(query);

        if (strcmp(query, "!q") == 0) {
            break;
        }

        searchByVectorModel(query);
    }

    return EXIT_SUCCESS;
}