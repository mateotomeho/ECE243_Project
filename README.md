# ECE243-Project: Tower of Hanoi

This project implements a digital recreation of the Tower of Hanoi puzzle that can run on the DE1-SoC board. To develop and test the game, you can use the [CPulator simulator](https://cpulator.01xz.net/?sys=rv32-de1soc).

## Instructions

The goal is to move a stack of disks from the first peg to the third peg, following these rules:

1. Only one disk can be moved at a time.  
2. A larger disk cannot be placed on top of a smaller disk.  
3. You must move the entire stack to the third peg within a 90-second timer.

### Difficulty Modes
- **Easy**: 3 disks  
- **Medium**: 4 disks  
- **Hard**: 5 disks  

Select the difficulty by pressing **E**, **M**, or **H** on the PS/2 keyboard. Move disks according to Tower of Hanoi rules until they are all stacked on the last peg. If the timer runs out, you lose. At the end of the game, a summary screen shows your current score and highest score achieved.

## Running the Game on CPulator

1. Go to [CPulator](https://cpulator.01xz.net/?sys=rv32-de1soc).
2. Click **File > Open** and select the `TowerOfHanoi.c` file.  
3. Click the **compile and load** button or press <kbd>F5</kbd>.  
4. Press **Continue** on the top bar or press <kbd>F3</kbd> to start execution.  
5. Optionally, increase the size of the VGA pixel buffer by clicking the triangle on the side of the pixel buffer window.  
6. Under **PS/2 Keyboard or Mouse**, click in the field labeled **"type here:"**. The game will now read your keyboard inputs.  
7. To restart the game, press the **Reload** button on the top of CPulator (remember to click back into the **"type here:"** field).

## Controls

- **E / M / H**: Select Easy, Medium, or Hard mode at the start screen.  
- **Arrow Keys or Specified Keys**: Move and/or pick up disks (exact controls may vary depending on your implementation).  
- **R**: Restart the game at any time.  

## Notes

- The timer starts immediately once a mode is selected.  
- A game-over screen displays if the timer reaches zero before you complete the puzzle.  
- A winning screen shows final stats and your high score if you solve the puzzle in time.  

Enjoy your Tower of Hanoi adventure on the CPUlator!
