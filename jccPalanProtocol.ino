#include <Keyboard.h>

/**
 * StenoFW is a firmware for Stenoboard keyboards.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2014 Emanuele Caruso. See LICENSE.txt for details.
 */
 
#define ROWS 4
#define COLS 12

/* The following matrix is shown here for reference only.
    KEYS_LAYOUT = '''
           P- M- N-         -N -M -P
        C- T- F- L-         -L -F -T -H
        S- H- R- Y- O- I -A -C -R -+ -S
          +1-  +2-  E- I -U  -^1  -^2
    '''
};*/

// Configuration variables
int rowPins[ROWS] = {0, 1, 2, 3};
int colPins[COLS] = {A0, A1, A2, A3, 4, 5, 6, 7, 8, 9, 10, 11};
long debounceMillis = 20;

// Keyboard state variables
boolean isStrokeInProgress = false;
boolean currentChord[ROWS][COLS];
boolean currentKeyReadings[ROWS][COLS];
boolean debouncingKeys[ROWS][COLS];
unsigned long debouncingMicros[ROWS][COLS];

// Protocol state
#define JCCPALAN 5
int protocol = JCCPALAN;

// This is called when the keyboard is connected
void setup() {
  Keyboard.begin();
  Serial.begin(9600);
  for (int i = 0; i < COLS; i++)
    pinMode(colPins[i], INPUT_PULLUP);
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  clearBooleanMatrixes();
}

// Read key states and handle all chord events
void loop() {
  readKeys();
  
  boolean isAnyKeyPressed = true;
  
  // If stroke is not in progress, check debouncing keys
  if (!isStrokeInProgress) {
    checkAlreadyDebouncingKeys();
    if (!isStrokeInProgress) checkNewDebouncingKeys();
  }
  
  // If any key was pressed, record all pressed keys
  if (isStrokeInProgress) {
    isAnyKeyPressed = recordCurrentKeys();
  }
  
  // If all keys have been released, send the chord and reset global state
  if (!isAnyKeyPressed) {
    sendChord();
    clearBooleanMatrixes();
    isStrokeInProgress = false;
  }
}

// Record all pressed keys into current chord. Return false if no key is currently pressed
boolean recordCurrentKeys() {
  boolean isAnyKeyPressed = false;
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true) {
        currentChord[i][j] = true;
        isAnyKeyPressed = true;
      }
    }
  }
  return isAnyKeyPressed;
}

// If a key is pressed, add it to debouncing keys and record the time
void checkNewDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (currentKeyReadings[i][j] == true && debouncingKeys[i][j] == false) {
        debouncingKeys[i][j] = true;
        debouncingMicros[i][j] = micros();
      }
    }
  }
}

// Check already debouncing keys. If a key debounces, start chord recording.
void checkAlreadyDebouncingKeys() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (debouncingKeys[i][j] == true && currentKeyReadings[i][j] == false) {
        debouncingKeys[i][j] = false;
        continue;
      }
      if (debouncingKeys[i][j] == true && micros() - debouncingMicros[i][j] / 1000 > debounceMillis) {
        isStrokeInProgress = true;
        currentChord[i][j] = true;
        return;
      }
    }
  }
}

// Set all values of all boolean matrixes to false
void clearBooleanMatrixes() {
  clearBooleanMatrix(currentChord, false);
  clearBooleanMatrix(currentKeyReadings, false);
  clearBooleanMatrix(debouncingKeys, false);
}

// Set all values of the passed matrix to the given value
void clearBooleanMatrix(boolean booleanMatrix[][COLS], boolean value) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      booleanMatrix[i][j] = value;
    }
  }
}

// Read all keys
void readKeys() {
  for (int i = 0; i < ROWS; i++) {
    digitalWrite(rowPins[i], LOW);
    for (int j = 0; j < COLS; j++)
      currentKeyReadings[i][j] = digitalRead(colPins[j]) == LOW ? true : false;
    digitalWrite(rowPins[i], HIGH);
  }
}
 
