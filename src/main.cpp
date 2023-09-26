#include <Arduino.h>
#include <EMA_Filters.h>
#include <movingAVG.h>
#include <NoDelay.h>
#include <Servo.h>

/*create instances for devices*/
Servo svo; // servo motor

/*create instances for data processing*/
EMA_Filters emaFilt; 
float f_s = 400; // sampling frequency
movingAvg mvavg(10); //average latest 10 data points

/*EMG parameters*/
int emg_pin = 0; // analog input pin
float emg_stat = 0; // resting EMG signal
int emg_evlp_thd = 3; // threshold for enveloped EMG signal

/*servo parameters
writing values to the servo too often draws a lot of power from the board, causing issues. e.g. affecting analog input.
Thus need a "clock" for writing servo values less rapidly.
*/
int svo_pin = 2; // digital output pin
float svo_pos = 0; // starting servo position
noDelay svo_clk(1000); //"clock" for servo




void setup() {
  Serial.begin(115200);

  svo.attach(svo_pin); 
  svo.write(svo_pos);
  mvavg.begin();

  /*resting EMG signal collection for calibration*/ 
  Serial.println("Resting EMG collection for calibration");
  float emg_stat_sum = 0;
  for (int i=0; i<10; i++)
  {
      for (int j=0; j<100; j++)
      {
          emg_stat_sum += analogRead(emg_pin);
          delay(10);
      }
      // led.setLevel(1 + i); 
      Serial.print("Done in ");
      Serial.println(10-i);
  }
  Serial.println("Done!");
  emg_stat = emg_stat_sum / 1000.0;
}



void loop() {
  /*EMG collection and visualization*/
  float emg_raw = (analogRead(emg_pin) - emg_stat); // raw centered emg signal 
  // bandpass
  float f_c_bplow = 50; // low cut off frequency
  float f_c_bphigh = 150; // high cut off frequency
  float emg_bp = emaFilt.BPF(emg_raw, f_c_bplow, f_c_bphigh, f_s); // bandpassed emg signal between 50 Hz and 150 Hz
  // plot signals using Teleplot
  // Serial.print(">raw EMG signal: ");
  // Serial.println(emg_raw);
  Serial.print(">bandpass EMG signal: ");
  Serial.println(emg_bp);

  /*capture EMG singal envelope*/
  float f_c_lp = 1; // lowpass cut off frequency 
  float emg_evlp =  pow(emaFilt.LPF(abs(emg_bp), f_c_lp, f_s), 2); // take magnitude, lowpass, then square
  int emg_evlp_avg = mvavg.reading(emg_evlp); // further smoothing
  Serial.print(">bandpass EMG signal envelope: ");
  Serial.println(emg_evlp_avg);
  
  /*simple thresholding for servo output*/
  if (svo_clk.update()){
    if (emg_evlp_avg > emg_evlp_thd) {
      svo_pos = 50;
    }
    else if (svo_pos > 0) {
      svo_pos -= 10; // set back to 0
    }
    svo.write(svo_pos);
  }
}