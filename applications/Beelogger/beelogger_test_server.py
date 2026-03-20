import http.server
import urllib.parse
import datetime
import os
import csv

# --- KONFIGURATION ---
PORT = 8081
LOG_FILE = "beelogger_test_data.csv"
PASSWORT = "beelogger" # Falls ein Passwort im ESP32 hinterlegt ist

class BeeloggerHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_path = urllib.parse.urlparse(self.path)
        params = urllib.parse.parse_qs(parsed_path.query)
        
        # Falls es sich um den Root-Pfad ohne Parameter handelt -> Info-Seite für Browser
        if parsed_path.path == "/" and not params:
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            html = """
            <html>
            <head><title>Beelogger Test Server</title></head>
            <body style='font-family:sans-serif; text-align:center; padding: 50px; background:#f0f4f8;'>
                <div style='background:white; padding:40px; border-radius:15px; display:inline-block; border:1px solid #ddd; box-shadow: 0 4px 6px rgba(0,0,0,0.1);'>
                    <h1 style='color:#fbbf24;'>🐝 Beelogger Test Server</h1>
                    <p style='color:#64748b;'>Status: <b>ONLINE</b></p>
                    <hr style='border:0; border-top:1px solid #eee; margin: 20px 0;'>
                    <p>Warte auf Daten von ESP32 (Pfad: /data oder beelogger_log.php)</p>
                    <small style='color:#94a3b8;'>Zeit am Server: """ + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + """</small>
                </div>
            </body>
            </html>
            """
            self.wfile.write(html.encode('utf-8'))
            return

        # Prozessierung der Logger-Anfragen
        if "beelogger_log.php" in parsed_path.path or parsed_path.path == "/data" or parsed_path.path == "/":
            # Parameter flach klopfen (parse_qs gibt Listen zurück)
            data = {k: v[0] for k, v in params.items()}
            if data:
                self.handle_data(data)
            else:
                self.send_response(400)
                self.end_headers()
        else:
            print(f"DEBUG: Unbekannter Pfad abgelehnt: {parsed_path.path}")
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length).decode('utf-8')
        data = {k: v[0] for k, v in urllib.parse.parse_qs(post_data).items()}
        self.handle_data(data)

    def handle_data(self, data):
        timestamp_now = datetime.datetime.now()
        timestamp_str = timestamp_now.strftime("%Y-%m-%d %H:%M:%S")
        
        # Mapping der Beelogger-Parameter
        # T1 = TempIn, F1 = HumidityIn, G1 = Weight, VB = Battery, VS = Solar, A1 = Aux1 (Pressure)
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

        print(f"[{timestamp_str}] Daten empfangen:")
        for k, v in mapped_data.items():
            if v != 'n/a':
                print(f"  {k}: {v}")

        # Checksumme validieren (wie im originalen PHP)
        try:
            # Summe aller relevanten numerischen Werte
            check_val = 0.0
            for key in ['T1', 'TO', 'F1', 'FO', 'L', 'VB', 'VS', 'G1', 'A1', 'A2', 'A3']:
                val = data.get(key, '')
                if val:
                    check_val += float(val)
            
            # Die PHP-Logik: $int_Check = round($int_Check + 0.5);
            calculated_checksum = round(check_val + 0.5)
            received_checksum = int(data.get('C', data.get('Check', -1)))
            
            if received_checksum == calculated_checksum:
                print(f"  Checksumme: {received_checksum} (OK)")
            else:
                print(f"  Checksumme: {received_checksum} (FEHLER! Erwartet: {calculated_checksum})")
        except Exception as e:
            print(f"  Checksummen-Prüfung fehlgeschlagen: {e}")

        # In CSV Datei speichern (Encoding utf-8-sig für einfaches Öffnen in Excel)
        file_exists = os.path.isfile(LOG_FILE)
        with open(LOG_FILE, mode='a', newline='', encoding='utf-8-sig') as f:
            writer = csv.DictWriter(f, fieldnames=mapped_data.keys(), delimiter=';')
            if not file_exists:
                writer.writeheader()
            writer.writerow(mapped_data)

        # Antwort senden (Beelogger Standard-Antwort)
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        
        # Ein Zeitstempel gefolgt von "ok *" signalisiert dem Logger Erfolg
        # Das PHP sendet oft den aktuellen Unix-Timestamp
        response = f"{int(timestamp_now.timestamp())}ok *"
        self.wfile.write(response.encode('utf-8'))
        print(f"[{timestamp_str}] Antwort gesendet: {response}")

if __name__ == '__main__':
    print(f"=== Beelogger Test Server ===")
    print(f"Server läuft auf Port {PORT}...")
    abs_log_path = os.path.abspath(LOG_FILE)
    print(f"Ergebnisse werden gespeichert in: {abs_log_path}")
    print(f"Drücken Sie Strg+C zum Beenden.")
    
    server = http.server.HTTPServer(('0.0.0.0', PORT), BeeloggerHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer wird beendet.")
