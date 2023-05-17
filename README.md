# ESBank

This is an LPC1768 project with the intent of interfacing with the SIM900A GSM module. This project simulates an ATM with SMS features using SIM900A, LCD and keyboard matrix. Overall, this project at its core shows how we can send and receive SMS using an LPC1768.

# Features

* Login via SMS: User can send a secret code via SMS to the number on the SIM. If the code matches corresponding account, the account gets logged in
* Check balance: General function that can check user balance. Results are displayed on both LCD and SMS message.
* Withdraw money: User can use keyboard matrix to enter upto 6 digit amount value. An SMS notification will be sent notifying user of successful withdrawl.

# Pin Connections

* 16x2 LCD: GPIO pins P0.23-P0.28
* Keyboard matrix: GPIO row pins P2.10-2.13 and column pins P1.23-1.26
* SIM900A: UART1 pins on P0.15 (TXD) and P0.16 (RXD)

# Possible Future Work

* Use GPRS to access database for data persistence.
* Use TFT screen.
* Use 4G/5G module offerings from SIMCOM (My pockets are not deep enough to purchase these)

# Screenshots

![image](https://github.com/Skryptonyte/ESBank/assets/40635533/ab200bd7-1353-4682-b2a3-00b9d03af4e4)
![image](https://github.com/Skryptonyte/ESBank/assets/40635533/a6b77003-e7a6-43a7-9ca6-f78196bd5cbc)
