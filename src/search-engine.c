#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <libxml/parser.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>

#include "search-engine.h"

Term *vocabulary[NUM_OF_TERMS];
Entry *entries[NUM_OF_DOCUMENTS];

int DESCRIPTION_SIZE = 0;
int TERM_SIZE = 0;

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
    int i;
    
    Entry *entryTmp;

    for (i = 0; i < NUM_OF_TERMS; i++) {
        if (vocabulary[i] == NULL) {
            continue;
        }
        
        Term *term = vocabulary[i];
        
        while (term != NULL) {
            
            term->idf = generateTermIDF(term);
            
            Document *documentTmp = term->document;
            
            for (; documentTmp != NULL; documentTmp = documentTmp->next) {
                double documentTF = getDocumentTF(documentTmp);

                int position = generateHashById(documentTmp->id);

                entryTmp = entries[position];

                if (entryTmp == NULL) {
                    continue;
                }

                /* This multiplication is to improve the performance of the pow (tf-idf, 2) */
                entryTmp->magnitude += (term->idf * documentTF) * (term->idf * documentTF);
            }
            
            term = term->next;
        }
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
    char *cpDocumentId = malloc(NUM_OF_DOCUMENTS * sizeof(char));
    
    strcpy(cpDocumentId, product->id);
    
    Entry *entry = malloc(sizeof (Entry));
    entry->documentId = malloc(NUM_OF_DOCUMENTS * sizeof(char));
    entry->documentName = malloc(DOCUMENT_NAME_SIZE * sizeof(char));
    
    strcpy(entry->documentId, cpDocumentId);
    strcpy(entry->documentName, product->imgFileName);
    
    int position = generateHashById(cpDocumentId);
    
    entries[position] = entry;
    
    char *cpDescription = malloc(DESCRIPTION_SIZE * sizeof(char));
    
    strcpy(cpDescription, product->description);
    
    char *token = malloc(TERM_SIZE * sizeof(char));
    token = strtok(cpDescription, " ");
    
    char *cpToken = NULL;
    
    char *cpDocumentNameTmp = malloc(DOCUMENT_NAME_SIZE * sizeof(char));
    strcpy(cpDocumentNameTmp, product->imgFileName);
    
    char *cpDocumentIdTmp = malloc(NUM_OF_DOCUMENTS * sizeof(char));
    strcpy(cpDocumentIdTmp, cpDocumentId);
    
    while (token != NULL) {
        cpToken = malloc(TERM_SIZE * sizeof(char));
        
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
            
            double cos1 = values[i]->sum / sqrt(values[i]->magnitude);
            double cos2 = values[j]->sum / sqrt(values[j]->magnitude);
            
            if (cos1 < cos2) {
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
Entry **searchByVectorModel(char termName[], bool verbose, Entry **paginatedResult) {
    
    if (strcmp(termName, "") == 0) {
        return NULL;
    }
    
    clock_t begin, end;
    
    double searchTimeSpent;
    
    begin = clock();
    
    Entry *results[NUM_OF_DOCUMENTS] = { NULL };
    
    char *cpTermName = (char *) malloc(QUERY_SIZE * sizeof(char));
    
    normalizeTerm(termName);
    
    strcpy(cpTermName, termName);
    
    char *token = strtok(termName, " ");
    
    int countSearchResult = 0;
    
    while (token != NULL) {
        // char *cpToken = (char *)malloc(sizeof(token));
        
        char *cpToken = (char *) malloc(TERM_SIZE * sizeof(char));
        
        strcpy(cpToken, token);
        
        unsigned int position = generateHash(cpToken);
        
        Term *term = vocabulary[position];
        
        while (term != NULL && strcmp(term->name, cpToken) != 0) {
            term = term->next;
        }
        
        if (term != NULL) {
            
            Document *document = term->document;
            
            int queryTF = getQueryTF(cpTermName, token);
            
            /* As the collection is not big, we are considering the whole collection. */
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
        }
        
        token = strtok(NULL, " ");
    }
    
    if (countSearchResult == 0) {
        if (verbose) {
            printf("\nNo results for query " ANSI_BOLD_WHITE "%.20s...\n" ANSI_COLOR_RESET, cpTermName);
        }
        
        return NULL;
    }
    
    /* As the collection is not big, we are considering the whole result for sorting. */
    sortEntriesByCosDesc(results);
    
    end = clock();
    
    searchTimeSpent = (double)(end - begin) / CLOCKS_PER_SEC;
    
    if(verbose) {
        printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------");
        
        printf(ANSI_COLOR_RESET "\n  List of documents for query " ANSI_BOLD_WHITE "%.20s..." ANSI_COLOR_RESET, cpTermName);
        
        printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------\n");
        
        printf(ANSI_COLOR_RESET "\t\t\t\t\t\t\t\t\t\tAbout " ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET " results (%lf seconds)\n", countSearchResult, searchTimeSpent);
        
        printf(ANSI_COLOR_RESET);
        
        char COLUMN_SPACE[4] = "\t\t";
        
        printf(ANSI_BOLD_WHITE "\n    ID%sRelevance (cos)%sName\n" ANSI_COLOR_RESET, COLUMN_SPACE, COLUMN_SPACE);
    }
    
    int x;
    
    int countResult = 0;
    
    for (x = 0; x < NUM_OF_DOCUMENTS && countResult < MAX_SEARCH_RESULT; x++) {
        if (results[x] == NULL) {
            continue;
        }
        
        Entry *entry = results[x];
        
        double cos = entry->sum / sqrt(entry->magnitude);
        
        if (verbose) {
            printf("    %-6s\t%-23lf\t%-12s\n", entry->documentId, cos, entry->documentName);
        } else {
            paginatedResult[countResult] = entry;
        }
        
        countResult++;
    }
    
    if (verbose) {
        printf(ANSI_COLOR_CYAN "\n----------------------------------------------------------------------------------------------------------------------\n" ANSI_COLOR_RESET);
        
        printf("\t\t\t\t\t\t\t\t\t\tMaximum result size per search: " ANSI_COLOR_YELLOW "%d\n" ANSI_COLOR_RESET, MAX_SEARCH_RESULT);
    }
    
    return paginatedResult;
}

/*
 * Creates and populates a product struct based on a XML cursor
 */
Product *createPopulatedStructProduct(xmlNodePtr in_cur) {
    
    Product *newProduct = (Product *) malloc(sizeof(Product));
    
    while (in_cur != NULL) {
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "id")) {
            newProduct->id = malloc(NUM_OF_DOCUMENTS * sizeof(char));
            newProduct->id = (char *) in_cur->children->content;
        }
        
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "descricao")) {
            newProduct->description = malloc(DESCRIPTION_SIZE * sizeof(char));
            newProduct->description = (char *) in_cur->children->content;
        }
        
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "preco")) {
            newProduct->price = malloc(10 * sizeof(char));
            newProduct->price = (char *) in_cur->children->content;
        }
        
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "img")) {
            newProduct->imgFileName = malloc(DOCUMENT_NAME_SIZE * sizeof(char));
            newProduct->imgFileName = (char *) in_cur->children->content;
        }
        
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "titulo")) {
            newProduct->title = malloc(50 * sizeof(char));
            newProduct->title = (char *) in_cur->children->content;
        }
        
        if(!xmlStrcmp(in_cur->name, (const xmlChar *) "categoria")) {
            newProduct->category = malloc(50 * sizeof(char));
            newProduct->category = (char *) in_cur->children->content;
        }
        
        in_cur = in_cur->next;
    }
    
    return newProduct;
}
/*
 * Generate the inverted index processing a XML file
 */
