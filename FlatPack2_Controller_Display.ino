//Modified and slimmed code from https://github.com/The6P4C/Flatpack2
//Serial at 115200 baud with Carriage return.  Set voltage with "Sxxyy" xx.yy volts
//Uses Adafruit SSD1306 library
//Uses mcp_can library

#include <mcp_can.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

#define OLED_RESET 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int CAN_CS_PIN = 10;
const int CAN_INT_PIN = 7;

const char *alarms0Strings[] = {"OVS_LOCK_OUT", "MOD_FAIL_PRIMARY", "MOD_FAIL_SECONDARY", "HIGH_MAINS", "LOW_MAINS", "HIGH_TEMP", "LOW_TEMP", "CURRENT_LIMIT"};
const char *alarms1Strings[] = {"INTERNAL_VOLTAGE", "MODULE_FAIL", "MOD_FAIL_SECONDARY", "FAN1_SPEED_LOW", "FAN2_SPEED_LOW", "SUB_MOD1_FAIL", "FAN3_SPEED_LOW", "INNER_VOLT"};

byte receivedChar;
byte receivedCommand[20];

int oldInputVoltage=-1; int oldOutputVoltage=-1; int oldCurrent=-1; int oldIntakeTemperature=-1; int oldOutputTemperature=-1;
int receivedIndex = 0;

MCP_CAN CAN(CAN_CS_PIN);

bool serialNumberReceived = false;

bool setVoltage = false;
int tempSetVoltage = 0;

uint8_t serialNumber[6];

unsigned long lastLogInTime = 0;

