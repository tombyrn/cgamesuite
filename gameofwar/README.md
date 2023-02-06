# Game of War
Take on Conway's Game of Life

Same rules as the Game of Life but with an added element of infighting between the cells.
Cells are randomly assigned a team on creation (0 -> Red, 1 -> Blue).
If at some point all Cells in the grid are the same color the game ends.


Modified Rules
    1. Any live cell with fewer than two live neighbours dies, as if by underpopulation.
    2. Any live cell with two or three live neighbours lives on to the next generation, as long as there are more neighbors on the same team.
    3. Any live cell with more than three live neighbours dies, given at least one is on the opposing team.
    4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
    5. Any live cell in which the top and bottom, or left and right neighbors are on the opposite team, dies.

Usage
    Compile
    ```
    make
    ```

    Run
    ```
    ./gameofwar
    ```



