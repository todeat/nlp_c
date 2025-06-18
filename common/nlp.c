#include "nlp.h"
#include <pcre.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>


static char* stopwords[] = {
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are",
    "as", "at", "be", "because", "been", "before", "being", "below", "between", "both", "but",
    "by", "could", "did", "do", "does", "doing", "down", "during", "each", "few", "for", "from",
    "further", "had", "has", "have", "having", "he", "he'd", "he'll", "he's", "her", "here",
    "here's", "hers", "herself", "him", "himself", "his", "how", "how's", "i", "i'd", "i'll",
    "i'm", "i've", "if", "in", "into", "is", "it", "it's", "its", "itself", "let's", "me",
    "more", "most", "my", "myself", "nor", "of", "on", "once", "only", "or", "other", "ought",
    "our", "ours", "ourselves", "out", "over", "own", "same", "she", "she'd", "she'll", "she's",
    "should", "so", "some", "such", "than", "that", "that's", "the", "their", "theirs", "them",
    "themselves", "then", "there", "there's", "these", "they", "they'd", "they'll", "they're",
    "they've", "this", "those", "through", "to", "too", "under", "until", "up", "very", "was",
    "we", "we'd", "we'll", "we're", "we've", "were", "what", "what's", "when", "when's", "where",
    "where's", "which", "while", "who", "who's", "whom", "why", "why's", "with", "would", "you",
    "you'd", "you'll", "you're", "you've", "your", "yours", "yourself", "yourselves",
    // RO stopwordss
    "si", "in", "a", "al", "ale", "pe", "la", "care", "ce", "cu", "din", "despre", "pentru",
    "este", "sunt", "ca", "mai", "sau", "de", "nu", "sa", "o", "la", "dar", "unui", "unei",
    "acest", "aceasta", "acesta", "aceștia", "acestea", "prin", "iar", "fi", "fost", "ei",
    "ea", "el", "lor", "lui", "său", "sa", "său"
};
#define STOPWORDS_COUNT (sizeof(stopwords) / sizeof(stopwords[0]))

// struct for keywords for diff topics
typedef struct {
    char* domain;
    char** keywords;
    int keywords_count;
} DomainKeywords;

// keywords for different topics
static char* sport_keywords[] = {"fotbal", "meci", "jucător", "echipă", "campionat", "sportiv", 
    "baschet", "tenis", "competiție", "olimpic", "scor", "turneu", "victorie", "înfrângere", "gol"};

static char* politics_keywords[] = {"președinte", "guvern", "parlament", "lege", "politică", 
    "alegeri", "ministru", "democrat", "partid", "vot", "stat", "constituție", "referendum", "senat"};

static char* tech_keywords[] = {
        "tehnologie", "computer", "software", "internet", "aplicație", 
        "digital", "rețea", "programare", "inovație", "device", "sistem", 
        "algoritm", "inteligență", "date", "inteligență artificială", "IA", 
        "AI", "învățare automată", "rețele neurale", "machine learning",
        "deep learning", "automatizare", "roboți", "neural", "procesare"
    };

static DomainKeywords domains[] = {
    {"Sport", sport_keywords, sizeof(sport_keywords) / sizeof(sport_keywords[0])},
    {"Politică", politics_keywords, sizeof(politics_keywords) / sizeof(politics_keywords[0])},
    {"Tehnologie", tech_keywords, sizeof(tech_keywords) / sizeof(tech_keywords[0])}
};
#define DOMAINS_COUNT (sizeof(domains) / sizeof(domains[0]))


// tokenizer resp
struct TokenizationResult {
    Token* tokens;
    int count;
    int capacity;
};

// stuct for sentence
typedef struct {
    char* text;
    double score;
} Sentence;


