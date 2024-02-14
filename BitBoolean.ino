uint8_t stateHolder = 0; // Holds up to 8 states, each bit represents a different state

// Function to set the state (0 or 1) of the first bit (bit 0)
void setState(int var, bool state) {
  if (state) {
    stateHolder |= 1 << var; // Set bit var to 1
  } else {
    stateHolder &= ~(1 << var); // Set bit var to 0
  }
}

// Function to get the state of the first bit (bit 0)
bool getState(int var) {
  return (stateHolder & (1 << var)) != 0;
}

void setup() {
  Serial.begin(115200);

  // Initially, both states are 0
  Serial.print("Initial State 0: ");
  Serial.println(getState(0));
  Serial.print("Initial State 1: ");
  Serial.println(getState(0));

  // Set states
  setState(0, true);
  setState(1, false); // This is redundant since it's already false, shown for example

  // Check states after setting
  Serial.print("After Setting State 0: ");
  Serial.println(getState(0));
  Serial.print("After Setting State 1: ");
  Serial.println(getState(1));
}

void loop() {

}