void setup() 
{
  Serial.begin(115200);

  pinMode(CAN_CS_PIN, OUTPUT);
  pinMode(CAN_INT_PIN, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  display.clearDisplay();
  display.display();
  
  if (CAN.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) 
  {
  } 
  else 
  {
    display.clearDisplay();
    display.setCursor(0,8);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print("MCP2515 FAIL");

    display.display();
    while(1);
  }

  CAN.setMode(MCP_NORMAL);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  display.setCursor(30,0);
  display.print("->");
  display.setCursor(30,8);
  display.print("->");

  display.display();
}

void loop() 
{
  while(Serial.available() > 0)
  {
    receivedChar = Serial.read();
    if(receivedChar != 13)
    {
      receivedCommand[receivedIndex] = receivedChar;
      receivedIndex++;
    }
    else
    {
      switch(receivedCommand[0])
      {
        case 83: //Sxxyy; set voltage, xx.yy
          tempSetVoltage = 0;

          for(int i = 1; i < receivedIndex; i++)
          {
            tempSetVoltage = tempSetVoltage + ((receivedCommand[i]-48)*pow(10,(receivedIndex-i-1)));
          }
          tempSetVoltage++;

          if(tempSetVoltage < 4350 || tempSetVoltage > 5760)
          {
            Serial.print("Voltage ");Serial.print(tempSetVoltage);Serial.println(" out of range(43.5~56.7)");

            break;
          }
          
          Serial.print("New voltage = ");
          Serial.println((float)tempSetVoltage/100,2);

          setVoltage = true;
          break;

        default:
          Serial.println("Invalid Command");

          break;
        
      }
      receivedIndex = 0;
    }
  }
  
  // Log in every second
  if (serialNumberReceived && millis() - lastLogInTime > 1000)
  {
    uint8_t txBuf[8] = { 0 };
    for (int i = 0; i < 6; ++i) 
    {
      txBuf[i] = serialNumber[i];
    }
    CAN.sendMsgBuf(0x05004804, 1, 8, txBuf);
    lastLogInTime = millis();

    if (setVoltage)
    {
      delay(100);

      uint8_t voltageSetTxBuf[5] = { 0x29, 0x15, 0x00, tempSetVoltage & 0xFF, (tempSetVoltage >> 8) & 0xFF };

      CAN.sendMsgBuf(0x05019C00, 1, 5, voltageSetTxBuf);

      Serial.println("Set completed, Cycle power.");      
      
      setVoltage = false;

      display.clearDisplay();
      display.fillScreen(BLACK);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.print("CYCLE PWR");

      display.display();
      while(1);
    }
  }


  // Active low
  if (!digitalRead(CAN_INT_PIN)) {
    uint32_t rxID;
    uint8_t len = 0;
    uint8_t rxBuf[8];

    CAN.readMsgBuf((unsigned long *)&rxID, &len, rxBuf);

    // Limit ID to lowest 29 bits (extended CAN)
    rxID &= 0x1FFFFFFF;

    if (!serialNumberReceived && (rxID & 0xFFFF0000) == 0x05000000){for (int i = 0; i < 6; ++i){serialNumber[i] = rxBuf[i + 1];}serialNumberReceived = true;} 
    
    else if ((rxID & 0xFFFFFF00) == 0x05014000) 
    {
      int intakeTemperature = rxBuf[0];
      int current = (0.1f * (rxBuf[1] | (rxBuf[2] << 8)))*10;//output current * 10
      int outputVoltage = (0.01f * (rxBuf[3] | (rxBuf[4] << 8)))*10; //output voltage * 10
      int inputVoltage = rxBuf[5] | (rxBuf[6] << 8);
      int outputTemperature = rxBuf[7];

      if(oldInputVoltage != inputVoltage)
      {
        display.setTextSize(1);
        display.setCursor(0,8);
        display.setTextColor(SSD1306_BLACK);
        display.print(oldInputVoltage);
        display.print("V");

        display.setCursor(0,8);
        display.setTextColor(SSD1306_WHITE);
        display.print(inputVoltage);
        display.print("V");

        display.display();
        
        oldInputVoltage = inputVoltage;
      }

      if(oldOutputVoltage != outputVoltage)
      {
        display.setTextSize(1);
        display.setCursor(48,8);
        display.setTextColor(SSD1306_BLACK);
        display.print((float)oldOutputVoltage/10,1);
        display.print("V");

        display.setCursor(48,8);
        display.setTextColor(SSD1306_WHITE);
        display.print((float)outputVoltage/10,1);
        display.print("V");

        display.display();

        oldOutputVoltage = outputVoltage;
      }

      if(oldIntakeTemperature != intakeTemperature)
      {
        display.setTextSize(1);
        display.setCursor(0,0);
        display.setTextColor(SSD1306_BLACK);
        display.print(oldIntakeTemperature);
        display.print("C");

        display.setCursor(0,0);
        display.setTextColor(SSD1306_WHITE);
        display.print(intakeTemperature);
        display.print("C");

        display.display();

        oldIntakeTemperature = intakeTemperature;
      }

      if(oldOutputTemperature != outputTemperature)
      {
        display.setTextSize(1);
        display.setCursor(48,0);
        display.setTextColor(SSD1306_BLACK);
        display.print(oldOutputTemperature);
        display.print("C");

        display.setCursor(48,0);
        display.setTextColor(SSD1306_WHITE);
        display.print(outputTemperature);
        display.print("C");

        display.display();

        oldOutputTemperature = outputTemperature;
      }

      if(oldCurrent != current)
      {
        display.setTextSize(2);
        display.setCursor(0,16);
        display.setTextColor(SSD1306_BLACK);
        display.print((float)oldCurrent/10,1);
        display.print("A");

        display.setCursor(0,16);
        display.setTextColor(SSD1306_WHITE);
        display.print((float)current/10,1);
        display.print("A");

        oldCurrent = current;
      }
      
      bool hasWarning = rxID == 0x05014008;
      bool hasAlarm = rxID == 0x0501400C;

      if (hasWarning) 
      {
        Serial.println("WARNING");
      } 
      else if (hasAlarm) 
      {
        Serial.println("ALARM");
      }

      if (hasWarning || hasAlarm) 
      {
        uint8_t txBuf[3] = {0x08, hasWarning ? 0x04 : 0x08, 0x00};
        CAN.sendMsgBuf(0x0501BFFC, 1, 3, txBuf);
      }
    } 
    else if (rxID == 0x0501BFFC) 
    {
      bool isWarning = rxBuf[1] == 0x04;

      Serial.println("--------");
      if (isWarning) 
      {
        Serial.print("Warnings:");
      } 
      else 
      {
        Serial.print("Alarms:");
      }

      uint8_t alarms0 = rxBuf[3];
      uint8_t alarms1 = rxBuf[4];
 
      for (int i = 0; i < 8; ++i) 
      {
        if (alarms0 & (1 << i)) 
        {
          Serial.print(" ");
          Serial.print(alarms0Strings[i]);
        }

        if (alarms1 & (1 << i)) 
        {
          Serial.print(" ");
          Serial.print(alarms1Strings[i]);
        }
      }

      Serial.println();
      Serial.println("--------");;
    }
  }
}
