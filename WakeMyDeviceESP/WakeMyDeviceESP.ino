#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>

// Server and Topic Configuration
const char* mqtt_server     = "xxxxxxxxx";
const int mqtt_port         = 8883;
const char* mqtt_username   = "xxxxxxxxx";
const char* mqtt_password   = "xxxxxxxxx";
const char* mqtt_topic      = "wakeup_list";

// Initialize the ESP8266 Web Server at port 80
ESP8266WebServer server(80);

// Initialize MQTT client
// WiFiClient espClient;
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Initialize UDP for Wake-on-LAN
WiFiUDP UDP;



// EEPROM storage structure
const int MAX_DEVICES = 10;  // Maximum number of devices that can be stored

struct DeviceInfo {
  char name[20];
  char mac[18];  // MAC address string "XX:XX:XX:XX:XX:XX\0"
};

DeviceInfo deviceList[MAX_DEVICES];  // Array to store multiple devices


void initializeEEPROM() {
  for (int i = 0; i < MAX_DEVICES; i++) {
    if (deviceList[i].name[0] != '\0') { // Check if the name field is not empty
      // Found data, assume EEPROM is initialized
      return;
    }
  }
  // EEPROM is not initialized, so clear it
  for (int i = 0; i < MAX_DEVICES; i++) {
    deviceList[i].name[0] = '\0'; // Mark as empty
    deviceList[i].mac[0] = '\0';
    EEPROM.put(i * sizeof(DeviceInfo), deviceList[i]);
  }
  EEPROM.commit();
}


// Function to load the device list from EEPROM
void loadDeviceListFromEEPROM() {
  for (int i = 0; i < MAX_DEVICES; i++) {
    EEPROM.get(i * sizeof(DeviceInfo), deviceList[i]);
    // Add checks here to validate the data
    // For example, make sure names are null-terminated strings
    deviceList[i].name[sizeof(deviceList[i].name) - 1] = '\0';
    deviceList[i].mac[sizeof(deviceList[i].mac) - 1] = '\0';
  }
}

// Function to save the device list to EEPROM
void saveDeviceListToEEPROM() {
  for (int i = 0; i < MAX_DEVICES; ++i) {
    EEPROM.put(i * sizeof(DeviceInfo), deviceList[i]);
  }
  EEPROM.commit();  // Ensure data is written to flash memory
}

// Function to add a new device to the EEPROM storage
bool addDeviceToEEPROM(const char* name, const char* mac) {
  for (int i = 0; i < MAX_DEVICES; ++i) {
    // Find an empty slot or overwrite the same name
    if (strlen(deviceList[i].name) == 0 || strcmp(deviceList[i].name, name) == 0) {
      strncpy(deviceList[i].name, name, sizeof(deviceList[i].name) - 1);
      strncpy(deviceList[i].mac, mac, sizeof(deviceList[i].mac) - 1);
      saveDeviceListToEEPROM();
      return true;
    }
  }
  return false;  // No space left or name is already taken
}

// Function to remove a device from the EEPROM storage
bool removeDeviceFromEEPROM(const char* name) {
  for (int i = 0; i < MAX_DEVICES; ++i) {
    if (strcmp(deviceList[i].name, name) == 0) {
      deviceList[i].name[0] = '\0';  // Clear the name to indicate an empty slot
      saveDeviceListToEEPROM();
      return true;
    }
  }
  return false;  // Device not found
}

void completelyClearEEPROM(){
    EEPROM.begin(512);  // Use a size appropriate for your application
    for (int i = 0; i < 512; i++) EEPROM.write(i, 0);
    EEPROM.commit();
    Serial.println("EEPROM cleared");
}

// Call this function during setup to initialize the EEPROM and load the devices
void setupEEPROM() {
  initializeEEPROM();
  EEPROM.begin(MAX_DEVICES * sizeof(DeviceInfo));
  loadDeviceListFromEEPROM();
}


