# Game of Life
- By Neonfuz

## Summary:
This is a game of life implementation written by me (neonfuz). It is
slightly more optimized than my earlier straight forward attempts at
making a game of life implementation, because it stores sums of
surrounding cells, rather than recalculating every frame. It also
directly interprets the buffers as images to speed up drawing.

## Controlls:
- Left mouse: Draw cells
- Right mouse: Erase cells
- P: Pause
- S: Step forward one frame
- R: Fill screen with random cells
- C: Clear the screen
- V: Change view

## Compilation:
Edit config.h to configure, then simply run make.

## Features to add:
- Looping edges
- Colors per frame (looping?)
- different rulesets
- resizing scale/size
- load/save full patterns / stamps

## Bugfixes:
- clean up palette loading


