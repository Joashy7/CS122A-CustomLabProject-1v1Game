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