// Send current chord over serial using the modified Gemini protocol.
// for Palan by JCC 
void sendChordJCCPalan() {
  // Initialize chord bytes
  byte chordBytes[] = {B10000000, B0, B0, B0, B0, B0};

/* KEYS_LAYOUT = '''
           P- M- N-         -N -M -P
        C- T- F- L-         -L -F -T -H
        S- H- R- Y- O- I -A -C -R -+ -S
          +1-  +2-  E- I -U  -^1  -^2
    '''
*/

  // Byte 0
  //                  "P-", "M-", "N-", "-N", "-M", "-P",
  //                  "C-",
  //P-
  if (currentChord[0][1]) {
    chordBytes[0] += B1000000;
  }
  //M-
  if (currentChord[0][2]) {
    chordBytes[0] += B0100000;
  }
  //N-
  if (currentChord[0][3]) {
    chordBytes[0] += B0010000;
  }
  //-N
  if (currentChord[0][8]) {
    chordBytes[0] += B0001000;
  }
  //-M
  if (currentChord[0][9]) {
    chordBytes[0] += B0000100;
  }
  //-P
  if (currentChord[0][10]) {
    chordBytes[0] += B0000010;
  }
  // C-
    if (currentChord[1][0]) {
    chordBytes[0] += B0000001;
  }
    // Byte 1
  //T-
  if (currentChord[1][1]) {
    chordBytes[1] += B1000000;
  }
  //F-
  if (currentChord[1][2]) {
    chordBytes[1] += B0100000;
  }
  //L-
  if (currentChord[1][3]) {
    chordBytes[1] += B0010000;
  }
  //-L
  if (currentChord[1][8]) {
    chordBytes[1] += B0001000;
  }
  //-F
  if (currentChord[1][9]) {
    chordBytes[1] += B0000100;
  }
  //-T
  if (currentChord[1][10]) {
    chordBytes[1] += B0000010;
  }
  //-H
    if (currentChord[1][11]) {
    chordBytes[1] += B0000001;
  }

//                    "S-", "H-", "R-", "Y-", "O-", "I", "-A"
    // Byte 2
  //S-
  if (currentChord[2][0]) {
    chordBytes[2] += B1000000;
  }
  //H-
  if (currentChord[2][1]) {
    chordBytes[2] += B0100000;
  }//R-
  if (currentChord[2][2]) {
    chordBytes[2] += B0010000;
  }
  //Y-
  if (currentChord[2][3]) {
    chordBytes[2] += B0001000;
  }
  //O-
  if (currentChord[2][4]) {
    chordBytes[2] += B0000100;
  }
  //I
  if (currentChord[2][5] || currentChord[2][6]) {
    chordBytes[2] += B0000010;
  }
  //-A
    if (currentChord[2][7]) {
    chordBytes[2] += B0000001;
  }

//                    "-C", "-R", "-+", "-S",
//                     "+1-", "+2-", "E-"
    // Byte 3
  //-C
  if (currentChord[2][8]) {
    chordBytes[3] += B1000000;
  }
  //-R
  if (currentChord[2][9]) {
    chordBytes[3] += B0100000;
  }//-+
  if (currentChord[2][10]) {
    chordBytes[3] += B0010000;
  }
  //-S
  if (currentChord[2][11]) {
    chordBytes[3] += B0001000;
  }
  //+1-
  if (currentChord[3][0] || currentChord[3][1]) {
    chordBytes[3] += B0000100;
  }
  //+2-
  if (currentChord[3][2] || currentChord[3][3]) {
    chordBytes[3] += B0000010;
  }
  //E-
    if (currentChord[3][4]) {
    chordBytes[3] += B0000001;
  }

// "I", "-U", "-^1", "-^2"
    // Byte 4
  //I
  if (currentChord[3][5] || currentChord[3][6]) {
    chordBytes[4] += B1000000;
  }
  //-U
  if (currentChord[3][7]) {
    chordBytes[4] += B0100000;
  }//-^1
  if (currentChord[3][8] || currentChord[3][9]) {
    chordBytes[4] += B0010000;
  }
  //-^2
  if (currentChord[3][10] || currentChord[3][11]) {
    chordBytes[4] += B0001000;
  }

  // Send chord bytes over serial
  for (int i = 0; i < 6; i++) {
    Serial.write(chordBytes[i]);
  }
}

// Send the chord using the current protocol. 
void sendChord() {
    sendChordJCCPalan();
}
