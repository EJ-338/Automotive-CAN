
#include <SPI.h>
#include <mcp_can.h>

// =======================
// Pin Definitions
// =======================

#define ENCODOUTPUT 540      // Pulses per revolution
#define PWM 9                // L298N PWM pin
#define encoder0pinA 3       // Hall sensor output

// =======================
// CAN Bus Setup
// =======================

unsigned char buf[8];
const int SPI_CS_PIN = 10;

MCP_CAN CAN(SPI_CS_PIN);

// =======================
// Motor Driver Pins
// =======================

int m_left = 4;   // L298N IN1
int t_left = 5;   // L298N IN2

// =======================
// Variables
// =======================

volatile unsigned long encoderValue = 0;

unsigned long lastMillis = 0;

float rpm = 0.0;
float wheelRadius = 0.03;   // meters

// =======================
// Setup
// =======================

void setup()
{
    Serial.begin(9600);

    // Motor pins
    pinMode(m_left, OUTPUT);
    pinMode(t_left, OUTPUT);

    // Encoder pin
    pinMode(encoder0pinA, INPUT_PULLUP);

    // PWM pin
    pinMode(PWM, OUTPUT);

    // Set motor direction
    digitalWrite(m_left, LOW);
    digitalWrite(t_left, HIGH);

    // Encoder interrupt
    attachInterrupt(
        digitalPinToInterrupt(encoder0pinA),
        updateEncoder,
        FALLING
    );

    // Initialize CAN bus
    while (CAN_OK != CAN.begin(CAN_500KBPS))
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println("Retrying...");
        delay(100);
    }

    Serial.println("CAN BUS Shield init OK!");
}

// =======================
// Main Loop
// =======================

void loop()
{
    unsigned char len = 0;

    // Check for CAN message
    if (CAN_MSGAVAIL == CAN.checkReceive())
    {
        // Read CAN data
        CAN.readMsgBuf(&len, buf);

        unsigned long canId = CAN.getCanId();

        Serial.println("--------------------");
        Serial.print("CAN ID: ");
        Serial.println(canId, HEX);

        // Safety check
        if (len >= 8)
        {
            int value = (int)buf[7];

            Serial.print("Received Value: ");
            Serial.println(value);

            // Map command value to PWM
            if (value >= 12 && value <= 25)
            {
                int pwmOutput = map(value, 12, 25, 0, 255);

                analogWrite(PWM, pwmOutput);

                // Calculate every second
                if (millis() - lastMillis >= 1000)
                {
                    // Disable interrupt while calculating
                    detachInterrupt(
                        digitalPinToInterrupt(encoder0pinA)
                    );

                    // RPM calculation
                    rpm = ((float)encoderValue * 60.0)
                          / ENCODOUTPUT;

                    // Linear velocity
                    float velocity =
                        ((2.0 * PI) / 60.0)
                        * rpm
                        * wheelRadius;

                    // Print data
                    Serial.print("RPM = ");
                    Serial.println(rpm);

                    Serial.print("Linear Velocity (m/s) = ");
                    Serial.println(velocity);

                    Serial.print("PWM = ");
                    Serial.println(pwmOutput);

                    // Reset encoder count
                    encoderValue = 0;

                    // Reset timer
                    lastMillis = millis();

                    // Re-enable interrupt
                    attachInterrupt(
                        digitalPinToInterrupt(encoder0pinA),
                        updateEncoder,
                        FALLING
                    );
                }
            }
            else
            {
                stopMotor();
            }
        }
    }
}

// =======================
// Encoder Interrupt
// =======================

void updateEncoder()
{
    encoderValue++;
}

// =======================
// Stop Motor Function
// =======================

void stopMotor()
{
    analogWrite(PWM, 0);

    Serial.println("Motor Stopped");
}