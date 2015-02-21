/* Size of the collection of documents */
#define NUM_OF_DOCUMENTS 23155
/* Number of terms that should be indexed */
#define NUM_OF_TERMS NUM_OF_DOCUMENTS * 1296
/* Size of the user query */
#define QUERY_SIZE 863 * 1296
/* Max size of the search result */
#define MAX_SEARCH_RESULT 10
/* Size of the document name */
#define DOCUMENT_NAME_SIZE 70
/* Number of queries to be evaluated */
#define NUMBER_OF_QUERIES_TO_EVAL 50 // 50 is the maximum value considering the given evaluated results

/* Just for printf colors purposes */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD_WHITE    "\x1b[1m"

/* This struct represents the dataset structure */
typedef struct Product {
    char *id;
    char *title;
    char *category;
    char *price;
    char *description;
    char *imgFileName;
} Product;

/* This struct represents all the existent documents and its magnitudes */
typedef struct Entry {
    char *documentId;
    char *documentName;
    double magnitude; /* vector magnitude without sqrt()*/
    double sum; /* accumulator (wi,j) */
} Entry;

/* This struct represents the documents of the collection */
typedef struct Document {
    const char *id;
    const char *name;
    const char *term;
    int tf; /* term frequency. The variable is named as 'tf' because the literature refers as it */
    struct Document *next;
} Document;

/* This struct represents the indexed terms */
typedef struct Term {
    const char *name;
    int totalNumOfOccurrences;
    int totalNumOfDocuments;
    double idf;
    struct Term *next;
    struct Document *document;
} Term;

/*
 * Generate the inverted index processing a XML file
 */
int processXMLData(const char datasetFileName[]);
