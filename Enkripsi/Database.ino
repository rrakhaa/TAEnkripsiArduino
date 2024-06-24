#include <WiFi.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <HTTPClient.h>

const char* ssid = "Ceria_plus"; // Nama SSID
const char* password = "risolmayo"; // Password SSID

const char* serverAddress = "http://192.168.1.17/update_gps_data.php"; // Alamat server yang menyimpan skrip PHP

SoftwareSerial serial_gps(16, 17); // RX2 dan TX2 pada ESP32
TinyGPSPlus gps;
double latitude, longitude;

// Fungsi untuk mengenkripsi angka menggunakan Caesar Cipher dengan pergeseran 3
String caesarCipher(String text) {
    String result = "";
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
          // Geser karakter numerik dan tetap dalam angka 0-9
          c = (c - '0' + 3) % 10 + '0'; 
        }
        result += c;
    }
    return result;
}

// Fungsi untuk mengenkripsi angka menggunakan substitusi peta
String substituteMap(String text) {
    String result = "";
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c == '-') {
            // Tambahkan tanda negatif tanpa enkripsi
            result += '-'; 
        } else if (c == '.') {
            // Tambahkan titik desimal tanpa enkripsi
            result += '.'; 
        } else if (isdigit(c)) {
            int digit = c - '0';
            // Enkripsi angka 0 hingga 9 menjadi A-J
            result += char('A' + digit); 
        } else {
            // Tambahkan karakter lain tanpa enkripsi
            result += c; 
        }
    }
    return result;
}

// Fungsi untuk mengubah string menjadi biner
String toBinary(String text) {
    String result = "";
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        for (int j = 7; j >= 0; j--) {
            result += ((c >> j) & 1) ? '1' : '0';
        }
    }
    return result;
}

// Fungsi untuk mengenkripsi biner menggunakan XOR
String xorEncrypt(String binaryText, char key) {
    String result = "";
    for (int i = 0; i < binaryText.length(); i++) {
        result += (binaryText[i] == '1') ^ ((key >> (i % 8)) & 1) ? '1' : '0';
    }
    return result;
}

void setup() {
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected with IP Address: ");
    Serial.println(WiFi.localIP());

    serial_gps.begin(9600);
    Serial.println("Starting GPS!");
}

void loop() {
    // Periksa jika GPS menerima sinyal
    while(serial_gps.available()){
        gps.encode(serial_gps.read());
    }

    if (gps.location.isUpdated()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();

        // Enkripsi koordinat menggunakan Caesar Cipher
        String caesar_lat = caesarCipher(String(latitude, 6));
        String caesar_lng = caesarCipher(String(longitude, 6));

        // Enkripsi hasil Caesar Cipher menggunakan substitusi peta
        String enc_lat = substituteMap(caesar_lat);
        String enc_lng = substituteMap(caesar_lng);

        // Ubah hasil enkripsi menjadi biner
        String binary_lat = toBinary(enc_lat);
        String binary_lng = toBinary(enc_lng);

         // Enkripsi biner menggunakan XOR dengan kunci 'K'
        char xorKey = 'O';
        String xor_enc_lat = xorEncrypt(binary_lat, xorKey);
        String xor_enc_lng = xorEncrypt(binary_lng, xorKey);

        // Debugging output
        Serial.print("XR Latitude : ");
        Serial.println(xor_enc_lat);
        Serial.print("XR Longitude : ");
        Serial.println(xor_enc_lng);
        Serial.print("BR Latitude : ");
        Serial.println(binary_lat);
        Serial.print("BR Longitude : ");
        Serial.println(binary_lng);
        Serial.print("SM Latitude : ");
        Serial.print(enc_lat);
        Serial.print(" SM Longitude : ");
        Serial.println(enc_lng);
        Serial.print("CC Latitude : ");
        Serial.print(caesar_lat);
        Serial.print(" CC Longitude : ");
        Serial.println(caesar_lng);
        Serial.println("");

        // Kirim data GPS terenkripsi ke server
        sendGPSData(xor_enc_lat, xor_enc_lng);

        delay(60000); // Kirim data setiap 1 menit (60 detik)
    }
}

void sendGPSData(String xor_enc_lat, String xor_enc_lng) {
    // Buat objek klien HTTP
    HTTPClient http;

    // Buat data payload
    String postData = "lat=" + xor_enc_lat + "&lng=" + xor_enc_lng;

    // Cetak data yang akan dikirim untuk debug
    Serial.print("Sending data: ");
    Serial.println(postData);

    // Mulai koneksi ke server
    http.begin(serverAddress);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Kirim permintaan POST dengan data GPS
    int httpResponseCode = http.POST(postData);

    // Periksa respons dari server
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
    }

    // Akhiri koneksi
    http.end();
}