int processXMLData(const char datasetFileName[]) {
    xmlDocPtr doc;
    xmlNodePtr cur;
    
    clock_t begin, end;

    doc = xmlParseFile(datasetFileName);
    
    if(doc == NULL) {
        fprintf(stderr, "Document not parsed sucessfully! \n");
        
        return EXIT_FAILURE;
    }
    
    cur = xmlDocGetRootElement(doc);
    
    if(cur == NULL) {
        fprintf(stderr, "Empty XML document! \n");
        
        xmlFreeDoc(doc);
        
        return EXIT_FAILURE;
    }
    
    if (xmlStrcmp(cur->name, (const xmlChar *) "produtos")) {
        fprintf(stderr,"Document with wrong type! (root node != produtos) \n");
        
        xmlFreeDoc(doc);
        
        return EXIT_FAILURE;
    }
    
    printf("Indexing the documents... ");
    
    begin = clock();

    fflush(stdout); /* Ensure that the printf above will be printed in the terminal before the indexing process */
    
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
    
    end = clock();

    double searchTimeSpent = (double)(end - begin) / CLOCKS_PER_SEC;

    printf(ANSI_BOLD_WHITE "[" ANSI_COLOR_GREEN " DONE " ANSI_COLOR_RESET 
        ANSI_BOLD_WHITE "]" ANSI_COLOR_RESET " - " ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET 
        " documents (text) were indexed in %lf seconds!\n" ANSI_COLOR_RESET, numOfDocuments, searchTimeSpent);
    
    return EXIT_SUCCESS;
}

