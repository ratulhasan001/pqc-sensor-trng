#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED Display Configuration ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define seaLevelPressure_hPa 1013.25
Adafruit_BMP085 bmp;

unsigned long mask[20] = {0}, mxor = 0, mseed = 4357;

#define N 624
#define M 397
#define MATRIX_A 0x9908b0df // constant vector a
#define UPPER_MASK 0x80000000 // most significant w-r bits
#define LOWER_MASK 0x7fffffff // least significant r bits

// Tempering parameters
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y) (y >> 11)
#define TEMPERING_SHIFT_S(y) (y << 7)
#define TEMPERING_SHIFT_T(y) (y << 15)
#define TEMPERING_SHIFT_L(y) (y >> 18)

static unsigned long mt[N]; // the array for the state vector
static int mti = N + 1; // mti==N+1 means mt[N] is not initialized

void sgenrand(unsigned long seed) {
  mt[0] = seed & 0xffffffff;
  for (mti = 1; mti < N; mti++) {
    mt[mti] = ((69069 * mt[mti - 1]) & 0xffffffff) ^ mask[mti % 20];
  }
}

double genrand() {
  unsigned long y;
  static unsigned long mag01[2] = {0x0, MATRIX_A};
  
  if (mti >= N) {
    int kk;
    if (mti == N + 1) {
      sgenrand(mseed); // a default initial seed is used
    }
    
    for (kk = 0; kk < N - M; kk++) {
      y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
      mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    
    for (; kk < N - 1; kk++) {
      y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
      mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
    }
    
    y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
    mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1]; 
    mti = 0;
  }
  
  y = mt[mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);
  
  return ((double)y / (unsigned long)0xffffffff);
}

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  
  // --- Initialize OLED Display ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 OLED allocation failed"));
    for(;;); // Loop forever if OLED fails
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("TRNG Booting..."));
  display.display();
  delay(1000);
  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
      // Loop forever if BMP085 sensor is not connected
    }
  }
  sgenrand(mseed);
}

void loop() {
  double temperature = bmp.readTemperature();
  double pressure = bmp.readPressure();
  double altitude = bmp.readAltitude();
  double sealevelPressure = bmp.readSealevelPressure();
  double altitude2 = bmp.readAltitude(seaLevelPressure_hPa * 100);
  
  int x = analogRead(A0); 
  int y = analogRead(A1); 
  int z = analogRead(A2); 
  int lightD = digitalRead(3); 
  int lightA = analogRead(A3); 
  int sound = analogRead(A4); 
  int hallD = digitalRead(4); 
  int hallA = analogRead(A5); 
  
  if (lightD == 1) { mxor = 1UL << 31; }
  if (hallD == 1) { mxor = mxor | 1UL << 30; }
  
  mask[0] = (((unsigned long)temperature) ^ 4150375424) ^ mxor;
  mask[1] = (((unsigned long)pressure) ^ 4100028416) ^ mxor;
  mask[2] = (((unsigned long)altitude) ^ 4281656320) ^ mxor;
  mask[3] = (((unsigned long)sealevelPressure) ^ 4102921216) ^ mxor;
  mask[4] = (((unsigned long)altitude2) ^ 4056912896) ^ mxor;
  mask[5] = (((unsigned long)x) ^ 4055836672) ^ mxor;
  mask[6] = (((unsigned long)y) ^ 4113331200) ^ mxor;
  mask[7] = (((unsigned long)z) ^ 4150385664) ^ mxor;
  mask[8] = (((unsigned long)lightA) ^ 4142441472) ^ mxor;
  mask[9] = (((unsigned long)sound) ^ 4170023936) ^ mxor;
  mask[10] = (((unsigned long)hallA) ^ 4142056448) ^ mxor;
  mask[11] = ((mask[1] ^ mask[3]) ^ 4115310592) ^ mxor;
  mask[12] = ((mask[2] | mask[4]) ^ 4203119616) ^ mxor;
  mask[13] = ((mask[5] + mask[7]) ^ 4129018880) ^ mxor;
  mask[14] = (((mask[6] + mask[8]) << 1) ^ 4192752640) ^ mxor;
  mask[15] = ((mask[9] << 1) ^ 4175273984) ^ mxor;
  mask[16] = ((mask[10] >> 1) ^ 4064406528) ^ mxor;
  mask[17] = ((mask[11] ^ mask[13]) ^ 4188546048) ^ mxor;
  mask[18] = ((mask[12] | mask[14]) ^ 4063539200) ^ mxor;
  mask[19] = ((mask[15] + mask[17]) ^ 4187773952) ^ mxor;
  
  mseed = mask[0] ^ mask[1] ^ mask[2] ^ mask[3] ^ mask[4] ^ mask[5] ^ mask[6] ^ mask[7] ^ 
          mask[8] ^ mask[9] ^ mask[10] ^ mask[11] ^ mask[12] ^ mask[13] ^ mask[14] ^ 
          mask[15] ^ mask[16] ^ mask[17] ^ mask[18] ^ mask[19];
          
  // Re-seed with the new physical entropy instead of calling setup()
  sgenrand(mseed);
  
  // Capture the first three generated numbers for the OLED screen
  double r1 = genrand();
  double r2 = genrand();
  double r3 = genrand();

  // --- Update OLED Display ---
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Seed: ");
  display.println(mseed);
  
  display.drawLine(0, 12, 128, 12, SSD1306_WHITE); // Visual separator line
  
  display.setCursor(0, 20);
  display.setTextSize(2); // Larger text for the random numbers
  display.println(r1, 4);
  display.println(r2, 4);
  display.println(r3, 4);
  display.display(); 

  // --- Print to Serial Monitor ---
  // Print the first three numbers we just captured
  Serial.print(r1, 5); Serial.print(" ");
  Serial.print(r2, 5); Serial.print(" ");
  Serial.print(r3, 5); Serial.print(" ");

  // Loop to print the remaining 621 numbers exactly as before
  for (int j = 3; j < 624; j++) {
    Serial.print(genrand(), 5);
    Serial.print(" ");
    if (j % 8 == 7) {
      Serial.println();
    }
  }
  
  Serial.println();
  delay(5000); 
}