static int is_stopword(const char* word) {
    for (int i = 0; i < STOPWORDS_COUNT; i++) {
        if (strcasecmp(word, stopwords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

BayesClassifier* init_bayes_classifier() {
    BayesClassifier* classifier = (BayesClassifier*)malloc(sizeof(BayesClassifier));
    if (!classifier) return NULL;
    
    classifier->count = 3; 
    classifier->total_documents = 0;
    classifier->domains = (DomainBayes*)malloc(classifier->count * sizeof(DomainBayes));
    
    if (!classifier->domains) {
        free(classifier);
        return NULL;
    }
    
    classifier->domains[0].domain = strdup("Sport");
    classifier->domains[0].probability = 1.0/3.0;
    classifier->domains[0].document_count = 0;
    classifier->domains[0].word_counts = NULL;
    classifier->domains[0].word_count_size = 0;
    
    classifier->domains[1].domain = strdup("Politică");
    classifier->domains[1].probability = 1.0/3.0;
    classifier->domains[1].document_count = 0;
    classifier->domains[1].word_counts = NULL;
    classifier->domains[1].word_count_size = 0;
    
    classifier->domains[2].domain = strdup("Tehnologie");
    classifier->domains[2].probability = 1.0/3.0;
    classifier->domains[2].document_count = 0;
    classifier->domains[2].word_counts = NULL;
    classifier->domains[2].word_count_size = 0;
    
    return classifier;
}

void train_bayes_classifier(BayesClassifier* classifier, const char* text, const char* domain) {
    if (!classifier || !text || !domain) return;
    
    int domain_idx = -1;
    for (int i = 0; i < classifier->count; i++) {
        if (strcmp(classifier->domains[i].domain, domain) == 0) {
            domain_idx = i;
            break;
        }
    }
    
    if (domain_idx == -1) {
        // TODO - more complicated: add new domain if it doesnt exists
        return;  
    }
    
    classifier->domains[domain_idx].document_count++;
    classifier->total_documents++;
    
    
    for (int i = 0; i < classifier->count; i++) {
        classifier->domains[i].probability = 
            (double)classifier->domains[i].document_count / classifier->total_documents;
    }
    
    // tokenize text and update word freq
    TokenizationResult* tokens = tokenize_text(text);
    if (!tokens) return;
    
    for (int i = 0; i < tokens->count; i++) {
        // add or update word in hash table
        // Implementare simplif: cautare liniara
        int found = 0;
        for (int j = 0; j < classifier->domains[domain_idx].word_count_size; j++) {
            if (strcmp(classifier->domains[domain_idx].word_counts[j].word, tokens->tokens[i].token) == 0) {
                classifier->domains[domain_idx].word_counts[j].count += tokens->tokens[i].count;
                found = 1;
                break;
            }
        }
        
        if (!found) {
            // add new word
            int new_size = classifier->domains[domain_idx].word_count_size + 1;
            classifier->domains[domain_idx].word_counts = 
                realloc(classifier->domains[domain_idx].word_counts, 
                       new_size * sizeof(*classifier->domains[domain_idx].word_counts));
            
            if (!classifier->domains[domain_idx].word_counts) continue;
            
            classifier->domains[domain_idx].word_counts[new_size - 1].word = 
                strdup(tokens->tokens[i].token);
            classifier->domains[domain_idx].word_counts[new_size - 1].count = 
                tokens->tokens[i].count;
            
            classifier->domains[domain_idx].word_count_size = new_size;
        }
    }
    
    free_tokenization_result(tokens);
}

char* classify_text_bayes(BayesClassifier* classifier, const char* text) {
    if (!classifier || !text) return strdup("Eroare");
    
    TokenizationResult* tokens = tokenize_text(text);
    if (!tokens) return strdup("Eroare la tokenizare");
    
    double max_prob = -1.0;
    int max_domain = -1;
    
    // calculate probability for each domain
    for (int d = 0; d < classifier->count; d++) {
        // use log to avoid underflow
        double prob = log(classifier->domains[d].probability);
        
        // for every token we add log(P(token|domain))
        for (int t = 0; t < tokens->count; t++) {
            // search for token in domains word counts
            int found = 0;
            int token_count = 0;
            
            for (int w = 0; w < classifier->domains[d].word_count_size; w++) {
                if (strcmp(classifier->domains[d].word_counts[w].word, tokens->tokens[t].token) == 0) {
                    token_count = classifier->domains[d].word_counts[w].count;
                    found = 1;
                    break;
                }
            }
            
            // calculate conditional probability
            // add 1 to token count if not found (Laplace smoothing)
            int total_words = 0;
            for (int w = 0; w < classifier->domains[d].word_count_size; w++) {
                total_words += classifier->domains[d].word_counts[w].count;
            }
            
            double cond_prob = (found ? token_count + 1.0 : 1.0) / 
                              (total_words + classifier->domains[d].word_count_size + 1.0);
            
            // add log conditional probability to the total probability
            prob += log(cond_prob) * tokens->tokens[t].count;
        }
        
        // verify if this domain has the highest probability
        if (prob > max_prob) {
            max_prob = prob;
            max_domain = d;
        }
    }
    
    free_tokenization_result(tokens);
    
    // return the domain with the highest probability
    if (max_domain >= 0) {
        return strdup(classifier->domains[max_domain].domain);
    } else {
        return strdup("Necunoscut");
    }
}



int count_words(const char* text) {
    const char* error;
    int erroffset;
    pcre* re;
    int ovector[30];
    int count = 0;

    const char* pattern = "\\b[a-zA-Z]+\\b";
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        fprintf(stderr, "PCRE compile error: %s\n", error);
        return -1;
    }

    int start = 0;
    int rc;
    while ((rc = pcre_exec(re, NULL, text, strlen(text), start, 0, ovector, 30)) >= 0) {
        count++;
        start = ovector[1];
    }

    pcre_free(re);
    return count;
}


TokenizationResult* tokenize_text(const char* text) {
    const char* error;
    int erroffset;
    pcre* re;
    int ovector[30];
    
    TokenizationResult* result = (TokenizationResult*)malloc(sizeof(TokenizationResult));
    if (!result) {
        return NULL;
    }
    
    result->capacity = 100;  // initial capacity
    result->count = 0;
    result->tokens = (Token*)malloc(result->capacity * sizeof(Token));
    if (!result->tokens) {
        free(result);
        return NULL;
    }
    
    const char* pattern = "\\b[a-zA-Z]+\\b";
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        fprintf(stderr, "PCRE compile error: %s\n", error);
        free(result->tokens);
        free(result);
        return NULL;
    }
    
    int start = 0;
    int rc;
    char token_buffer[256];
    
    while ((rc = pcre_exec(re, NULL, text, strlen(text), start, 0, ovector, 30)) >= 0) {
        int token_length = ovector[1] - ovector[0];
        if (token_length < sizeof(token_buffer)) {
            strncpy(token_buffer, text + ovector[0], token_length);
            token_buffer[token_length] = '\0';
            
            
            for (int i = 0; i < token_length; i++) {
                token_buffer[i] = tolower(token_buffer[i]);
            }
            
            // verify if the token is not a stopword
            if (!is_stopword(token_buffer)) {
                // verify if the token already exists in the result
                int found = 0;
                for (int i = 0; i < result->count; i++) {
                    if (strcmp(result->tokens[i].token, token_buffer) == 0) {
                        result->tokens[i].count++;
                        found = 1;
                        break;
                    }
                }
                
                if (!found) {
                    // verrify if we need to expand the result array
                    if (result->count >= result->capacity) {
                        result->capacity *= 2;
                        Token* new_tokens = (Token*)realloc(result->tokens, result->capacity * sizeof(Token));
                        if (!new_tokens) {
                            for (int i = 0; i < result->count; i++) {
                                free(result->tokens[i].token);
                            }
                            free(result->tokens);
                            free(result);
                            pcre_free(re);
                            return NULL;
                        }
                        result->tokens = new_tokens;
                    }
                    
                    result->tokens[result->count].token = strdup(token_buffer);
                    result->tokens[result->count].count = 1;
                    result->count++;
                }
            }
        }
        
        start = ovector[1];
    }
    
    pcre_free(re);
    return result;
}


void free_tokenization_result(TokenizationResult* result) {
    if (result) {
        for (int i = 0; i < result->count; i++) {
            free(result->tokens[i].token);
        }
        free(result->tokens);
        free(result);
    }
}


char* determine_topic(const char* text) {
    TokenizationResult* tokens = tokenize_text(text);
    if (!tokens) {
        return strdup("Eroare la procesare");
    }
    
    int domain_scores[DOMAINS_COUNT] = {0};
    
    // calculate scores for each domain based on keywords
    for (int i = 0; i < tokens->count; i++) {
        for (int d = 0; d < DOMAINS_COUNT; d++) {
            for (int k = 0; k < domains[d].keywords_count; k++) {
                if (strcasecmp(tokens->tokens[i].token, domains[d].keywords[k]) == 0) {
                    domain_scores[d] += tokens->tokens[i].count;
                }
            }
        }
    }
    
    int max_score = -1;
    int max_domain = -1;
    
    for (int d = 0; d < DOMAINS_COUNT; d++) {
        if (domain_scores[d] > max_score) {
            max_score = domain_scores[d];
            max_domain = d;
        }
    }
    
    char* result;
    if (max_domain >= 0 && max_score > 0) {
        result = strdup(domains[max_domain].domain);
    } else {
        result = strdup("Necunoscut");
    }
    
    free_tokenization_result(tokens);
    return result;
}

static Sentence* split_sentences(const char* text, int* count) {
    const char* error;
    int erroffset;
    pcre* re;
    int ovector[30];
    
    const char* pattern = "[^.!?]+[.!?]";
    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (!re) {
        fprintf(stderr, "PCRE compile error: %s\n", error);
        return NULL;
    }
    
    int sentence_count = 0;
    int start = 0;
    int rc;
    
    while ((rc = pcre_exec(re, NULL, text, strlen(text), start, 0, ovector, 30)) >= 0) {
        sentence_count++;
        start = ovector[1];
    }
    
    if (sentence_count == 0) {
        pcre_free(re);
        return NULL;
    }
    
    Sentence* sentences = (Sentence*)malloc(sentence_count * sizeof(Sentence));
    if (!sentences) {
        pcre_free(re);
        return NULL;
    }
    
    start = 0;
    int index = 0;
    
    pcre_extra* study = pcre_study(re, 0, &error);
    
    while ((rc = pcre_exec(re, study, text, strlen(text), start, 0, ovector, 30)) >= 0) {
        int len = ovector[1] - ovector[0];
        sentences[index].text = (char*)malloc(len + 1);
        if (sentences[index].text) {
            strncpy(sentences[index].text, text + ovector[0], len);
            sentences[index].text[len] = '\0';
            sentences[index].score = 0.0;
            index++;
        }
        start = ovector[1];
    }
    
    if (study) pcre_free(study);
    pcre_free(re);
    
    *count = index;
    return sentences;
}

static void score_sentences(Sentence* sentences, int sentence_count, TokenizationResult* tokens) {
    
    for (int i = 0; i < sentence_count; i++) {
        for (int j = 0; j < tokens->count; j++) {
            
            char* found = strcasestr(sentences[i].text, tokens->tokens[j].token);
            if (found) {
                
                sentences[i].score += tokens->tokens[j].count;
            }
        }
        
        int length = count_words(sentences[i].text);
        if (length > 0) {
            sentences[i].score /= length;
        }
    }
}

void calculate_tf_idf(TokenizationResult* result, DocumentCollection* collection) {
    int doc_length = 0;
    
    for (int i = 0; i < result->count; i++) {
        doc_length += result->tokens[i].count;
    }
    
    for (int i = 0; i < result->count; i++) {
        result->tokens[i].tf = (double)result->tokens[i].count / doc_length;
    }
    
    // calculate IDF for each token
    for (int i = 0; i < result->count; i++) {
        int doc_with_term = 0;
        
        for (int j = 0; j < collection->document_count; j++) {
            if (strcasestr(collection->documents[j], result->tokens[i].token) != NULL) {
                doc_with_term++;
            }
        }
    
        result->tokens[i].idf = log((double)(collection->document_count + 1) / (doc_with_term + 1));
        
        result->tokens[i].tf_idf = result->tokens[i].tf * result->tokens[i].idf;
    }
}



char* generate_summary(const char* text, int max_sentences, DocumentCollection* collection) {
    TokenizationResult* tokens = tokenize_text(text);
    if (!tokens) {
        return strdup("Eroare la procesare text.");
    }
    
    calculate_tf_idf(tokens, collection);
    
    int sentence_count = 0;
    Sentence* sentences = split_sentences(text, &sentence_count);
    if (!sentences) {
        free_tokenization_result(tokens);
        return strdup("Eroare la împărțirea textului în propoziții.");
    }
    
    for (int i = 0; i < sentence_count; i++) {
        sentences[i].score = 0.0;
        
        for (int j = 0; j < tokens->count; j++) {
        
            if (strcasestr(sentences[i].text, tokens->tokens[j].token)) {
                
                sentences[i].score += tokens->tokens[j].tf_idf;
            }
        }
        
        
        int length = count_words(sentences[i].text);
        if (length > 0) {
            sentences[i].score /= length;
        }
        
        if (i == 0 || i == sentence_count - 1) {
            sentences[i].score *= 1.5; 
        }
    }
    
    
    for (int i = 0; i < sentence_count - 1; i++) {
        for (int j = i + 1; j < sentence_count; j++) {
            if (sentences[j].score > sentences[i].score) {
                Sentence temp = sentences[i];
                sentences[i] = sentences[j];
                sentences[j] = temp;
            }
        }
    }
    
    
    int summary_length = (max_sentences < sentence_count) ? max_sentences : sentence_count;
    if (summary_length <= 0) {
        summary_length = 1;  
    }
    
    Sentence selected[summary_length];
    for (int i = 0; i < summary_length; i++) {
        selected[i] = sentences[i];
    }
    
    for (int i = 0; i < summary_length - 1; i++) {
        for (int j = i + 1; j < summary_length; j++) {

            const char* pos_i = strstr(text, selected[i].text);
            const char* pos_j = strstr(text, selected[j].text);
            
            if (pos_j < pos_i) {
                Sentence temp = selected[i];
                selected[i] = selected[j];
                selected[j] = temp;
            }
        }
    }
    
    
    size_t buffer_size = 1;  
    for (int i = 0; i < summary_length; i++) {
        buffer_size += strlen(selected[i].text) + 1;  
    }
    

    char* summary = (char*)malloc(buffer_size);
    if (!summary) {
        for (int i = 0; i < sentence_count; i++) {
            free(sentences[i].text);
        }
        free(sentences);
        free_tokenization_result(tokens);
        return strdup("Eroare la alocarea memoriei pentru rezumat.");
    }
    
    summary[0] = '\0';
    for (int i = 0; i < summary_length; i++) {
        strcat(summary, selected[i].text);
        strcat(summary, " ");
    }
    
    for (int i = 0; i < sentence_count; i++) {
        free(sentences[i].text);
    }
    free(sentences);
    free_tokenization_result(tokens);
    
    return summary;
}


#ifndef HAVE_STRCASESTR
char* strcasestr(const char* haystack, const char* needle) {
    char *p, *startn = NULL, *np = NULL;

    for (p = (char *)haystack; *p; p++) {
        if (np) {
            if (toupper(*p) == toupper(*np)) {
                if (!*++np)
                    return startn;
            } else {
                np = NULL;
            }
        } else if (toupper(*p) == toupper(*needle)) {
            np = (char *)needle + 1;
            startn = p;
        }
    }
    return NULL;
}
#endif