void setup() {
    Serial.begin(115200);

    // Setup insecure connection for demonstration purposes (Not recommended for production)
    espClient.setInsecure();

    WiFiManager wifiManager;
    wifiManager.autoConnect("ESP8266WakeMyDevice");
    Serial.println("Connected to WiFi");

    // MQTT client configuration
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    // Initialize EEPROM and load the device list
    // completelyClearEEPROM(); // RUN THIS ONCE TO CLEAR EEPROM: You can comment this line after running it once.
    setupEEPROM();

    // Web server configuration
    setupWebServer();

    // Begin listening for UDP packets (for Wake-on-LAN)
    UDP.begin(9);
}


void loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
    server.handleClient();
}

void handleRoot_old() {
    loadDeviceListFromEEPROM();  // Make sure to load the device list

    String page = "<html><head><title>Device List</title></head><body>"
                  "<h1>Device List</h1>"
                  "<table><tr><th>Name</th><th>Actions</th></tr>";
    
    page += getDeviceListHTML();

    page += "</table>"
            "<h2>Add Device</h2>"
            "<form action='/save' method='POST'>"
            "Name:<input type='text' name='name'><br>"
            "MAC:<input type='text' name='mac'><br>"
            "<input type='submit' value='Save'>"
            "</form></body></html>";

    server.send(200, "text/html", page);
}

void handleRoot() {
    loadDeviceListFromEEPROM();  // Make sure to load the device list

    String page = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Device List</title>
    <link href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css' rel='stylesheet'>
    <script src='https://code.jquery.com/jquery-3.3.1.slim.min.js'></script>
    <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.7/umd/popper.min.js'></script>
    <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js'></script>
    <script>
    $(document).ready(function() {
        $('button.delete').click(function(event) {
        event.preventDefault();
        var name = $(this).data('name');
        deleteDevice(name); // Call the deleteDevice function instead of using $.get
        });

    });
    function deleteDevice(name) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/delete?name=' + encodeURIComponent(name), true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
        // Refresh the page to update the device list
        location.reload();
        }
    };
    xhr.send();
    }
    function saveDevice() {
        var name = document.getElementById('name').value;
        var mac = document.getElementById('mac').value;
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/save', true);
        xhr.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
        xhr.onreadystatechange = function () {
            if (xhr.readyState == 4 && xhr.status == 200) {
                $('#alerts').html('<div class="alert alert-success" role="alert">' + xhr.responseText + '</div>');
                location.reload();
            }
        };
        xhr.send('name=' + name + '&mac=' + mac);
    }
    </script>
    <style>
    .table th, .table td { vertical-align: middle; }
    </style>
</head>
<body>
    <div class='container'>
        <h1 class='mt-4 mb-3'>🌄 Wake My Device</h1>
        <h2 class='mt-4 mb-3'>Device List</h1>
        <div id='alerts'></div>
        <table class='table table-bordered'><thead class='thead-dark'><tr><th>Name</th><th>Actions</th></tr></thead><tbody>
    )";

    page += getDeviceListHTML();

    page += R"(
        </tbody></table>
        <h2>Add Device</h2>
        <form id='saveForm' onsubmit='saveDevice(); return false;'>
            <div class='form-group'>
                <label for='name'>Name:</label>
                <input type='text' class='form-control' id='name' name='name' required>
            </div>
            <div class='form-group'>
                <label for='mac'>MAC:</label>
                <input type='text' class='form-control' id='mac' name='mac' required>
            </div>
            <button type='submit' class='btn btn-primary'>Save</button>
        </form>
    </div>
</body>
</html>
    )";

    server.send(200, "text/html", page);
}

String getDeviceListHTML() {
    String html;
    for (int i = 0; i < MAX_DEVICES; i++) {
        String nameStr = String(deviceList[i].name);
        String macStr = String(deviceList[i].mac);
        if (nameStr[0] != '\0') {  // Check if the first character is not the null terminator
            html += "<tr><td>" + nameStr + "</td>";
            html += "<td><a href='/wake?mac=" + macStr + "' class='btn btn-sm btn-warning'>Wake-up</a> ";
            html += "<button onclick='deleteDevice(\"" + nameStr + "\")' class='btn btn-sm btn-danger'>Delete</button></td></tr>";
        }
    }
    return html;
}


