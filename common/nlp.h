#ifndef NLP_H
#define NLP_H

/* Structura pentru rezultatul tokenizării */
typedef struct TokenizationResult TokenizationResult;


typedef struct {
    char* token;
    int count;        // TF raw count
    double tf;        // Term Frequency
    double idf;       // Inverse Document Frequency
    double tf_idf;    // TF-IDF score
} Token;

// Structură pentru a stoca documentele procesate
typedef struct {
    char** documents;
    int document_count;
} DocumentCollection;

typedef struct {
    char* domain;
    double probability;  // Probabilitatea inițială a clasei
    int document_count;  // Numărul de documente în această clasă
    // Tabel hash pentru cuvinte și frecvențele lor în această clasă
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

// Inițializare clasificator
BayesClassifier* init_bayes_classifier();

// Antrenare clasificator
void train_bayes_classifier(BayesClassifier* classifier, const char* text, const char* domain);

// Clasificare text
char* classify_text_bayes(BayesClassifier* classifier, const char* text);

// Eliberare resurse
void free_bayes_classifier(BayesClassifier* classifier);

// Funcția pentru calculul TF-IDF
void calculate_tf_idf(TokenizationResult* result, DocumentCollection* collection);


/**
 * Numără cuvintele dintr-un text
 * @param text Textul în care se numără cuvintele
 * @return Numărul de cuvinte sau -1 în caz de eroare
 */
int count_words(const char* text);

/**
 * Tokenizează un text și returnează cuvintele și frecvența lor
 * (după eliminarea stopwords)
 * @param text Textul de tokenizat
 * @return Structura cu rezultatul tokenizării sau NULL în caz de eroare
 */
TokenizationResult* tokenize_text(const char* text);

/**
 * Eliberează memoria alocată pentru rezultatul tokenizării
 * @param result Rezultatul tokenizării
 */
void free_tokenization_result(TokenizationResult* result);

/**
 * Determină domeniul tematic al unui text
 * @param text Textul de analizat
 * @return Domeniul tematic identificat ("Sport", "Politică", "Tehnologie", "Necunoscut")
 *         sau "Eroare la procesare" în caz de eroare
 */
char* determine_topic(const char* text);

/**
 * Generează un rezumat pentru un text
 * @param text Textul de rezumat
 * @param max_sentences Numărul maxim de propoziții din rezumat
 * @param collection Colecția de documente pentru analiză avansată
 * @return Rezumatul generat sau un mesaj de eroare
 */
char* generate_summary(const char* text, int max_sentences, DocumentCollection* collection);

#endif