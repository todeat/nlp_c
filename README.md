# NLP Server - Distributed Text Processing System

A multi-threaded server-client system for natural language processing tasks including word counting, topic determination, and text summarization.

## Features

- **Multi-threaded TCP Server**: Handles multiple concurrent client connections
- **Persistent Connections**: Clients can perform multiple operations in the same session
- **Request Queue**: FIFO queue for processing requests with proper synchronization
- **Admin Interface**: Real-time monitoring of connected clients and server status
- **NLP Operations**:
  - Word counting
  - Topic classification (Sport, Politics, Technology)
  - Text summarization using TF-IDF
- **Interactive & Single-command Modes**: Flexible client usage patterns

## Architecture

```
┌─────────────┐    TCP/IP    ┌─────────────┐    Queue    ┌─────────────┐
│   Client    │◄────────────►│   Server    │◄───────────►│ Processing  │
│             │              │             │             │  Thread     │
└─────────────┘              └─────────────┘             └─────────────┘
                                     │
                                     │ Unix Socket
                                     ▼
                              ┌─────────────┐
                              │   Admin     │
                              │   Client    │
                              └─────────────┘
```

## Requirements

- **C Compiler**: GCC or Clang
- **Libraries**:
  - PCRE (Perl Compatible Regular Expressions)
  - pthreads
  - Standard C libraries
- **Operating System**: Unix-like (Linux, macOS)

### Installing Dependencies

#### macOS (using Homebrew):
```bash
brew install pcre
```

#### Ubuntu/Debian:
```bash
sudo apt-get install libpcre3-dev
```

#### CentOS/RHEL:
```bash
sudo yum install pcre-devel
```

## Compilation

The project includes a Makefile for easy compilation:

```bash
# Clean previous builds
make clean

# Compile all components
make

# Or compile individual components
make client_bin    # Client only
make server_bin    # Server only
make admin_bin     # Admin client only
```

## Usage

### 1. Starting the Server

```bash
./server_bin
```

The server will:
- Listen on TCP port `12345` for client connections
- Create a Unix socket at `/tmp/nlp_admin_socket` for admin connections
- Display "Serverul așteaptă conexiuni..." when ready

### 2. Client Usage

#### Interactive Mode (Recommended)
```bash
./client_bin --interactive
```

Once connected, you can use these commands:
- `count-words FILENAME` - Count words in a file
- `determine-topic FILENAME` - Determine the topic of a file
- `generate-summary FILENAME` - Generate a summary of a file
- `help` - Show available commands
- `exit` - Close connection and quit

**Example Interactive Session:**
```
$ ./client_bin --interactive
Conectat la server. Folosește 'help' pentru comenzi disponibile.

> help
Comenzi disponibile:
  count-words FIȘIER       - Numără cuvintele din fișier
  determine-topic FIȘIER   - Determină domeniul tematic al fișierului
  generate-summary FIȘIER  - Generează un rezumat al fișierului
  exit                     - Închide conexiunea și ieși din aplicație
  help                     - Afișează acest mesaj de ajutor

> count-words resources/test.txt
Numărul de cuvinte: 80
Timpul de procesare: 0.00 secunde

> determine-topic resources/test_sport.txt
Domeniul tematic: Sport
Timpul de procesare: 0.00 secunde

> exit
```

#### Single Command Mode (Legacy)
```bash
./client_bin --count-words resources/test.txt
./client_bin --determine-topic resources/test_sport.txt
./client_bin --generate-summary resources/test.txt
```

### 3. Admin Monitoring

Monitor server status and connected clients:

```bash
# View connected clients
./admin_bin --clients

# View processing queue status
./admin_bin --queue-status
```

**Example Admin Output:**
```
$ ./admin_bin --clients
Număr total de clienți: 2
ID    Adresă              Conectat la               Cereri         
-------------------------------------------------------------
1     127.0.0.1            2025-06-18 23:25:09       3              
2     127.0.0.1            2025-06-18 23:26:15       1              
```

## File Structure

```
├── client/
│   └── client.c          # Client implementation
├── server/
│   └── server.c          # Server implementation
├── admin/
│   └── admin_client.c    # Admin client implementation
├── common/
│   ├── protocol.h        # Communication protocol definitions
│   ├── protocol.c        # Protocol implementation
│   ├── nlp.h            # NLP functions header
│   └── nlp.c            # NLP algorithms implementation
├── resources/           # Sample text files for testing
│   ├── test.txt
│   ├── test_sport.txt
│   ├── test_politica.txt
│   └── test_tehnologie.txt
└── Makefile            # Build configuration
```

## Protocol Specification

### Request Types
- `REQUEST_COUNT_WORDS = 1` - Word counting
- `REQUEST_DETERMINE_TOPIC = 2` - Topic classification
- `REQUEST_GENERATE_SUMMARY = 3` - Text summarization
- `REQUEST_EXIT = 4` - Close connection

### Message Format
Each message contains:
1. **Request Type** (4 bytes)
2. **Text Length** (size_t bytes)
3. **Text Content** (variable length)

### Response Format
1. **Status Code** (4 bytes) - OK or ERROR
2. **Data Fields** (variable, depending on request type):
   - Word count (int)
   - Processing time (double)
   - Topic string (with length prefix)
   - Summary string (with length prefix)
   - Error message (for errors)

## NLP Algorithms

### Word Counting
Uses PCRE regex `\b[a-zA-Z]+\b` to identify and count words.

### Topic Classification
Keyword-based classification supporting three domains:
- **Sport**: Keywords like "fotbal", "meci", "jucător", etc.
- **Politics**: Keywords like "președinte", "guvern", "parlament", etc.
- **Technology**: Keywords like "tehnologie", "computer", "AI", etc.

### Text Summarization
1. **Tokenization**: Extract and filter words (remove stopwords)
2. **TF-IDF Calculation**: Compute term frequency and inverse document frequency
3. **Sentence Scoring**: Score sentences based on TF-IDF values
4. **Selection**: Choose top-scoring sentences
5. **Ordering**: Maintain original sentence order in summary

## Configuration

### Server Settings
- **TCP Port**: 12345 (defined in `server.c`)
- **Max Clients**: 10 concurrent connections
- **Queue Size**: 100 pending requests
- **Max Text Size**: 65536 bytes per request

### Client Settings
- **Server IP**: 127.0.0.1 (localhost)
- **Connection**: Persistent until explicit exit

## Error Handling

The system handles various error conditions:
- Connection failures
- File not found
- Memory allocation errors
- Protocol violations
- Server overload

All errors are reported with descriptive messages.

## Thread Safety

- **Mutex Protection**: Client list and request queue are protected
- **Condition Variables**: Used for queue synchronization
- **Detached Threads**: Automatic cleanup of client handler threads

## Testing

Sample files are provided in the `resources/` directory:

```bash
# Test all functionality
./client_bin --interactive
> count-words resources/test.txt
> determine-topic resources/test_sport.txt
> generate-summary resources/test_tehnologie.txt
> exit

# Monitor with admin
./admin_bin --clients
./admin_bin --queue-status
```

## Troubleshooting

### Common Issues

1. **"Address already in use"**
   ```bash
   # Kill existing server process
   pkill -f server_bin
   # Or wait a few seconds for port to be released
   ```

2. **"Connection refused"**
   - Ensure server is running
   - Check if port 12345 is available
   - Verify firewall settings

3. **PCRE library not found**
   - Install PCRE development headers
   - Update library paths in Makefile if needed






