//=========================== SECRET TEMPLATE ===========================
// Copy this file to "secret.h" (same folder) and fill in the real values.
// secret.h is in .gitignore, so your passwords stay on your laptop.

//=========================== WIFI ===========================
// Up to three access points, the ESP picks whichever is around.
#define ssid_AP_1 "ssid_AP_1"
#define password_AP_1 "password_AP_1"

#define ssid_AP_2 "ssid_AP_2"
#define password_AP_2 "password_AP_2"

#define ssid_AP_3 "ssid_AP_3"
#define password_AP_3 "password_AP_3"

//=========================== NODE-RED BASIC AUTH ===========================
// Used to ask Node-RED which port ngrok gave to the MQTT broker today.
#define httpsUname "uname"
#define httpsPassword "password"

//=========================== MQTT BROKER LOGIN ===========================
// The Kubernetes broker (deploy/k8s) refuses anonymous clients.
// Use the "device" login from the mosquitto-auth Secret.
#define mqttUser "device"
#define mqttPassword "password"

//=========================== FIREBASE ===========================
#define firebaseDatabaseUrl "https://your-project-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define firebaseDatabaseSecret "your-legacy-database-secret"