char *getImageWord(const char imagePath[]) {
    const int WORD_SIZE = 863 * 1296;

    char *resultWord = malloc (WORD_SIZE * sizeof(char));

    FILE *in;

    char *pythonCmd = "python ../../img-histogram-gen/src/img-histogram-gen.py ";

    char *command = malloc(strlen(pythonCmd) + strlen(imagePath) + 1);

    strcat(command, pythonCmd);

    strcat(command, imagePath); // image filename

    if (!(in = popen(command, "r"))) {
        exit(1);
    }

    fgets(resultWord, WORD_SIZE, in);

    pclose(in);

    return resultWord;
}

/*
 * Process all the images in a specific folder
 */
int processImageDataOnFolder(const char imgDatasetFolder[]) {
    DIR *d;
    
    clock_t begin, end;

    struct dirent *dir;

    extern FILE *popen();

    d = opendir(imgDatasetFolder);

    printf("Indexing the images from folder %s... ", imgDatasetFolder);
    
    fflush(stdout); /* Ensure that the printf above will be printed in the terminal before the indexing process */

    int count = 0;

    begin = clock();

    if (d) {
        while ((dir = readdir(d)) != NULL && count < NUM_OF_DOCUMENTS) {
            if (dir->d_type == DT_REG) {
                
                char *imagePath = malloc(strlen(imgDatasetFolder) + strlen(dir->d_name) + 1);

                strcat(imagePath, imgDatasetFolder);

                strcat(imagePath, dir->d_name); // image filename
                
                char* word = getImageWord(imagePath);

                Product *newProduct = (Product *) malloc(sizeof(Product));
                
                char *documentId = malloc(sizeof(int));
        
                sprintf(documentId, "%d", ++count);
                
                newProduct->id = documentId;
                newProduct->imgFileName = (char *) dir->d_name;
                newProduct->description = (char *) word;
            
                indexEntry(newProduct);
            }
        }

        closedir(d);
    }
    
    generateDocMagnitudeAndVocabularyTermsIDF();

    end = clock();
    
    double searchTimeSpent = (double)(end - begin) / CLOCKS_PER_SEC;
    
    printf(ANSI_BOLD_WHITE "[" ANSI_COLOR_GREEN " DONE " ANSI_COLOR_RESET 
        ANSI_BOLD_WHITE "]" ANSI_COLOR_RESET " - " 
        ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET 
        " documents (images) were indexed in %lf seconds!\n" ANSI_COLOR_RESET, count, searchTimeSpent);

    return EXIT_SUCCESS;
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

/**
 * Return an array of relevant documents for an specific query number
 */
char **loadRelevantsForQueryNumber(int queryNumber, char **resultRelevants) {
    xmlDocPtr doc;
    xmlNodePtr cur;
    
    fflush(stdout); /* Ensure that the printf above will be printed in the terminal before the indexing process */
    
    char *path = "../dataset/evaluation/relevants/";
    
    char *strQueryNumber = malloc(sizeof(int));
    
    sprintf(strQueryNumber, "%d", queryNumber);
    
    int newSize = strlen(path) + strlen(strQueryNumber) + strlen("_relevante.xml") + 1;
    
    char *filename = (char *) malloc(newSize);
    
    strcpy(filename, path);
    
    strcat(filename, strQueryNumber);
    
    strcat(filename, "_relevante.xml");
    
    doc = xmlParseFile(filename);
    
    if (doc == NULL) {
        fprintf(stderr, "Document not parsed sucessfully. Aborting... \n");
        
        return NULL;
    }
    
    cur = xmlDocGetRootElement(doc);
    
    if (cur == NULL) {
        fprintf(stderr, "Empty XML document! \n");
        
        xmlFreeDoc(doc);
        
        return NULL;
    }
    
    if (xmlStrcmp(cur->name, (const xmlChar *) "relevantes")) {
        fprintf(stderr,"Document with wrong type! (root node != relevantes) \n");
        
        xmlFreeDoc(doc);
        
        return NULL;
    }
    
    cur = cur->xmlChildrenNode;
    
    Product *currentProduct = NULL;
    
    int count = 0;
    
    while (cur != NULL) {
        
        if (!xmlStrcmp(cur->name, (const xmlChar *) "relevante")) {
            xmlNodePtr in_cur = cur->xmlChildrenNode;
            
            while (in_cur != NULL) {
                
                if(!xmlStrcmp(in_cur->name, (const xmlChar *) "img")) {
                    resultRelevants[count++] = (char *) in_cur->children->content;
                    
                    break;
                }
                
                in_cur = in_cur->next;
            }
        }
        
        cur = cur->next;
    }
    
    return resultRelevants;
}

/**
 * Calculate the precision in an specific point
 */
double getPrecisionAtPoint(int position, char *relevants[], Entry *resultsToEvaluate[]) {
    
    int numberOfRelevants = 0;
    
    int i;
    int j;
    
    for (i = 0; i < position; i++) {
        
        for (j = 0; j < 110; j++) {
            if (relevants[j] == NULL) {
                break;
            }
            
            if (strcmp(resultsToEvaluate[i]->documentName, relevants[j]) == 0) {
                numberOfRelevants++;
            }
        }
    }
    
    double precision = (double) numberOfRelevants / (double) position;
    
    return precision;
}

/**
 * Calculate the MAP (Mean Average Precision)
 */
double getMAP(char *relevants[], Entry *resultsToEvaluate[]) {
    
    double sumOfPrecisions = 0;
    
    int i;
    int j;
    
    for (i = 0; i < MAX_SEARCH_RESULT; i++) {
        
        for (j = 0; j < 110; j++) {
            if (relevants[j] == NULL) {
                break;
            }
            
            if (strcmp(resultsToEvaluate[i]->documentName, relevants[j]) == 0) {
                sumOfPrecisions += getPrecisionAtPoint(i + 1, relevants, resultsToEvaluate);
            }
        }
    }
    
    long totalRelevants = 0;
    
    /* Workaround to count the number of relevant documents in the array */
    for (j = 0; j < 110; j++) {
        if (relevants[j] == NULL) {
            break;
        }
        
        totalRelevants++;
    }
    
    double map = (double) sumOfPrecisions / (double) totalRelevants;
    
    return map;
}

/**
 * Evaluate the model by using the metrics:
 * - MAP - Mean Average Precision
 * - P@10 - Precision at point 10
 *
 * This evaluation executes 50 text queries or 50 image queries and evaluates the results 
 * comparing them with a file containing the  relevant results for each of these queries.
 */
void evaluateModelByMAPAndPat10(const char option[]) {
    Entry **resultsToEvaluate = NULL;
    
    double resultPAt10 = 0; // Precision at point 10 (P@10)
    double resultMAP = 0; // Mean Average Precision (MAP)
    
    int i;
    
    char **relevants = NULL;
    
    for(i = 0; i < NUMBER_OF_QUERIES_TO_EVAL; i++) { // Fix to the normal value: 50
        
        relevants = realloc(relevants, 110 * 70 * sizeof(char));
        
        int x;
        
        for(x = 0; x < 110; x++) {
            relevants[x] = NULL;
        }
        
        relevants = loadRelevantsForQueryNumber(i + 1, relevants);
        
        resultsToEvaluate = realloc(resultsToEvaluate, MAX_SEARCH_RESULT * sizeof(Entry));
        
        char *path = "../dataset/evaluation/queries/";
        
        char *strQueryNumber = malloc(sizeof(int));
        
        sprintf(strQueryNumber, "%d", (i + 1));
        
        int extensionSize = 0;
        
        char *extension = "";

        if (strcmp(option, "1") == 0) {
            extension = ".txt";

            extensionSize = strlen(extension);
        } else if(strcmp(option, "2") == 0) {
            extension = ".jpg";
            
            extensionSize = strlen(extension);
        }

        int newSize = strlen(path)  + strlen(strQueryNumber) + extensionSize + 1;
        
        char *filename = (char *) malloc(newSize);
        
        strcpy(filename, path);
        
        strcat(filename, strQueryNumber);
        
        strcat(filename, extension);
       
        if (strcmp(option, "1") == 0) {

            FILE *file = fopen (filename, "r");
        
            if (file != NULL) {
                char line [40];
                
                while (fgets(line, sizeof line, file) != NULL ) {
                    removeNewLineCharFromString(line);

                    // each line of the file is a query
                    resultsToEvaluate = searchByVectorModel(line, false, resultsToEvaluate); 
                    
                    if (resultsToEvaluate == NULL) {
                        continue;
                    }
                    
                    resultPAt10 += getPrecisionAtPoint(10, relevants, resultsToEvaluate); // P@10
                    
                    resultMAP += getMAP(relevants, resultsToEvaluate);
                }
                
                fclose (file);
            } else {
                printf("Could not open the file containing the queries. Aborting...\n"); 
                
                return;
            }
        } else if (strcmp(option, "2") == 0) {
            char* query = getImageWord(filename);

            resultsToEvaluate = searchByVectorModel(query, false, resultsToEvaluate);

            if (resultsToEvaluate == NULL) {
                continue;
            }
            
            resultPAt10 += getPrecisionAtPoint(10, relevants, resultsToEvaluate); // P@10
            
            resultMAP += getMAP(relevants, resultsToEvaluate);
        }
    }
    
    printf("\nP@10 for %d query(ies): " ANSI_COLOR_YELLOW "%lf" ANSI_COLOR_RESET, NUMBER_OF_QUERIES_TO_EVAL,
           resultPAt10 / NUMBER_OF_QUERIES_TO_EVAL);
    printf("\nMAP for %d query(ies): " ANSI_COLOR_YELLOW "%lf" ANSI_COLOR_RESET, NUMBER_OF_QUERIES_TO_EVAL,
           resultMAP / NUMBER_OF_QUERIES_TO_EVAL);
    
    printf("\n");
    
    free(resultsToEvaluate);
    free(relevants);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("\nsearch-engine USAGE:");
        printf("\n");
        printf("\n%s <option>", argv[0]);
        printf("\nwhere <option> values are:");
        printf("\n1 - Text searching");
        printf("\n2 - Image searching");
        printf("\n\n");

        return EXIT_FAILURE;
    }

    char *message = "";
    
    int result = 0;

    if(strcmp(argv[1], "1") == 0) {
        message = "Please, input the text to search";

        DESCRIPTION_SIZE = 1000;
        TERM_SIZE = 50;

        result = processXMLData("../dataset/textDescDafitiPosthaus.xml");
    } else if (strcmp(argv[1], "2") == 0) {
        message = "Please, input the image path to search";

        DESCRIPTION_SIZE = 863 * 1296;
        TERM_SIZE = 863;

        result = processImageDataOnFolder("../dataset/images/colecaoDafitiPosthaus/");
    }

    if (result == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    char query[QUERY_SIZE] = { 0 };

    while (true) {
        
        printf("\n%s," ANSI_COLOR_YELLOW " !m " 
            ANSI_COLOR_RESET "for model mestrics and " ANSI_COLOR_RED "!q" 
            ANSI_COLOR_RESET " to exit: ", message);
        
        fgets(query, sizeof(query), stdin);
        
        removeNewLineCharFromString(query);
        
        if (strcmp(query, "!q") == 0) {
            break;
        }
        
        if (strcmp(query, "!m") == 0) {
            evaluateModelByMAPAndPat10(argv[1]);
        } else {
            if(strcmp(argv[1], "1") == 0) {
                searchByVectorModel(query, true, NULL);
            } else if (strcmp(argv[1], "2") == 0) {
                char* word = getImageWord(query);

                searchByVectorModel(word, true, NULL);
            }
        }
    }

    return EXIT_SUCCESS;
}