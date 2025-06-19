#ifndef NLP_H
#define NLP_H

/* Structura pentru rezultatul tokenizarii */
typedef struct TokenizationResult TokenizationResult;


typedef struct {
    char* token;
    int count;        // TF raw count
    double tf;        // Term Frequency
    double idf;       // Inverse Document Frequency
    double tf_idf;    // TF-IDF score
} Token;

// Struct pentru a stoca documentele procesate
typedef struct {
    char** documents;
    int document_count;
} DocumentCollection;

typedef struct {
    char* domain;
    double probability;  // Probabilitatea initiala a clasei
    int document_count;  // Nr de documente in aceasta clasa
    // Tabel hash pentru cuvinte si frecv lor in aceasta clasa
    struct {
        char* word;
        int count;
    } *word_counts;
    int word_count_size;
} DomainBayes;

typedef struct {
    DomainBayes* domains;
    int count;
    int total_documents;
} BayesClassifier;

// init clasificator
BayesClassifier* init_bayes_classifier();

// Antrenare clasificator
void train_bayes_classifier(BayesClassifier* classifier, const char* text, const char* domain);

// Clasificare text
char* classify_text_bayes(BayesClassifier* classifier, const char* text);

// Eliberare resurse
void free_bayes_classifier(BayesClassifier* classifier);

// functia pentru calculul TF-IDF
void calculate_tf_idf(TokenizationResult* result, DocumentCollection* collection);


int count_words(const char* text);


TokenizationResult* tokenize_text(const char* text);

void free_tokenization_result(TokenizationResult* result);

char* determine_topic(const char* text);

char* generate_summary(const char* text, int max_sentences, DocumentCollection* collection);

#endif