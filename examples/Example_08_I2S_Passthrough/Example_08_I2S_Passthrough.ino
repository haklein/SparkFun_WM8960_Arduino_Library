/******************************************************************************
  Example_08_I2S_Passthrough.ino
  Demonstrates reading I2S audio from the ADC, and passing that back to the DAC.
  
  This example sets up analog audio input (on INPUT1s), ADC/DAC enabled as I2S peripheral, sets volume control, and Headphone output on the WM8960 Codec.

  Audio should be connected to both the left and right "INPUT1" inputs, 
  they are labeled "RIN1" and "LIN1" on the board.

  This example will pass your audio source through the mixers and gain stages of the codec 
  into the ADC. Read the audio from the ADC via I2S.
  Then send audio immediately back to the DAC via I2S.
  Then send the output of the DAC to the headphone outs.

  HARDWARE CONNECTIONS

  **********************
  ESP32 ------- CODEC
  **********************
  QWIIC ------- QWIIC       *Note this connects GND/3.3V/SDA/SCL
  GND --------- GND         *optional, but not a bad idea
  5V ---------- VIN         *needed for source of codec's onboard AVDD (3.3V vreg)
  4 ----------- DDT         *aka DAC_DATA/I2S_SDO/"serial data out", this carries the I2S audio data from ESP32 to codec DAC
  16 ---------- BCK         *aka BCLK/I2S_SCK/"bit clock", this is the clock for I2S audio, can be conntrolled via controller or peripheral.
  17 ---------- ADAT        *aka ADC_DATA/I2S_SD/"serial data in", this carries the I2S audio data from codec's ADC to ESP32 I2S bus.
  25 ---------- DLRC        *aka I2S_WS/LRC/"word select"/"left-right-channel", this toggles for left or right channel data.
  25 ---------- ALR         *for this example WS is shared for both the ADC WS (ALR) and the DAC WS (DLRC)

  **********************
  CODEC ------- AUDIO IN
  **********************
  GND --------- TRS INPUT SLEEVE        *ground connection for line level input via TRS breakout
  INPUT1 ------ TRS INPUT TIP           *left audio
  INPUT2 ------ TRS INPUT RING1         *right audio

  **********************
  CODEC -------- AUDIO OUT
  **********************
  OUT3 --------- TRS OUTPUT SLEEVE          *buffered "vmid" connection for headphone output (aka "HP GND")
  HPL ---------- TRS OUTPUT TIP             *left HP output
  HPR ---------- TRS OUTPUT RING1           *right HP output


  You can now control the volume of the codecs built in headphone amp using this fuction:

  codec.setHeadphoneVolume(120); Valid inputs are 47-127. 0-47 = mute, 48 = -73dB, ... 1dB steps ... , 127 = +6dB

  Pete Lewis @ SparkFun Electronics
  October 14th, 2022
  https://github.com/sparkfun/SparkFun_WM8960_Arduino_Library
  
  This code was created using some code by Mike Grusin at SparkFun Electronics
  Included with the LilyPad MP3 example code found here:
  Revision history: version 1.0 2012/07/24 MDG Initial release
  https://github.com/sparkfun/LilyPad_MP3_Player

  This code was created using some modified code from DroneBot Workshop.
  Specifically, the I2S configuration setup was super helpful to get I2S working.
  This example has a similar I2S config to what we are using here: Microphone to serial plotter example.
  Although, here we are doing a full duplex I2S port, in order to do reads and writes.
  To see the original Drone Workshop code and learn more about I2S in general, please visit:
  https://dronebotworkshop.com/esp32-i2s/

  Do you like this library? Help support SparkFun. Buy a board!

    SparkFun Audio Codec Breakout - WM8960 (QWIIC)
    https://www.sparkfun.com/products/21250
	
	All functions return 1 if the read/write was successful, and 0
	if there was a communications failure. You can ignore the return value
	if you just don't care anymore.

	For information on the data sent to and received from the CODEC,
	refer to the WM8960 datasheet at:
	https://github.com/sparkfun/SparkFun_Audio_Codec_Breakout_WM8960/blob/main/Documents/WM8960_datasheet_v4.2.pdf

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <Wire.h>
#include <SparkFun_WM8960_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_WM8960
WM8960 codec;

// Include I2S driver
#include <driver/i2s.h>

// Connections to I2S
#define I2S_WS 25
#define I2S_SD 17
#define I2S_SDO 4
#define I2S_SCK 16

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

// Define input buffer length
#define bufferLen 64
int16_t sBuffer[bufferLen];

void setup()
{
  Serial.begin(115200);
  Serial.println("Example 8 - I2S Passthough");

  Wire.begin();

  if (codec.begin() == false) //Begin communication over I2C
  {
    Serial.println("The device did not respond. Please check wiring.");
    while (1); //Freeze
  }
  Serial.println("Device is connected properly.");

  codec_setup();

  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
}

void loop()
{
  // Get I2S data and place in data buffer
  size_t bytesIn = 0;
  size_t bytesOut = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);

  if (result == ESP_OK)
  {
    // send what we just received back to the codec
    esp_err_t result_w = i2s_write(I2S_PORT, &sBuffer, bytesIn, &bytesOut, portMAX_DELAY);
  }
  // delayMicroseconds(300); // Only hear to demonstrate how much time you have to do things.
  // Do not do much in this main loop, or the audio won't pass through correctly.
  // With default settings (64 samples in buffer), you can spend up to 300 microseconds doing something in between passing each buffer of data
  // you can tweak the buffer length to get more time if you need it.
  // when bufferlength is 64, then you get ~300 microseconds
  // when bufferlength is 128, then you get ~600 microseconds
  // note, as you increase bufferlength, then you are increasing latency between ADC input to DAC output,
  // Latency may or may not be desired, depending on the project.
}

void codec_setup()
{
  // General setup needed
  codec.enableVREF();
  codec.enableVMID();

  // setup signal flow to the ADC

  codec.enable_LMIC();
  codec.enable_RMIC();

  // connect from INPUT1 to "n" (aka inverting) inputs of PGAs.
  codec.connect_LMN1();
  codec.connect_RMN1();

  // disable mutes on PGA inputs (aka INTPUT1)
  codec.disable_LINMUTE();
  codec.disable_RINMUTE();

  // set pga volumes
  //codec.set_LINVOL(63);
  //codec.set_RINVOL(63);

  // set input boosts to get inputs 1 to the boost mixers
  codec.set_LMICBOOST(0); // 0-3, 0 = +0dB, 1 = +13dB, 2 = +20dB, 3 = +29dB
  codec.set_RMICBOOST(0); // 0-3, 0 = +0dB, 1 = +13dB, 2 = +20dB, 3 = +29dB

  codec.connect_LMIC2B();
  codec.connect_RMIC2B();

  // enable boost mixers
  codec.enable_AINL();
  codec.enable_AINR();

  // disconnect LB2LO (booster to output mixer (analog bypass)
  // for this example, we are going to pass audio throught the ADC and DAC
  codec.disableLB2LO();
  codec.disableRB2RO();

  // connect from DAC outputs to output mixer
  codec.enableLD2LO();
  codec.enableRD2RO();

  // set gainstage between booster mixer and output mixer
  // for this loopback example, we are going to keep these as low as they go
  codec.setLB2LOVOL(0); // 0 = -21dB
  codec.setRB2ROVOL(0); // 0 = -21dB

  // enable output mixers
  codec.enableLOMIX();
  codec.enableROMIX();

  // CLOCK STUFF, These settings will get you 44.1KHz sample rate, and class-d freq at 705.6kHz
  codec.enablePLL(); // needed for class-d amp clock
  codec.set_PLLPRESCALE(PLLPRESCALE_DIV_2);
  codec.set_SMD(PLL_MODE_FRACTIONAL);
  codec.set_CLKSEL(CLKSEL_PLL);
  codec.set_SYSCLKDIV(SYSCLK_DIV_BY_2);
  codec.set_BCLKDIV(4);
  codec.set_DCLKDIV(DCLKDIV_16);
  codec.set_PLLN(7);
  codec.set_PLLK(0x86, 0xC2, 0x26); // PLLK=86C226h
  //codec.set_ADCDIV(0); // default is 000 (what we need for 44.1KHz), so no need to write this.
  //codec.set_DACDIV(0); // default is 000 (what we need for 44.1KHz), so no need to write this.
  codec.set_WL(WL_16BIT);

  codec.enablePeripheralMode();
  //codec.enableMasterMode();
  //codec.set_ALRCGPIO(); // note, should not be changed while ADC is enabled.

  // enable ADCs and DACs
  codec.enableAdcLeft();
  codec.enableAdcRight();
  codec.enableDacLeft();
  codec.enableDacRight();
  codec.disableDacMute();

  //codec.enableLoopBack(); // Loopback sends ADC data directly into DAC
  codec.disableLoopBack();
  codec.disableDacMute(); // default is "soft mute" on, so we must disable mute to make channels active

  codec.enableHeadphones();
  codec.enableOUT3MIX(); // provides VMID as buffer for headphone ground

  Serial.println("Volume set to +0dB");
  codec.setHeadphoneVolume(120);

  Serial.println("Codec Setup complete. Listen to left/right INPUT1 on Headphone outputs.");
}

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_SDO,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}