# Final Project Files
`serverCode/` and `clientCode/` are both seperate folders that build the LVGL based Pico project for 2 Picos, where one is the Server Host, and the other will act as the Client Device connecting to the Host.

`/sw` will be the main file path where all project code is submitted to

`battleship.h`, `cells.h`, `lvlg_example_grid.h`, `lvlg_example_grid.cpp`, `lwipopts.h` and `main.cpp` are all files editted and created by us in order to make our project work.

`hmasr001_jmarc046_custom_lab_report.pdf` is our Final Project Report

Keep in mind almost all files are utilizing the icesugar-pro-framebuffer provided by Allan Knight, they are required to build the project, those instructions are in the README in either "Code" folder. LVGL located in `sw/` is also a submodule connected to the LVGL github repo.


# CS122A Custom Lab Project Proposal

## Introduction
We are planning on doing an online 2 player game (specifically battleship) across two devices. It follows the classic battleship rules, and players will be able to play remotely without seeing each other’s screens or setups.

## Hardware
- 2 x Icesugar-pro FPGA
- 2 x RGB LCD
- 2 x Raspberry Pi Pico
- 2 x Joysticks
- Buttons
- Resistors
- Wires

 ## Functionality
Each player will have an identical setup of an FPGA, LCD, and Pico. The FPGA is used for image processing and displaying to the RGB LCD. The picos will handle game logic, memory, and wireless communication. One Pico will be used as the host and run the web server that will run the game and interact with it, while the other Pico will connect to it and interact with the game as well. The RGB screens will be used to display the ships grid and the target grid of a player. On a player’s turn, they choose a cell on their target grid using the joystick. Then, they press a button to shoot an attack on the other player. The players are trying to hit all the cells in their opponents grid. They each take turns attacking the other player. Whoever hits all the ships of their opponent wins.

