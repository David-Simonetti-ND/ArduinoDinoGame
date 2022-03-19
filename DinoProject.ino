#include <LiquidCrystal.h>
#define WIDTH 16 // define the width of the lcd
#define HEIGHT 2 // define the height of the lcd

#define SPEEDPOT 5 // pin for speed potentiometer
#define JUMPBUTTON 2 // pin for jump button interrupt
#define GREEN 8 // green led pin 
#define YELLOW 9 // yellow led pin
#define RED 10 // red led pin

LiquidCrystal lcd(11, 12, 4, 5, 6, 7);

enum sprites {DINO, CACTUS, BIRD, NONE}; // used when updating and displaying game board
enum dino_states {RUNNING, JUMPING}; // states for dinosaur

volatile int baseJumpSpeed = 800; // used to make jumps the same time in game for different speeds
volatile int potVal = 0; // store value of speedpot
volatile int screen[WIDTH * HEIGHT]; // array used to store game board
volatile long unsigned int jump_start = 0; // time when jump began
volatile int dino_state = RUNNING; // store state of dino
volatile int num_lives = 3; // store how many lives left

// byte arrays to make dinosaur and cactus and bird sprites
byte dinosaur[8] = {
  B00111,
  B00101,
  B00111,
  B01100,
  B11111,
  B11100,
  B10100,
};

byte cactus[8] = {
  B00101,
  B10101,
  B10101,
  B11111,
  B01110,
  B00100,
  B00100,
};

byte bird[8] = {
  B00011,
  B00110,
  B01100,
  B10100,
  B01100,
  B00110,
  B00011,
};

void dinoJump() // interrupt routine used to begin jumping
{
  if (jump_start == 0) // jump_start is only 0 when we arent currently jumping. If we are currently jumping we ignore the button press so we don't need to debounce the button
  {
    jump_start = millis(); // get start time of jump
    dino_state = JUMPING; // change dino state
  }
}

void drawScreen()
{
  for (int i = 0; i < WIDTH * HEIGHT; i++) // loop through entire screen array
  {
    lcd.setCursor(i % WIDTH, i / 16); // set cursor to each element of the screen and display the sprite if its not empty
    if (screen[i] == NONE) continue;
    lcd.write(byte(screen[i]));
  }
}

void initScreen()
{
  for (int i = 0; i < WIDTH * HEIGHT; i++) // initialize the screen to entirely empty
  {
    screen[i] = NONE;
  }
}

void gameOver()
{
  displayLives(); // clear out lives
  lcd.clear();
  lcd.write("Game over!"); // display end game messages
  lcd.setCursor(0, 1);
  lcd.write("Press reset");
  while(1); // force reset to be pressed by busy waiting
}

void updateScreen()
{
  if ( (millis() - jump_start) >= (baseJumpSpeed * map(potVal, 0, 1023, 1, 4)) ) // if the jump has taken-time scaled for all speeds-800 milliseconds to complete
  {
    dino_state = RUNNING; // the dino stops jumping
    jump_start = 0;
  }
  if (dino_state == RUNNING) // set dino location for when we are running
  {
    screen[0] = NONE;
    screen[16] = DINO;
    if (screen[17] != NONE) // check for collision
    {
      num_lives--; // if we hit something, subtract a life
      if (num_lives == 0) // check if we have no lives
      {
        gameOver();
      }
    }
  }
  else if (dino_state == JUMPING) // set dino location for when we are jumping
  {
    screen[0] = DINO;
    screen[16] = NONE;
    if (screen[1] != NONE) // check collision
    {
      num_lives--; // subtract life and check for game over
      if (num_lives == 0)
      {
        gameOver();
      }
    }
  }
  for (int i = 0; i < WIDTH * HEIGHT; i++) // loop through and update the entire screen
  {
    if ( (i == 0) || (i == 16)) continue;  // skip where the dino is located
    if ((i == 15) || (i == 31)) // if we are at the endpoints, these are special cases so we ignore them for now
    {
      screen[i] = NONE;
      continue;
    }
    screen[i] = screen[i + 1]; // move every object one to the left
  }
  int random_num = random(1, 8); // get a random num between 1 and 8
  switch(random_num) // if its a 1 or 2, then we add a cactus or a bird to the screen
  {
    case CACTUS:
      if ( (screen[13] == BIRD) || (screen[14] == BIRD)) break; // make it so the dino doesnt have an impossible obstacle 
      if ( (screen[30] == CACTUS) && (screen[29] == CACTUS)) break;
      screen[31] = CACTUS; // add new cactus to the screen
      break;
    case BIRD:
      if ( (screen[30] == CACTUS) || (screen[29] == CACTUS)) break; // make it so the dino doesnt have an impossible obstacle 
      if ( (screen[13] == BIRD) && (screen[14] == BIRD)) break;
      screen[15] = BIRD; // add new cactus to the screen
      break;
    default: // add no new obstacle to the screen
      break;
  }
  lcd.clear();
}

void displayLives() // based on how many lives the dino lost, turn off one or more leds
{
  switch (num_lives)
  {
    case 2:
      digitalWrite(GREEN, LOW);
      break;
    case 1:
      digitalWrite(YELLOW, LOW);
      break;
    case 0:
      digitalWrite(RED, LOW);
      break;
  }
}

void setup() {
  randomSeed(analogRead(0)); // seed random num generator for creating new obstacles
  lcd.createChar(DINO, dinosaur); // create dino, cactus, bird sprites
  lcd.createChar(CACTUS, cactus);
  lcd.createChar(BIRD, bird);
  lcd.begin(16, 2);  
  pinMode(JUMPBUTTON, INPUT); // set input/output pins
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);
  digitalWrite(GREEN, HIGH); // initialize to three lives left
  digitalWrite(YELLOW, HIGH);
  digitalWrite(RED, HIGH);
  attachInterrupt(digitalPinToInterrupt(JUMPBUTTON), dinoJump, RISING); // create interrupt handler
  initScreen();
  screen[16] = DINO; // create initial game state
  screen[18] = CACTUS;
  screen[5] = BIRD;
}

void loop() {
  drawScreen();
  potVal = analogRead(SPEEDPOT); // get speedpot value
  displayLives(); // show how many lives are left
  delay(map(potVal, 0, 1023, 200, 800)); // wait a time based on the value of the speedpot
  updateScreen(); // update the game

}
