#include <SPI.h>
#include <MFRC522.h>
#include <WiFiClient.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include "proto/employee_service.grpc.pb.h"

#define SS_PIN D8
#define RST_PIN D0

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

int redLED = D3;
int greenLED = D2;
int yellowLED = D1;
int buzzer = D9;

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
const char* server_address = "10.15.43.77:50051"; // Address to the server

void setup()
{
  pinMode(redLED, OUTPUT); // Red LED
  pinMode(greenLED, OUTPUT); // Green LED
  pinMode(yellowLED, OUTPUT); // Yellow LED
  pinMode(buzzer, OUTPUT); // Set buzzer pin as an output

  Serial.begin(9600); // Initiate a serial communication
  SPI.begin();        // Initiate SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522

  Serial.println("Approximate your card to the reader...");
  Serial.println();

  digitalWrite(yellowLED, HIGH); // Turn on RFID ready LED

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop()
{
  digitalWrite(yellowLED, HIGH);

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    digitalWrite(yellowLED, HIGH);
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
  {
    digitalWrite(yellowLED, LOW);
    return;
  }

  // Show UID on serial monitor
  Serial.print("UID tag: ");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message: ");
  content.toUpperCase();

  // Connect to the gRPC server
  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
  employee_service::EmployeeService::Stub stub(channel);

  employee_service::Employee employee;
  employee.set_card_uid(content);

  // Check if the card is authorized
  grpc::ClientContext context;
  employee_service::AuthorizationReply reply;
  grpc::Status status = stub.Authorize(&context, employee, &reply);

  if (status.ok()) {
    if (reply.authorized()) {
      digitalWrite(yellowLED, LOW);
      Serial.println("Authorized access");
      Serial.println();

      // Buzzer sound for authorized access
      tone(buzzer, 1000); // Set buzzer to 1000 Hz
      delay(200);
      noTone(buzzer); // Stop buzzer

      // Green LED indicator
      digitalWrite(greenLED, HIGH);
      delay(200);
      digitalWrite(greenLED, LOW);
      delay(200);
    } else {
      digitalWrite(yellowLED, LOW);
      Serial.println("Access denied");
      Serial.println();

      // Buzzer sound for denied access
      tone(buzzer, 500); // Set buzzer to 500 Hz
      delay(200);
      noTone(buzzer); // Stop buzzer

      // Red LED indicator
      digitalWrite(redLED, HIGH);
      delay(200);
      digitalWrite(redLED, LOW);
      delay(200);
    }
  } else {
    Serial.println("Could not connect to the server");
    Serial.println();
  }

  digitalWrite(yellowLED, LOW);
}
