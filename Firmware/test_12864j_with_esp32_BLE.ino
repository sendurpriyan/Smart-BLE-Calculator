#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <vector>
#include <string>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

// Display setup
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* CS=*/ 5, /* reset=*/ 22); // ESP32

std::string equation = "";     // Full equation stored
std::string currentValue = ""; // Current input after last operator
float result = 0;
bool showResult = false;       // Flag to show result
bool errorOccurred = false;    // Error flag

BLECharacteristic *pCharacteristic;

// Function to extract the last entered number
std::string getCurrentNumber(const std::string &eq) {
    size_t lastOp = eq.find_last_of("+-*/");
    return (lastOp == std::string::npos) ? eq : eq.substr(lastOp + 1);
}

// Function to evaluate the expression manually
float evaluateExpression(const std::string &expr) {
    std::vector<float> numbers;
    std::vector<char> operators;
    std::string num = "";

    errorOccurred = false; // Reset error flag

    for (char c : expr) {
        if (isdigit(c) || c == '.') {
            num += c;
        } else {
            if (!num.empty()) {
                numbers.push_back(std::stof(num));
                num = "";
            }
            if (c == '+' || c == '-' || c == '*' || c == '/') {
                operators.push_back(c);
            } else {
                errorOccurred = true; // Invalid character detected
                return 0;
            }
        }
    }
    if (!num.empty()) numbers.push_back(std::stof(num));

    if (numbers.empty()) {
        errorOccurred = true;
        return 0;
    }

    // Perform multiplication and division first
    for (size_t i = 0; i < operators.size(); i++) {
        if (operators[i] == '*' || operators[i] == '/') {
            float left = numbers[i];
            float right = numbers[i + 1];
            if (operators[i] == '/' && right == 0) {
                errorOccurred = true; // Division by zero
                return 0;
            }
            float res = (operators[i] == '*') ? (left * right) : (left / right);
            numbers[i] = res;
            numbers.erase(numbers.begin() + i + 1);
            operators.erase(operators.begin() + i);
            i--;  // Adjust index after deletion
        }
    }

    // Perform addition and subtraction
    result = numbers[0];
    for (size_t i = 0; i < operators.size(); i++) {
        if (operators[i] == '+') result += numbers[i + 1];
        else if (operators[i] == '-') result -= numbers[i + 1];
    }
    return result;
}

// Function to update the display
void updateDisplay() {
    u8g2.clearBuffer();

    if (showResult) {
        // ** Display ONLY the grand total when '=' is pressed **
        char resultStr[20];
        snprintf(resultStr, sizeof(resultStr), "%.2f", result);
        u8g2.setFont(u8g2_font_fub14_tr);  // Large font to fit screen
        u8g2.drawStr(2, 18, resultStr);
    } else {
        // ** Limit equation display to the last 18 characters **
        std::string displayEquation = (equation.length() > 18) ? equation.substr(equation.length() - 18) : equation;

        // ** Display equation at the top-right **
        u8g2.setFont(u8g2_font_7x14B_tr);
        int equationX = 128 - (displayEquation.length() * 7);
        equationX = equationX < 1 ? 1 : equationX;
        u8g2.drawStr(equationX, 12, displayEquation.c_str());

        // ** Display current input in medium font **
        u8g2.setFont(u8g2_font_fub14_tr);
        u8g2.drawStr(5, 50, currentValue.c_str());
    }

    u8g2.sendBuffer();
}

// BLE Callback class
class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String receivedValue = pCharacteristic->getValue();
        if (!receivedValue.isEmpty()) {
            if (receivedValue == "ac") {
                // ** Reset everything when "ac" is received **
                equation.clear();
                currentValue.clear();
                result = 0;
                showResult = false;
                errorOccurred = false;
            } 
            else if (receivedValue == "=") {
                // ** Evaluate equation and show only the grand total **
                result = evaluateExpression(equation);
                showResult = true;
                currentValue.clear();
            } 
            else if (receivedValue == "c") {
                // ** Clear only the last entered number after last operator **
                size_t lastOp = equation.find_last_of("+-*/");
                if (lastOp != std::string::npos) {
                    equation = equation.substr(0, lastOp + 1);  // Keep operators
                } else {
                    equation.clear();  // If no operator found, clear all
                }
                currentValue.clear();
                showResult = false;
            } 
            else {
                // ** Append input to equation normally **
                equation += receivedValue.c_str();
                currentValue = getCurrentNumber(equation);
                showResult = false;
            }
            updateDisplay();
        }
    }
};

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    updateDisplay();

    BLEDevice::init("ESP32_BLE_Calculator");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    Serial.println("ESP32 BLE Calculator Started");
}

void loop() {
    // BLE handling in the background
}
