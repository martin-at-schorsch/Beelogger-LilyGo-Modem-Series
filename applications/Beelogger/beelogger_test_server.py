import http.server
import socketserver
import urllib.parse
import datetime
import os
import csv
import logging
import sys

# --- KONFIGURATION ---
PORT = 8081
DATA_FILE = "beelogger_test_data.csv"
ACCESS_LOG = "beelogger_access.log"
PASSWORT = "beelogger" # Falls ein Passwort im ESP32 hinterlegt ist

# Logging-Setup für Zugriffe
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler(ACCESS_LOG, encoding='utf-8'),
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger("BeeloggerServer")

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    """Der ThreadingMixIn sorgt dafür, dass Anfragen in separaten Threads bearbeitet werden.
    Dies verhindert, dass eine einzelne langsame oder blockierende Anfrage den gesamten Server einfriert."""
    daemon_threads = True

class BeeloggerHandler(http.server.BaseHTTPRequestHandler):
    
    def log_message(self, format, *args):
        """Überschreibt die Standard-Logfunktion, um sie in unsere Logdatei umzuleiten."""
        logger.info("%s - %s" % (self.address_string(), format % args))

    def log_error(self, format, *args):
        """Überschreibt die Standard-Fehlerlogfunktion."""
        logger.error("%s - %s" % (self.address_string(), format % args))

    def send_error_response(self, code, message=None):
        """Hilfsfunktion für Fehlerantworten mit Logging."""
        logger.warning(f"Abgelehnte Anfrage von {self.address_string()} auf {self.path}: {code} {message or ''}")
        self.send_error(code, message)

    def do_GET(self):
        try:
            parsed_path = urllib.parse.urlparse(self.path)
            params = urllib.parse.parse_qs(parsed_path.query)
            
            # Info-Seite für Browser
            if parsed_path.path == "/" and not params:
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.end_headers()
                html = f"""
                <html>
                <head><title>Beelogger Test Server</title></head>
                <body style='font-family:sans-serif; text-align:center; padding: 50px; background:#f0f4f8;'>
                    <div style='background:white; padding:40px; border-radius:15px; display:inline-block; border:1px solid #ddd; box-shadow: 0 4px 6px rgba(0,0,0,0.1);'>
                        <h1 style='color:#fbbf24;'>🐝 Beelogger Test Server</h1>
                        <p style='color:#64748b;'>Status: <b>ONLINE (Multithreaded)</b></p>
                        <hr style='border:0; border-top:1px solid #eee; margin: 20px 0;'>
                        <p>Warte auf Daten von ESP32 (Pfad: /data oder beelogger_log.php)</p>
                        <small style='color:#94a3b8;'>Zeit am Server: {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</small>
                    </div>
                </body>
                </html>
                """
                self.wfile.write(html.encode('utf-8'))
                return

            # Logger-Anfragen
            is_valid_path = "beelogger_log.php" in parsed_path.path or parsed_path.path in ["/data", "/"]
            
            if is_valid_path:
                data = {k: v[0] for k, v in params.items()}
                if data:
                    self.handle_data(data)
                else:
                    # Falls der Pfad stimmt, aber keine Daten da sind (z.B. Test-Aufruf)
                    self.send_error_response(400, "Bad Request: No Data")
            else:
                self.send_error_response(404, "Not Found")
                
        except Exception as e:
            logger.exception(f"Fehler bei GET-Anfrage von {self.address_string()}: {e}")
            self.send_error(500, "Internal Server Error")

    def do_POST(self):
        try:
            content_length = int(self.headers.get('Content-Length', 0))
            if content_length == 0:
                self.send_error_response(400, "Bad Request: No Content")
                return
                
            post_data = self.rfile.read(content_length).decode('utf-8')
            parsed_data = urllib.parse.parse_qs(post_data)
            data = {k: v[0] for k, v in parsed_data.items()}
            
            if data:
                self.handle_data(data)
            else:
                self.send_error_response(400, "Bad Request: Parse Error")
        except Exception as e:
            logger.exception(f"Fehler bei POST-Anfrage von {self.address_string()}: {e}")
            self.send_error(500, "Internal Server Error")

    def handle_data(self, data):
        timestamp_now = datetime.datetime.now()
        timestamp_str = timestamp_now.strftime("%Y-%m-%d %H:%M:%S")
        
        # Mapping der Beelogger-Parameter
        mapped_data = {
            "Zeitstempel": timestamp_str,
            "Temperatur": data.get('T1', data.get('TO', 'n/a')),
            "Feuchte": data.get('F1', data.get('FO', 'n/a')),
            "Gewicht": data.get('G1', 'n/a'),
            "Batterie": data.get('VB', 'n/a'),
            "Solar": data.get('VS', 'n/a'),
            "Luftdruck": data.get('A1', 'n/a'),
            "Check": data.get('C', data.get('Check', 'n/a')),
            "Passwort": data.get('PW', data.get('Passwort', 'n/a'))
        }

        # Checksumme validieren
        try:
            check_val = 0.0
            for key in ['T1', 'TO', 'F1', 'FO', 'L', 'VB', 'VS', 'G1', 'A1', 'A2', 'A3']:
                val = data.get(key, '')
                if val:
                    check_val += float(val.replace(',', '.')) # Robustheit bei Dezimaltrennern
            
            calculated_checksum = round(check_val + 0.5)
            received_checksum = int(data.get('C', data.get('Check', -1)))
            
            checksum_status = "OK" if received_checksum == calculated_checksum else f"FEHLER (Erwartet: {calculated_checksum})"
            logger.info(f"Daten von {self.address_string()} erhalten. Gewicht: {mapped_data['Gewicht']}, Checksumme: {received_checksum} ({checksum_status})")
        except Exception as e:
            logger.warning(f"Checksummen-Prüfung fehlgeschlagen: {e}")

        # In CSV Datei speichern
        try:
            file_exists = os.path.isfile(DATA_FILE)
            with open(DATA_FILE, mode='a', newline='', encoding='utf-8-sig') as f:
                writer = csv.DictWriter(f, fieldnames=mapped_data.keys(), delimiter=';')
                if not file_exists:
                    writer.writeheader()
                writer.writerow(mapped_data)
        except Exception as e:
            logger.error(f"Konnte CSV nicht schreiben: {e}")

        # Antwort senden (Beelogger Standard-Antwort)
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        
        response = f"{int(timestamp_now.timestamp())}ok *"
        self.wfile.write(response.encode('utf-8'))

if __name__ == '__main__':
    logger.info("=== Beelogger Test Server Start ===")
    logger.info(f"Server läuft auf Port {PORT}")
    logger.info(f"Daten werden gespeichert in: {os.path.abspath(DATA_FILE)}")
    logger.info(f"Zugriff-Log: {os.path.abspath(ACCESS_LOG)}")
    
    # Der Server läuft nun mit Threading, um Blockierungen zu vermeiden
    with ThreadedHTTPServer(('0.0.0.0', PORT), BeeloggerHandler) as server:
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            logger.info("Server wird beendet...")
            server.shutdown()
            sys.exit(0)

