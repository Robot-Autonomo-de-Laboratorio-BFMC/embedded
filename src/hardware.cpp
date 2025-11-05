#include <Arduino.h>
#include "hardware.h"
#include <ESP32Servo.h>

static const char *TAG = "hardware";
static Servo steerServo;

void hardware_init(void) {
    // GPIO configuration for outputs
    pinMode(GPIO_MOTOR_IN3, OUTPUT);
    pinMode(GPIO_MOTOR_IN4, OUTPUT);
    pinMode(GPIO_MOTOR_ENB, OUTPUT);
    pinMode(GPIO_HEADLIGHTS, OUTPUT);
    pinMode(GPIO_REVERSE_LIGHTS, OUTPUT);
    pinMode(GPIO_LED_BUILTIN, OUTPUT);
    pinMode(GPIO_ESTOP, INPUT_PULLUP);
    
    // LDR is analog input, no pinMode needed for GPIO 35
    
    // Initialize GPIO states
    digitalWrite(GPIO_MOTOR_IN3, LOW);
    digitalWrite(GPIO_MOTOR_IN4, LOW);
    digitalWrite(GPIO_HEADLIGHTS, LOW);
    digitalWrite(GPIO_REVERSE_LIGHTS, LOW);
    digitalWrite(GPIO_LED_BUILTIN, LOW);
    
    // Initialize servo
    steerServo.setPeriodHertz(SERVO_PWM_FREQ_HZ);
    steerServo.attach(GPIO_SERVO, 500, 2500);
    steerServo.write(SERVO_CENTER);
    
    // Initialize Serial1 for UART communication
    Serial1.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    // Center servo on startup
    steer_set_angle(SERVO_CENTER);
    
    Serial.println("Hardware initialized");
}

void motor_set_speed(uint8_t speed) {
    if (speed > MOTOR_SPEED_MAX) {
        speed = MOTOR_SPEED_MAX;
    }
    analogWrite(GPIO_MOTOR_ENB, speed);
}

void motor_set_direction(bool forward) {
    if (forward) {
        digitalWrite(GPIO_MOTOR_IN3, HIGH);
        digitalWrite(GPIO_MOTOR_IN4, LOW);
    } else {
        digitalWrite(GPIO_MOTOR_IN3, LOW);
        digitalWrite(GPIO_MOTOR_IN4, HIGH);
    }
}

void motor_stop(void) {
    digitalWrite(GPIO_MOTOR_IN3, LOW);
    digitalWrite(GPIO_MOTOR_IN4, LOW);
    analogWrite(GPIO_MOTOR_ENB, 0);
}

void steer_set_angle(uint16_t angle) {
    steerServo.write(angle);
}

void lights_set_headlights(bool on) {
    digitalWrite(GPIO_HEADLIGHTS, on ? HIGH : LOW);
}

void lights_set_reverse(bool on) {
    digitalWrite(GPIO_REVERSE_LIGHTS, on ? HIGH : LOW);
}

uint16_t ldr_read(void) {
    return analogRead(GPIO_LDR);
}

bool estop_is_triggered(void) {
    // E-STOP is active low (pulled up, triggers when grounded)
    return digitalRead(GPIO_ESTOP) == LOW;
}