void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void handleSave() {
    String name = server.arg("name");
    String mac = server.arg("mac");
    
    // Here you would add functionality to save 'name' and 'mac' to persistent storage
    
    if (addDeviceToEEPROM(name.c_str(), mac.c_str())) {
        server.send(200, "text/plain", "Device saved");
    } else {
        server.send(500, "text/plain", "Failed to save device");
    }
    server.sendHeader("Location", "/", true);
    server.send(303);
}

void handleWake() {
    String mac = server.arg("mac");
    
    // Send Wake-on-LAN packet
    sendWakeOnLAN(mac);
    
    // Redirect to main page after waking device
    server.sendHeader("Location", String("/"), true);
    server.send(302, "text/plain", "Wake-up packet sent");
}

void handleDelete() {
    String name = server.arg("name");
      
    if (removeDeviceFromEEPROM(name.c_str())) {
        server.send(200, "text/plain", "Device deleted");
    } else {
        server.send(500, "text/plain", "Failed to delete device");
    }
    server.sendHeader("Location", "/", true);
    server.send(303);
}

void reconnect() {
    // Loop until we're reconnected to the MQTT server
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect with username and password
        if (mqttClient.connect("ESP8266Client", mqtt_username, mqtt_password)) {
            Serial.println("connected");
            // Subscribe to the specified topic
            mqttClient.subscribe(mqtt_topic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

IPAddress calculateBroadcastIP(IPAddress ip, IPAddress subnetMask) {
    IPAddress broadcastIp = IPAddress((uint32_t)ip | ~(uint32_t)subnetMask);
    return broadcastIp;
}

// Wake-on-LAN-related functions
void sendWakeOnLAN(String macStr) {
    // Normalize MAC address by removing any separators
    macStr.replace(":", "");
    macStr.replace("-", "");
    Serial.print("Normalized MAC: ");
    Serial.println(macStr);

    if (macStr.length() != 12) {
        Serial.println("MAC address length is incorrect.");
        return;
    }

    byte mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = strtoul(macStr.substring(i*2, i*2+2).c_str(), NULL, 16);
    }

    Serial.print("Parsed MAC: ");
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) {
            Serial.print("0");
        }
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();

    byte magicPacket[102];
    memset(magicPacket, 0xFF, 6); // Start with 6 bytes of 0xFF
    for (int i = 1; i <= 16; i++) {
        memcpy(magicPacket + i * 6, mac, 6); // Repeat MAC 16 times
    }

    IPAddress broadcastIp(255, 255, 255, 255); // Global broadcast IP address
    UDP.beginPacket(broadcastIp, 9); // Standard Wake-on-LAN port
    UDP.write(magicPacket, sizeof(magicPacket));
    UDP.endPacket();

    Serial.println("Wake-on-LAN packet sent.");
}


bool parseMacAddress(String macStr, byte mac[6]) {
    // Remove colons and hyphens from the MAC address string
    macStr.replace(":", "");
    macStr.replace("-", "");

    // Check if the MAC address has a valid length
    if (macStr.length() != 12) {
        return false;
    }

    // Parse the MAC address
    for (int i = 0; i < 6; i++) {
        mac[i] = (hexCharacterToByte(macStr.charAt(i * 2)) << 4) | hexCharacterToByte(macStr.charAt(i * 2 + 1));
    }

    return true;
}


byte hexCharacterToByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    return 0; // Not a valid hexadecimal character
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Convert byte* to String for easy manipulation
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("MQTT message received: ");
    Serial.println(message);

    // Assuming the message is a MAC address directly
    sendWakeOnLAN(message);
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/wake", HTTP_GET, handleWake);
    server.on("/delete", HTTP_GET, handleDelete);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}