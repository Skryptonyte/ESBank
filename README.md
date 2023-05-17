# ESBank

This is an LPC1768 project with the intent of interfacing with the SIM900A GSM module. This project simulates an ATM with SMS features using SIM900A, LCD and keyboard matrix. Overall, this project at its core shows how we can send and receive SMS using an LPC1768.

# Features

* Login via SMS: User can send a secret code via SMS to the number on the SIM. If the code matches corresponding account, the account gets logged in
* Check balance: General function that can check user balance. Results are displayed on both LCD and SMS message.
* Withdraw money: User can use keyboard matrix to enter upto 6 digit amount value. An SMS notification will be sent notifying user of successful withdrawl.

# Possible Future Work

* Use GPRS to access database for data persistence.
* Use TFT screen.