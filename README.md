# 🔐 Smart Bank Locker System Using RFID and Password Authentication

## 📌 Project Overview

The **Smart Bank Locker System** is an Embedded Systems project developed using the LPC2129 ARM7 microcontroller and Embedded C.

The system provides two-level authentication to control locker access:

1. **RFID Authentication:** An authorized RFID card is scanned using the EM-18 RFID reader.
2. **Password Authentication:** After successful RFID verification, the user must enter a four-digit password using a 4x4 matrix keypad.

If both authentication steps are successful, access is granted and the motor-driven locking mechanism is activated. If the user enters an incorrect password three consecutive times, access is denied and an alert is generated using an LED and buzzer.

## 🎯 Project Objectives

- Implement RFID-based user identification.
- Provide an additional password verification mechanism.
- Control a motor-driven locker mechanism.
- Display system status through a 16x2 LCD.
- Generate alerts for unauthorized access.
- Understand microcontroller interfacing and peripheral programming.

## 🛠️ Hardware Components

| Component | Purpose |
|---|---|
| LPC2129 ARM7 Microcontroller | Main controller |
| EM-18 RFID Reader | Reads RFID card identification data |
| RFID Card/Tag | User identification |
| 4x4 Matrix Keypad | Password entry |
| 16x2 LCD | Displays system messages |
| DC Motor | Locker locking/unlocking mechanism |
| Motor Driver | Controls the DC motor |
| LED | Access status indication |
| Buzzer | Unauthorized access alert |

## 💻 Software and Technologies

- Programming Language: Embedded C
- Microcontroller: LPC2129 ARM7
- IDE: Keil µVision
- Communication Protocol: UART
- Hardware Interfacing: GPIO
- Version Control: GitHub

## ⚙️ Working Principle

### 1. System Initialization
The microcontroller initializes the LCD, UART communication, GPIO pins and keypad interface.

### 2. RFID Authentication
The EM-18 RFID reader reads the RFID tag and sends its identification data to the microcontroller through UART0.

The received identification data is compared with the authorized RFID ID stored in the program.

### 3. Password Verification
If the RFID card is authorized, the LCD prompts the user to enter a four-digit password using the matrix keypad.

The entered password is compared with the predefined password.

### 4. Access Granted
If the password is correct:
- The LCD displays ACCESS GRANTED.
- The LED indicates successful authentication.
- The motor driver activates the DC motor.
- The motor operates for a predefined duration.

### 5. Access Denied
If the password is incorrect:
- The LCD displays WRONG PASSWORD.
- The number of attempts is updated.
- After three incorrect attempts, the system displays ACCESS DENIED.
- The LED and buzzer generate an alert.

### 6. Invalid RFID Card
If an unauthorized RFID card is scanned, the LCD displays INVALID CARD and access is not granted.

## 📂 Project Structure

```text
RFID-Smart-Locker-System/
│
├── main.c
├── RFID.C
├── lcd_header.h
├── matrix_keypad.c
├── matrix_keypad.h
├── Startup.s
├── smart_bank_locker_sysytem.uvproj
├── smart_bank_locker_sysytem.sct
└── README.md
```

## 🔌 Communication and Interfacing

- **UART0:** Communication between the EM-18 RFID reader and LPC2129.
- **GPIO:** Controls the LED, buzzer and motor driver.
- **LCD Interface:** Displays authentication and system status messages.
- **Matrix Keypad:** Accepts the user's password input.

Refer to the source code and hardware configuration for the complete pin connections.

## 🚀 Future Improvements

- Store multiple authorized RFID cards.
- Implement dynamic password management.
- Integrate EEPROM-based credential storage.
- Add administrator authentication.
- Implement an access log system.
- Introduce stronger credential protection and tamper detection.

## 📚 Learning Outcomes

Through this project, I gained practical experience in:

- ARM7 microcontroller programming.
- Embedded C development.
- UART serial communication.
- RFID interfacing.
- LCD and matrix keypad interfacing.
- GPIO configuration and control.
- Motor driver interfacing.
- Hardware and software integration.

## ⚠️ Project Disclaimer

This is an educational Embedded Systems prototype. RFID IDs and passwords are currently stored in the program, so this implementation is not suitable for real banking or production security applications.

## 👨‍💻 Author

**Allada Sri Harish**

B.Tech – Electronics and Communication Engineering

Interested in Embedded Systems, Firmware Development and ARM Microcontrollers.
