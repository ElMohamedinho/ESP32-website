#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h> 
#include <WiFi.h>
#include <WebServer.h>

#define PIN 48
#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500
Servo verticalServo;   
const int trigPin = 15;
const int echoPin = 16;
int previous_height = 0;
// Constants
const int H_max = 100; // Maximum height in centimeters
const float speedOfSound = 0.0343; // Speed of sound in cm/µs


const char* ssid = "SSID";
const char* password = "PASSWORD";


String systemStatus = "Idle";
float currentEnergyLevel; // 0% to 100%
float totalStoredEnergy;  // MWh
float thermalOutput;    // kW

// Web server
WebServer server(80);

int verticalPosition = 100;
// Ultrasonic sensor pins


void handleRoot() {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <title>System Dashboard</title>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0-alpha1/dist/css/bootstrap.min.css" rel="stylesheet">
      <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css" rel="stylesheet">
      <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600&display=swap" rel="stylesheet">
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <style>
        body {
          background: linear-gradient(135deg, #0f0c29, #302b63, #24243e);
          font-family: 'Poppins', sans-serif;
          color: #fff;
          margin: 0;
          padding: 0;
          overflow-x: hidden;
        }
        .glass-card {
          background: rgba(255, 255, 255, 0.1);
          border-radius: 15px;
          backdrop-filter: blur(10px);
          border: 1px solid rgba(255, 255, 255, 0.2);
          padding: 20px;
          margin-bottom: 20px;
          transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        .glass-card:hover {
          transform: translateY(-10px);
          box-shadow: 0 15px 30px rgba(0, 0, 0, 0.3);
        }
        .card-header {
          font-weight: 600;
          font-size: 1.2rem;
          color: #fff;
          margin-bottom: 15px;
        }
        .progress {
          height: 20px;
          border-radius: 10px;
          background: rgba(255, 255, 255, 0.2);
          overflow: hidden;
        }
        .progress-bar {
          background: linear-gradient(90deg, #00c6ff, #0072ff);
          border-radius: 10px;
          transition: width 0.5s ease;
        }
        .status-charging { color: #00ff88; }
        .status-discharging { color: #ff4d4d; }
        .status-idle { color: #cccccc; }
        h1 {
          font-weight: 600;
          text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.2);
          margin-bottom: 30px;
        }
        .chart-container {
          background: rgba(255, 255, 255, 0.1);
          border-radius: 15px;
          padding: 20px;
          backdrop-filter: blur(10px);
          border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .parallax {
          position: absolute;
          top: 0;
          left: 0;
          width: 100%;
          height: 100%;
          z-index: -1;
          background: url('https://www.transparenttextures.com/patterns/dark-stripes.png');
          opacity: 0.1;
        }
      </style>
    </head>
    <body>
      <div class="parallax"></div>
      <div class="container mt-5">
        <h1 class="text-center mb-4">System Dashboard</h1>
        <div class="row">
          <div class="col-md-6 mb-4">
            <div class="glass-card">
              <div class="card-header">
                <i class="fas fa-info-circle"></i> System Status
              </div>
              <div class="card-body">
                <h5 class="card-title" id="status">Loading...</h5>
              </div>
            </div>
          </div>
          <div class="col-md-6 mb-4">
            <div class="glass-card">
              <div class="card-header">
                <i class="fas fa-battery-half"></i> Current Energy Storage Level
              </div>
              <div class="card-body">
                <div class="progress">
                  <div id="energyProgress" class="progress-bar" role="progressbar" style="width: 0%;"></div>
                </div>
                <h5 class="card-title mt-2" id="energyLevel">Loading...</h5>
              </div>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="col-md-6 mb-4">
            <div class="glass-card">
              <div class="card-header">
                <i class="fas fa-mountain"></i> Total Stored Potential Energy
              </div>
              <div class="card-body">
                <h5 class="card-title" id="storedEnergy">Loading...</h5>
              </div>
            </div>
          </div>
          <div class="col-md-6 mb-4">
            <div class="glass-card">
              <div class="card-header">
                <i class="fas fa-bolt"></i> Current Output from Thermal Power Plant
              </div>
              <div class="card-body">
                <h5 class="card-title" id="thermalOutput">Loading...</h5>
              </div>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="col-md-12 mb-4">
            <div class="chart-container">
              <canvas id="energyChart"></canvas>
            </div>
          </div>
        </div>
      </div>
      <script>
        const ctx = document.getElementById('energyChart').getContext('2d');
        const energyChart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],
            datasets: [{
              label: 'Energy Storage Level (%)',
              data: [],
              borderColor: '#00c6ff',
              backgroundColor: 'rgba(0, 198, 255, 0.1)',
              fill: true,
              tension: 0.4
            }]
          },
          options: {
            scales: {
              y: { beginAtZero: true, max: 100 }
            },
            plugins: {
              legend: { display: false }
            },
            animation: {
              duration: 1000,
              easing: 'easeInOutQuad'
            }
          }
        });

        async function fetchData() {
          const response = await fetch('/data');
          const data = await response.json();
          document.getElementById('status').innerText = data.status;
          document.getElementById('status').className = `card-title status-${data.status.toLowerCase()}`;
          document.getElementById('energyLevel').innerText = data.energyLevel + '%';
          document.getElementById('energyProgress').style.width = data.energyLevel + '%';
          document.getElementById('storedEnergy').innerText = data.storedEnergy + ' MWh';
          document.getElementById('thermalOutput').innerText = data.thermalOutput + ' kW';

          // Update chart
          const labels = energyChart.data.labels;
          const dataset = energyChart.data.datasets[0].data;
          labels.push(new Date().toLocaleTimeString());
          dataset.push(data.energyLevel);
          if (labels.length > 10) {
            labels.shift();
            dataset.shift();
          }
          energyChart.update();
        }
        setInterval(fetchData, 1000); // Update every second
      </script>
    </body>
    </html>
  )";
  server.send(200, "text/html", html);
}


void updateSystemStatus(int previous_height , int height) {
  if (height < previous_height){
      systemStatus = "Discharging";
      }
  if (height > previous_height){
      systemStatus = "Charging";
      }
  if (height == previous_height){
      systemStatus = "Idle";
      }
  
}


void updateEnergyLevel(int height, int H_max) {

  int currentEnergyLevel1 = height;
  currentEnergyLevel = constrain(currentEnergyLevel1,0,100);
}


void updateStoredEnergy(int height) {
  // Calculate total stored energy (MWh)
  float mass = 1000.0; // Example: 1000 kg // Example: 50 meters
  totalStoredEnergy = (mass * 9.81 * height) / 3.6e6; // Convert Joules to MWh
}


void updateThermalOutput(float irradiance) {
  float collector_area = 10000  ;
  float efficiency = 0.20  ;
  thermalOutput = ((irradiance * collector_area * efficiency) / 1000);
}

void handleData() {
  // Create a JSON response with system data
  String json = "{";
  json += "\"status\":\"" + systemStatus + "\",";
  json += "\"energyLevel\":" + String(currentEnergyLevel) + ",";
  json += "\"storedEnergy\":" + String(totalStoredEnergy) + ",";
  json += "\"thermalOutput\":" + String(thermalOutput);
  json += "}";
  server.send(200, "application/json", json);
}


float estimateIrradiance(int ldrValue) {
  // Example calibration curve (replace with your own calibration data)
  float m = 0.1; // Slope (calibration factor)
  float b = 0.0; // Intercept (calibration factor)
  return m * ldrValue + b;
}




int measureHeight() {
  // Send a 10µs pulse to the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure the duration of the echo pulse
  long duration = pulseIn(echoPin, HIGH);

  // Calculate distance in centimeters
  int distance = duration * speedOfSound / 2;

  // Calculate height (assuming the sensor is mounted at the top)
  int height = H_max - distance;

  return height;
}





void setup() {
  Serial.begin(115200);
  verticalServo.attach(4);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode (5,INPUT);
  pinMode  (6,INPUT);
  pinMode (7,INPUT);
  verticalServo.write(verticalPosition);
  pixels.begin();
  Serial.println("Initializing...");

  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  // Wait for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
      pixels.setPixelColor(0, pixels.Color(60, 60, 0));
      pixels.show();
    attempts++;
    if (attempts > 20) { // Timeout after 20 seconds
      Serial.println("\nFailed to connect to Wi-Fi. Please check credentials.");
      pixels.setPixelColor(0, pixels.Color(60, 0, 0));
      pixels.show();
      return;
    }
  }

  Serial.println("\nConnected to Wi-Fi");
  pixels.setPixelColor(0, pixels.Color(0, 60, 0));
  pixels.show();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started");


}





void loop() {
  // put your main code here, to run   repeatedly:

  int leftTop = analogRead(5);
  int rightBottom = analogRead(6);
  int leftBottom = analogRead(7);
  
  int avg_down = max(leftBottom , rightBottom);
  int verticalLight =  leftTop- avg_down;
  float ldrValue = ((leftBottom + avg_down)/2);
  float irradiance = estimateIrradiance(ldrValue);

  if (verticalLight > 0) {
    verticalPosition += 1;  // Move up
  } else if (verticalLight < 0) {
    verticalPosition -= 1;  // Move down
  }

  // Constrain the servo positions to stay within the range of 0-180 degrees
  verticalPosition = constrain(verticalPosition, 40, 150);

  // Move the servos to the new positions
  verticalServo.write(verticalPosition);
  int height = measureHeight();
  // Print values for debugging

  
  // Wait for a short period before the next loop iteration


    // Update system variables (simulated or measured)
  updateSystemStatus(previous_height,height);
  updateEnergyLevel(height,H_max);
  updateStoredEnergy(height);
  updateThermalOutput(irradiance);

  // Handle client requests
  server.handleClient();
  delay(15);
  previous_height = height;
}
