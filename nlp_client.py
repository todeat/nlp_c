#!/usr/bin/env python3
"""
Client Python pentru serverul NLP
Implementează punctul 4 obligatoriu - client pe alt limbaj/platformă
"""

import socket
import struct
import sys
import os
import time

SERVER_IP = "127.0.0.1"
PORT = 12345
MAX_TEXT_SIZE = 65536

REQUEST_COUNT_WORDS = 1
REQUEST_DETERMINE_TOPIC = 2
REQUEST_GENERATE_SUMMARY = 3


STATUS_OK = 0
STATUS_ERROR = 1

class NLPClient:
    def __init__(self, server_ip=SERVER_IP, port=PORT):
        self.server_ip = server_ip
        self.port = port
        self.sock = None
    
    def connect(self):
        """Conectează la server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.server_ip, self.port))
            print(f" Conectat la serverul NLP pe {self.server_ip}:{self.port}")
            return True
        except Exception as e:
            print(f" Eroare de conexiune: {e}")
            return False
    
    def disconnect(self):
        """Deconectează de la server"""
        if self.sock:
            self.sock.close()
            self.sock = None
            print(" Deconectat de la server")
    
    def send_request(self, request_type, text):
        """Trimite cerere către server"""
        if not self.sock:
            raise Exception("Nu sunt conectat la server")
        
        self.sock.send(struct.pack('<i', request_type))
        
        text_bytes = text.encode('utf-8') + b'\0'
        
        self.sock.send(struct.pack('<Q', len(text_bytes)))
        
        self.sock.send(text_bytes)
    
    def receive_response(self):
        """Primește răspuns de la server"""
        if not self.sock:
            raise Exception("Nu sunt conectat la server")
        
        status_data = self._recv_exact(4)
        status = struct.unpack('<i', status_data)[0]
        
        if status == STATUS_OK:
            
            word_count_data = self._recv_exact(4)
            word_count = struct.unpack('<i', word_count_data)[0]
            
            
            time_data = self._recv_exact(8)
            processing_time = struct.unpack('<d', time_data)[0]
            
            
            topic_len_data = self._recv_exact(8)
            topic_len = struct.unpack('<Q', topic_len_data)[0]
            
            topic = None
            if topic_len > 0:
                topic_data = self._recv_exact(topic_len)
                topic = topic_data.decode('utf-8').rstrip('\0')
            
            
            summary_len_data = self._recv_exact(8)
            summary_len = struct.unpack('<Q', summary_len_data)[0]
            
            summary = None
            if summary_len > 0:
                summary_data = self._recv_exact(summary_len)
                summary = summary_data.decode('utf-8').rstrip('\0')
            
            return {
                'status': 'OK',
                'word_count': word_count,
                'processing_time': processing_time,
                'topic': topic,
                'summary': summary
            }
        else:
            
            error_len_data = self._recv_exact(8)
            error_len = struct.unpack('<Q', error_len_data)[0]
            
            error_data = self._recv_exact(error_len)
            error_msg = error_data.decode('utf-8').rstrip('\0')
            
            return {
                'status': 'ERROR',
                'error': error_msg
            }
    
    def _recv_exact(self, size):
        """Primește exact `size` bytes"""
        data = b''
        while len(data) < size:
            chunk = self.sock.recv(size - len(data))
            if not chunk:
                raise Exception("Conexiunea s-a închis neașteptat")
            data += chunk
        return data
    
    def process_file(self, command, filename):
        """Procesează un fișier cu comanda specificată"""
        
        command_map = {
            '--count-words': REQUEST_COUNT_WORDS,
            '--determine-topic': REQUEST_DETERMINE_TOPIC,
            '--generate-summary': REQUEST_GENERATE_SUMMARY
        }
        
        if command not in command_map:
            return None, f"Comandă necunoscută: {command}"
        
        request_type = command_map[command]
        
        
        try:
            with open(filename, 'r', encoding='utf-8') as f:
                text = f.read()
            
            if len(text) > MAX_TEXT_SIZE:
                return None, f"Fișierul este prea mare, maxim {MAX_TEXT_SIZE} bytes permis"
        
        except FileNotFoundError:
            return None, f"Fișierul {filename} nu a fost găsit"
        except Exception as e:
            return None, f"Eroare la citirea fișierului: {e}"
        
        
        try:
            print(f"📤 Trimit cererea: {command} pentru {filename}")
            self.send_request(request_type, text)
            
            print("⏳ Aștept răspunsul de la server...")
            response = self.receive_response()
            
            return response, None
            
        except Exception as e:
            return None, f"Eroare de comunicare: {e}"

def print_help():
    """Afișează mesajul de ajutor"""
    print("=" * 60)
    print("🐍 CLIENT PYTHON PENTRU SERVERUL NLP")
    print("=" * 60)
    print("Utilizare: python3 nlp_client.py COMANDA FIȘIER")
    print("\nComenzi disponibile:")
    print("  --count-words FIȘIER        Numără cuvintele din fișier")
    print("  --determine-topic FIȘIER    Determină domeniul tematic")
    print("  --generate-summary FIȘIER  Generează un rezumat")
    print("\nExemple:")
    print("  python3 nlp_client.py --count-words ../resources/test.txt")
    print("  python3 nlp_client.py --determine-topic ../resources/test_sport.txt")
    print("  python3 nlp_client.py --generate-summary ../resources/test_lung.txt")

def print_response(command, response):
    """Afișează răspunsul în format frumos"""
    print("\n" + "=" * 60)
    print(" REZULTATUL PROCESĂRII")
    print("=" * 60)
    
    if command == '--count-words':
        print(f" Numărul de cuvinte: {response['word_count']}")
    elif command == '--determine-topic':
        print(f"  Domeniul tematic: {response['topic'] or 'Necunoscut'}")
    elif command == '--generate-summary':
        print(f" Rezumat:")
        print("-" * 40)
        print(response['summary'] or 'Nu s-a putut genera rezumat')
        print("-" * 40)
    
    print(f" Timpul de procesare: {response['processing_time']:.3f} secunde")
    print("=" * 60)

def main():
    if len(sys.argv) != 3:
        print_help()
        return 1
    
    command = sys.argv[1]
    filename = sys.argv[2]
    
    print(" Inițializare client Python NLP...")
    
    
    client = NLPClient()
    
    
    if not client.connect():
        return 1
    
    try:
        
        response, error = client.process_file(command, filename)
        
        if error:
            print(f"✗ Eroare: {error}")
            return 1
        
        if response['status'] == 'OK':
            print_response(command, response)
        else:
            print(f"✗ Eroare de la server: {response['error']}")
            return 1
            
    finally:
        client.disconnect()
    
    print(" Procesare completă!")
    return 0

if __name__ == "__main__":
    sys.exit(main())